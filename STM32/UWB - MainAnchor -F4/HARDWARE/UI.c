#include "UI.h"
#include "bsp_xpt2046_lcd.h"
#include "lcd.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "delay.h"
#include "trilateration_basic.h"

//****************************************************************************************************
//*
//*  VARIABLES - INTERNAL USE ONLY
//*
//****************************************************************************************************

//* UWB Project
static struct MAP_CANVAS
{
    unsigned int margin_x, margin_y;
    unsigned int width, height;
    float xscale, yscale;
} map;

//* UNIVERSAL INPUT
static enum INPUT_STATE {
    NONE,
    NUM,
    OPT_anchorpos
} input_state; // 1-Num

struct BUTTON
{
    char state; // judge pressed
    unsigned int x0, xx, y0, yy;
    char value[1];
};

static char INPUTRES[20];
static char INPUTRES_IDX = 0;

//* Num Pad *//
static void UI_Intput_Numpad(void);
static char judge_NUMPAD(unsigned int x, unsigned int y);
static struct BUTTON btn_NUM_num[12] = {0}; // Numbers : 1, 2, 3, 4, 5, 6, 7, 8, 9, -, 0, .
static struct BUTTON btn_NUM_cmd[3] = {0};  // Commands : Clr, Back, OK

//* Options Surface *//
static void UI_Opts_opt(char *name, char *value, char *opt, unsigned int x0, unsigned int xx, unsigned int y0, unsigned int yy);
static char judge_OPTS(unsigned int x, unsigned int y);
static void UI_Opts_cmd(void);
static struct BUTTON btn_OPTS_opt[6] = {0};
static struct BUTTON btn_OPTS_cmd[3] = {0};

//****************************************************************************************************
//*
//*  FUNCTIONS - UWB PROJECT
//*
//****************************************************************************************************

void UI_Map_Refresh(void)
{
    LCD_Fill(0, 32, lcddev.width, lcddev.height, WHITE);
    UI_Init();
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn UI_Map_Draw_Border()
 *
 * @brief  (UWB project) Draw maps border. 
 * //todo: polygon support latter
 * 
 * @param  POINT_COLOR Needed first
 */
void UI_Map_Draw_Border()
{
    LCD_DrawRectangle(map.margin_x, map.margin_y + 32, lcddev.width - map.margin_x, lcddev.height - map.margin_y);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn UI_Draw_Anchor()
 *
 * @brief  (UWB project) Draw anchor point with specified position in map region.
 *
 * @param  x  Point of x axis
 * @param  y  Point of y axis
 * @param  POINT_COLOR Needed first
 */
void UI_Map_Draw_Anchor(unsigned int x, unsigned int y, unsigned char *name)
{
    char r = 10;
    char font_size = 12;
    unsigned int posx, posy;

    posx = map.margin_x + x * map.xscale;
    posy = 32 + map.margin_y + y * map.yscale;
    LCD_Draw_Circle(posx, posy, r);
    LCD_ShowString_Transcluent(posx - 4, posy - font_size / 2, r - 2, font_size, font_size, name);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn UI_Draw_Anchor()
 *
 * @brief  (UWB project) Draw anchor point with specified position in map region.
 *
 * @param  x  Point of x axis
 * @param  y  Point of y axis
 * @param  name  Name of the Tag
 * @param  note  1-show position beside point on map
 * @param  POINT_COLOR Needed first
 */
void UI_Map_Draw_Tag(unsigned int x, unsigned int y, unsigned char *name, char note)
{
    char r = 3;
    char font_size = 12;
    unsigned int posx, posy;
    char str[20];

    if ((x <= map.width) && (y <= map.height))
    {
        posx = map.margin_x + x * map.xscale;
        posy = 32 + map.margin_y + y * map.yscale;
        POINT_COLOR = RED;
        LCD_Draw_Circle(posx, posy, r);
        if (note)
        {
            POINT_COLOR = BLACK;
            LCD_ShowString_Transcluent(posx + r, posy - r, 40, font_size, font_size, name);
            sprintf(str, "(%d,%d)", x, y);
            LCD_ShowString_Transcluent(posx + r, posy - r + 12, 80, font_size, font_size, str);
        }
    }
    POINT_COLOR = BLACK;
    LCD_Fill(60, lcddev.height - 12, 200, lcddev.height, WHITE);
    sprintf(str, "%s:(%d,%d)", name, x, y);
    LCD_ShowString(60, lcddev.height - 13, 200, 12, 12, str);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn UI_Map_Draw_Horizen_Distance()
 *
 * @brief  (UWB project) Draw the horizental distance between Anchor1 and Anchor2
 *
 * @param  dist  Distance showing
 * @param  POINT_COLOR Needed first
 */
void UI_Map_Draw_Horizen_Distance(unsigned int dist)
{
    char str[16];
    POINT_COLOR = BROWN;
    sprintf(str, "DISTANCE:%dcm", dist);
    LCD_ShowString_Transcluent(3 * map.margin_x, 32, 160, 16, 16, str);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn UI_Map_Draw_Vertical_Distance()
 *
 * @brief  (UWB project) Draw the vertical distance between Anchor2 and Anchor3
 *
 * @param  dist  Distance showing
 * @param  POINT_COLOR Needed first
 */
void UI_Map_Draw_Vertical_Distance(unsigned int dist)
{
    char i;
    char str[18];
    POINT_COLOR = BROWN;
    sprintf(str, "DISTANCE:%dcm", dist);
    for (i = 0; i < strlen(str); i++)
    {
        LCD_ShowChar(5, map.margin_y + 50 + 12 * i, str[i], 16, 1);
    }
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn UI_Map_Draw_Radius_Distance()
 *
 * @brief  (UWB project) Show the distance results of the Anchor-Tag distance
 * //* TEST ONLY *
 * @param  d  Anchor to Tag distance array
 * @param  POINT_COLOR Needed first
 */
void UI_Map_Draw_Radius_Distance(double d[3])
{
    char x = 20;
    char y = 140;
    LCD_ShowString(x, y, 200, 20, 16, "ADJ RESULT:");
    y += 20;
    LCD_ShowString(x, y, 200, 20, 16, "ANCHOR1:      cm");
    LCD_ShowNum(x + 65, y, d[0], 5, 16);
    y += 20;
    LCD_ShowString(x, y, 200, 20, 16, "ANCHOR2:      cm");
    LCD_ShowNum(x + 65, y, d[1], 5, 16);
    y += 20;
    LCD_ShowString(x, y, 200, 20, 16, "ANCHOR3:      cm");
    LCD_ShowNum(x + 65, y, d[2], 5, 16);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn UI_Map_Draw_Point()
 *
 * @brief  (UWB project) Draw a point(tag) on the map
 * //* TEST ONLY *
 * @param  point_x  Point on x
 * @param  point_y  Point on y
 * @param  POINT_COLOR Needed first
 */
void UI_Map_Draw_Point(double point_x, double point_y)
{
    char x = 20;
    char y = 220;
    LCD_ShowString(x, y, 120, 20, 16, "X:        cm");
    LCD_ShowNum(x + 16, y, point_x, 5, 16);
    y += 20;
    LCD_ShowString(x, y, 120, 20, 16, "Y:        cm");
    LCD_ShowNum(x + 16, y, point_y, 5, 16);
}

//****************************************************************************************************
//*
//*  FUNCTIONS - UNIVERSAL BASIC
//*
//****************************************************************************************************

static void UI_Display_Header(char *header)
{
    LCD_Clear(WHITE);
    LCD_Fill(0, 0, lcddev.width, 32, GREEN);
    LCD_ShowString_Transcluent(20, 8, lcddev.width, lcddev.height, 16, header);
}

void UI_Init(void)
{
    //* Header *//
    UI_Display_Header("UWB Positioning System");
    //* Map canvas *//
    map.margin_x = 15;
    map.margin_y = 15;

    UI_Map_Draw_Horizen_Distance((unsigned int)(Anchor_vec[1].x - Anchor_vec[0].x));
    UI_Map_Draw_Vertical_Distance((unsigned int)(Anchor_vec[2].y - Anchor_vec[0].y));

    //! Anchor1 as system origin by default
    float temp1, temp2, temp3;
    temp1 = Anchor_vec[1].x - Anchor_vec[0].x;
    temp2 = Anchor_vec[2].x - Anchor_vec[0].x;
    if (temp2 > temp1)
        temp3 = temp2;
    else
        temp3 = temp1;
    map.width = temp3;
    map.xscale = (lcddev.width - 2 * map.margin_x) / temp3;

    temp1 = Anchor_vec[1].y - Anchor_vec[0].y;
    temp2 = Anchor_vec[2].y - Anchor_vec[0].y;
    if (temp2 > temp1)
        temp3 = temp2;
    else
        temp3 = temp1;
    map.height = temp3;
    map.yscale = (lcddev.height - 2 * map.margin_y - 32) / temp3;

    POINT_COLOR = BLACK;
    UI_Map_Draw_Border();

    POINT_COLOR = RED;
    BACK_COLOR = GRAY;
    UI_Map_Draw_Anchor(Anchor_vec[0].x, Anchor_vec[0].y, "A1");
    UI_Map_Draw_Anchor(Anchor_vec[1].x, Anchor_vec[1].y, "A2");
    UI_Map_Draw_Anchor(Anchor_vec[2].x, Anchor_vec[2].y, "A3");
}

//****************************************************************************************************
//*
//*  FUNCTIONS - CALLBACKS
//*
//****************************************************************************************************

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn UI_Input_Button_down()
 *
 * @brief  CALLBACK from xpt2046 - Button down
 *
 * @param  x  Point on x
 * @param  y  Point on y
 */
void UI_Input_Button_down(unsigned int x, unsigned int y)
{
    if (input_state == NUM)
    {
        judge_NUMPAD(x, y);
    }
    if (input_state == OPT_anchorpos)
    {
        judge_OPTS(x, y);
    }
    delay_ms(300);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn UI_Input_Button_up()
 *
 * @brief  CALLBACK from xpt2046 - Button up
 *
 * @param  x  Point on x
 * @param  y  Point on y
 */
void UI_Input_Button_up(unsigned int x, unsigned int y)
{
    char i;
    if (input_state == NUM)
    {
        //* Clr all button pressed state
        for (i = 0; i < 12; i++)
        {
            btn_NUM_num[i].state = 0;
        }
        for (i = 0; i < 3; i++)
        {
            btn_NUM_cmd[i].state = 0;
        }
        //* Refresh Numpad
        UI_Intput_Numpad();
    }
    if (input_state == OPT_anchorpos)
    {
        for (i = 0; i < 6; i++)
        {
            btn_OPTS_opt[i].state = 0;
        }
    }
}

//****************************************************************************************************
//*
//*  FUNCTIONS - INPUT SURFACE - NUMBER
//*
//****************************************************************************************************

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn UI_Input_Num()
 *
 * @brief  Display NUMBER input pannel
 *
 * @param  header  Indicates which variable is being input
 * @param  raw     Origin variable value
 * @return Input number  
 */
float UI_Input_Num(char *header, float raw)
{
    //! If any param changes, remeber to change the corresponding pad and judge function too!
    float res;
    int y = 40;
    char i;
    input_state = NUM;
    LCD_Clear(WHITE);
    //* Header *//
    UI_Display_Header(header);
    //* Display *//
    POINT_COLOR = BLACK;
    // sprintf(INPUTRES, "%4.2f", raw);
    LCD_ShowString_Transcluent(30, y, 200, 24, 24, INPUTRES);
    // LCD_ShowNum_Float(30, y, raw, 12, 24, 0);
    y += 30;
    LCD_DrawLine(20, y, lcddev.width - 20, y);

    //* Button *//
    UI_Intput_Numpad();

    delay_ms(300); // MAGIC! Set a delay to start detect touch can optimize the page switch problem
    while (input_state == NUM)
    {
        XPT2046_TouchEvenHandler();
    }

    res = atof(INPUTRES);
    for (i = 0; i < 20; i++)
    {
        INPUTRES[i] = ' ';
    }
    INPUTRES_IDX = 0;

    return res;
}

static void UI_Intput_Numpad(void)
{
    char i, j, temp;
    char num[] = "123456789-0.";
    char addon[][4] = {"Clr", "Backups", "OK"};
    char width = 50;
    int height = 40;
    char x = (lcddev.width - 3 * width - 20 * 2) / 2; // align center
    int y = 80;
    // LCD_Fill(0, y, lcddev.width, lcddev.height, WHITE);
    POINT_COLOR = WHITE;
    // Numbers
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 3; j++)
        {
            temp = (x + j * (width + 20));
            btn_NUM_num[j + 3 * i].x0 = temp;
            btn_NUM_num[j + 3 * i].xx = temp + width;
            btn_NUM_num[j + 3 * i].y0 = y;
            btn_NUM_num[j + 3 * i].yy = y + height;
            btn_NUM_num[j + 3 * i].value[0] = num[j + 3 * i];

            if (btn_NUM_num[j + 3 * i].state == 0) // judge whether pressed
                LCD_Fill(temp, y, temp + width, y + height, GRAY);
            else
                LCD_Fill(temp, y, temp + width, y + height, BROWN);

            LCD_ShowChar((temp + (width - 8) / 2), (y + (height - 16) / 2), num[j + 3 * i], 16, 1);
        }
        y += height + 10;
    }
    // Commands
    height = 30;
    for (j = 0; j < 3; j++)
    {
        temp = (x + j * (width + 20));
        btn_NUM_cmd[j].x0 = temp;
        btn_NUM_cmd[j].xx = temp + width;
        btn_NUM_cmd[j].y0 = y;
        btn_NUM_cmd[j].yy = y + height;

        if (btn_NUM_cmd[j].state == 0) // judge whether pressed
            LCD_Fill(temp, y, temp + width, y + height, BLUE);
        else
            LCD_Fill(temp, y, temp + width, y + height, BROWN);

        LCD_ShowString_Transcluent((temp + 10), (y + (height - 16) / 2), 32, 16, 16, addon[j]);
    }
}

static char judge_NUMPAD(unsigned int x, unsigned int y)
{
    char i;
    // NUMs
    for (i = 0; i < 12; i++)
    {
        if ((x > btn_NUM_num[i].x0) && (x < btn_NUM_num[i].xx) && (y > btn_NUM_num[i].y0) && (y < btn_NUM_num[i].yy))
        {
            btn_NUM_num[i].state = 1;
            UI_Intput_Numpad();
            INPUTRES[INPUTRES_IDX++] = btn_NUM_num[i].value[0];
            POINT_COLOR = BLACK;
            LCD_Fill(20, 40, 200, 64, WHITE);
            LCD_ShowString(20, 40, 200, 24, 24, INPUTRES);
        }
    }
    // CMDs
    for (i = 0; i < 3; i++)
    {
        if ((x > btn_NUM_cmd[i].x0) && (x < btn_NUM_cmd[i].xx) && (y > btn_NUM_cmd[i].y0) && (y < btn_NUM_cmd[i].yy))
        {
            btn_NUM_cmd[i].state = 1;
            UI_Intput_Numpad();
            if (i == 0)
            {
                for (i = 0; i < 20; i++)
                {
                    INPUTRES[i] = ' ';
                }
                INPUTRES_IDX = 0;
            }
            if (i == 1)
            {
                INPUTRES_IDX -= 1;
                INPUTRES[INPUTRES_IDX] = ' ';
            }
            if (i == 2)
            {
                input_state = NONE;
            }
            POINT_COLOR = BLACK;
            LCD_Fill(20, 40, 200, 64, WHITE);
            LCD_ShowString(20, 40, 200, 24, 24, INPUTRES);
        }
    }
    return 1;
}

//****************************************************************************************************
//*
//*  FUNCTIONS - OPTIONs SURFACE - UWB PROJECT
//*
//****************************************************************************************************

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn UI_Opts_Position()
 *
 * @brief  Change Anchors popitions 
 */
void UI_Opts_Position(void)
{
    //! If any param changes, remeber to change the corresponding pad and judge function too!
    int width = lcddev.width - 20;
    int height = 32;
    int x = 10;
    int y = 40;
    char i;
    char value[20];

    LCD_Clear(WHITE);
    input_state = OPT_anchorpos;

    //* Header *//
    UI_Display_Header("Confirm Anchors Positions");

    //* OPTs *//
    // Button
    for (i = 0; i < 6; i++)
    {
        btn_OPTS_opt[i].x0 = x;
        btn_OPTS_opt[i].xx = x + width;
        btn_OPTS_opt[i].y0 = y + (height + 5) * i;
        btn_OPTS_opt[i].yy = btn_OPTS_opt[i].y0 + height;
    }
    // Display
    sprintf(value, "%4.2f", Anchor_vec[0].x);
    UI_Opts_opt("Anchor1: X", value, "Change", x, x + width, y, y + height);
    y += height + 5;
    sprintf(value, "%4.2f", Anchor_vec[0].y);
    UI_Opts_opt("Anchor1: Y", value, "Change", x, x + width, y, y + height);
    y += height + 5;
    sprintf(value, "%4.2f", Anchor_vec[1].x);
    UI_Opts_opt("Anchor2: X", value, "Change", x, x + width, y, y + height);
    y += height + 5;
    sprintf(value, "%4.2f", Anchor_vec[1].y);
    UI_Opts_opt("Anchor2: Y", value, "Change", x, x + width, y, y + height);
    y += height + 5;
    sprintf(value, "%4.2f", Anchor_vec[2].x);
    UI_Opts_opt("Anchor3: X", value, "Change", x, x + width, y, y + height);
    y += height + 5;
    sprintf(value, "%4.2f", Anchor_vec[2].y);
    UI_Opts_opt("Anchor3: Y", value, "Change", x, x + width, y, y + height);

    btn_OPTS_cmd[0].state = 1;
    btn_OPTS_cmd[1].state = 1;
    //* Commands *//
    UI_Opts_cmd(); // Refresh cmd buttons independently to indicate the ok button is pressed, due to a pretty long flash erase procedure follows

    delay_ms(300); // MAGIC! Set a delay to start detect touch can optimize the page switching problem
    while (input_state == OPT_anchorpos)
    {
        XPT2046_TouchEvenHandler();
    }
}

// Width and Height control is in the calling function, while opt button control is in this function
static void UI_Opts_opt(char *name, char *value, char *opt, unsigned int x0, unsigned int xx, unsigned int y0, unsigned int yy)
{
    unsigned int height = 32;
    unsigned int width = lcddev.width - 20;
    POINT_COLOR = BLACK;
    // Option background
    LCD_Fill(x0, y0, xx, yy, GRAY);
    LCD_ShowString_Transcluent(x0 + 5, y0, width, 16, 16, name);
    LCD_ShowString_Transcluent(x0 + 10, yy - 16, width, 16, 16, value);
    // Button
    POINT_COLOR = BLUE;
    LCD_ShowString_Transcluent(xx - 60, y0 + (height - 16) / 2, 50, 16, 16, opt);
    POINT_COLOR = BLACK;
}

static void UI_Opts_cmd(void)
{
    char i;
    int width, height, x, y, temp;
    char addon[][4] = {"Prev", "Next", "OK"};
    width = 50;
    height = 30;
    POINT_COLOR = WHITE;
    x = (lcddev.width - 3 * width - 20 * 2) / 2;
    y = lcddev.height - height - 10;

    for (i = 0; i < 3; i++)
    {
        temp = (x + i * (width + 20));
        btn_OPTS_cmd[i].x0 = temp;
        btn_OPTS_cmd[i].xx = temp + width;
        btn_OPTS_cmd[i].y0 = y;
        btn_OPTS_cmd[i].yy = y + height;

        if (btn_OPTS_cmd[i].state == 1)
            LCD_Fill(temp, y, temp + width, y + height, GRAY); // BUTTON does not work
        else if (btn_OPTS_cmd[i].state == 0)
            LCD_Fill(temp, y, temp + width, y + height, BLUE); // BUTTON can be touched
        else if (btn_OPTS_cmd[i].state == 2)
            LCD_Fill(temp, y, temp + width, y + height, BROWN); // BUTTON is touched

        LCD_ShowString_Transcluent((temp + 10), (y + (height - 16) / 2), 32, 16, 16, addon[i]);
    }
    POINT_COLOR = BLACK;
}

static char judge_OPTS(unsigned int x, unsigned int y)
{
    char i;
    //! When using SWITCH()
    // The i whill end at 7, so that there is no conflict between the last opt and no opt.
    for (i = 0; i < 6; i++)
    {
        if ((x > btn_OPTS_opt[i].x0) && (x < btn_OPTS_opt[i].xx) && (y > btn_OPTS_opt[i].y0) && (y < btn_OPTS_opt[i].yy))
        {
            btn_OPTS_opt[i].state = 1;
            break;
        }
    }
    switch (i)
    {
    case 0:
        Anchor_vec[0].x = UI_Input_Num("ANCHOR1 : X (cm)", Anchor_vec[0].x);
        break;
    case 1:
        Anchor_vec[0].y = UI_Input_Num("ANCHOR1 : Y (cm)", Anchor_vec[0].y);
        break;
    case 2:
        Anchor_vec[1].x = UI_Input_Num("ANCHOR2 : X (cm)", Anchor_vec[1].x);
        break;
    case 3:
        Anchor_vec[1].y = UI_Input_Num("ANCHOR2 : Y (cm)", Anchor_vec[1].y);
        break;
    case 4:
        Anchor_vec[2].x = UI_Input_Num("ANCHOR3 : X (cm)", Anchor_vec[2].x);
        break;
    case 5:
        Anchor_vec[2].y = UI_Input_Num("ANCHOR3 : Y (cm)", Anchor_vec[2].y);
        break;
    default:
        break;
    }
    if (i < 6)
    {
        UI_Opts_Position();
    }

    for (i = 0; i < 3; i++)
    {
        if ((x > btn_OPTS_cmd[i].x0) && (x < btn_OPTS_cmd[i].xx) && (y > btn_OPTS_cmd[i].y0) && (y < btn_OPTS_cmd[i].yy))
        {
            btn_OPTS_cmd[i].state = 2;
            break;
        }
    }
    switch (i)
    {
    case 0:
        /* Prev func */
        break;
    case 1:
        /* Next func */
        break;
    case 2:
        input_state = NONE;
        UI_Opts_cmd();
        break;
    default:
        break;
    }

    return 1;
}
