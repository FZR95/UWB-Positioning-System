////////////////////////////
//  File Name : ESP8266
//  Description : Using STM32 drive ESP8266 with AT commands to connect to Ali-IOT using MQTT.
//  Init Date : 2020.11
//  Last Change : 2021.03.20
//  Version : 1.5
//  Made by FZR
////////////////////////////
#ifndef __ESP8266_H
#define __ESP8266_H
#include "stdio.h"
#include "sys.h"
/*
ALIYUN PRODUCT : ANCHOR1
{
  "ProductKey": "a1lI6KdcEY7",
  "DeviceName": "Anchor1",
  "DeviceSecret": "807eb7eeb2e6523f4d8868e1f6514ea9"
}
*/
void Esp8266_Init(void);
void Esp8266_IOT(void);
void Esp8266_Send(unsigned char *arr, unsigned int length);
void Esp8266_Ping(void);
void Esp8266_Disconnect(void);

void IOT_Publish_Temphum(int temp, unsigned int hum);
void IOT_Pulish_Anchorpos(void);
void IOT_Pulish_Tagpos(void);
void IOT_Subscribe_Weather(void);

void Esp8266_Recv(void);
void MSG_Judge(unsigned char *strori);
void Valueacquire(unsigned char *strori, unsigned char *strf, unsigned char *value, int type);
#endif
