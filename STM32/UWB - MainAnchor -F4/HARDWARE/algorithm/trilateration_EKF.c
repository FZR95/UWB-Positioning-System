#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"
#include <math.h>
#include "trilateration_basic.h"
#include "trilateration_EKF.h"
#include "UI.h"

static int N = 25;
static float T = 1;
static float Sf = 1;
static float Sg = 0.01;
static float sigma = 2;
static float Qb[4] = {0.00037, 0.00001, 0.00001, 0.00002}; //{Sf*T+Sg*T*T*T/3, Sg*T*T/2, Sg*T*T/2, Sg*T}
static float Qxyz[4] = {1.3333, 2.000, 2.000, 4.0000};	   //{pow(sigma,2)*pow(T,3)/3, pow(sigma,2)*pow(T,2)/2, pow(sigma,2)*pow(T,2)/2, pow(sigma,2)*T};
static float eye8[64] = {1, 0, 0, 0, 0, 0, 0, 0,
						 0, 1, 0, 0, 0, 0, 0, 0,
						 0, 0, 1, 0, 0, 0, 0, 0,
						 0, 0, 0, 1, 0, 0, 0, 0,
						 0, 0, 0, 0, 1, 0, 0, 0,
						 0, 0, 0, 0, 0, 1, 0, 0,
						 0, 0, 0, 0, 0, 0, 1, 0,
						 0, 0, 0, 0, 0, 0, 0, 1};
float P[64] = {10, 0, 0, 0, 0, 0, 0, 0,
			   0, 10, 0, 0, 0, 0, 0, 0,
			   0, 0, 10, 0, 0, 0, 0, 0,
			   0, 0, 0, 10, 0, 0, 0, 0,
			   0, 0, 0, 0, 10, 0, 0, 0,
			   0, 0, 0, 0, 0, 10, 0, 0,
			   0, 0, 0, 0, 0, 0, 10, 0,
			   0, 0, 0, 0, 0, 0, 0, 10};
static float Rhoerror = 5;
float R[9] = {1 * 5, 0, 0,
			  0, 1 * 5, 0,
			  0, 0, 1 * 5};
float G[64] = {1, 0, 0, 0, 0, 0, 0, 0,
			   0, 1, 0, 0, 0, 0, 0, 0,
			   0, 0, 1, 0, 0, 0, 0, 0,
			   0, 0, 0, 1, 0, 0, 0, 0,
			   0, 0, 0, 0, 1, 0, 0, 0,
			   0, 0, 0, 0, 0, 1, 0, 0,
			   0, 0, 0, 0, 0, 0, 1, 0,
			   0, 0, 0, 0, 0, 0, 0, 1};

static float Q[64] = {0};
static float F[64] = {0};
static float FP[64] = {0};
static float FPF_trans[64] = {0};
static float GQG_trans[64] = {0};
static float GQ[64] = {0};
static float G_trans[64] = {0};
static float F_trans[64] = {0};
static float Val[3] = {0};
static float H[24] = {0};
static float H_trans[24] = {0};
static float PH_trans[24] = {0};
static float HP[24] = {0};
static float HPH_trans[9] = {0};
static float HPH_transR[9] = {0};
static float HPH_transR_inv[9] = {0};
static float pr_Val[3] = {0};
static float K[24] = {0};
static float Kpr_Val[8] = {0};
static float XKpr_Val[8] = {0};
static float KH[64] = {0};
static float eye8KH[64] = {0};
static float Fx[8] = {0};
static float Peye8KH[64] = {0};

//入口参数 x,dt
//出口参数 x
void state_equ(float *x, float dt, float *Fx)
{
	float F[64] = {0};
	float Jacob[4] = {1, dt, 0, 1};
	blkdiag(Jacob, Jacob, Jacob, Jacob, F); //F:8*8
	MatrixMul(F, x, Fx, 8, 8, 1);
}
//入口参数 x,dt
//出口参数 F,G
void state_propagation(float dt, float *F)
{
	float Jacob[4] = {1, dt, 0, 1};

	//F[64] = {0};
	blkdiag(Jacob, Jacob, Jacob, Jacob, F); //F:8*8
}
//入口参数x,SV
//出口参数Val,H
void measurement_gps(float *x, float *SV, float *Val, float *H)
{
	// Each row of SV is the coordinate of a satellite.
	float dX[6] = {0};
	//H[24] = {0};
	int i = 0;

	dX[0] = x[0] - SV[0];
	dX[1] = x[2] - SV[1];
	dX[2] = x[0] - SV[2];
	dX[3] = x[2] - SV[3];
	dX[4] = x[0] - SV[4];
	dX[5] = x[2] - SV[5];

	Val[0] = pow(pow(dX[0], 2) + pow(dX[1], 2), 0.5) + x[4];
	Val[1] = pow(pow(dX[2], 2) + pow(dX[3], 2), 0.5) + x[4];
	Val[2] = pow(pow(dX[4], 2) + pow(dX[5], 2), 0.5) + x[4];

	H[0] = dX[0] / Val[0];
	H[2] = dX[1] / Val[0];
	H[8] = dX[2] / Val[1];
	H[10] = dX[3] / Val[1];
	H[16] = dX[4] / Val[2];
	H[18] = dX[5] / Val[2];
	H[4] = 1;
	H[12] = 1;
	H[20] = 1;

	for (i = 0; i < 24; i++)
	{
		if (i != 0 && i != 2 && i != 8 && i != 10 && i != 16 && i != 18 && i != 4 && i != 12 && i != 20)
		{
			H[i] = 0;
		}
	}
}
//============================================================================//
//==                          卡尔曼滤波                                    ==//
//============================================================================//
//==入口参数: 基站位置sv_pos、目标与各基站距离pr                            ==//
//==出口参数: 当前时刻的最优值pos                                           ==//
//============================================================================//
void algor_EKF(float *X, float *pos, float *sv_pos, float *pr)
{
	char k = 1;
	int i = 0;

	blkdiag(Qxyz, Qxyz, Qxyz, Qb, Q);
	state_equ(X, T, Fx);
	Matrixmulc(Fx, 1, X, 8, 1);
	state_propagation(T, F);
	//Time update of the Kalman filter state covariance.
	MatrixTrans(G, G_trans, 8, 8);
	MatrixTrans(F, F_trans, 8, 8);
	MatrixMul(F, P, FP, 8, 8, 8);
	MatrixMul(FP, F_trans, FPF_trans, 8, 8, 8);
	MatrixMul(G, Q, GQ, 8, 8, 8);
	MatrixMul(GQ, G_trans, GQG_trans, 8, 8, 8);
	MatrixAdd(FPF_trans, GQG_trans, P, 8, 8);
	//measument update
	measurement_gps(X, sv_pos, Val, H);
	MatrixTrans(H, H_trans, 3, 8);
	MatrixMul(P, H_trans, PH_trans, 8, 8, 3);
	MatrixMul(H, P, HP, 3, 8, 8);
	MatrixMul(HP, H_trans, HPH_trans, 3, 8, 3);
	MatrixAdd(HPH_trans, R, HPH_transR, 3, 3);
	MatrixInv3(HPH_transR, HPH_transR_inv);
	MatrixMul(PH_trans, HPH_transR_inv, K, 8, 3, 3);
	//update state
	MatrixMinus(pr, Val, pr_Val, 3, 1);
	MatrixMul(K, pr_Val, Kpr_Val, 8, 3, 1);
	MatrixAdd(X, Kpr_Val, X, 8, 1);
	//Update the Kalman filter state covariance.
	MatrixMul(K, H, KH, 8, 3, 8);
	MatrixMinus(eye8, KH, eye8KH, 8, 8);
	MatrixMul(eye8KH, P, Peye8KH, 8, 8, 8);
	Matrixmulc(Peye8KH, 1, P, 8, 8);
	//log data
	//positioning using Kalman Filter
	pos[0] = X[0];
	pos[1] = X[2];
	//Result
	Result_2d.x = (double)pos[0] - 10;
	Result_2d.y = (double)pos[1] - 10;

	printf("TAG LOC My1:x = %4.0f,y = %4.0f\r\n", pos[0] - 10, pos[1] - 10);
}

int Trilateration_EKF(double distance[3])
{
	char k;
	float X[8] = {0};
	float pos[2] = {0};							  //目标位置
	float sv_pos[6] = {0}; //基站位置坐标组成数组
	float pr[3] = {0};

	X[0] = (Anchor_vec[1].x - Anchor_vec[0].x) / 2 + 10;
	X[2] = (Anchor_vec[2].y - Anchor_vec[0].y) / 2 + 10; //Initial position
	X[1] = 0;
	X[3] = 0; //Initial velocity
	X[4] = 0;
	X[5] = 0; //Initial clock bias  //Initial clock drift

	//基站位置数组
	sv_pos[0] = (float)Anchor_vec[0].x + 10;
	sv_pos[1] = (float)Anchor_vec[0].y + 10;
	sv_pos[2] = (float)Anchor_vec[1].x + 10;
	sv_pos[3] = (float)Anchor_vec[1].y + 10;
	sv_pos[4] = (float)Anchor_vec[2].x + 10;
	sv_pos[5] = (float)Anchor_vec[2].y + 10;

	//distance
	for (k = 0; k < 3; k++)
	{
		pr[k] = (float)distance[k];
	}
	
	//UI_Map_Draw_Radius_Distance(distance);
	printf("\n");
	//printf("ANCHOR1:X:%3.2f,Y:%3.2f\n", sv_pos[0] - 10, sv_pos[1] - 10);
	//printf("ANCHOR2:X:%3.2f,Y:%3.2f\n", sv_pos[2]-10, sv_pos[3]-10);
	//printf("ANCHOR3:X:%3.2f,Y:%3.2f\n", sv_pos[4]-10, sv_pos[5]-10);
	printf("------EKF_PROCESS--\nDistance:\nAnchor1:%4.0f,\nAnchor2:%4.0f,\nAnchor3:%4.0f.\n", distance[0], distance[1], distance[2]);

	algor_EKF(X, pos, sv_pos, pr);
	// algor_EKF(X, pos, sv_pos, pr);
	// algor_EKF(X, pos, sv_pos, pr);
	// algor_EKF(X, pos, sv_pos, pr);
	// algor_EKF(X, pos, sv_pos, pr);
	//UI_Map_Draw_Point(Result_2d.x, Result_2d.y);

	return 0;
}
