/*! ----------------------------------------------------------------------------
 *  @file    ds_twr_init.c
 *  @brief   ANCHOR
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "delay.h"
#include "usart.h"
#include "timer.h"

#include "deca_device_api.h"
#include "deca_regs.h"
#include "DW1000.h"
#include "twr.h"

#include "trilateration_basic.h"
#include "trilateration_EKF.h"

#include "lcd.h"
#include "UI.h"
#include "ESP8266.h"

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
#ifndef SUB
// If all anchor unknown we must record those anchor measured distance independently
#endif

// MAIN anchor needs a distance char to store all distance measured
struct TAG
{
    unsigned char sn;
    double distance[3]; //ANCHOR1, ANCHOR2, ANCHOR3
    unsigned char emergency;

} tag[MAX_TAG_NUM];
unsigned char tag_sn = 0x01;  // Indicates the anchor_sn currently connected
unsigned char tag_sn_idx = 0; // Indicates the anchor currently connected
unsigned char tag_total = 0;  // Indicates the total anchor num have been connected

//* Frames used in the ranging process. *//
//! The last to 0 is preserved for automatic fill chars
static uint8 tx_poll_msg[] = {0x41, 0x88, 0, PAN_L, PAN_M, POLL_FUNCMODE, 'T', ALL_SN, 'A', ANCHOR_SN, 0, 0}; // When initiate, the destination remains unknown, use ALL_SN as SN
static uint8 rx_resp_msg[] = {0x41, 0x88, 0, PAN_L, PAN_M, RESP_FUNCMODE, 'A', ANCHOR_SN, 'T', ALL_SN, 0, 0}; // Follows DESTINATION then SOURCE sequence
static uint8 tx_final_msg[] = {0x41, 0x88, 0, PAN_L, PAN_M, FINAL_FUNCMODE, 'T', EMPTY_SN, 'A', ANCHOR_SN,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//* Functional MSG frames *//
static uint8 rx_distance_msg[] = {0x41, 0x88, 0, PAN_L, PAN_M, DISTANCE_FUNCMODE, 'A', ANCHOR_SN, 'T', EMPTY_SN,
                                  0, 0, 0, 0};
static uint8 rx_emergency_msg[] = {0x41, 0x88, 0, PAN_L, PAN_M, EMERGENCY_FUNCMODE, 'A', ALL_SN, 'T', TAG_SN, 0, 0}; // Every anchor under RESP receiving state could receive
static uint8 trx_disupload_msg[] = {0x41, 0x88, PAN_L, PAN_M, DISUPLOAD_FUNCMODE, 'A', MAIN_ANCHOR_SN, 'A', ANCHOR_SN,
                                    'T', 0, 0, 0, 'T', 0, 0, 0, 'T', 0, 0, 0, 'T', 0, 0, 0, 'T', 0, 0, 0, 0, 0}; // Reserve 3Tag spaces

//* Assists MSG validation *//
/* Length of the common part of the message (up to and including the function code, see NOTE 2 below). Starts from 1. */
#define ALL_MSG_COMMON_LEN 10
#define RESP_MSG_LEN 8

//* Assists fill and check frame *//
/* Indexes to access some of the fields in the frames defined above. Starts from 0. */
#define ALL_MSG_SN_IDX 2
#define RESP_MSG_RX_SN_IDX 8
#define FINAL_MSG_TX_EMPTYSN_IDX 7
#define FINAL_MSG_POLL_TX_TS_IDX 10
#define FINAL_MSG_RESP_RX_TS_IDX 14
#define FINAL_MSG_FINAL_TX_TS_IDX 18
#define FINAL_MSG_TS_LEN 4
#define DIST_MSG_RX_EMPTYSN_IDX 9
#define DISTANCE_MSG_VALUE_IDX 10
#define DISUPLOAD_MSG_VALUE_IDX 8
/* Frame sequence number, incremented after each transmission. */
static uint8 frame_seq_nb = 0;

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
#define RX_BUF_LEN 70
static uint8 rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) conversion factor.
 * 1 uus = 512 / 499.2 �s and 1 �s = 499.2 * 128 dtu. */
#define UUS_TO_DWT_TIME 65536

/* Delay between frames, in UWB microseconds. See NOTE 4 below. */
/* This is the delay from the end of the frame transmission to the enable of the receiver, as programmed for the DW1000's wait for response feature. */
#define POLL_TX_TO_RESP_RX_DLY_UUS 150 //! ori: 150
/* This is the delay from Frame RX timestamp to TX reply timestamp used for calculating/setting the DW1000's delayed TX function. This includes the
 * frame length of approximately 2.66 ms with above configuration. */
#define RESP_RX_TO_FINAL_TX_DLY_UUS 3100 //! ori: 3100
/* Receive response timeout. See NOTE 5 below. */
#define RESP_RX_TIMEOUT_UUS 6750
/* Preamble timeout, in multiple of PAC size. See NOTE 6 below. */
#define PRE_TIMEOUT 128

/* Time-stamps of frames transmission/reception, expressed in device time units.
 * As they are 40-bit wide, we need to define a 64-bit int type to handle them. */
static uint64 poll_tx_ts;
static uint64 resp_rx_ts;
static uint64 final_tx_ts;

/* Declaration of static functions. */
static char Src_acquire(char *rx_msg);
static uint64 get_tx_timestamp_u64(void);
static uint64 get_rx_timestamp_u64(void);
static void final_msg_set_ts(uint8 *ts_field, uint64 ts);
static void TWR_AXIS_CALC(void);
unsigned int LSB, MSB;

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn ds_twr_poll_tx()
 *
 * @brief TWR Procedure : POLL Send.
 *
 * @param  none
 *
 * @return none
 */
static void ds_twr_poll_tx(void)
{
    /* Write frame data to DW1000 and prepare transmission. See NOTE 8 below. */
    tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
    dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1);          /* Zero offset in TX buffer, ranging. */
    /* Start transmission, indicating that a response is expected so that reception is enabled automatically after the frame is sent and the delay */
    /* set by dwt_setrxaftertxdelay() has elapsed. */
    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn ds_twr_resp_rx()
 *
 * @brief TWR Procedure : RESP(any other resp) Receive.
 *
 * @param  none
 *
 * @return none
 */
static void ds_twr_resp_rx(void)
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

    /* Check that the frame is the expected response from the companion "DS TWR responder" example.*/
    /* As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
    rx_buffer[ALL_MSG_SN_IDX] = 0;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn ds_twr_final_tx()
 *
 * @brief TWR Procedure : FINAL Send.
 *
 * @param  none
 *
 * @return none
 */
static int ds_twr_final_tx(void)
{
    uint32 final_tx_time;
    int ret;

    /* Retrieve poll transmission and response reception timestamp. */
    poll_tx_ts = get_tx_timestamp_u64();
    resp_rx_ts = get_rx_timestamp_u64();

    /* Compute final message transmission time. See NOTE 10 below. */
    final_tx_time = (resp_rx_ts + (RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
    dwt_setdelayedtrxtime(final_tx_time);

    /* Final TX timestamp is the transmission time we programmed plus the TX antenna delay. */
    final_tx_ts = (((uint64)(final_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

    /* Write all timestamps in the final message. See NOTE 11 below. */
    final_msg_set_ts(&tx_final_msg[FINAL_MSG_POLL_TX_TS_IDX], poll_tx_ts);
    final_msg_set_ts(&tx_final_msg[FINAL_MSG_RESP_RX_TS_IDX], resp_rx_ts);
    final_msg_set_ts(&tx_final_msg[FINAL_MSG_FINAL_TX_TS_IDX], final_tx_ts);

    /* Write and send final message. See NOTE 8 below. */
    tx_final_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
    tx_final_msg[FINAL_MSG_TX_EMPTYSN_IDX] = tag_sn;
    dwt_writetxdata(sizeof(tx_final_msg), tx_final_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_final_msg), 0, 1);           /* Zero offset in TX buffer, ranging. */
    ret = dwt_starttx(DWT_START_TX_DELAYED);

    /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. See NOTE 12 below. */
    if (ret == DWT_SUCCESS)
    {
        /* Poll DW1000 until TX frame sent event set. See NOTE 9 below. */
        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS))
        {
        };

        /* Clear TXFRS event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

        /* Increment frame sequence number after transmission of the final message (modulo 256). */
        frame_seq_nb++;
    }
    return ret;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn ds_twr_distance_rx()
 *
 * @brief TWR Procedure : DISTANCE Receive (Distance result calculated from responder).
 *
 * @param  none
 *
 * @return none
 */
static void ds_twr_distance_rx(void)
{
    /* Clear reception timeout to start next ranging process. */
    dwt_setrxtimeout(0);

    /* Activate reception immediately. */
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
    {
    };
    if (status_reg & SYS_STATUS_RXFCG)
    {
        ds_twr_resp_rx();

        rx_buffer[DIST_MSG_RX_EMPTYSN_IDX] = tag_sn;

        //* RECEIVE : DISTANCE_SENDBACK_MSG *//
        if (memcmp(rx_buffer, rx_distance_msg, ALL_MSG_COMMON_LEN - 1) == 0)
        {
            LSB = rx_buffer[DISTANCE_MSG_VALUE_IDX + 1];
            MSB = rx_buffer[DISTANCE_MSG_VALUE_IDX];
            tag[tag_sn_idx].distance[0] = MSB * 100 + LSB;
        }
        printf("receive: DISTANCE: %4.0fcm\n", tag[tag_sn_idx].distance[0]);
    }
    else
    {
        /* Clear RX error/timeout events in the DW1000 status register. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);

        /* Reset RX to properly reinitialise LDE operation. */
        dwt_rxreset();
    }
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn ds_twr_upload_distance_rx()
 *
 * @brief DISTANCE GATHERING Procedure : UPLOAD Receive (Distance uploaded by Subs).
 *
 * @param  none
 *
 * @return none
 */
static void ds_twr_upload_distance_rx(void)
{
    //* RECEIVE : DISTANCE_UPLOAD_MSG *//
    /* Clear reception timeout to start next ranging process. */
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    /* Activate reception immediately. */
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
    {
    };
    if (status_reg & SYS_STATUS_RXFCG)
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

        if (memcmp(rx_buffer, trx_disupload_msg, ALL_MSG_COMMON_LEN - 6) == 0)
        {
            //* Extract DISTANCE MSG
            char i, j;
            char sub_anchor_sn;
            char sub_tag_sn;
            char idx = DISUPLOAD_MSG_VALUE_IDX;
            sub_anchor_sn = rx_buffer[idx++] + 0; // which sub anchor

            //printf("\nreceive: DISTANCE UPLOAD from SUB%d\n", sub_anchor_sn);
            //! Retrive all 3 distances from a single sub anchor
            // TODO : real situation may various with tag number , then use while
            for (i = 0; i < 3; i++)
            {
                idx++;
                sub_tag_sn = rx_buffer[idx++];
                //printf("TAG%d: ", sub_tag_sn);
                for (j = 0; j < tag_total; j++)
                {
                    //! Let's only think the situation that main anchor must involved in the distance measure
                    //! We compare every tag_sn Main anchor measured to determine which struct tag[] to record
                    if (tag[j].sn == sub_tag_sn)
                    {
                        //! We use sub_anchor_sn as the distance sequence for a single tag,
                        //! if the anchor_sn is not ordered, this must be changed.
                        MSB = rx_buffer[idx++];
                        LSB = rx_buffer[idx++];
                        tag[j].distance[sub_anchor_sn - 1] = MSB * 100 + LSB;
                        //printf("%3.2fcm\r\n", tag[j].distance[sub_anchor_sn - 1]);
                    }
                }
            }
            //printf("\n");
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

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn main()
 *
 * @brief Application entry point.
 *
 * @param  none
 *
 * @return none
 */
void ds_twr_initiator(void)
{
    dw_init();
    printf("now ANCHOR(SN: 0x%x) working as INITIATOR.\r\n\r\n", ANCHOR_SN);

    /* Configure DW1000. See NOTE 7 below. */
    dwt_configure(&config);

    /* Apply default antenna delay value. See NOTE 1 below. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);

    /* Set expected response's delay and timeout. See NOTE 4, 5 and 6 below. */
    /* As this example only handles one incoming frame with always the same delay and timeout, those values can be set here once for all. */
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    dwt_setpreambledetecttimeout(PRE_TIMEOUT);

    TIM3_Int_Init(4999, 8399);
    while (1)
    {
        //******************************
        //* DISTANCEs gathering Procedure
        // Waits if received any upload from SUBs
        //******************************
        long i = 100;
        while (i--)
        {
            delay_ms(3);
            ds_twr_upload_distance_rx();
        }

        //* TWR : Broadcast POLL *//
        ds_twr_poll_tx();
        printf("send: POLL ");

        //******************************
        //* following msgs may RECEIVE:
        //* 1) TWR : RESP
        //* 2) EMERGENCY
        //******************************
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
        {
        };
        /* Increment frame sequence number after transmission of the poll message (modulo 256). */
        frame_seq_nb++;
        if (status_reg & SYS_STATUS_RXFCG)
        {
            ds_twr_resp_rx();
            //* TWR : judge if RESP *//
            if (memcmp(rx_buffer, rx_resp_msg, RESP_MSG_LEN) == 0)
            {
                char i;
                // Get RESP src and record into buffer
                tag_sn = Src_acquire(rx_buffer);
                // Find if tag have connected, if not, record it
                for (i = 0; i < tag_total + 1; i++)
                {
                    if (tag[i].sn == tag_sn)
                    {
                        tag_sn_idx = i;
                        break;
                    }
                    else if (i == tag_total)
                    {
                        tag[tag_total].sn = tag_sn;
                        tag_sn_idx = tag_total;
                        tag_total++;
                        break;
                    }
                }
                //* TWR : Send FINAL to the source TAG *//
                if (ds_twr_final_tx() == 0)
                {
                    //* TWR : Receive Distance result calculated from TAG *//
                    ds_twr_distance_rx();
                    ds_twr_distance_rx();
                }
                printf("-----------CONNECTION-TO-TAG%d----END--\r\n", tag_sn);
            }

            //* RECEIVE : EMERGENCY_MSG *//
            //TODO: EMERGENCY MSG
            //TODO: sub anchor receive emergency msg, check src and replace src with the tag_sn, then all sub anchor would receive emergency msg
            //TODO: even it's not send from tag, but the msg includes where it originally starts. Untill Main Anchor receive.
            if (memcmp(rx_buffer, rx_emergency_msg, ALL_MSG_COMMON_LEN) == 0)
            {
                //* Main anchor would receive msg only *//
                char i;
                unsigned char emergency_tag_sn = rx_buffer[ALL_MSG_COMMON_LEN - 1];
                for (i = 0; i < tag_total; i++)
                {
                    if (tag[i].sn == emergency_tag_sn)
                    {
                        tag[i].emergency = 1;
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
        //******************************
        //* Result Handling Procedure
        //******************************
        if (TIM3_TIMEUP)
        {
            TIM3_TIMEUP = 0;
            TWR_AXIS_CALC();
        }
    }
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn TWR_AXIS_CALC()
 *
 * @brief Correction DATA, Calculate RESULT, Handle DISPLAY or UPLOAD.
 *
 * @param  none
 *
 * @return none
 */
static void TWR_AXIS_CALC(void)
{
    //! The sequence is already follows ANCHOR1,2,3 in STRUCT tag.
    int i;

    //******************************
    //*       CORRECTIONS:
    //******************************
    double tag_dist[3]; // the CORRECTION distance

    // copy TAG1 distance data
    for (i = 0; i < 3; i++)
    {
        tag_dist[i] = tag[0].distance[i];
    }
    //* Bias correction:
    for (i = 0; i < 3; i++)
    {
        if (tag_dist[i] < 60.0)
        {
            tag_dist[i] -= 0;
        }
        else if ((tag_dist[i] >= 60.0) && (tag_dist[i] < 360.0))
        {
            tag_dist[i] -= 20;
        }
        else if ((tag_dist[i] >= 360.0) && (tag_dist[i] < 650.0))
        {
            tag_dist[i] -= 30;
        }
        else if ((tag_dist[i] >= 650.0) && (tag_dist[i] < 1140.0))
        {
            tag_dist[i] -= 40;
        }
        else if (tag_dist[i] >= 1140.0)
        {
            tag_dist[i] -= 50;
        }
    }

    //* Altitude correction:
    for (i = 0; i < 3; i++)
    {
        //tag_dist[i] = sqrt((tag_dist[i] * tag_dist[i]) - (Anchor_vec[i].z * Anchor_vec[i].z));
    }

    //******************************
    //*       ALGORITHM:
    //******************************

    //* Basic Trilateration:
    // Trilateration_Basic(tag[0].distance);
    // Trilateration_Basic(tag[1].distance); // if there are more tags

    //* EKF:
    Trilateration_EKF(tag_dist);

    //******************************
    //*     RESULT OPTIONS:
    //******************************

    //* Keep Tracks:
    // UI_Map_Refresh();

    //* Display on LCD:
    UI_Map_Draw_Tag(Result_2d.x, Result_2d.y, "TAG1", 0);

    //* Publish to IOT system:
    IOT_Pulish_Tagpos();
}

static char Src_acquire(char *rx_msg)
{
    // Check again if Src is from T
    if (rx_msg[RESP_MSG_RX_SN_IDX] == 'T')
    {
        return rx_msg[RESP_MSG_RX_SN_IDX + 1] + 0;
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
 * @fn final_msg_set_ts()
 *
 * @brief Fill a given timestamp field in the final message with the given value. In the timestamp fields of the final
 *        message, the least significant byte is at the lower address.
 *
 * @param  ts_field  pointer on the first byte of the timestamp field to fill
 *         ts  timestamp value
 *
 * @return none
 */
static void final_msg_set_ts(uint8 *ts_field, uint64 ts)
{
    int i;
    for (i = 0; i < FINAL_MSG_TS_LEN; i++)
    {
        ts_field[i] = (uint8)ts;
        ts >>= 8;
    }
}
