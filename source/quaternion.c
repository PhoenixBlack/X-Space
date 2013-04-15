#include "quaternion.h"
#include "math.h"

void qmul(quaternion q, quaternion q1,quaternion q2)
{
	double q11 = q1[0];
	double q12 = q1[1];
	double q13 = q1[2];
	double q14 = q1[3];

	double q21 = q2[0];
	double q22 = q2[1];
	double q23 = q2[2];
	double q24 = q2[3];

	q[0] = q11 * q21 - q12 * q22 - q13 * q23 - q14 * q24;
	q[1] = q11 * q22 + q12 * q21 + q13 * q24 - q14 * q23;
	q[2] = q11 * q23 - q12 * q24 + q13 * q21 + q14 * q22;
	q[3] = q11 * q24 + q12 * q23 - q13 * q22 + q14 * q21;
}

void qmul_vec(quaternion q, quaternion q1, double x, double y, double z)
{
	double q11 = q1[0];
	double q12 = q1[1];
	double q13 = q1[2];
	double q14 = q1[3];

	double q22 = x;
	double q23 = y;
	double q24 = z;

	q[0] =           - q12 * q22 - q13 * q23 - q14 * q24;
	q[1] = q11 * q22             + q13 * q24 - q14 * q23;
	q[2] = q11 * q23 - q12 * q24             + q14 * q22;
	q[3] = q11 * q24 + q12 * q23 - q13 * q22;
}

void qdiv(quaternion q, quaternion q1,quaternion q2)
{
	double q11 = q1[0];
	double q12 = q1[1];
	double q13 = q1[2];
	double q14 = q1[3];

	double q21 = q2[0];
	double q22 = q2[1];
	double q23 = q2[2];
	double q24 = q2[3];

	q[0] =   q11 * q21 + q12 * q22 + q13 * q23 + q14 * q24;
	q[1] = - q11 * q22 + q12 * q21 - q13 * q24 + q14 * q23;
	q[2] = - q11 * q23 + q12 * q24 + q13 * q21 - q14 * q22;
	q[3] = - q11 * q24 - q12 * q23 + q13 * q22 + q14 * q21;
}

void qconj(quaternion q1, quaternion q2)
{
	q1[0] =  q2[0];
	q1[1] = -q2[1];
	q1[2] = -q2[2];
	q1[3] = -q2[3];
}

void qeuler_from(quaternion q, double x, double y, double z)
{
	double c1 = cos(x*0.5);
	double c2 = cos(y*0.5);
	double c3 = cos(z*0.5);

	double s1 = sin(x*0.5);
	double s2 = sin(y*0.5);
	double s3 = sin(z*0.5);

	q[0] = c1*c2*c3 + s1*s2*s3;
	q[1] = s1*c2*c3 - c1*s2*s3;
	q[2] = c1*s2*c3 + s1*c2*s3;
	q[3] = c1*c2*s3 - s1*s2*c3;
}

void qeuler_to(quaternion q, double* x, double* y, double* z)
{
	double q1 = q[0];
	double q2 = q[1];
	double q3 = q[2];
	double q4 = q[3];

	double sine = 2*(q1*q3 - q4*q2);
	if (sine > 1.0) sine = 1.0;
	if (sine < -1.0) sine = -1.0;

	*x = atan2(2*q1*q2+2*q3*q4 , 1 - 2*q2*q2 - 2*q3*q3);
	*y = asin(sine);
	*z = atan2(2*q1*q4+2*q2*q3 , 1 - 2*q3*q3 - 2*q4*q4);
}

void qrot_vec(quaternion q, quaternion v)
{
	double l2 = q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
	double m2 = q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
	double s = 2 * acos(q[0]/sqrt(l2));
	if (s > 3.14159265358979323846) s = s - 2*3.14159265358979323846;
	s = s / sqrt(m2);

	if ((fabs(l2) < 1e-8) || (fabs(m2) < 1e-8)) s = 0;

	v[0] = s;
	v[1] = q[1];
	v[2] = q[2];
	v[3] = q[3];
}

void qset_vec(quaternion q, double x, double y, double z)
{
	q[0] = 0;
	q[1] = x;
	q[2] = y;
	q[3] = z;
}

void qget_vec(quaternion q, double* x, double* y, double* z)
{
	*x = q[1];
	*y = q[2];
	*z = q[3];
}