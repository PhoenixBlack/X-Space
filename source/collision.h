#ifndef COLLISION_H
#define COLLISION_H

void trace_get_view_ray(float outNear[4], float outFar[4]);
int trace_clockness_test(float p1[3], float p2[3], float p3[3], float tri_normal[3]);
int trace_segment_vs_triangle(float p1[3], float p2[3], float p3[3], float line_start[3], float line_end[3], float collision_point[3]);

#endif