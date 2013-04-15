#include "matrix.h"

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include "rendering.h"
#include <XPLMDisplay.h>
#endif

/*******************************************************************************
 * Traces currently picked ray
 ******************************************************************************/
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void trace_get_view_ray(float outNear[4], float outFar[4])
{
	float view[16], proj[16];
	float invViewProj[16], viewProj[16];
	float inV[4];
	int mx,my,sx,sy;
	glGetFloatv(GL_MODELVIEW_MATRIX,  view);
	glGetFloatv(GL_PROJECTION_MATRIX, proj);

	mult_matrix(viewProj,view,proj);
	invert_matrix(invViewProj,viewProj);

	XPLMGetMouseLocation(&mx,&my);
	XPLMGetScreenSize(&sx,&sy);
	sx /= 2; sy /= 2;

	inV[0] = 1.0f*(mx-sx)/(1.0f*sx);
	inV[1] = 1.0f*(my-sy)/(1.0f*sy);
	inV[2] = -1.0f;
	inV[3] = 1.0f;
	transform_vector(outNear,invViewProj,inV);

	inV[2] = 1.0f;
	transform_vector(outFar,invViewProj,inV);
}
#endif


/*******************************************************************************
 * Clockness test for segment vs triangle
 ******************************************************************************/
int trace_clockness_test(float p1[3], float p2[3], float p3[3], float tri_normal[3])
{
	float test_normal[3];

	//quick cross prouct
	test_normal[0] = ((p2[1] - p1[1])*(p3[2] - p1[2])) - ((p3[1] - p1[1])*(p2[2] - p1[2]));
	test_normal[1] = ((p2[2] - p1[2])*(p3[0] - p1[0])) - ((p3[2] - p1[2])*(p2[0] - p1[0]));
	test_normal[2] = ((p2[0] - p1[0])*(p3[1] - p1[1])) - ((p3[0] - p1[0])*(p2[1] - p1[1]));

	if ((test_normal[0]*tri_normal[0]+
	        test_normal[1]*tri_normal[1]+
	        test_normal[2]*tri_normal[2]) < 0) return 0;
	else return 1; //same clockness
}


/*******************************************************************************
 * Traces currently picked ray
 ******************************************************************************/
int trace_segment_vs_triangle(float p1[3], float p2[3], float p3[3], float line_start[3], float line_end[3], float collision_point[3])
{
	float V1[3], V2[3];
	float tri_normal[3], line_vector[3];
	float nvdot;

	//triangle vectors
	V1[0] = p2[0] - p1[0];
	V1[1] = p2[1] - p1[1];
	V1[2] = p2[2] - p1[2];
	V2[0] = p3[0] - p2[0];
	V2[1] = p3[1] - p2[1];
	V2[2] = p3[2] - p2[2];

	tri_normal[0] = V1[1]*V2[2] - V1[2]*V2[1];
	tri_normal[1] = V1[2]*V2[0] - V1[0]*V2[2];
	tri_normal[2] = V1[0]*V2[1] - V1[1]*V2[0];

	//Invalid triangle
	if ((tri_normal[0]*tri_normal[0]+
	        tri_normal[1]*tri_normal[1]+
	        tri_normal[2]*tri_normal[2]) == 0) return 0;

	line_vector[0] = line_end[0] - line_start[0];
	line_vector[1] = line_end[1] - line_start[1];
	line_vector[2] = line_end[2] - line_start[2];

	nvdot = (tri_normal[0]*line_vector[0]+
	         tri_normal[1]*line_vector[1]+
	         tri_normal[2]*line_vector[2]);

	//dot product of normal and line's vector if zero line is parallel to triangle
	if (nvdot < 0) {
		int t1,t2,t3,t4;

		//distance
		float distance = -(tri_normal[0]*(line_start[0]-p1[0])+
		                   tri_normal[1]*(line_start[1]-p1[1])+
		                   tri_normal[2]*(line_start[2]-p1[2]))/nvdot;

		//line is past triangle
		if (distance < 0) return 0;

		//segment doesn't reach triangle
		//if (distance > 1) return 0;

		collision_point[0] = line_start[0] + line_vector[0] * distance;
		collision_point[1] = line_start[1] + line_vector[1] * distance;
		collision_point[2] = line_start[2] + line_vector[2] * distance;

		t1 = trace_clockness_test(p1,p2,collision_point, tri_normal);
		t2 = trace_clockness_test(p2,p3,collision_point, tri_normal);
		t3 = trace_clockness_test(p3,p1,collision_point, tri_normal);
		t4 = 1;

		if (t1 && t2 && t3 && t4) {
			return 1;
		}
	}
	return 0;
}