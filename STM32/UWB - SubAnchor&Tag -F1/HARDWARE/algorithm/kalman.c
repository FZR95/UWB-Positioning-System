/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"	
#include "kalman.h"	
#include "matrix.h"				 				 		

/*==============================================================================
1.预估计
   X(k|k-1) = F(k,k-1)*X(k-1|k-1)        //控制量为0


2.计算预估计协方差矩阵
   P(k|k-1) = F(k,k-1)*P(k-1|k-1)*F(k,k-1)'+Q(k)
   Q(k) = U(k)×U(k)' 


3.计算卡尔曼增益矩阵
   Kg(k) = P(k|k-1)*H' / (H*P(k|k-1)*H' + R(k))
   R(k) = N(k)×N(k)' 


4.更新估计
   X(k|k) = X(k|k-1)+Kg(k)*(Z(k)-H*X(k|k-1))


5.计算更新后估计协防差矩阵
   P(k|k) =（I-Kg(k)*H）*P(k|k-1)


6. 更新最优值


F(k,k-1):     状态转移矩阵
X(k|k-1):     根据k-1时刻的最优值估计k时刻的值
X(k-1|k-1):   k-1时刻的最优值
P(k|k-1):     X(k|k-1)对应的covariance
P(k-1|k-1):   X(k-1|k-1)对应的covariance
Q(k):         系统过程的covariance
R(k):         测量过程的协方差
H(k):         观测矩阵转移矩阵
Z(k):         k时刻的测量值


基本思路: 首先根据上一次(如果是第一次则根据预赋值计算)的数据计算出本次的估计值,
          同理,根据上一次的数据计算出本次估计值的协方差;  接着,由本次估计值的协
          方差计算出卡尔曼增益;  最后,根据估测值和测量值计算当前最优值及其协方差
==============================================================================*/



//================================================//
//==             最优值方差结构体               ==//
//================================================//
typedef struct  _tCovariance
{
  float PNowOpt[LENGTH];
  float PPreOpt[LENGTH];
}tCovariance;







static tOptimal      tOpt;
static tCovariance   tCov;
//float         Z[LENGTH]  = {4000};           //  测量值(每次测量的数据需要存入该数组)
static float         I[LENGTH]  = {1};              //  单位矩阵
static float         X[LENGTH]  = {9.8};              //  当前状态的预测值
static float         P[LENGTH]  = {0};              //  当前状态的预测值的协方差
static float         K[LENGTH]  = {0};              //  卡尔曼增益
static float         Temp3[LENGTH] = {0};           //  辅助变量
//============================================================================//
//==                    卡尔曼滤波需要配置的变量                            ==//
//============================================================================//
static float         F[LENGTH]  = {1};              //  状态转移矩阵   即：坚信当前的状态与上一次状态的关系，如果坚信是一样的，则状态转移矩阵就为单位矩阵
static float         Q[LENGTH]  = {0.0001f};//0.0001f              //  系统过程的协方差	协方差的定义：真实值与期望值之差的平方的期望值
static float         R[LENGTH]  = {2};              //  测量过程的协方差	协方差的定义：真实值与期望值之差的平方的期望值   
//如果你需要滤波结果更依赖于观测量，那就调小R，增大Q；反之，调大R，调小Q，这样估计值就取决于系统。
//如果R大Q小，就是说，状态估计值比测量值要可靠，这时，所得出的结果就是更接近估计值；
//如果R小Q大，这时，计算出来的结果就会更接近测量值。
static float         H[LENGTH]  = {1};              //  观测矩阵转移矩阵	测量值与状态预测值之间的单位换算关系，即把预测值单位换算成测量值单位
static float         Temp1[LENGTH] = {1};           //  辅助变量, 同时保存tOpt.XPreOpt[]的初始化值
static float         Temp2[LENGTH] = {10000};       //  辅助变量, 同时保存tCov.PPreOpt[]的初始化值


void KalMan_PramInit(void)
{
  unsigned char   i;
  
  for (i=0; i<LENGTH; i++)
  {
    tOpt.XPreOpt[i] = Temp1[i];           //零值初始化
  }
  for (i=0; i<LENGTH; i++)
  {
    tCov.PPreOpt[i] = Temp2[i];           //零值初始化
  }
}


//============================================================================//
//==                          卡尔曼滤波                                    ==//
//============================================================================//
//==入口参数: 当前时刻的测量值                                                            ==//
//==出口参数: 当前时刻的最优值                                                            ==//
//==返回值:   当前时刻的最优值                                                            ==//
//============================================================================//
float KalMan_Update(double *Z)
{
	u8 i;  
	MatrixMul(F, tOpt.XPreOpt, X, ORDER, ORDER, ORDER);       //  基于系统的上一状态而预测现在状态; X(k|k-1) = F(k,k-1)*X(k-1|k-1)

	MatrixCal(F, tCov.PPreOpt, Temp1, ORDER);
	MatrixAdd(Temp1, Q, P, ORDER, ORDER);                     //  预测数据的协方差矩阵; P(k|k-1) = F(k,k-1)*P(k-1|k-1)*F(k,k-1)'+Q

	MatrixCal(H, P, Temp1, ORDER);
	MatrixAdd(Temp1, R, Temp1, ORDER, ORDER);
	Gauss_Jordan(Temp1, ORDER);
	MatrixTrans(H, Temp2, ORDER, ORDER);
	MatrixMul(P, Temp2, Temp3, ORDER, ORDER, ORDER);
	MatrixMul(Temp1, Temp3, K, ORDER, ORDER, ORDER);          //  计算卡尔曼增益; Kg(k) = P(k|k-1)*H' / (H*P(k|k-1)*H' + R)

	MatrixMul(H, X, Temp1, ORDER, ORDER, ORDER);
	MatrixMinus(Z, Temp1, Temp1, ORDER, ORDER);
	MatrixMul(K, Temp1, Temp2, ORDER, ORDER, ORDER);
	MatrixAdd(X, Temp2, tOpt.XNowOpt, ORDER, ORDER);          //  根据估测值和测量值计算当前最优值; X(k|k) = X(k|k-1)+Kg(k)*(Z(k)-H*X(k|k-1))

	MatrixMul(K, H, Temp1, ORDER, ORDER, ORDER);
	MatrixMinus((double *)I, Temp1, Temp1, ORDER, ORDER);
	MatrixMul(Temp1, P, tCov.PNowOpt, ORDER, ORDER, ORDER);   //  计算更新后估计协防差矩阵; P(k|k) =（I-Kg(k)*H）*P(k|k-1)

	for (i=0; i<LENGTH; i++)
	{
	  tOpt.XPreOpt[i] = tOpt.XNowOpt[i];
	  tCov.PPreOpt[i] = tCov.PNowOpt[i];
	}
	
	return tOpt.XNowOpt[0];
}
