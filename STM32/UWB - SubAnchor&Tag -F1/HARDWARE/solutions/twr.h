#ifndef __twr_H
#define __twr_H
#include "sys.h"
#include "delay.h"

// #define ANCHOR
#define SUB
// #define DEBUGMODE

//* SN SETTINGS *//
#define MAIN_ANCHOR_SN 0xF1
#define ANCHOR_SN 0x03
#define TAG_SN 0x01
#define ALL_SN 0xFF
#define EMPTY_SN 0

#define MAX_TAG_NUM 5
#define MAX_ANCHOR_NUM 5

//* PAN ID *//
#define PAN_L 0xCA
#define PAN_M 0xDE

//* FUNCTION MODE *//
#define POLL_FUNCMODE 0x11
#define RESP_FUNCMODE 0x21
#define FINAL_FUNCMODE 0x31
#define DISTANCE_FUNCMODE 0x41
#define EMERGENCY_FUNCMODE 0x51
#define DISUPLOAD_FUNCMODE 0x61

//* ACITIVITY CODE *//
#define KEEP_ACTCODE 0x02 // 0x02 to tell the initiator to go on with the ranging exchange

int ss_twr_init(void);
int ss_twr_resp(void);

void ds_twr_initiator(void);
void ds_twr_responder(void);

#endif
