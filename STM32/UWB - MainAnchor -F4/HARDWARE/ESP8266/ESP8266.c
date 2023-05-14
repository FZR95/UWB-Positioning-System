#include <stdlib.h>
#include <string.h>
#include "ESP8266.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "usart3.h"
#include "ADC.h"
#include "trilateration_basic.h"

//  content
const unsigned char Product_Connect[116] = {
    0x10, 0x72, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0xC2, 0x00, 0xB4, 0x00, 0x27, 0x31, 0x32,
    0x33, 0x34, 0x35, 0x7c, 0x73, 0x65, 0x63, 0x75, 0x72, 0x65, 0x6d, 0x6f, 0x64, 0x65, 0x3d, 0x33,
    0x2c, 0x73, 0x69, 0x67, 0x6e, 0x6d, 0x65, 0x74, 0x68, 0x6f, 0x64, 0x3d, 0x68, 0x6d, 0x61, 0x63,
    0x73, 0x68, 0x61, 0x31, 0x7c, 0x00, 0x13, 0x41, 0x6e, 0x63, 0x68, 0x6f, 0x72, 0x31, 0x26, 0x61,
    0x31, 0x6c, 0x49, 0x36, 0x4b, 0x64, 0x63, 0x45, 0x59, 0x37, 0x00, 0x28, 0x32, 0x33, 0x61, 0x31,
    0x37, 0x38, 0x37, 0x39, 0x38, 0x35, 0x32, 0x66, 0x61, 0x39, 0x62, 0x35, 0x63, 0x66, 0x31, 0x37,
    0x32, 0x35, 0x62, 0x62, 0x32, 0x35, 0x39, 0x33, 0x37, 0x39, 0x37, 0x64, 0x62, 0x32, 0x35, 0x32,
    0x30, 0x34, 0x39, 0x37};

//  Length +TOPIC:/sys/a1lI6KdcEY7/Anchor1/thing/event/property/post
const unsigned char Product_Publish_post[52] = {
    0x00, 0x32, 0x2F, 0x73, 0x79, 0x73, 0x2F, 0x61, 0x31, 0x6C, 0x49, 0x36, 0x4B, 0x64, 0x63, 0x45,
    0x59, 0x37, 0x2F, 0x41, 0x6E, 0x63, 0x68, 0x6F, 0x72, 0x31, 0x2F, 0x74, 0x68, 0x69, 0x6E, 0x67,
    0x2F, 0x65, 0x76, 0x65, 0x6E, 0x74, 0x2F, 0x70, 0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x2F,
    0x70, 0x6F, 0x73, 0x74};

//**************************************************
//  BASIC DRIVES FOR ESP8266
//**************************************************

static unsigned char EN_Receive = 0;
static unsigned char ReceiveLen = 0;

//  Check AT command OK
unsigned char Esp8266_Ready()
{
    int i;
    delay_ms(10);
    for (i = 0; i < (USART3_RX_STA & 0x3FFF) - 1; i++)
    {
        if ((USART3_RX_BUF[i] == 'O') && (USART3_RX_BUF[i + 1] == 'K'))
            return 1;
    }
    return 0;
}

//  ESP8266 Initialize
void Esp8266_Init(void)
{
    printf3("AT+RST\r\n");
    delay_ms(500);
    printf3("AT+CWMODE_CUR=1\r\n");
    delay_ms(100);
    printf3("AT+CIPMUX=0\r\n");
    delay_ms(100);
    printf3("AT+CWJAP_CUR=\"316_iot\",\"ioT_082e\"\r\n");
    delay_ms(12500);

    // printf3("AT+CIPSNTPCFG=1,8,\"cn.pool.ntp.org\"\r\n");
    // delay_ms(100);
}

// Connect to IOT platform
void Esp8266_IOT(void)
{
    printf3("AT+CIPSTART=\"TCP\",\"a1lI6KdcEY7.iot-as-mqtt.cn-shanghai.aliyuncs.com\",1883\r\n");
    delay_ms(1000);
    delay_ms(1000);
    delay_ms(1000);
    printf3("AT+CIPRECVMODE=1\r\n");
    delay_ms(100);
    Esp8266_Send(Product_Connect, 116);
}

// Pass-through MSGs to IOT platform
void Esp8266_Send(unsigned char *arr, unsigned int length)
{
    unsigned int i = 0;
    printf3("AT+CIPMODE=1\r\n");
    delay_ms(500);
    printf3("AT+CIPSEND\r\n");
    delay_ms(500);
    while (i < length)
    {
        while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
            ;
        USART_SendData(USART3, arr[i++]);
    }
    delay_ms(40);
    printf3("+++");
    delay_ms(1200);
    printf3("AT+CIPMODE=0\r\n");
    delay_ms(500);
}

// Ping to IOT platform
void Esp8266_Ping(void)
{
    unsigned char Ping[2] = {0xC0, 0x00};
    Esp8266_Send(Ping, 2);
}

// Disconnect to IOT platform
void Esp8266_Disconnect(void)
{
    unsigned char Disconnect[2] = {0xE0, 0x00};
    Esp8266_Send(Disconnect, 2);
    printf3("AT+CIPCLOSE\r\n");
    delay_ms(100);
}

//Generate Random numbers using adc
unsigned char getRandom()
{
    return Get_Adc(0) % 10;
}
//**************************************************
//  BASIC STRING HANDLING
//**************************************************

unsigned char toHex(unsigned char achar)
{
    return 0x20 + (achar - 32);
}

unsigned char toAsc(unsigned char ahex)
{
    unsigned char achar;
    if ((ahex >= 0x30) && (ahex <= 0x39))
    {
        achar = 48 + ahex - 0x30;
    }
    else if ((ahex >= 0x41) && (ahex <= 0x90))
    {
        achar = 65 + ahex - 0x41;
    }
    else if ((ahex >= 0x61) && (ahex <= 0x7A))
    {
        achar = 97 + ahex - 0x61;
    }
    return achar;
}

void toJson(char *oristr, const char *str, int value)
{
    char temp[] = "000";
    strcat(oristr, "\"");
    strcat(oristr, str);
    strcat(oristr, "\":");
    if (value < 0)
    {
        temp[0] = '-';
    }
    else
    {
        temp[0] = ' ';
    }
    temp[1] = value / 10 + '0';
    temp[2] = value % 10 + '0';
    strcat(oristr, temp);
    strcat(oristr, ",");
}

//**************************************************
//  MSG Publishing
//**************************************************

void IOT_Pulish_Anchorpos(void)
{
    unsigned int i;
    unsigned char id[8] = "";
    unsigned char str[512] = {0x30};
    unsigned char content[512];
    char info_anchorpos[512];
    unsigned char totalnum = 0; // TOPIC+CONTENT
    unsigned char enlargedbit = 0;
    // Random id
    Adc_Init();
    for (i = 0; i < 8; i++)
    {
        id[i] = '0' + (getRandom() % 10);
    }
    // {
    // "id":"78168640",
    // "version":"1.0",
    // "params":{
    //     "anchorpos":"{{\"name\":\"A1\",\"x\":\"   0\",\"y\":\"   0\",\"z\":\"   0\"},{\"name\":\"A2\",\"x\":\" 512\",\"y\":\"   0\",\"z\":\"   0\"},{\"name\":\"A3\",\"x\":\"   0\",\"y\":\"1280\",\"z\":\"   0\"}}"
    // },
    // "method":"thing.event.property.set"
    // }
    //{"name":"A1","y":"0.0","x":"0.0"};{"name":"A2","y":"0.0","x":"512.0"};{"name":"A3","y":"1280.0","x":"0.0"}
    sprintf(info_anchorpos,
            "A1:%4.0f,%4.0f,%4.0f;",
            Anchor_vec[0].x, Anchor_vec[0].y, Anchor_vec[0].z);
    sprintf(info_anchorpos,
            "%sA2:%4.0f,%4.0f,%4.0f;",
            info_anchorpos, Anchor_vec[1].x, Anchor_vec[1].y, Anchor_vec[1].z);
    sprintf(info_anchorpos,
            "%sA3:%4.0f,%4.0f,%4.0f",
            info_anchorpos, Anchor_vec[2].x, Anchor_vec[2].y, Anchor_vec[2].z);
    sprintf(content,
            "{\"id\":\"%s\",\"version\":\"1.0\",\"params\":{\"anchorpos\":\"%s\"},\"method\":\"thing.event.property.set\"}",
            id, info_anchorpos);

    totalnum = sizeof(Product_Publish_post) + strlen(content);
    if (totalnum > 127)
    {
        str[1] = totalnum + 0x00;
        str[2] = totalnum / 128;
        enlargedbit = 1;
    }
    else
    {
        str[1] = totalnum + 0x00;
        enlargedbit = 0;
    }
    //TOPIC
    for (i = 0; i < sizeof(Product_Publish_post); i++)
    {
        str[2 + enlargedbit + i] = Product_Publish_post[i];
    }
    //CONTENT
    for (i = 0; i < strlen(content); i++)
    {
        str[2 + enlargedbit + sizeof(Product_Publish_post) + i] = toHex(content[i]);
    }
    Esp8266_Send(str, 2 + enlargedbit + totalnum);
}

void IOT_Pulish_Tagpos(void)
{
    unsigned int i;
    unsigned char id[8] = "";
    unsigned char str[512] = {0x30};
    unsigned char info_tagpos[255];
    unsigned char content[512];
    unsigned char totalnum = 0; // TOPIC+CONTENT
    unsigned char enlargedbit = 0;
    // Random id
    Adc_Init();
    for (i = 0; i < 8; i++)
    {
        id[i] = '0' + (getRandom() % 10);
    }
    sprintf(info_tagpos,
            "T1:%4.0f,%4.0f",
            Result_2d.x, Result_2d.y);
    sprintf(content,
            "{\"id\":\"%s\",\"version\":\"1.0\",\"params\":{\"tagpos\":\"%s\"},\"method\":\"thing.event.property.set\"}",
            id, info_tagpos);

    totalnum = sizeof(Product_Publish_post) + strlen(content);
    if (totalnum > 127)
    {
        str[1] = totalnum + 0x00;
        str[2] = totalnum / 128;
        enlargedbit = 1;
    }
    else
    {
        str[1] = totalnum + 0x00;
        enlargedbit = 0;
    }
    //TOPIC
    for (i = 0; i < sizeof(Product_Publish_post); i++)
    {
        str[2 + enlargedbit + i] = Product_Publish_post[i];
    }
    //CONTENT
    for (i = 0; i < strlen(content); i++)
    {
        str[2 + enlargedbit + sizeof(Product_Publish_post) + i] = toHex(content[i]);
    }
    Esp8266_Send(str, 2 + enlargedbit + totalnum);
}

void IOT_Publish_Temphum(int temp, unsigned int hum)
{
    // example: {"id":"12","version":"1.0","params":{"Temp":4,"Hum":4,"Gas":4},"method":"thing.event.property.set"}
    unsigned int i;
    unsigned char id[8] = "";
    unsigned char str[300] = {0x30};
    unsigned char content[200] = "{\"id\":\""; // 7nums
    //unsigned char contentnum = 7;              // id from 8th
    unsigned char totalnum = 0; // TOPIC+CONTENT
    unsigned char enlargedbit = 0;
    // Random id
    Adc_Init();
    for (i = 0; i < 8; i++)
    {
        id[i] = '0' + (getRandom() % 10);
    }
    strcat(content, id);
    // Payload
    strcat(content, "\",\"version\":\"1.0\",\"params\":{");
    toJson(content, "Temperature", temp);
    toJson(content, "Humidity", hum);
    content[strlen(content) - 1] = '}';
    strcat(content, ",\"method\":\"thing.event.property.set\"}");
    // Length
    totalnum = sizeof(Product_Publish_post) + strlen(content);
    if (totalnum > 127)
    {
        str[1] = totalnum + 0x00;
        str[2] = totalnum / 128;
        enlargedbit = 1;
    }
    else
    {
        str[1] = totalnum + 0x00;
        enlargedbit = 0;
    }
    //TOPIC
    for (i = 0; i < sizeof(Product_Publish_post); i++)
    {
        str[2 + enlargedbit + i] = Product_Publish_post[i];
    }
    //CONTENT
    for (i = 0; i < strlen(content); i++)
    {
        str[2 + enlargedbit + sizeof(Product_Publish_post) + i] = toHex(content[i]);
    }
    Esp8266_Send(str, 2 + enlargedbit + totalnum);
}

//**************************************************
//  TOPIC SUBSCRIBES
//  MSGs are automatically received, no need to subescribe sys events/properties
//**************************************************

// void IOT_Subscribe_Weather()
// {
//     unsigned int i;
//     unsigned char str[50] = {0x82};
//     str[1] = (sizeof(STC15_Subscribe_weather) + 2 + 1) + 0x00;
//     // Variable
//     str[2] = 0x00;
//     str[3] = 0x0A;
//     //TOPIC
//     for (i = 0; i < sizeof(STC15_Subscribe_weather); i++)
//     {
//         str[4 + i] = STC15_Subscribe_weather[i];
//     }
//     //QOS
//     str[4 + sizeof(STC15_Subscribe_weather)] = 0x00;
//     Esp8266_Send(str, 2 + 2 + sizeof(STC15_Subscribe_weather) + 1);
// }

//**************************************************
//  RECIEVED MSGs HANDLING
//**************************************************

//  MSGs receving
void Esp8266_Recv(void)
{
    unsigned int i, j;
    unsigned int flag = 0;
    unsigned char templen[4];
    unsigned char temp[100];
    EN_Receive = 1;
    ReceiveLen = 0;
    printf3("AT+CIPRECVLEN?\r\n");
    // Data:
    // +CIPRECVLEN:149,0,0,0,0
    delay_ms(10);
    for (i = 0; i < ReceiveLen; i++)
    {
        if (USART3_RX_BUF[i] == ':')
        {
            flag = i;
            break;
        }
    }
    if (flag)
    {
        i = flag + 1;
        flag = 0;
        j = 0;
        while (USART3_RX_BUF[i] != ',')
        {
            templen[j++] = USART3_RX_BUF[i++];
        }
        // templen will be the length(149)
        if (atoi(templen))
        {
            EN_Receive = 1;
            ReceiveLen = 0;
            printf3("AT+CIPRECVDATA=300\r\n");
            // {"method":"thing.service.property.set","id":"934414241","params":{"Light":1},"version":"1.0.0"}
            delay_ms(50);
            for (i = 0; i < ReceiveLen; i++)
            {
                if (USART3_RX_BUF[i] == '{')
                {
                    flag = i;
                    break;
                }
            }
            if (flag)
            {
                i = flag + 1;
                flag = 0;
                for (; i < ReceiveLen; i++)
                {
                    if (USART3_RX_BUF[i] == '{')
                    {
                        flag = i;
                        break;
                    }
                }
            }
            if (flag)
            {
                i = flag + 1;
                j = 0;
                while (i < ReceiveLen)
                {
                    if (USART3_RX_BUF[i] == '}')
                        break;
                    else
                        temp[j++] = USART3_RX_BUF[i++];
                }
                MSG_Judge(temp);
            }
        }
        //  Clear temp arr
        for (i = 0; i < 3; i++)
        {
            templen[i] = ' ';
        }
    }
}

//  Classification received msgs
void MSG_Judge(unsigned char *strori)
{
    // Define params name string first
    unsigned char str1[] = "Light";
    unsigned char temp[10];
    Valueacquire(strori, str1, temp, 0);
}

//  Finds specified string
int strfind(unsigned char *str, unsigned char *sub_str, int fromwhere)
{
    int len = strlen(str);
    int len_f = strlen(sub_str);
    int i;
    for (i = fromwhere; i < len && len - i >= len_f; i++)
    {
        int k = i;
        int j = 0;
        while (j < len_f)
        {
            if (str[k] == sub_str[j])
            {
                k++;
                j++;
            }
            else
                break;
        }
        if (j == len_f)
        {
            return i + strlen(sub_str);
        }
    }
    return -1;
}

//  Finds specified params and copies the value to array
//  type: 0 - num, 1 - string(requires ~ at the end)
void Valueacquire(unsigned char *strori, unsigned char *strf, unsigned char *value, int type)
{
    int i, j;
    i = j = 0;
    // clear value array
    for (i = 0; i < strlen(value); i++)
    {
        value[i] = ' ';
    }
    i = strfind(strori, strf, 0);
    i += 2;
    if (type)
    {
        //Strings with ',' or '}'
        for (; i < strlen(strori) - 1; i++)
        {
            if ((strori[i] == '~') && ((strori[i + 1] == ',') || (strori[i + 1] == '}')))
            {
                j--;
                break;
            }
            value[j++] = strori[i];
        }
    }
    else
    {
        for (; i < strlen(strori); i++)
        {
            if ((strori[i] == ',') || (strori[i] == '}'))
            {
                j--;
                break;
            }
            value[j++] = strori[i];
        }
    }
}
