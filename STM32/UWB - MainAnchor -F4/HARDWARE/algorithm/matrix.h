#ifndef _MATRIX_H
#define _MATRIX_H

extern void MatrixAdd(float *a, float *b, float *c, unsigned char m, unsigned char n);
extern void MatrixMinus(float *a, float *b, float *c, unsigned char m, unsigned char n);
extern void MatrixMul(float *a, float *b, float *c, unsigned char m, unsigned char p, unsigned char n);
extern void MatrixTrans(float *a, float *c, unsigned char m, unsigned char n);
extern float MatrixDet1(float *a, unsigned char m, unsigned char n);
extern void MatrixInv1(float *a, float *c, unsigned char m, unsigned char n);
extern unsigned char Gauss_Jordan(float *a, unsigned char n);
extern void MatrixCal(float *a, float *b, float *c, unsigned char n);
extern void Matrixmulc(float *a, float b, float *c, unsigned char m, unsigned char n);
float Matrixvnorm(float *a, unsigned char m);
extern void MatrixMinus_1(float *a, float *b, float *c, unsigned char m, unsigned char n);
extern void Matrixpointex(float *a, float *b, float *c, unsigned char m, unsigned char p, unsigned char n);
extern void Matrixdistance(float *a, float *b, float *distance, unsigned char m, unsigned char n);
extern void MatrixInv2(float *a, float *c);
extern void blkdiag(float *x1, float *x2, float *x3, float *x4, float *x5);
extern void MatrixInv3(float *a, float *c);

#endif
