/*! ----------------------------------------------------------------------------
 *  @file    ds_twr_resp.c
 *  @brief   TAG
 */
#include <stdio.h>
#include <string.h>
#include "deca_device_api.h"
#include "deca_regs.h"
#include "timer.h"
#include "DW1000.h"
#include "twr.h"

/* Default communication configuration. We use here EVK1000's default mode (mode 3). */
static dwt_config_t config = {
    2,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_1024,   /* Preamble length. Used in TX only. */
    DWT_PAC32,       /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_110K,     /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (1025 + 64 - 32) /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};

/* Default antenna delay values for 64 MHz PRF. See NOTE 1 below. */
#define TX_ANT_DLY 16436
#define RX_ANT_DLY 16436

//* Record anchor_sn orderly *//
struct ANCHOR
{
    unsigned char sn;
    double distance;
    char resp_enable;
} anchor[MAX_ANCHOR_NUM];
unsigned char anchor_sn = 0x01;  // Indicates the anchor_sn currently connected
unsigned char anchor_sn_idx = 0; // Indicates the anchor currently connected
unsigned char anchor_total = 0;  // Indicates the total anchor num have been connected

//* Frames used in the ranging process. *//
static uint8 rx_poll_msg[] = {0x41, 0x88, 0, PAN_L, PAN_M, POLL_FUNCMODE, 'T', ALL_SN, 'A', ALL_SN, 0, 0};         // The poll must contain src in order to judge whether to response
static uint8 tx_resp_msg[] = {0x41, 0x88, 0, PAN_L, PAN_M, RESP_FUNCMODE, 'A', EMPTY_SN, 'T', TAG_SN, 0, 0, 0, 0}; // Follows DESTINATION then SOURCE sequence
static uint8 rx_final_msg[] = {0x41, 0x88, 0, PAN_L, PAN_M, FINAL_FUNCMODE, 'T', TAG_SN, 'A', EMPTY_SN, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//* Functional MSG frames *//
static uint8 tx_distance_msg[] = {0x41, 0x88, 0, PAN_L, PAN_M, DISTANCE_FUNCMODE, 'A', EMPTY_SN, 'T', TAG_SN, 0, 0, 0, 0};
static uint8 tx_emergency_msg[] = {0x41, 0x88, 0, PAN_L, PAN_M, EMERGENCY_FUNCMODE, 'A', ALL_SN, 'T', TAG_SN, 0, 0}; // Every anchor under RESP receiving state could receive
// static uint8 tx_dist_upload_msg[] = {0x41, 0x88, 0, PAN_L, PAN_M, DISUPLOAD_FUNCMODE, 'A', MAIN_ANCHOR_SN, 'T', TAG_SN, 'A', 0, 0, 0, 'A', 0, 0, 0, 'A', 0, 0, 0, 0, 0};

//* Assists MSG validation *//
/* Length of the common part of the message (up to and including the function code, see NOTE 2 below). Starts from 1. */
#define ALL_MSG_COMMON_LEN 10
#define RESP_MSG_LEN 8

//* Assists fill and check frame *//
/* Index to access some of the fields in the frames involved in the process. Starts from 0. */
#define ALL_MSG_SN_IDX 2
#define RESP_MSG_TX_EMPTYSN_IDX 7
#define POLL_MSG_RX_SN_IDX 8
#define FINAL_MSG_POLL_TX_TS_IDX 10
#define FINAL_MSG_RESP_RX_TS_IDX 14
#define FINAL_MSG_FINAL_TX_TS_IDX 18
#define DISTANCE_MSG_VALUE_IDX 10
#define FINAL_MSG_TS_LEN 4
// #define DIST_UPLOAD_MSG_TX_VALUE_IDX 10

/* Frame sequence number, incremented after each transmission. */
static uint8 frame_seq_nb = 0;

/* Buffer to store received messages.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
#define RX_BUF_LEN 24
static uint8 rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) conversion factor.
 * 1 uus = 512 / 499.2 �s and 1 �s = 499.2 * 128 dtu. */
#define UUS_TO_DWT_TIME 65536

/* Delay between frames, in UWB microseconds. See NOTE 4 below. */
/* This is the delay from Frame RX timestamp to TX reply timestamp used for calculating/setting the DW1000's delayed TX function. This includes the
 * frame length of approximately 2.46 ms with above configuration. */
#define POLL_RX_TO_RESP_TX_DLY_UUS 6500 //! ori: 2600
/* This is the delay from the end of the frame transmission to the enable of the receiver, as programmed for the DW1000's wait for response feature. */
#define RESP_TX_TO_FINAL_RX_DLY_UUS 1500 //! ori: 500
/* Receive final timeout. See NOTE 5 below. */
#define FINAL_RX_TIMEOUT_UUS 3300 //! ori: 3300
/* Preamble timeout, in multiple of PAC size. See NOTE 6 below. */
#define PRE_TIMEOUT 8

/* Timestamps of frames transmission/reception.
 * As they are 40-bit wide, we need to define a 64-bit int type to handle them. */
static uint64 poll_rx_ts;
static uint64 resp_tx_ts;
static uint64 final_rx_ts;

/* Speed of light in air, in metres per second. */
#define SPEED_OF_LIGHT 299702547

/* Hold copies of computed time of flight and distance here for reference so that it can be examined at a debug breakpoint. */
static double tof;
static double distance;

/* Declaration of static functions. */
static char Src_acquire(char *rx_msg);
static uint64 get_tx_timestamp_u64(void);
static uint64 get_rx_timestamp_u64(void);
static void final_msg_get_ts(const uint8 *ts_field, uint32 *ts);

static void ds_twr_poll_rx(void)
{
    uint32 frame_len;

    /* Clear good RX frame event in the DW1000 status register. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);

    /* A frame has been received, read it into the local buffer. */
    frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
    if (frame_len <= RX_BUFFER_LEN)
    {
        dwt_readrxdata(rx_buffer, frame_len, 0);
    }

    /* Check that the frame is a poll sent by "DS TWR initiator" example.
    /* As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
    rx_buffer[ALL_MSG_SN_IDX] = 0;
}

static int ds_twr_resp_tx(void)
{
    uint32 resp_tx_time;

    /* Retrieve poll reception timestamp. */
    poll_rx_ts = get_rx_timestamp_u64();

    /* Set send time for response. See NOTE 9 below. */
    resp_tx_time = (poll_rx_ts + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
    dwt_setdelayedtrxtime(resp_tx_time);

    /* Set expected delay and timeout for final message reception. See NOTE 4 and 5 below. */
    dwt_setrxaftertxdelay(RESP_TX_TO_FINAL_RX_DLY_UUS);
    dwt_setrxtimeout(FINAL_RX_TIMEOUT_UUS);

    /* Write and send the response message. See NOTE 10 below.*/
    tx_resp_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
    tx_resp_msg[RESP_MSG_TX_EMPTYSN_IDX] = anchor_sn;
    dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_resp_msg), 0, 1);          /* Zero offset in TX buffer, ranging. */

    return dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED);
}

static void ds_twr_final_rx(void)
{
    uint32 frame_len;

    /* Clear good RX frame event and TX frame sent in the DW1000 status register. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG | SYS_STATUS_TXFRS);

    /* A frame has been received, read it into the local buffer. */
    frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
    if (frame_len <= RX_BUF_LEN)
    {
        dwt_readrxdata(rx_buffer, frame_len, 0);
    }

    /* Check that the frame is a final message sent by "DS TWR initiator" example.*/
    /*As the sequence number field of the frame is not used in this example, it can be zeroed to ease the validation of the frame.*/
    rx_buffer[ALL_MSG_SN_IDX] = 0;
}

static void ds_twr_final_handle(void)
{
    uint32 poll_tx_ts, resp_rx_ts, final_tx_ts;
    uint32 poll_rx_ts_32, resp_tx_ts_32, final_rx_ts_32;
    double Ra, Rb, Da, Db;
    int64 tof_dtu;

    /* Retrieve response transmission and final reception timestamps. */
    resp_tx_ts = get_tx_timestamp_u64();
    final_rx_ts = get_rx_timestamp_u64();

    /* Get timestamps embedded in the final message. */
    final_msg_get_ts(&rx_buffer[FINAL_MSG_POLL_TX_TS_IDX], &poll_tx_ts);
    final_msg_get_ts(&rx_buffer[FINAL_MSG_RESP_RX_TS_IDX], &resp_rx_ts);
    final_msg_get_ts(&rx_buffer[FINAL_MSG_FINAL_TX_TS_IDX], &final_tx_ts);

    /* Compute time of flight. 32-bit subtractions give correct answers even if clock has wrapped. See NOTE 12 below. */
    poll_rx_ts_32 = (uint32)poll_rx_ts;
    resp_tx_ts_32 = (uint32)resp_tx_ts;
    final_rx_ts_32 = (uint32)final_rx_ts;
    Ra = (double)(resp_rx_ts - poll_tx_ts);
    Rb = (double)(final_rx_ts_32 - resp_tx_ts_32);
    Da = (double)(final_tx_ts - resp_rx_ts);
    Db = (double)(resp_tx_ts_32 - poll_rx_ts_32);
    tof_dtu = (int64)((Ra * Rb - Da * Db) / (Ra + Rb + Da + Db));

    tof = tof_dtu * DWT_TIME_UNITS;
    distance = tof * SPEED_OF_LIGHT * 100;
}

static void ds_twr_distance_tx(void)
{
    int temp = (int)(distance * 100);                         // makes a float into a integer
    tx_distance_msg[DISTANCE_MSG_VALUE_IDX] = temp / 100;     // integer
    tx_distance_msg[DISTANCE_MSG_VALUE_IDX + 1] = temp % 100; //decimal

    tx_distance_msg[ALL_MSG_SN_IDX] = frame_seq_nb++;
    tx_distance_msg[RESP_MSG_TX_EMPTYSN_IDX] = anchor_sn;

    dwt_writetxdata(sizeof(tx_distance_msg), tx_distance_msg, 0);
    dwt_writetxfctrl(sizeof(tx_distance_msg), 0, 0);
    int ret = dwt_starttx(DWT_START_TX_IMMEDIATE);
    /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 12 below. */
    if (ret == DWT_SUCCESS)
    {
        /* Poll DW1000 until TX frame sent event set. See NOTE 9 below. */
        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS))
        {
        };

        /* Clear TXFRS event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
        printf("send: DISTANCE\n");
    }
}

// static void ds_twr_dist_upload_tx(void)
// {
//     char i, j;
//     double temp;
//     char value_buffer[] = {'A', 0x01, 0, 0, 'A', 0x02, 0, 0, 'A', 0x03, 0, 0};
//     /* Write frame data to DW1000 and prepare transmission. See NOTE 8 below. */
//     tx_dist_upload_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
//     j = 0;
//     for (i = 0; i < sizeof(value_buffer);)
//     {
//         temp = anchor[j++].distance;
//         i += 2;
//         value_buffer[i++] = / 100;
//         value_buffer[i++] = % 100;
//     }
//     for (i = 0; i < sizeof(value_buffer); i++)
//     {
//         tx_dist_upload_msg[DIST_UPLOAD_MSG_TX_VALUE_IDX + i] = value_buffer[i];
//     }

//     dwt_writetxdata(sizeof(tx_dist_upload_msg), tx_dist_upload_msg, 0); /* Zero offset in TX buffer. */
//     dwt_writetxfctrl(sizeof(tx_dist_upload_msg), 0, 0);                 /* Zero offset in TX buffer, no ranging. */
//     /* Start transmission, indicating that a response is expected so that reception is enabled automatically after the frame is sent and the delay */
//     /* set by dwt_setrxaftertxdelay() has elapsed. */
//     dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED); //* wait ack
//     /* Increment frame sequence number after transmission of the poll message (modulo 256). */
//     frame_seq_nb++;
// }

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn main()
 *
 * @brief Application entry point.
 *
 * @param  none
 *
 * @return none
 */
void ds_twr_responder(void)
{
    dw_init();
    printf("Now TAG(SN: 0x%x) working as RESPONDER.\r\n\r\n", TAG_SN);
    /* Configure DW1000. See NOTE 7 below. */
    dwt_configure(&config);

    /* Apply default antenna delay value. See NOTE 1 below. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);

    /* Set preamble timeout for expected frames. See NOTE 6 below. */
    dwt_setpreambledetecttimeout(PRE_TIMEOUT);

    TIM3_Int_Init(14999, 7199); // reset all enable
    /* Loop forever responding to ranging requests. */
    while (1)
    {
        //* Wating for a broadcast *//
        /* Clear reception timeout to start next ranging process. */
        dwt_setrxtimeout(0);

        /* Activate reception immediately. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        /* Poll for reception of a frame or error/timeout. See NOTE 8 below. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
        {
        };
        if (status_reg & SYS_STATUS_RXFCG)
        {
            ds_twr_poll_rx();
            //* RECEIVE : POLL *//
            if (memcmp(rx_buffer, rx_poll_msg, ALL_MSG_COMMON_LEN - 1) == 0)
            {
                char i;
                char respen = 0;
                // Acquire src anchor and make a judgement whether to reply
                anchor_sn = Src_acquire(rx_buffer);
                // Find if anchor connected is being cooling down
                for (i = 0; i < anchor_total + 1; i++)
                {
                    if (anchor[i].sn == anchor_sn)
                    {
                        respen = anchor[i].resp_enable;
                        anchor_sn_idx = i;
                        printf("re    ");
                        break;
                    }
                    else if (i == anchor_total)
                    {
                        anchor[anchor_total].sn = anchor_sn;
                        anchor_sn_idx = anchor_total;
                        anchor_total++;
                        respen = 1;
                        printf("first ");
                        break;
                    }
                }
                printf("connected ANCHOR%d (%d/%d)\r\n", anchor_sn, i, anchor_total - 1);
                if (respen)
                {
                    //* SEND : RESP *//
                    /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 11 below. */
                    if (ds_twr_resp_tx() == DWT_ERROR)
                    {
                        continue;
                    }
                    printf("send: RESP ");

                    //* Waits for FINAL *//
                    /* Poll for reception of expected "final" frame or error/timeout. See NOTE 8 below. */
                    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
                    {
                    };
                    /* Increment frame sequence number after transmission of the response message (modulo 256). */
                    frame_seq_nb++;

                    if (status_reg & SYS_STATUS_RXFCG)
                    {
                        ds_twr_final_rx();
                        // Final msg received may not for the specific device  */
                        // If final msg destination doesn't fit , it will start all over again. */
                        rx_buffer[9] = anchor_sn;
                        if (memcmp(rx_buffer, rx_final_msg, ALL_MSG_COMMON_LEN - 1) == 0)
                        {
                            //* RECEIVE : FINAL *//
                            printf("receive: FINAL ");
                            ds_twr_final_handle();
                            ds_twr_distance_tx();
                            ds_twr_distance_tx();
                            // Finish measuring for one time, starts cooling down.
                            anchor[anchor_sn_idx].resp_enable = 0;
                            anchor[anchor_sn_idx].distance = distance;
                            // printf("DIST from ANCHOR%d: %3.2f cm\r\n", anchor_sn, distance);
                            printf("-------------------------END--------------\r\n");
                            printf("RESULT\r\n");
                            for (i = 0; i < anchor_total; i++)
                            {
                                printf("ANCHOR%d : %3.2fcm\r\n", anchor[i].sn, anchor[i].distance);
                            }
                            printf("-------------------------END--------------\r\n");
                        }
                    }
                    else
                    {
                        /* Clear RX error/timeout events in the DW1000 status register. */
                        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);

                        /* Reset RX to properly reinitialise LDE operation. */
                        dwt_rxreset();
                    }
                }
            }
        }
        else
        {
            /* Clear RX error/timeout events in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);

            /* Reset RX to properly reinitialise LDE operation. */
            dwt_rxreset();
        }
        if (TIM3_TIMEUP)
        {
            char i;
            TIM3_TIMEUP = 0;
            //* Clear Enable *//
            for (i = 0; i < MAX_ANCHOR_NUM; i++)
            {
                anchor[i].resp_enable = 1;
            }
            //* send : dist upload msg to main anchor *//
            //ds_twr_dist_upload_tx();// TODO: SEND DIST TO MAIN ANCHOR
        }
    }
}

static char Src_acquire(char *rx_msg)
{
    // Check again if Src is from T
    if (rx_msg[POLL_MSG_RX_SN_IDX] == 'A')
    {
        return rx_msg[POLL_MSG_RX_SN_IDX + 1] + 0;
    }
    else
        return 0;
}

static uint64 get_tx_timestamp_u64(void)
{
    uint8 ts_tab[5];
    uint64 ts = 0;
    int i;
    dwt_readtxtimestamp(ts_tab);
    for (i = 4; i >= 0; i--)
    {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}

static uint64 get_rx_timestamp_u64(void)
{
    uint8 ts_tab[5];
    uint64 ts = 0;
    int i;
    dwt_readrxtimestamp(ts_tab);
    for (i = 4; i >= 0; i--)
    {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn final_msg_get_ts()
 *
 * @brief Read a given timestamp value from the final message. In the timestamp fields of the final message, the least
 *        significant byte is at the lower address.
 *
 * @param  ts_field  pointer on the first byte of the timestamp field to read
 *         ts  timestamp value
 *
 * @return none
 */
static void final_msg_get_ts(const uint8 *ts_field, uint32 *ts)
{
    int i;
    *ts = 0;
    for (i = 0; i < FINAL_MSG_TS_LEN; i++)
    {
        *ts += ts_field[i] << (i * 8);
    }
}
