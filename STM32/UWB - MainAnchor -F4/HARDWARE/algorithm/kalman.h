#ifndef _KALMAN_H
#define _KALMAN_H


#define   LENGTH      1*1
#define   ORDER       1
#define   N           100
#define   SEED        1567

//================================================//
//==               最优值结构体                 ==//
//================================================//
typedef struct  _tOptimal
{
  float XNowOpt[LENGTH];
  float XPreOpt[LENGTH];
}tOptimal;

extern void   KalMan_PramInit(void);
extern float KalMan_Update(double *Z);


#endif

