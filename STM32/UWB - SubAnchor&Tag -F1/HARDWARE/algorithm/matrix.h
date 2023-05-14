#ifndef _MATRIX_H
#define _MATRIX_H


extern void   MatrixAdd(float *a, float *b, float *c, unsigned char m, unsigned char n);
extern void   MatrixMinus(double *a, float *b, float *c, unsigned char m, unsigned char n);
extern void   MatrixMul(float *a, float *b, float *c, unsigned char m, unsigned char p, unsigned char n);
extern void   MatrixTrans(float *a, float *c, unsigned char m, unsigned char n);
extern float  MatrixDet1(float *a, unsigned char m, unsigned char n);
extern void   MatrixInv1(float *a, float *c, unsigned char m, unsigned char n);
extern unsigned char Gauss_Jordan(float *a, unsigned char n);
extern void   MatrixCal(float *a, float *b, float *c, unsigned char n);


#endif

