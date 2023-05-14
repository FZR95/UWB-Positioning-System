#include <math.h>
#include "trilateration_basic.h"
#include "UI.h"

struct vec3d Anchor_vec[3];
struct vec2d Result_2d;

static double norm(struct vec2d point)
{
    return pow(pow(point.x, 2) + pow(point.y, 2), 0.5);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn Trilateration_Basic()
 *
 * @brief Calcuate single Tag x,y,z use basic TRILATERATION algorithm
 *
 * @param  distance  follows the sequence of Anchor1,2,3
 * @param  Anchor_vec  <MUST BE PRESET> Struct of vec3d, indicates the position of every anchor. 
 */
void Trilateration_Basic(double distance[3])
{
    char k;
    UI_Map_Draw_Horizen_Distance((unsigned int)(Anchor_vec[1].x - Anchor_vec[0].x));
    UI_Map_Draw_Vertical_Distance((unsigned int)(Anchor_vec[2].y - Anchor_vec[0].y));

    //* Disposal distance by experience
    for (k = 0; k < 3; k++)
    {
        if (distance[k] < 30)
            distance[k];
        else if (distance[k] < 110)
            distance[k] -= 40;
        else
            distance[k] -= 40;
    }
    UI_Map_Draw_Radius_Distance(distance);

    //! only calc 2d
    //* Distance : Anchor2 to Anchor1
    double D_A2A1 = pow(pow(Anchor_vec[1].x - Anchor_vec[0].x, 2) + pow(Anchor_vec[1].y - Anchor_vec[0].y, 2), 0.5);
    //* Normalized Vector on X axis : Anchor1 to Anchor2
    struct vec2d ex = {(Anchor_vec[1].x - Anchor_vec[0].x) / D_A2A1, (Anchor_vec[1].y - Anchor_vec[0].y) / D_A2A1};
    //* Vector : Anchor3 to Anchor1
    struct vec2d aux = {Anchor_vec[2].x - Anchor_vec[0].x, Anchor_vec[2].y - Anchor_vec[0].y};
    //* The component of aux on X axis
    double i = ex.x * aux.x + ex.y * aux.y;
    //* Get the component of aux on Y axis
    struct vec2d aux2 = {Anchor_vec[2].x - Anchor_vec[0].x - i * ex.x, Anchor_vec[2].y - Anchor_vec[0].y - i * ex.y};
    //* Normalrized Vector on Y axis : Anchor3 to Anchor1
    double norm_aux2 = norm(aux2);
    struct vec2d ey = {aux2.x / norm_aux2, aux2.y / norm_aux2};
    //* The component of aux on Y axis
    double j = ey.x * aux.x + ey.y * aux.y;

    double x = (pow(distance[0], 2) - pow(distance[1], 2) + pow(D_A2A1, 2)) / (2 * D_A2A1);
    double y = (pow(distance[0], 2) - pow(distance[3], 2) + pow(i, 2) + pow(j, 2)) / (2 * j) - i * x / j;

    double finalX = Anchor_vec[0].x + x * ex.x + y * ey.x;
    double finalY = Anchor_vec[0].y + x * ex.y + y * ey.y;
    Result_2d.x = finalX;
    Result_2d.y = finalY;
    if (finalY < 0)
        Result_2d.y = 888;
}
