////////////////////////////
//  File Name : Bmp
//  Description : Support Bmps display on screen with basic, general functions.
//  Init Date : 2020.03
//  Last Change : 2020.10.07
//  Version : 1.2
//  Made by FZR
////////////////////////////
////////////////////////////
//取模方式：竖直取模，对于8X16，有8列，每列16位，每位是一个2进制数，黑为1，灭为0，然后转换为16进制。
//	0
//	1
//  row:2
//	...
// 	16
//  col:0     1 2 3 4 5 6 7
////////////////////////////

const unsigned char *judgenum(unsigned int num);
void Display_Para(unsigned char x, unsigned char y, float para);
void Display_Hex(unsigned char x, unsigned char y, unsigned int num);
void Display_String(unsigned char x, unsigned char y, unsigned char str[], int strlen);
void Display_Main(void);
void Display_Info(int temp, int hum);
