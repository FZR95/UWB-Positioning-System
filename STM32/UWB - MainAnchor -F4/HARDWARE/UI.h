#ifndef __UI_H
#define __UI_H
#include "sys.h"

void UI_Init(void);
void UI_Map_Refresh(void);

//* UWB project
void UI_Map_Draw_Anchor(unsigned int x, unsigned int y, unsigned char *name);
void UI_Map_Draw_Tag(unsigned int x, unsigned int y, unsigned char *name, char note);

void UI_Map_Draw_Horizen_Distance(unsigned int dist);
void UI_Map_Draw_Vertical_Distance(unsigned int dist);
void UI_Map_Draw_Radius_Distance(double d[3]);
void UI_Map_Draw_Point(double point_x, double point_y);

void UI_Opts_Position(void);

//* Universal
float UI_Input_Num(char *header, float raw);

//* Callback from touch
void UI_Input_Button_down(unsigned int x, unsigned int y);
void UI_Input_Button_up(unsigned int x, unsigned int y);

#endif
