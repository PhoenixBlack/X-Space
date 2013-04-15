#ifndef COORDSYS_H
#define COORDSYS_H

#ifndef _QUATERNION_TYPE
#define _QUATERNION_TYPE
typedef double quaternion[4];
#endif

//Coordinate systems used:
//Rotation: (+: clockwise, -: counterclockwise)
//   ( X Y Z ) == ( roll pitch yaw)
// Local:
// +Roll, +Pitch, +Yaw
// X-Plane:
// +Roll  -Pitch, -Yaw
//
//Main coordinate systems:
// Simulator:
//   X+ east   Y+ up     Z+ south
//   X- west   Y- down   Z- north
// Local:
//   X+ aft    Y+ left   Z+ up
//   X- fwd    Y- right  Z- down
// Inertial global (0 deg planet rotation):
//   X+ lat 0, lon 0    Y+ lat 0, lon  90  Z+ north
//   X- lat 0, lon 180  Y- lat 0, lon -90  Z- south
// Non-inertial global (around earth center):
//   X+ lat 0, lon 0    Y+ lat 0, lon  90  Z+ north
//   X- lat 0, lon 180  Y- lat 0, lon -90  Z- south
// Warning: velocities in non-inertial systems are not actually in
// Earth rotating frame (relative to Earth). They are just inertial velocities
// rotated correctly into frame
//
//If vector (lx,ly,lz) is rotated by vehicle attitude, the
//resulting vector (gx,gy,gz) relates to real global values as:
// gx =  rgz  or  rgx = -gy
// gy = -rgx      rgy =  gz
// gz =  rgy      rgz =  gx

quaternion planet_rotation;
quaternion i2sim_rotation;
quaternion sim2i_rotation;
quaternion i2rel_rotation;
quaternion ni2sim_rotation;
quaternion sim2ni_rotation;
quaternion i2ni_rotation;
quaternion ni2i_rotation;

void coordsys_update(float dt);
void coordsys_initialize();
void coordsys_reinitialize();
void coordsys_draw();
void coordsys_update_datarefs(vessel* base, vessel* v);

//Convert velocity in simulator coordinate between inertial and non-inertial
void vel_i2ni_vessel(vessel* v);
void vel_ni2i_vessel(vessel* v);

//Coordinate transform
void coord_sim2i(double sx, double sy, double sz, double* ix, double* iy, double* iz);
void coord_i2sim(double ix, double iy, double iz, double* sx, double* sy, double* sz);

void coord_sim2ni(double sx, double sy, double sz, double* x, double* y, double* z);
void coord_ni2sim(double x, double y, double z, double* sx, double* sy, double* sz);
void coord_sim2l(vessel* v, double sx, double sy, double sz, double* lx, double* ly, double* lz);
void coord_l2sim(vessel* v, double lx, double ly, double lz, double* sx, double* sy, double* sz);
void coord_l2i(vessel* v, double lx, double ly, double lz, double* ix, double* iy, double* iz);
//void coord_i2ni(double ix, double iy, double iz, double* x, double* y, double* z);
//void coord_ni2i(double x, double y, double z, double* ix, double* iy, double* iz);
//Two systems share centerpoint
#define coord_i2ni vec_i2ni
#define coord_ni2i vec_ni2i

//Arbitrary vector transform
void vec_sim2i(double sx, double sy, double sz, double* ix, double* iy, double* iz);
void vec_i2sim(double ix, double iy, double iz, double* sx, double* sy, double* sz);
void vec_i2rel(double ix, double iy, double iz, double* rx, double* ry, double* rz);
void vec_sim2ni(double x, double y, double z, double* ix, double* iy, double* iz);
void vec_ni2sim(double ix, double iy, double iz, double* sx, double* sy, double* sz);
void vec_sim2l(vessel* v, double sx, double sy, double sz, double* lx, double* ly, double* lz);
void vec_l2sim(vessel* v, double lx, double ly, double lz, double* sx, double* sy, double* sz);
void vec_i2l(vessel* v, double ix, double iy, double iz, double* lx, double* ly, double* lz);
void vec_l2i(vessel* v, double lx, double ly, double lz, double* ix, double* iy, double* iz);
void vec_i2ni(double ix, double iy, double iz, double* x, double* y, double* z);
void vec_ni2i(double x, double y, double z, double* ix, double* iy, double* iz);
void vec_ni2l(vessel* v, double ix, double iy, double iz, double* lx, double* ly, double* lz);
void vec_l2ni(vessel* v, double lx, double ly, double lz, double* ix, double* iy, double* iz);

//Quaternion transformation
void quat_sim2i(quaternion sq, quaternion iq);
void quat_i2sim(quaternion iq, quaternion sq);
void quat_i2rel(quaternion iq, quaternion rq);
void quat_ni2sim(quaternion nq, quaternion sq);
void quat_sim2ni(quaternion sq, quaternion nq);
void quat_ni2i(quaternion nq, quaternion iq);
void quat_i2ni(quaternion iq, quaternion nq);

#endif
