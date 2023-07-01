#include <cmath>
#include "positioning.h"

#ifndef EPSILON
#define EPSILON 3
#endif


bool calculateThreeCircleIntersection(double x0, double y0, double r0,
                                      double x1, double y1, double r1,
                                      double x2, double y2, double r2,
                                      double* result_x, double* result_y)
{
  double a, dx, dy, d, h, rx, ry;
  double point2_x, point2_y;

  /* dx and dy are the vertical and horizontal distances between
   * the circle centers.
   */
  dx = x1 - x0;
  dy = y1 - y0;

  /* Determine the straight-line distance between the centers. */
  d = sqrt((dy * dy) + (dx * dx));

  /* Check for solvability. */
  if (d > (r0 + r1))
  {
    /* no solution. circles do not intersect. */
    return false;
  }
  if (d < abs(r0 - r1))
  {
    /* no solution. one circle is contained in the other */
    return false;
  }

  /* 'point 2' is the point where the line through the circle
   * intersection points crosses the line between the circle
   * centers.
   */

  /* Determine the distance from point 0 to point 2. */
  a = ((r0 * r0) - (r1 * r1) + (d * d)) / (2.0 * d);

  /* Determine the coordinates of point 2. */
  point2_x = x0 + (dx * a / d);
  point2_y = y0 + (dy * a / d);

  /* Determine the distance from point 2 to either of the
   * intersection points.
   */
  h = sqrt((r0 * r0) - (a * a));

  /* Now determine the offsets of the intersection points from
   * point 2.
   */
  rx = -dy * (h / d);
  ry = dx * (h / d);

  /* Determine intersection points. */
  double intersecPoint1_x = point2_x + rx;
  double intersecPoint2_x = point2_x - rx;
  double intersecPoint1_y = point2_y + ry;
  double intersecPoint2_y = point2_y - ry;

  /* Lets determine if circle 3 intersects at either of the above intersection points. */
  dx = intersecPoint1_x - x2;
  dy = intersecPoint1_y - y2;
  double d1 = sqrt((dy * dy) + (dx * dx));

  dx = intersecPoint2_x - x2;
  dy = intersecPoint2_y - y2;
  double d2 = sqrt((dy * dy) + (dx * dx));

  if (abs(d1 - r2) < EPSILON)
  {
    *result_x = intersecPoint1_x;
    *result_y = intersecPoint1_y;
    return true;
  }
  else if (abs(d2 - r2) < EPSILON)
  {
    *result_x = intersecPoint2_x;
    *result_y = intersecPoint2_y;
    return true;
  }

  /* The individual distances between the intersection points and the radius of the third beacon are tot great. */
  return false;
}