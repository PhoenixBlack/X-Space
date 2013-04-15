#ifndef QUATERNION_H
#define QUATERNION_H

#ifndef _QUATERNION_TYPE
#define _QUATERNION_TYPE
typedef double quaternion[4];
#endif

void qmul(quaternion q, quaternion q1,quaternion q2);
void qmul_vec(quaternion q, quaternion q1, double x, double y, double z);
void qdiv(quaternion q, quaternion q1,quaternion q2);
void qconj(quaternion q1, quaternion q2);
void qeuler_from(quaternion q, double x, double y, double z);
void qeuler_to(quaternion q, double* x, double* y, double* z);
void qrot_vec(quaternion q, quaternion v);
void qset_vec(quaternion q, double x, double y, double z);
void qget_vec(quaternion q, double* x, double* y, double* z);

#endif
