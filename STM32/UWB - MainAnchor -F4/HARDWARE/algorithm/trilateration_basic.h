#ifndef _TRILATERATION_BASIC_H
#define _TRILATERATION_BASIC_H

// UNIT : cm
struct vec3d
{
    double x, y, z;
};

// UNIT : cm
struct vec2d
{
    double x, y;
};

extern struct vec3d Anchor_vec[3];
extern struct vec2d Result_2d;

void Trilateration_Basic(double distance[3]);
#endif
