void mult_matrix(float dst[16], const float src1[16], const float src2[16]);
void invert_matrix(float* out, const float* m);
void transform_vector(float dst[4], const float mat[16], const float vec[4]);