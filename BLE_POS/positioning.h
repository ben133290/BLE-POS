#ifndef POSITIONING_H
#define POSITIONING_H

#include <cmath>
#include <Arduino.h>

#ifndef EPSILON
#define EPSILON 3
#endif

bool calculateThreeCircleIntersection(double x0, double y0, double r0,
                                      double x1, double y1, double r1,
                                      double x2, double y2, double r2,
                                      double* result_x, double* result_y);

#endif // POSITIONING_H
