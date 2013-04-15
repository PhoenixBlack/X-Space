#include "matrix.h"
#include "math.h"

/* Simple 4x4 matrix by 4x4 matrix multiply. */
void mult_matrix(float dst[16], const float src1[16], const float src2[16])
{
	float tmp[16];
	int i, j;

	for (i=0; i<4; i++) {
		for (j=0; j<4; j++) {
			tmp[i*4+j] = src1[i*4+0] * src2[0*4+j] +
			             src1[i*4+1] * src2[1*4+j] +
			             src1[i*4+2] * src2[2*4+j] +
			             src1[i*4+3] * src2[3*4+j];
		}
	}
	/* Copy result to dst (so dst can also be src1 or src2). */
	for (i=0; i<16; i++)
		dst[i] = tmp[i];
}

/* Invert a row-major (C-style) 4x4 matrix. */
void invert_matrix(float* out, const float* m)
{
	/* Assumes matrices are ROW major. */
#define SWAP_ROWS(a, b) { double *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(r)*4+(c)]

	double wtmp[4][8];
	double m0, m1, m2, m3, s;
	double* r0, *r1, *r2, *r3;

	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

	r0[0] = MAT(m,0,0), r0[1] = MAT(m,0,1),
	                            r0[2] = MAT(m,0,2), r0[3] = MAT(m,0,3),
	                                    r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,

	                                            r1[0] = MAT(m,1,0), r1[1] = MAT(m,1,1),
	                                                    r1[2] = MAT(m,1,2), r1[3] = MAT(m,1,3),
	                                                            r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,

	                                                                    r2[0] = MAT(m,2,0), r2[1] = MAT(m,2,1),
	                                                                            r2[2] = MAT(m,2,2), r2[3] = MAT(m,2,3),
	                                                                                    r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,

	                                                                                            r3[0] = MAT(m,3,0), r3[1] = MAT(m,3,1),
	                                                                                                    r3[2] = MAT(m,3,2), r3[3] = MAT(m,3,3),
	                                                                                                            r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

	/* Choose myPivot, or die. */
	if (fabs(r3[0])>fabs(r2[0])) SWAP_ROWS(r3, r2);
	if (fabs(r2[0])>fabs(r1[0])) SWAP_ROWS(r2, r1);
	if (fabs(r1[0])>fabs(r0[0])) SWAP_ROWS(r1, r0);
	if (0.0 == r0[0]) {
		return;
	}

	/* Eliminate first variable. */
	m1 = r1[0]/r0[0]; m2 = r2[0]/r0[0]; m3 = r3[0]/r0[0];
	s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
	s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
	s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) {
		r1[4] -= m1 * s;
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r0[5];
	if (s != 0.0) {
		r1[5] -= m1 * s;
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r0[6];
	if (s != 0.0) {
		r1[6] -= m1 * s;
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r0[7];
	if (s != 0.0) {
		r1[7] -= m1 * s;
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}

	/* Choose myPivot, or die. */
	if (fabs(r3[1])>fabs(r2[1])) SWAP_ROWS(r3, r2);
	if (fabs(r2[1])>fabs(r1[1])) SWAP_ROWS(r2, r1);
	if (0.0 == r1[1]) {
		return;
	}

	/* Eliminate second variable. */
	m2 = r2[1]/r1[1]; m3 = r3[1]/r1[1];
	r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
	s = r1[4]; if (0.0 != s) {
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r1[5]; if (0.0 != s) {
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r1[6]; if (0.0 != s) {
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r1[7]; if (0.0 != s) {
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}

	/* Choose myPivot, or die. */
	if (fabs(r3[2])>fabs(r2[2])) SWAP_ROWS(r3, r2);
	if (0.0 == r2[2]) {
		return;
	}

	/* Eliminate third variable. */
	m3 = r3[2]/r2[2];
	r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
	                              r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6],
	                                       r3[7] -= m3 * r2[7];

	/* Last check. */
	if (0.0 == r3[3]) {
		return;
	}

	s = 1.0/r3[3];              /* Now back substitute row 3. */
	r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;

	m2 = r2[3];                 /* Now back substitute row 2. */
	s  = 1.0/r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
	        r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
	                              r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
	                              r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

	m1 = r1[2];                 /* Now back substitute row 1. */
	s  = 1.0/r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
	        r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
	                              r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

	m0 = r0[1];                 /* Now back substitute row 0. */
	s  = 1.0/r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
	        r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

	MAT(out,0,0) = (float)r0[4]; MAT(out,0,1) = (float)r0[5],
	MAT(out,0,2) = (float)r0[6]; MAT(out,0,3) = (float)r0[7],
	MAT(out,1,0) = (float)r1[4]; MAT(out,1,1) = (float)r1[5],
	MAT(out,1,2) = (float)r1[6]; MAT(out,1,3) = (float)r1[7],
	MAT(out,2,0) = (float)r2[4]; MAT(out,2,1) = (float)r2[5],
	MAT(out,2,2) = (float)r2[6]; MAT(out,2,3) = (float)r2[7],
	MAT(out,3,0) = (float)r3[4]; MAT(out,3,1) = (float)r3[5],
	MAT(out,3,2) = (float)r3[6]; MAT(out,3,3) = (float)r3[7];

#undef MAT
#undef SWAP_ROWS
}

/* Simple 4x4 matrix by 4-component column vector multiply. */
void transform_vector(float dst[4], const float mat[16], const float vec[4])
{
	double tmp[4], invW;
	int i;

	for (i=0; i<4; i++) {
		tmp[i] = mat[0*4+i] * vec[0] +
		         mat[1*4+i] * vec[1] +
		         mat[2*4+i] * vec[2] +
		         mat[3*4+i] * vec[3];
	}
	invW = 1 / (tmp[3]+1e-12);

	/* Apply perspective divide and copy to dst (so dst can vec). */
	for (i=0; i<3; i++) dst[i] = (float)(tmp[i] * invW);
	dst[3] = 1;
}