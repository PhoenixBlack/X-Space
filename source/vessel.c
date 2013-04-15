//==============================================================================
// Working with vessels
//==============================================================================
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "x-space.h"
#include "config.h"
#include "dataref.h"
#include "vessel.h"
#include "quaternion.h"
#include "coordsys.h"
#include "planet.h"
#include "geomagnetic.h"
#include "material.h"
#include "highlevel.h"
#include "curtime.h"
#include "radiosys.h"

//Include X-Plane SDK and X-Plane API
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include "x-plane.h"

#include <XPLMDataAccess.h>
#include <XPLMGraphics.h>
#include <XPLMPlanes.h>
#include <XPLMUtilities.h>
#include <XPLMScenery.h>
#endif




//==============================================================================
// Datarefs for writing/reading to simulator
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
//Aircraft local state
XPLMDataRef dataref_vessel_x;
XPLMDataRef dataref_vessel_y;
XPLMDataRef dataref_vessel_z;
XPLMDataRef dataref_vessel_vx;
XPLMDataRef dataref_vessel_vy;
XPLMDataRef dataref_vessel_vz;
XPLMDataRef dataref_vessel_ax;
XPLMDataRef dataref_vessel_ay;
XPLMDataRef dataref_vessel_az;
XPLMDataRef dataref_vessel_P;
XPLMDataRef dataref_vessel_Q;
XPLMDataRef dataref_vessel_R;
XPLMDataRef dataref_vessel_q;

//Aircraft physics
XPLMDataRef dataref_vessel_extra_mass;
XPLMDataRef dataref_vessel_mass;
XPLMDataRef dataref_vessel_jxx;
XPLMDataRef dataref_vessel_jyy;
XPLMDataRef dataref_vessel_jzz;
XPLMDataRef dataref_fuel_mass;

//Instrument datarefs
double		        instrument_pitch;
XPLMDataRef dataref_instrument_pitch;
double		        instrument_roll;
XPLMDataRef dataref_instrument_roll;
double		        instrument_yaw;
double		        instrument_heading;
XPLMDataRef dataref_instrument_heading;
double		        instrument_vvi;
XPLMDataRef dataref_instrument_vvi;
double		        instrument_alt;
XPLMDataRef dataref_instrument_alt;
double		        instrument_airspeed;
XPLMDataRef dataref_instrument_airspeed;
double		        instrument_magvar;
XPLMDataRef dataref_instrument_magvar;
double		        instrument_P;
double		        instrument_Q;
double		        instrument_R;
double		        instrument_hpath;
XPLMDataRef dataref_instrument_hpath;
double		        instrument_vpath;
XPLMDataRef dataref_instrument_vpath;
double		        instrument_gx;
double		        instrument_gy;
double		        instrument_gz;

//AI aircraft state
XPLMDataRef dataref_plane_x[20];
XPLMDataRef dataref_plane_y[20];
XPLMDataRef dataref_plane_z[20];
XPLMDataRef dataref_plane_vx[20];
XPLMDataRef dataref_plane_vy[20];
XPLMDataRef dataref_plane_vz[20];
XPLMDataRef dataref_plane_pitch[20];
XPLMDataRef dataref_plane_roll[20];
XPLMDataRef dataref_plane_yaw[20];
#endif

//Vessels data
int vessel_count;
int vessel_alloc_count;
vessel* vessels;




//==============================================================================
// ID-based interface to vessel internal variables
//==============================================================================
double vessels_read_param_d(int idx, int paramidx, int arridx)
{
	switch (paramidx) {
			//Misc variables (0-29)
		case  0: return (double)vessels[idx].exists;
		case  1: return (double)vessels[idx].attached;
		case  2: return (double)vessels[idx].net_id;
		case  3: return (double)vessels[idx].networked;
		case  4: return (double)vessels[idx].physics_type;
		case  5: return (double)vessels[idx].is_plane;
		case  6: return (double)vessels[idx].plane_index;
		case  7: return (double)vessels[idx].ixx;
		case  8: return (double)vessels[idx].iyy;
		case  9: return (double)vessels[idx].izz;
		case 10: return (double)vessels[idx].jxx;
		case 11: return (double)vessels[idx].jyy;
		case 12: return (double)vessels[idx].jzz;
		case 13: return (double)vessels[idx].mass;
		case 14: return (double)vessels[idx].latitude;
		case 15: return (double)vessels[idx].longitude;
		case 16: return (double)vessels[idx].elevation;
		case 17: return (double)vessels[idx].weight.chassis;
		case 18: return (double)vessels[idx].weight.hull;
		case 19: return (double)vessels[idx].weight.fuel[0];
		case 20: return (double)vessels[idx].weight.fuel[1];
		case 21: return (double)vessels[idx].weight.fuel[2];
		case 22: return (double)vessels[idx].weight.fuel[3];
		case 23: return (double)vessels[idx].geometry.dx;
		case 24: return (double)vessels[idx].geometry.dy;
		case 25: return (double)vessels[idx].geometry.dz;
		case 26: return (double)vessels[idx].weight.fuel[arridx];
		case 27: return (double)vessels[idx].weight.fuel_location[arridx].x;
		case 28: return (double)vessels[idx].weight.fuel_location[arridx].y;
		case 29: return (double)vessels[idx].weight.fuel_location[arridx].z;
			//Inertial coordinates (30-49)
		case 30: return vessels[idx].inertial.x;
		case 31: return vessels[idx].inertial.y;
		case 32: return vessels[idx].inertial.z;
		case 33: return vessels[idx].inertial.vx;
		case 34: return vessels[idx].inertial.vy;
		case 35: return vessels[idx].inertial.vz;
		case 36: return vessels[idx].inertial.ax;
		case 37: return vessels[idx].inertial.ay;
		case 38: return vessels[idx].inertial.az;
		case 39: return vessels[idx].inertial.q[0];
		case 40: return vessels[idx].inertial.q[1];
		case 41: return vessels[idx].inertial.q[2];
		case 42: return vessels[idx].inertial.q[3];
		case 43: return vessels[idx].inertial.P;
		case 44: return vessels[idx].inertial.Q;
		case 45: return vessels[idx].inertial.R;
			//Non-inertial coordinates (50-69)
		case 50: return vessels[idx].noninertial.x;
		case 51: return vessels[idx].noninertial.y;
		case 52: return vessels[idx].noninertial.z;
		case 53: return vessels[idx].noninertial.vx;
		case 54: return vessels[idx].noninertial.vy;
		case 55: return vessels[idx].noninertial.vz;
		case 56: return vessels[idx].noninertial.ax;
		case 57: return vessels[idx].noninertial.ay;
		case 58: return vessels[idx].noninertial.az;
		case 59: return vessels[idx].noninertial.q[0];
		case 60: return vessels[idx].noninertial.q[1];
		case 61: return vessels[idx].noninertial.q[2];
		case 62: return vessels[idx].noninertial.q[3];
		case 63: return vessels[idx].noninertial.P;
		case 64: return vessels[idx].noninertial.Q;
		case 65: return vessels[idx].noninertial.R;
		case 66: return vessels[idx].noninertial.cx;
		case 67: return vessels[idx].noninertial.cy;
		case 68: return vessels[idx].noninertial.cz;
			//Geomagnetic variables (70-89)
		case 70: return vessels[idx].geomagnetic.inclination;
		case 71: return vessels[idx].geomagnetic.declination;
		case 72: return vessels[idx].geomagnetic.F;
		case 73: return vessels[idx].geomagnetic.H;
		case 74: return vessels[idx].geomagnetic.noninertial.x;
		case 75: return vessels[idx].geomagnetic.noninertial.y;
		case 76: return vessels[idx].geomagnetic.noninertial.z;
		case 77: return vessels[idx].geomagnetic.local.x;
		case 78: return vessels[idx].geomagnetic.local.y;
		case 79: return vessels[idx].geomagnetic.local.z;
		case 80: return vessels[idx].geomagnetic.GV;
		case 81: return vessels[idx].geomagnetic.g;
		case 82: return vessels[idx].geomagnetic.V;
			//Misc variables (90-99)
		case 90: return 1.0 - (double)vessels[idx].attached;
		case 91: return vessels[idx].mount.x;
		case 92: return vessels[idx].mount.y;
		case 93: return vessels[idx].mount.z;
		case 94: return vessels[idx].weight.tps;
		case 95: return vessels[idx].geometry.heat_fps;
		case 96: return (double)vessels[idx].client_id;
		case 97: return vessels[idx].last_update;
			//Atmospheric variables (100-119)
		case 100: return vessels[idx].air.density;
		case 101: return vessels[idx].air.concentration;
		case 102: return vessels[idx].air.temperature;
		case 103: return vessels[idx].air.pressure;
		case 104: return vessels[idx].air.Q;
		case 105: return vessels[idx].air.exospheric_temperature;
		case 106: return vessels[idx].air.concentration_He;
		case 107: return vessels[idx].air.concentration_O;
		case 108: return vessels[idx].air.concentration_N2;
		case 109: return vessels[idx].air.concentration_O2;
		case 110: return vessels[idx].air.concentration_Ar;
		case 111: return vessels[idx].air.concentration_H;
		case 112: return vessels[idx].air.concentration_N;
		case 113: return vessels[idx].air.density_He;
		case 114: return vessels[idx].air.density_O;
		case 115: return vessels[idx].air.density_N2;
		case 116: return vessels[idx].air.density_O2;
		case 117: return vessels[idx].air.density_Ar;
		case 118: return vessels[idx].air.density_H;
		case 119: return vessels[idx].air.density_N;
			//Orbital elements and information (120-129)
		case 120: return vessels[idx].orbit.smA;
		case 121: return vessels[idx].orbit.e;
		case 122: return vessels[idx].orbit.i;
		case 123: return vessels[idx].orbit.MnA;
		case 124: return vessels[idx].orbit.AgP;
		case 125: return vessels[idx].orbit.AsN;
		case 126: return vessels[idx].orbit.BSTAR;
		case 127: return vessels[idx].orbit.period;
			//Statistics (130-139)
		case 130: return vessels[idx].statistics.time_in_space;
		case 131: return vessels[idx].statistics.total_orbits;
		case 132: return vessels[idx].statistics.total_true_orbits;
		case 133: return vessels[idx].statistics.total_distance;
			//Local & relative coordinates (140-169)
		case 140: return vessels[idx].local.x; 
		case 141: return vessels[idx].local.y; 
		case 142: return vessels[idx].local.z; 
		case 143: return vessels[idx].local.vx;
		case 144: return vessels[idx].local.vy;
		case 145: return vessels[idx].local.vz;
		case 146: return vessels[idx].local.ax;
		case 147: return vessels[idx].local.ay;
		case 148: return vessels[idx].local.az;
		case 149: return vessels[idx].relative.q[0];
		case 150: return vessels[idx].relative.q[1];
		case 151: return vessels[idx].relative.q[2];
		case 152: return vessels[idx].relative.q[3];
		case 153: return vessels[idx].relative.P;
		case 154: return vessels[idx].relative.Q;
		case 155: return vessels[idx].relative.R;
		case 156: return vessels[idx].relative.roll;
		case 157: return vessels[idx].relative.pitch;
		case 158: return vessels[idx].relative.yaw;
		case 159: return vessels[idx].relative.heading;
		case 160: return vessels[idx].relative.airspeed;
		case 161: return vessels[idx].relative.hpath;
		case 162: return vessels[idx].relative.vpath;
		case 163: return vessels[idx].relative.vv;
		case 164: return vessels[idx].relative.gx;
		case 165: return vessels[idx].relative.gy;
		case 166: return vessels[idx].relative.gz;
		default: return 0.0;
	}
}

void vessels_write_param_d(int idx, int paramidx, int arridx, double val)
{
	switch (paramidx) {
			//Misc variables (0-10)
		case  0: vessels[idx].exists = (int)val; break;
		case  1: vessels[idx].attached = (int)val; break;
		case  2: vessels[idx].net_id = (int)val; break;
		case  3: vessels[idx].networked = (int)val; break;
		case  4: vessels[idx].physics_type = (int)val; break;
			//case  5: vessels[idx].is_plane = (int)val; break;
			//case  6: vessels[idx].plane_index = (int)val; break;
			//case  7: vessels[idx].ixx = val; break;
			//case  8: vessels[idx].iyy = val; break;
			//case  9: vessels[idx].izz = val; break;
		case 10: vessels[idx].jxx = val; break;
		case 11: vessels[idx].jyy = val; break;
		case 12: vessels[idx].jzz = val; break;
		case 13: vessels[idx].mass = val; break;
		case 14: vessels[idx].latitude = val; break;
		case 15: vessels[idx].longitude = val; break;
		case 16: vessels[idx].elevation = val; break;
		case 17: vessels[idx].weight.chassis = val; break;
		case 18: vessels[idx].weight.hull = val; break;
		case 19: vessels[idx].weight.fuel[0] = val; break;
		case 20: vessels[idx].weight.fuel[1] = val; break;
		case 21: vessels[idx].weight.fuel[2] = val; break;
		case 22: vessels[idx].weight.fuel[3] = val; break;
		case 23: vessels[idx].geometry.dx = val; break;
		case 24: vessels[idx].geometry.dy = val; break;
		case 25: vessels[idx].geometry.dz = val; break;
		case 26: vessels[idx].weight.fuel[arridx] = val; break;
		case 27: vessels[idx].weight.fuel_location[arridx].x = val; break;
		case 28: vessels[idx].weight.fuel_location[arridx].y = val; break;
		case 29: vessels[idx].weight.fuel_location[arridx].z = val; break;
			//Inertial coordinates (30-49)
		case 30: vessels[idx].inertial.x = val; break;
		case 31: vessels[idx].inertial.y = val; break;
		case 32: vessels[idx].inertial.z = val; break;
		case 33: vessels[idx].inertial.vx = val; break;
		case 34: vessels[idx].inertial.vy = val; break;
		case 35: vessels[idx].inertial.vz = val; break;
		case 36: vessels[idx].inertial.ax = val; break;
		case 37: vessels[idx].inertial.ay = val; break;
		case 38: vessels[idx].inertial.az = val; break;
		case 39: vessels[idx].inertial.q[0] = val; break;
		case 40: vessels[idx].inertial.q[1] = val; break;
		case 41: vessels[idx].inertial.q[2] = val; break;
		case 42: vessels[idx].inertial.q[3] = val; break;
		case 43: vessels[idx].inertial.P = val; break;
		case 44: vessels[idx].inertial.Q = val; break;
		case 45: vessels[idx].inertial.R = val; break;
			//Non-inertial coordinates (50-69)
		case 50: vessels[idx].noninertial.x = val; break;
		case 51: vessels[idx].noninertial.y = val; break;
		case 52: vessels[idx].noninertial.z = val; break;
		case 53: vessels[idx].noninertial.vx = val; break;
		case 54: vessels[idx].noninertial.vy = val; break;
		case 55: vessels[idx].noninertial.vz = val; break;
		case 56: vessels[idx].noninertial.ax = val; break;
		case 57: vessels[idx].noninertial.ay = val; break;
		case 58: vessels[idx].noninertial.az = val; break;
		case 59: vessels[idx].noninertial.q[0] = val; break;
		case 60: vessels[idx].noninertial.q[1] = val; break;
		case 61: vessels[idx].noninertial.q[2] = val; break;
		case 62: vessels[idx].noninertial.q[3] = val; break;
		case 63: vessels[idx].noninertial.P = val; break;
		case 64: vessels[idx].noninertial.Q = val; break;
		case 65: vessels[idx].noninertial.R = val; break;
		case 66: vessels[idx].noninertial.cx = val; break;
		case 67: vessels[idx].noninertial.cy = val; break;
		case 68: vessels[idx].noninertial.cz = val; break;
			//Geomagnetic variables (70-89)
		case 70: vessels[idx].geomagnetic.inclination = val; break;
		case 71: vessels[idx].geomagnetic.declination = val; break;
		case 72: vessels[idx].geomagnetic.F = val; break;
		case 73: vessels[idx].geomagnetic.H = val; break;
		case 74: vessels[idx].geomagnetic.noninertial.x = val; break;
		case 75: vessels[idx].geomagnetic.noninertial.y = val; break;
		case 76: vessels[idx].geomagnetic.noninertial.z = val; break;
		case 77: vessels[idx].geomagnetic.local.x = val; break;
		case 78: vessels[idx].geomagnetic.local.y = val; break;
		case 79: vessels[idx].geomagnetic.local.z = val; break;
		case 80: vessels[idx].geomagnetic.GV = val; break;
		case 81: vessels[idx].geomagnetic.g = val; break;
		case 82: vessels[idx].geomagnetic.V = val; break;
				//Misc variables (90-99)
		case 90: 
			if (val > 0.5) { 
				vessels[idx].attached = 0;
				if (vessels[0].physics_type == VESSEL_PHYSICS_SIM) {
					vessels[0].detach_time = curtime();
					if (config.staging_wait_time > 0.0) vessels[0].physics_type = VESSEL_PHYSICS_INERTIAL;
				}
			}
			break;
		case 91: vessels[idx].mount.x = val; break;
		case 92: vessels[idx].mount.y = val; break;
		case 93: vessels[idx].mount.z = val; break;
		case 94: vessels[idx].weight.tps = val; break;
		//case 95: vessels[idx].geometry.heat_fps = val; break;
		case 96: vessels[idx].client_id = (int)val; break;
		case 97: vessels[idx].last_update = val; break;
			//Atmospheric variables (100-119)
			//Orbital elements and information (120-129)
			//Statistics (130-139)
		case 130: vessels[idx].statistics.time_in_space = val;
		case 131: vessels[idx].statistics.total_orbits = val;
		case 132: vessels[idx].statistics.total_true_orbits = val;
		case 133: vessels[idx].statistics.total_distance = val;
			//Local & relative coordinates (140-169)
		case 140: vessels[idx].local.x = val; break;
		case 141: vessels[idx].local.y = val; break;
		case 142: vessels[idx].local.z = val; break;
		case 143: vessels[idx].local.vx = val; break;
		case 144: vessels[idx].local.vy = val; break;
		case 145: vessels[idx].local.vz = val; break;
		case 146: vessels[idx].local.ax = val; break;
		case 147: vessels[idx].local.ay = val; break;
		case 148: vessels[idx].local.az = val; break;
		case 149: vessels[idx].relative.q[0] = val; break;
		case 150: vessels[idx].relative.q[1] = val; break;
		case 151: vessels[idx].relative.q[2] = val; break;
		case 152: vessels[idx].relative.q[3] = val; break;
		case 153: vessels[idx].relative.P = val; break;
		case 154: vessels[idx].relative.Q = val; break;
		case 155: vessels[idx].relative.R = val; break;
		case 156: vessels[idx].relative.roll = val; break;
		case 157: vessels[idx].relative.pitch = val; break;
		case 158: vessels[idx].relative.yaw = val; break;
		case 159: vessels[idx].relative.heading = val; break;
		case 160: vessels[idx].relative.airspeed = val; break;
		case 161: vessels[idx].relative.hpath = val; break;
		case 162: vessels[idx].relative.vpath = val; break;
		case 163: vessels[idx].relative.vv = val; break;
		case 164: vessels[idx].relative.gx = val; break;
		case 165: vessels[idx].relative.gy = val; break;
		case 166: vessels[idx].relative.gz = val; break;
		default: break;
	}
}

void vessels_set_parameter(vessel* v, int index, int array_index, double value)
{
	vessels_write_param_d(v->index,index,array_index,value);
}

double vessels_get_parameter(vessel* v, int index, int array_index)
{
	return vessels_read_param_d(v->index,index,array_index);
}


//==============================================================================
// Dataref interface to vessel internal variables
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
typedef struct __vessels_param_tag {
	int idx;
	int arraysize;
} __vessels_param;

static long vessels_get_param_fv(void* dataptr, float* values, int inOffset, int inMax)
{
	int i;
	__vessels_param* param = (__vessels_param*)dataptr;
	if (!values) return vessel_count;

	for (i = inOffset; i < inOffset+inMax; i++) {
		int vidx = i / param->arraysize;
		if ((vidx >= 0) && (vidx < vessel_alloc_count)) {
			if (vidx < vessel_count) {
				values[i-inOffset] = (float)(vessels_read_param_d(vidx,param->idx,i%param->arraysize));
			} else {
				if (param->idx == 90) {
					values[i-inOffset] = 1.0f;
				} else {
					values[i-inOffset] = 0.0f;
				}
			}
		}
	}
	return vessel_count;
}

static void vessels_set_param_fv(void* dataptr, float* values, int inOffset, int inMax)
{
	int i;
	__vessels_param* param = (__vessels_param*)dataptr;
	if (!values) return;

	for (i = inOffset; i < inOffset+inMax; i++) {
		int vidx = i / param->arraysize;
		if ((vidx >= 0) && (vidx < vessel_count)) {
			vessels_write_param_d(vidx,param->idx,i%param->arraysize,(double)values[i-inOffset]);
		}
	}
}

void vessels_register_dataref(char* name, int paramidx)
{
	__vessels_param* param = (__vessels_param*)malloc(sizeof(__vessels_param));
	param->idx = paramidx;
	param->arraysize = 1;

	XPLMRegisterDataAccessor(
	    name,xplmType_FloatArray,1,0,0,0,0,0,0,0,0,
	    vessels_get_param_fv,vessels_set_param_fv,0,0,
	    (void*)param,(void*)param);
}

void vessels_register_dataref_v(char* name, int paramidx, int arraysize)
{
	__vessels_param* param = (__vessels_param*)malloc(sizeof(__vessels_param));
	param->idx = paramidx;
	param->arraysize = arraysize;

	XPLMRegisterDataAccessor(
	    name,xplmType_FloatArray,1,0,0,0,0,0,0,0,0,
	    vessels_get_param_fv,vessels_set_param_fv,0,0,
	    (void*)param,(void*)param);
}
#endif




//==============================================================================
// Update vessel coordinates (synchronize sim with inertial)
//==============================================================================
void vessels_update_coordinates(vessel* v)
{
	//Update simulator coordinates
	if ((v->physics_type == VESSEL_PHYSICS_INERTIAL) ||
	    (v->physics_type == VESSEL_PHYSICS_INERTIAL_TO_SIM)) {
		coord_i2sim(v->inertial.x,v->inertial.y,v->inertial.z,&v->sim.x,&v->sim.y,&v->sim.z);
		vec_i2sim(v->inertial.vx,v->inertial.vy,v->inertial.vz,&v->sim.vx,&v->sim.vy,&v->sim.vz);
		quat_i2sim(v->inertial.q,v->sim.q);
		vel_i2ni_vessel(v);
		v->sim.P = v->inertial.P;
		v->sim.Q = v->inertial.Q;
		v->sim.R = v->inertial.R;
		v->sim.vx = v->sim.ivx;
		v->sim.vy = v->sim.ivy;
		v->sim.vz = v->sim.ivz;
	}
}




//==============================================================================
// Read state/coordinates from simulator
//  update_inertial: update inertial coordinates (otherwise just read sim vars)
//==============================================================================
double nan_filter(double x) {
	if (x != x) {
		return 0.0;
	} else {
		return x;
	}
}

void quat_filter(quaternion q) {
	double s;
	q[0] = nan_filter(q[0]);
	q[1] = nan_filter(q[1]);
	q[2] = nan_filter(q[2]);
	q[3] = nan_filter(q[3]);
	
	s = q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3];
	if (s == 0.0) {
		q[0] = 1.0;
	}
}

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void vessels_read(vessel* v, int update_inertial)
{
	//Temporary quaternion (must be single precision)
	float q[4];

	//Read differently if vessel is not an X-Plane object
	if ((v->index != 0) && (!v->is_plane)) {
		//v->mass = v->geometry.obj_mass; //Mass of the object
		//v->jxx = v->geometry.obj_jxx;
		//v->jyy = v->geometry.obj_jyy;
		//v->jzz = v->geometry.obj_jzz;
		return;
	}

	//Read state from simulator
	if (v->index == 0) {
		float fuel[9];
		double mass;
		int i;

		v->sim.x     = nan_filter(XPLMGetDatad(dataref_vessel_x));
		v->sim.y     = nan_filter(XPLMGetDatad(dataref_vessel_y));
		v->sim.z     = nan_filter(XPLMGetDatad(dataref_vessel_z));
		v->sim.vx    = nan_filter(XPLMGetDataf(dataref_vessel_vx));
		v->sim.vy    = nan_filter(XPLMGetDataf(dataref_vessel_vy));
		v->sim.vz    = nan_filter(XPLMGetDataf(dataref_vessel_vz));
		v->sim.ax    = nan_filter(XPLMGetDataf(dataref_vessel_ax));
		v->sim.ay    = nan_filter(XPLMGetDataf(dataref_vessel_ay));
		v->sim.az    = nan_filter(XPLMGetDataf(dataref_vessel_az));
		v->sim.P     = nan_filter(XPLMGetDataf(dataref_vessel_P));
		v->sim.Q     = nan_filter(XPLMGetDataf(dataref_vessel_Q));
		v->sim.R     = nan_filter(XPLMGetDataf(dataref_vessel_R));
		v->jxx	     = nan_filter(XPLMGetDataf(dataref_vessel_jzz));
		v->jyy       = nan_filter(XPLMGetDataf(dataref_vessel_jxx));
		v->jzz       = nan_filter(XPLMGetDataf(dataref_vessel_jyy));
		mass         = nan_filter(XPLMGetDataf(dataref_vessel_mass));
		XPLMGetDatavf(dataref_fuel_mass,fuel,0,9);
		XPLMGetDatavf(dataref_vessel_q,q,0,4);

		for (i = 0; i < 9; i++) v->weight.fuel[i] = nan_filter(fuel[i]);
		v->weight.chassis = 0.8*mass;
		v->weight.hull = 0.2*mass;

		//FIXME: Pd, Qd, Rd
	} else if (v->is_plane) { //This cause is horribly broken due to X-Plane legacy
		double p,y,r;
		double qt[4];

		v->sim.x     = nan_filter(XPLMGetDatad(dataref_plane_x[v->plane_index]));
		v->sim.y     = nan_filter(XPLMGetDatad(dataref_plane_y[v->plane_index])+3*current_planet.radius);
		v->sim.z     = nan_filter(XPLMGetDatad(dataref_plane_z[v->plane_index]));
		//v->sim.vx    = XPLMGetDataf(dataref_plane_vx[v->plane_index]); //Velocity is pointless, might as well just ignore it...
		//v->sim.vy    = XPLMGetDataf(dataref_plane_vy[v->plane_index]);
		//v->sim.vz    = XPLMGetDataf(dataref_plane_vz[v->plane_index]);
		//v->sim.P     = 0;
		//v->sim.Q     = 0;
		//v->sim.R     = 0;
		p            = nan_filter(XPLMGetDataf(dataref_plane_pitch[v->plane_index]));
		y            = nan_filter(XPLMGetDataf(dataref_plane_yaw[v->plane_index]));
		r            = nan_filter(XPLMGetDataf(dataref_plane_roll[v->plane_index]));

		qeuler_from(qt,RAD(r),RAD(p),RAD(y));
		q[0] = (float)nan_filter(qt[0]);
		q[1] = (float)nan_filter(qt[1]);
		q[2] = (float)nan_filter(qt[2]);
		q[3] = (float)nan_filter(qt[3]);	
	}

	//Set quaternion
	v->sim.q[0] = q[0];
	v->sim.q[1] = q[1];
	v->sim.q[2] = q[2];
	v->sim.q[3] = q[3];
	quat_filter(v->sim.q);

	//Geographical coordinates
	if (v->physics_type == VESSEL_PHYSICS_SIM) {
		XPLMLocalToWorld(v->sim.x,v->sim.y,v->sim.z,&v->latitude,&v->longitude,&v->elevation);
		v->latitude = nan_filter(v->latitude);
		v->longitude = nan_filter(v->longitude);
		v->elevation = nan_filter(v->elevation);
	} else if (v->physics_type == VESSEL_PHYSICS_INERTIAL) {
		double sx,sy,sz;
		coord_i2sim(v->inertial.x,v->inertial.y,v->inertial.z,&sx,&sy,&sz);
		XPLMLocalToWorld(sx,sy,sz,&v->latitude,&v->longitude,&v->elevation);
		v->latitude = nan_filter(v->latitude);
		v->longitude = nan_filter(v->longitude);
		v->elevation = nan_filter(v->elevation);
	}

	//Calculate in inertial coordinate system
	if (update_inertial) {
		vel_ni2i_vessel(v);
		coord_sim2i(v->sim.x,v->sim.y,v->sim.z,&v->inertial.x,&v->inertial.y,&v->inertial.z);
		vec_sim2i(v->sim.ivx,v->sim.ivy,v->sim.ivz,&v->inertial.vx,&v->inertial.vy,&v->inertial.vz);
		vec_sim2i(v->sim.ax,v->sim.ay,v->sim.az,&v->inertial.ax,&v->inertial.ay,&v->inertial.az);
		quat_sim2i(v->sim.q,v->inertial.q);
		v->inertial.P = v->sim.P;
		v->inertial.Q = v->sim.Q;
		v->inertial.R = v->sim.R;
		v->inertial.Pd = v->sim.Pd;
		v->inertial.Qd = v->sim.Qd;
		v->inertial.Rd = v->sim.Rd;
	}
}
#endif




//==============================================================================
// Write state/coordinates to simulator
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void vessels_write(vessel* v)
{
	//Temporary quaternion (must be single precision)
	float q[4];

	//Do not write if vessel is not X-Plane object
	if ((v->index != 0) && (!v->is_plane)) return;

	//Convert quaternion
	quat_filter(v->sim.q);
	q[0] = (float)v->sim.q[0];
	q[1] = (float)v->sim.q[1];
	q[2] = (float)v->sim.q[2];
	q[3] = (float)v->sim.q[3];

	//Write data to simulator
	if (v->index == 0) {
		XPLMSetDatavf(dataref_vessel_q,q,0,4);
		XPLMSetDatad(dataref_vessel_x,nan_filter(v->sim.x));
		XPLMSetDatad(dataref_vessel_y,nan_filter(v->sim.y));
		XPLMSetDatad(dataref_vessel_z,nan_filter(v->sim.z));

		if ((v->physics_type == VESSEL_PHYSICS_SIM) ||
		    (v->physics_type == VESSEL_PHYSICS_INERTIAL_TO_SIM)) {
			XPLMSetDataf(dataref_vessel_vx,(float)nan_filter(v->sim.vx));
			XPLMSetDataf(dataref_vessel_vy,(float)nan_filter(v->sim.vy));
			XPLMSetDataf(dataref_vessel_vz,(float)nan_filter(v->sim.vz));
		} else {
			XPLMSetDataf(dataref_vessel_vx,0);
			XPLMSetDataf(dataref_vessel_vy,0);
			XPLMSetDataf(dataref_vessel_vz,0);
		}
		XPLMSetDataf(dataref_vessel_P, (float)nan_filter(v->sim.P));
		XPLMSetDataf(dataref_vessel_Q, (float)nan_filter(v->sim.Q));
		XPLMSetDataf(dataref_vessel_R, (float)nan_filter(v->sim.R));

		if (v->attached_mass > 0.1) XPLMSetDataf(dataref_vessel_extra_mass,(float)nan_filter(v->attached_mass));

		if (v->physics_type == VESSEL_PHYSICS_INERTIAL) { //FIXME: this is partially obsolete
			double p,y,r,mvar;
			double vx,vy,vz;
			quaternion q;
			quat_i2rel(v->inertial.q,q);
			qeuler_to(q,&r,&p,&y);

			//Magnetic variation
			mvar = geomagnetic_inertial_elements.Decl;

			//Update instrument readings
			instrument_pitch		= DEGf(p);
			instrument_roll			= DEGf(r);
			instrument_yaw			= DEGf(y);
			instrument_heading		= DEGf(y)+mvar;
			instrument_vvi			= vessels[0].sim.vy;
			instrument_alt			= vessels[0].elevation;
			instrument_airspeed		= 0;
			instrument_magvar		= mvar;

			instrument_P			= DEG(v->inertial.P);
			instrument_Q			= DEG(v->inertial.Q);
			instrument_R			= DEG(v->inertial.R);

			vec_i2rel(v->inertial.vx,v->inertial.vy,v->inertial.vz,&vx,&vy,&vz);
			instrument_hpath		= DEG(atan2(vx,-vz));
			instrument_vpath		= DEG(atan2(vy,sqrt(vx*vx+vz*vz)));
		} else {
			//Update instrument displays
			instrument_pitch		= XPLMGetDataf(dataref_instrument_pitch);
			instrument_roll			= XPLMGetDataf(dataref_instrument_roll);
			instrument_yaw			= XPLMGetDataf(dataref_instrument_heading)-XPLMGetDataf(dataref_instrument_magvar);
			instrument_heading		= XPLMGetDataf(dataref_instrument_heading);
			instrument_vvi			= XPLMGetDataf(dataref_instrument_vvi)*0.00508;
			instrument_alt			= XPLMGetDataf(dataref_instrument_alt)*0.3048;
			instrument_airspeed		= XPLMGetDataf(dataref_instrument_airspeed)/1.94384449;
			instrument_magvar		= XPLMGetDataf(dataref_instrument_magvar);
			instrument_hpath		= XPLMGetDataf(dataref_instrument_hpath);
			instrument_vpath		= XPLMGetDataf(dataref_instrument_vpath);

			instrument_P			= DEG(v->sim.P);
			instrument_Q			= DEG(v->sim.Q);
			instrument_R			= DEG(v->sim.R);
		}

		//Update g-forces
		if (v->physics_type == VESSEL_PHYSICS_INERTIAL) {
			double gx,gy,gz;
			vec_i2l(v,v->inertial.ax,v->inertial.ay,v->inertial.az,&gx,&gy,&gz);

			instrument_gx = -gx/9.81;
			instrument_gy = -gy/9.81;
			instrument_gz = gz/9.81;
		} else {
			double g,gx,gy,gz,r,r2,nx,ny,nz;
			nx = v->sim.x;
			ny = v->sim.y+current_planet.radius;
			nz = v->sim.z;
			r2 = nx*nx+ny*ny+nz*nz;
			r = sqrt(r2);
			g = current_planet.mu/r2;
			nx = g*nx/r;
			ny = g*ny/r;
			nz = g*nz/r;

			vec_sim2l(v,v->sim.ax+nx,v->sim.ay+ny,v->sim.az+nz,&gx,&gy,&gz);
			instrument_gx = -gx/9.81;
			instrument_gy = -gy/9.81;
			instrument_gz = gz/9.81;
		}
	} else if (v->is_plane) {
		double p,y,r;
		//if (v->attached && (v->physics_type == VESSEL_PHYSICS_SIM)) {
		//XPLMSetDatad(dataref_plane_x[v->plane_index], v->sim.x);
		//XPLMSetDatad(dataref_plane_y[v->plane_index], v->sim.y+500e3);
		//XPLMSetDatad(dataref_plane_z[v->plane_index], v->sim.z);
		//} else {
		XPLMSetDatad(dataref_plane_x[v->plane_index], nan_filter(v->sim.x));
		XPLMSetDatad(dataref_plane_y[v->plane_index], nan_filter(v->sim.y-3*current_planet.radius));//500e3);
		XPLMSetDatad(dataref_plane_z[v->plane_index], nan_filter(v->sim.z));
		//}
		//if (v->physics_type == VESSEL_PHYSICS_SIM) {
		//(v->physics_type == VESSEL_PHYSICS_INERTIAL_TO_SIM)) {
			//XPLMSetDataf(dataref_plane_vx[v->plane_index],(float)v->sim.vx);
			//XPLMSetDataf(dataref_plane_vy[v->plane_index],(float)v->sim.vy);
			//XPLMSetDataf(dataref_plane_vz[v->plane_index],(float)v->sim.vz);
		//} else {
			XPLMSetDataf(dataref_plane_vx[v->plane_index],0);//vessels[0].sim.vx);
			XPLMSetDataf(dataref_plane_vy[v->plane_index],0);//vessels[0].sim.vy);
			XPLMSetDataf(dataref_plane_vz[v->plane_index],0);//vessels[0].sim.vz);
		//}

		quat_filter(v->sim.q);
		qeuler_to(v->sim.q,&r,&p,&y); //FIXME: check for NaN
		XPLMSetDataf(dataref_plane_pitch[v->plane_index],nan_filter(DEGf(p)));
		XPLMSetDataf(dataref_plane_yaw[v->plane_index],nan_filter(DEGf(y)));
		XPLMSetDataf(dataref_plane_roll[v->plane_index],nan_filter(DEGf(r)));
	}
}
#endif




//==============================================================================
// Set vessel cordinates by non-inertial coordinates
//==============================================================================
void vessels_set_ni(vessel* v)
{
	if (v->physics_type == VESSEL_PHYSICS_SIM) {
		coord_ni2sim(v->noninertial.x,v->noninertial.y,v->noninertial.z,
		             &v->sim.x,&v->sim.y,&v->sim.z);
		vec_ni2sim(v->noninertial.vx,v->noninertial.vy,v->noninertial.vz,
		           &v->sim.vx,&v->sim.vy,&v->sim.vz);
		vec_ni2sim(v->noninertial.ax,v->noninertial.ay,v->noninertial.az,
		           &v->sim.ax,&v->sim.ay,&v->sim.az);
		quat_ni2sim(v->noninertial.q,v->sim.q);
		v->sim.P = v->noninertial.P;
		v->sim.Q = v->noninertial.Q;
		v->sim.R = v->noninertial.R;
	} else if (v->physics_type == VESSEL_PHYSICS_INERTIAL) {
		coord_ni2i(v->noninertial.x,v->noninertial.y,v->noninertial.z,
		           &v->inertial.x,&v->inertial.y,&v->inertial.z);
		vec_ni2i(v->noninertial.vx,v->noninertial.vy,v->noninertial.vz,
		         &v->inertial.vx,&v->inertial.vy,&v->inertial.vz);
		vec_ni2i(v->noninertial.ax,v->noninertial.ay,v->noninertial.az,
		         &v->inertial.ax,&v->inertial.ay,&v->inertial.az);
		quat_ni2i(v->noninertial.q,v->inertial.q);
		v->inertial.P = v->noninertial.P;
		v->inertial.Q = v->noninertial.Q;
		v->inertial.R = v->noninertial.R;
	}
}


//==============================================================================
// Set by inertial coordinates
//==============================================================================
void vessels_set_i(vessel* v)
{
	if (v->physics_type == VESSEL_PHYSICS_SIM) {
		coord_i2sim(v->inertial.x,v->inertial.y,v->inertial.z,
		            &v->sim.x,&v->sim.y,&v->sim.z);
		vec_i2sim(v->inertial.vx,v->inertial.vy,v->inertial.vz,
		          &v->sim.vx,&v->sim.vy,&v->sim.vz);
		vec_i2sim(v->inertial.ax,v->inertial.ay,v->inertial.az,
		          &v->sim.ax,&v->sim.ay,&v->sim.az);
		quat_i2sim(v->inertial.q,v->sim.q);
		v->sim.P = v->inertial.P;
		v->sim.Q = v->inertial.Q;
		v->sim.R = v->inertial.R;
	} else if (v->physics_type == VESSEL_PHYSICS_NONINERTIAL) {
		coord_i2ni(v->inertial.x,v->inertial.y,v->inertial.z,
		           &v->noninertial.x,&v->noninertial.y,&v->noninertial.z);
		vec_i2ni(v->inertial.vx,v->inertial.vy,v->inertial.vz,
		         &v->noninertial.vx,&v->noninertial.vy,&v->noninertial.vz);
		vec_i2ni(v->inertial.ax,v->inertial.ay,v->inertial.az,
		         &v->noninertial.ax,&v->noninertial.ay,&v->noninertial.az);
		quat_i2ni(v->inertial.q,v->noninertial.q);
		v->noninertial.P = v->inertial.P;
		v->noninertial.Q = v->inertial.Q;
		v->noninertial.R = v->inertial.R;
	}
}


//==============================================================================
// Set by local coordinates
//==============================================================================
void vessels_set_local(vessel* v)
{
	if (v->physics_type == VESSEL_PHYSICS_SIM) {
		coord_l2sim(v,v->local.x,v->local.y,v->local.z,
		            &v->sim.x,&v->sim.y,&v->sim.z);
		vec_l2sim(v,v->local.vx,v->local.vy,v->local.vz,
		          &v->sim.vx,&v->sim.vy,&v->sim.vz);
		vec_l2sim(v,v->local.ax,v->local.ay,v->local.az,
		          &v->sim.ax,&v->sim.ay,&v->sim.az);
	} else if (v->physics_type == VESSEL_PHYSICS_INERTIAL) {
		coord_l2i(v,v->local.x,v->local.y,v->local.z,
		          &v->inertial.x,&v->inertial.y,&v->inertial.z);
		vec_l2i(v,v->local.vx,v->local.vy,v->local.vz,
		        &v->inertial.vx,&v->inertial.vy,&v->inertial.vz);
		vec_l2i(v,v->local.ax,v->local.ay,v->local.az,
		        &v->inertial.ax,&v->inertial.ay,&v->inertial.az);
	} else if (v->physics_type == VESSEL_PHYSICS_INERTIAL) {
		//Not supported
	}
}


//==============================================================================
// Get non-inertial coordinates from vessel
//==============================================================================
void vessels_get_ni(vessel* v)
{
	//Compute non-inertial coordinates
	if (v->physics_type == VESSEL_PHYSICS_SIM) {
		coord_sim2ni(v->sim.x,v->sim.y,v->sim.z,
		             &v->noninertial.x,&v->noninertial.y,&v->noninertial.z);
		vec_sim2ni(v->sim.vx,v->sim.vy,v->sim.vz,
		           &v->noninertial.vx,&v->noninertial.vy,&v->noninertial.vz);
		vec_sim2ni(v->sim.ax,v->sim.ay,v->sim.az,
		           &v->noninertial.ax,&v->noninertial.ay,&v->noninertial.az);
		quat_sim2ni(v->sim.q,v->noninertial.q);
		v->noninertial.P = v->sim.P;
		v->noninertial.Q = v->sim.Q;
		v->noninertial.R = v->sim.R;
	} else if (v->physics_type == VESSEL_PHYSICS_INERTIAL) {
		coord_i2ni(v->inertial.x,v->inertial.y,v->inertial.z,
		           &v->noninertial.x,&v->noninertial.y,&v->noninertial.z);
		vec_i2ni(v->inertial.vx,v->inertial.vy,v->inertial.vz,
		         &v->noninertial.vx,&v->noninertial.vy,&v->noninertial.vz);
		vec_i2ni(v->inertial.ax,v->inertial.ay,v->inertial.az,
		         &v->noninertial.ax,&v->noninertial.ay,&v->noninertial.az);
		quat_i2ni(v->inertial.q,v->noninertial.q);
		v->noninertial.P = v->inertial.P;
		v->noninertial.Q = v->inertial.Q;
		v->noninertial.R = v->inertial.R;
	}

	//Compute correct offset point
	{
		double distance2 =
		    (v->noninertial.cx-v->noninertial.x)*(v->noninertial.cx-v->noninertial.x)+
		    (v->noninertial.cy-v->noninertial.y)*(v->noninertial.cy-v->noninertial.y)+
		    (v->noninertial.cz-v->noninertial.z)*(v->noninertial.cz-v->noninertial.z);
		if (distance2 > 50000.0*50000.0) { //Centerpoint changes every 50 km
			v->noninertial.cx = v->noninertial.x;
			v->noninertial.cy = v->noninertial.y;
			v->noninertial.cz = v->noninertial.z;
		}
	}
}




//==============================================================================
// Initialize datarefs
//==============================================================================
void vessels_initialize()
{
	//Load datarefs
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	int i;

	dataref_vessel_x			= XPLMFindDataRef("sim/flightmodel/position/local_x");
	dataref_vessel_y			= XPLMFindDataRef("sim/flightmodel/position/local_y");
	dataref_vessel_z			= XPLMFindDataRef("sim/flightmodel/position/local_z");
	dataref_vessel_vx			= XPLMFindDataRef("sim/flightmodel/position/local_vx");
	dataref_vessel_vy			= XPLMFindDataRef("sim/flightmodel/position/local_vy");
	dataref_vessel_vz			= XPLMFindDataRef("sim/flightmodel/position/local_vz");
	dataref_vessel_ax			= XPLMFindDataRef("sim/flightmodel/position/local_ax");
	dataref_vessel_ay			= XPLMFindDataRef("sim/flightmodel/position/local_ay");
	dataref_vessel_az			= XPLMFindDataRef("sim/flightmodel/position/local_az");
	dataref_vessel_P			= XPLMFindDataRef("sim/flightmodel/position/Qrad");
	dataref_vessel_Q			= XPLMFindDataRef("sim/flightmodel/position/Rrad");
	dataref_vessel_R			= XPLMFindDataRef("sim/flightmodel/position/Prad");
	dataref_vessel_q			= XPLMFindDataRef("sim/flightmodel/position/q");
	dataref_vessel_mass			= XPLMFindDataRef("sim/aircraft/weight/acf_m_empty");
	dataref_vessel_extra_mass	= XPLMFindDataRef("sim/flightmodel/weight/m_fixed");
	dataref_vessel_jxx			= XPLMFindDataRef("sim/aircraft/weight/acf_Jxx_unitmass");
	dataref_vessel_jyy			= XPLMFindDataRef("sim/aircraft/weight/acf_Jyy_unitmass");
	dataref_vessel_jzz			= XPLMFindDataRef("sim/aircraft/weight/acf_Jzz_unitmass");
	dataref_fuel_mass			= XPLMFindDataRef("sim/flightmodel/weight/m_fuel");

	for (i = 0; i < 19; i++) {
		char buf[ARBITRARY_MAX] = { 0 };
		snprintf(buf,ARBITRARY_MAX-1,"sim/multiplayer/position/plane%d_x",i+1);
		dataref_plane_x[i] = XPLMFindDataRef(buf);
		snprintf(buf,ARBITRARY_MAX-1,"sim/multiplayer/position/plane%d_y",i+1);
		dataref_plane_y[i] = XPLMFindDataRef(buf);
		snprintf(buf,ARBITRARY_MAX-1,"sim/multiplayer/position/plane%d_z",i+1);
		dataref_plane_z[i] = XPLMFindDataRef(buf);
		snprintf(buf,ARBITRARY_MAX-1,"sim/multiplayer/position/plane%d_v_x",i+1);
		dataref_plane_vx[i] = XPLMFindDataRef(buf);
		snprintf(buf,ARBITRARY_MAX-1,"sim/multiplayer/position/plane%d_v_y",i+1);
		dataref_plane_vy[i] = XPLMFindDataRef(buf);
		snprintf(buf,ARBITRARY_MAX-1,"sim/multiplayer/position/plane%d_v_z",i+1);
		dataref_plane_vz[i] = XPLMFindDataRef(buf);
		snprintf(buf,ARBITRARY_MAX-1,"sim/multiplayer/position/plane%d_the",i+1);
		dataref_plane_pitch[i] = XPLMFindDataRef(buf);
		snprintf(buf,ARBITRARY_MAX-1,"sim/multiplayer/position/plane%d_phi",i+1);
		dataref_plane_roll[i] = XPLMFindDataRef(buf);
		snprintf(buf,ARBITRARY_MAX-1,"sim/multiplayer/position/plane%d_psi",i+1);
		dataref_plane_yaw[i] = XPLMFindDataRef(buf);
	}
#endif

	//Datarefs for instruments
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	dataref_instrument_pitch	= XPLMFindDataRef("sim/cockpit2/gauges/indicators/pitch_AHARS_deg_pilot");
	dataref_instrument_roll		= XPLMFindDataRef("sim/cockpit2/gauges/indicators/roll_AHARS_deg_pilot");
	dataref_instrument_heading	= XPLMFindDataRef("sim/cockpit2/gauges/indicators/heading_AHARS_deg_mag_pilot");
	dataref_instrument_vvi		= XPLMFindDataRef("sim/cockpit2/gauges/indicators/vvi_fpm_pilot");
	dataref_instrument_alt		= XPLMFindDataRef("sim/cockpit2/gauges/indicators/altitude_ft_pilot");
	dataref_instrument_airspeed	= XPLMFindDataRef("sim/cockpit2/gauges/indicators/airspeed_kts_pilot");
	dataref_instrument_magvar	= XPLMFindDataRef("sim/flightmodel/position/magnetic_variation");
	dataref_instrument_hpath	= XPLMFindDataRef("sim/flightmodel/position/hpath");
	dataref_instrument_vpath	= XPLMFindDataRef("sim/flightmodel/position/vpath");
#endif

	//X-Space datarefs for instruments
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	dataref_d("xsp/relative/pitch",		&instrument_pitch);
	dataref_d("xsp/relative/roll",		&instrument_roll);
	dataref_d("xsp/relative/yaw",		&instrument_yaw);
	dataref_d("xsp/relative/heading",	&instrument_heading);
	dataref_d("xsp/relative/vv",		&instrument_vvi);
	dataref_d("xsp/relative/alt",		&instrument_alt);
	dataref_d("xsp/relative/airspeed",	&instrument_airspeed);
	dataref_d("xsp/relative/magvar",	&instrument_magvar);
	dataref_d("xsp/relative/hpath",		&instrument_hpath);
	dataref_d("xsp/relative/vpath",		&instrument_vpath);
	dataref_d("xsp/relative/P",			&instrument_P);
	dataref_d("xsp/relative/Q",			&instrument_Q);
	dataref_d("xsp/relative/R",			&instrument_R);
	dataref_d("xsp/local/gx",			&instrument_gx);
	dataref_d("xsp/local/gy",			&instrument_gy);
	dataref_d("xsp/local/gz",			&instrument_gz);
#endif

	//No vessels
	vessel_alloc_count = 1024;
	vessels = malloc(vessel_alloc_count*sizeof(vessel));

	//Datarefs to all internal vessel information
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	vessels_register_dataref("xsp/vessel/exists",0);
	vessels_register_dataref("xsp/vessel/attached",1);
	vessels_register_dataref("xsp/vessel/net_id",2);
	vessels_register_dataref("xsp/vessel/networked",3);
	vessels_register_dataref("xsp/vessel/physics_type",4);
	vessels_register_dataref("xsp/vessel/is_plane",5);
	vessels_register_dataref("xsp/vessel/plane_index",6);
	vessels_register_dataref("xsp/vessel/ixx",7);
	vessels_register_dataref("xsp/vessel/iyy",8);
	vessels_register_dataref("xsp/vessel/izz",9);
	vessels_register_dataref("xsp/vessel/jxx",10);
	vessels_register_dataref("xsp/vessel/jyy",11);
	vessels_register_dataref("xsp/vessel/jzz",12);
	vessels_register_dataref("xsp/vessel/mass",13);
	vessels_register_dataref("xsp/vessel/latitude",14);
	vessels_register_dataref("xsp/vessel/longitude",15);
	vessels_register_dataref("xsp/vessel/elevation",16);
	vessels_register_dataref("xsp/vessel/weight/chassis",17);
	vessels_register_dataref("xsp/vessel/weight/hull",18);
	vessels_register_dataref("xsp/vessel/weight/fuel0",19);
	vessels_register_dataref("xsp/vessel/weight/fuel1",20);
	vessels_register_dataref("xsp/vessel/weight/fuel2",21);
	vessels_register_dataref("xsp/vessel/weight/fuel3",22);
	vessels_register_dataref("xsp/vessel/geometry/dx",23);
	vessels_register_dataref("xsp/vessel/geometry/dy",24);
	vessels_register_dataref("xsp/vessel/geometry/dz",25);
	vessels_register_dataref_v("xsp/vessel/weight/fuel",26,50);
	vessels_register_dataref_v("xsp/vessel/weight/fuel_x",27,50);
	vessels_register_dataref_v("xsp/vessel/weight/fuel_y",28,50);
	vessels_register_dataref_v("xsp/vessel/weight/fuel_z",29,50);

	vessels_register_dataref("xsp/vessel/inertial/x",30);
	vessels_register_dataref("xsp/vessel/inertial/y",31);
	vessels_register_dataref("xsp/vessel/inertial/z",32);
	vessels_register_dataref("xsp/vessel/inertial/vx",33);
	vessels_register_dataref("xsp/vessel/inertial/vy",34);
	vessels_register_dataref("xsp/vessel/inertial/vz",35);
	vessels_register_dataref("xsp/vessel/inertial/ax",36);
	vessels_register_dataref("xsp/vessel/inertial/ay",37);
	vessels_register_dataref("xsp/vessel/inertial/az",38);
	vessels_register_dataref("xsp/vessel/inertial/q0",39);
	vessels_register_dataref("xsp/vessel/inertial/q1",40);
	vessels_register_dataref("xsp/vessel/inertial/q2",41);
	vessels_register_dataref("xsp/vessel/inertial/q3",42);
	vessels_register_dataref("xsp/vessel/inertial/P",43);
	vessels_register_dataref("xsp/vessel/inertial/Q",44);
	vessels_register_dataref("xsp/vessel/inertial/R",45);

	vessels_register_dataref("xsp/vessel/noninertial/x",50);
	vessels_register_dataref("xsp/vessel/noninertial/y",51);
	vessels_register_dataref("xsp/vessel/noninertial/z",52);
	vessels_register_dataref("xsp/vessel/noninertial/vx",53);
	vessels_register_dataref("xsp/vessel/noninertial/vy",54);
	vessels_register_dataref("xsp/vessel/noninertial/vz",55);
	vessels_register_dataref("xsp/vessel/noninertial/ax",56);
	vessels_register_dataref("xsp/vessel/noninertial/ay",57);
	vessels_register_dataref("xsp/vessel/noninertial/az",58);
	vessels_register_dataref("xsp/vessel/noninertial/q0",59);
	vessels_register_dataref("xsp/vessel/noninertial/q1",60);
	vessels_register_dataref("xsp/vessel/noninertial/q2",61);
	vessels_register_dataref("xsp/vessel/noninertial/q3",62);
	vessels_register_dataref("xsp/vessel/noninertial/P",63);
	vessels_register_dataref("xsp/vessel/noninertial/Q",64);
	vessels_register_dataref("xsp/vessel/noninertial/R",65);

	vessels_register_dataref("xsp/vessel/local/magnetic/incl",70);
	vessels_register_dataref("xsp/vessel/local/magnetic/decl",71);
	vessels_register_dataref("xsp/vessel/local/magnetic/F",72);
	vessels_register_dataref("xsp/vessel/local/magnetic/H",73);
	vessels_register_dataref("xsp/vessel/noninertial/magnetic/X",74);
	vessels_register_dataref("xsp/vessel/noninertial/magnetic/Y",75);
	vessels_register_dataref("xsp/vessel/noninertial/magnetic/Z",76);
	vessels_register_dataref("xsp/vessel/local/magnetic/X",77);
	vessels_register_dataref("xsp/vessel/local/magnetic/Y",78);
	vessels_register_dataref("xsp/vessel/local/magnetic/Z",79);
	vessels_register_dataref("xsp/vessel/local/magnetic/GV",80);
	vessels_register_dataref("xsp/vessel/gravitational/g",81);
	vessels_register_dataref("xsp/vessel/gravitational/V",82);

	vessels_register_dataref("xsp/vessel/detach",90);
	vessels_register_dataref("xsp/vessel/mount/x",91);
	vessels_register_dataref("xsp/vessel/mount/y",92);
	vessels_register_dataref("xsp/vessel/mount/z",93);
	vessels_register_dataref("xsp/vessel/weight/tps",94);
	vessels_register_dataref("xsp/vessel/client_id",96);

	vessels_register_dataref("xsp/vessel/local/atmosphere/density",100);
	vessels_register_dataref("xsp/vessel/local/atmosphere/concentration",101);
	vessels_register_dataref("xsp/vessel/local/atmosphere/temperature",102);
	vessels_register_dataref("xsp/vessel/local/atmosphere/pressure",103);
	vessels_register_dataref("xsp/vessel/local/atmosphere/Q",104);
	vessels_register_dataref("xsp/vessel/local/atmosphere/exospheric_temperature",105);
	vessels_register_dataref("xsp/vessel/local/atmosphere/concentration_He",106);
	vessels_register_dataref("xsp/vessel/local/atmosphere/concentration_O",107);
	vessels_register_dataref("xsp/vessel/local/atmosphere/concentration_N2",108);
	vessels_register_dataref("xsp/vessel/local/atmosphere/concentration_O2",109);
	vessels_register_dataref("xsp/vessel/local/atmosphere/concentration_Ar",110);
	vessels_register_dataref("xsp/vessel/local/atmosphere/concentration_H",111);
	vessels_register_dataref("xsp/vessel/local/atmosphere/concentration_N",112);
	vessels_register_dataref("xsp/vessel/local/atmosphere/density_He",113);
	vessels_register_dataref("xsp/vessel/local/atmosphere/density_O",114);
	vessels_register_dataref("xsp/vessel/local/atmosphere/density_N2",115);
	vessels_register_dataref("xsp/vessel/local/atmosphere/density_O2",116);
	vessels_register_dataref("xsp/vessel/local/atmosphere/density_Ar",117);
	vessels_register_dataref("xsp/vessel/local/atmosphere/density_H",118);
	vessels_register_dataref("xsp/vessel/local/atmosphere/density_N",119);

	vessels_register_dataref("xsp/vessel/local/x", 140);
	vessels_register_dataref("xsp/vessel/local/y", 141);
	vessels_register_dataref("xsp/vessel/local/z", 142);
	vessels_register_dataref("xsp/vessel/local/vx",143);
	vessels_register_dataref("xsp/vessel/local/vy",144);
	vessels_register_dataref("xsp/vessel/local/vz",145);
	vessels_register_dataref("xsp/vessel/local/ax",146);
	vessels_register_dataref("xsp/vessel/local/ay",147);
	vessels_register_dataref("xsp/vessel/local/az",148);
	vessels_register_dataref("xsp/vessel/relative/q0",149);
	vessels_register_dataref("xsp/vessel/relative/q1",150);
	vessels_register_dataref("xsp/vessel/relative/q2",151);
	vessels_register_dataref("xsp/vessel/relative/q3",152);
	vessels_register_dataref("xsp/vessel/relative/P", 153);
	vessels_register_dataref("xsp/vessel/relative/Q", 154);
	vessels_register_dataref("xsp/vessel/relative/R", 155);
	vessels_register_dataref("xsp/vessel/relative/roll", 156);
	vessels_register_dataref("xsp/vessel/relative/pitch", 157);
	vessels_register_dataref("xsp/vessel/relative/yaw", 158);
	vessels_register_dataref("xsp/vessel/relative/heading", 159);
	vessels_register_dataref("xsp/vessel/relative/airspeed", 160);
	vessels_register_dataref("xsp/vessel/relative/hpath", 161);
	vessels_register_dataref("xsp/vessel/relative/vpath", 162);
	vessels_register_dataref("xsp/vessel/relative/vv", 163);
	vessels_register_dataref("xsp/vessel/local/gx", 164);
	vessels_register_dataref("xsp/vessel/local/gy", 165);
	vessels_register_dataref("xsp/vessel/local/gz", 166);
#endif

	//Initialize highlevel interface
	vessels_highlevel_initialize();

	//Re-initialize aircraft
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	vessels_update_aircraft();
#endif
}


//==============================================================================
// Reinitialize vessels list
//==============================================================================
void vessels_reinitialize()
{
	//Initialize main vessel (on dedicated server this represents the server itself)
	vessel_count = 0;
	vessels_add();
	//memset(&vessels[0],0,sizeof(vessel));
	//vessels[0].index = 0;
	vessels[0].geometry.hull = material_get("Aluminium");
	vessels[0].exists = 1;
}


//==============================================================================
// Free resources
//==============================================================================
void vessels_deinitialize()
{
	free(vessels);
	vessels = 0;
}




//==============================================================================
// Add new body
//==============================================================================
int vessels_add()
{
	int new_index = -1;
	int i;

	//Try to find non-existant vessel
	for (i = 0; i < vessel_count; i++) {
		if (!vessels[i].exists) {
			new_index = i;
			break;
		}
	}

	//Initialize more memory (FIXME: no out of mem check)
	if (new_index == -1) {
		new_index = vessel_count;
		vessel_count++;
	}

	//Reset vessel
	memset(&vessels[new_index],0,sizeof(vessel));
	vessels[new_index].geometry.hull = material_get("Aluminium");
	vessels[new_index].index = new_index;
	vessels[new_index].net_id = 0;

	//Network signature
	memset(vessels[new_index].net_signature,0,sizeof(vessels[new_index].net_signature));

#if defined(DEDICATED_SERVER)
	radiosys_initialize_vessel(&vessels[new_index]);
#endif
	return new_index;
}


//==============================================================================
// Load vessel object
//==============================================================================
int vessels_load(char* filename)
{
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	char path[MAX_FILENAME] = { 0 }, model[MAX_FILENAME] = { 0 };
	int new_index = vessels_add();
	//char* acfpath;

	XPLMGetNthAircraftModel(0, model, path);
	XPLMExtractFileAndPath(path);

	//Try to fetch aicraft path
	//acfpath = strstr(path,"Aircraft");
	//if (acfpath) snprintf(path,MAX_FILENAME-1,"./%s/",acfpath);
#ifdef APL
	{
		char* tpath;
		while (tpath = strchr(path,':')) *tpath = '/';
		strncpy(model,path,MAX_FILENAME-1);
		strncpy(path,"/Volumes/",MAX_FILENAME-1);
		strncat(path,model,MAX_FILENAME-1);
	}
#endif
#else
	char path[MAX_FILENAME] = { 0 };
	int new_index = vessels_add();

	strncpy(path,FROM_PLUGINS("aircraft/"),MAX_FILENAME-1);
#endif

	//Fetch vessel path
	//strncat(path,"/objects/",MAX_FILENAME-1);
	strncat(path,"/",MAX_FILENAME-1);
	strncat(path,filename,MAX_FILENAME-1);

	//Initialize vessel
	vessels[new_index].exists = 1;
	vessels[new_index].is_plane = 1;
	strncpy(vessels[new_index].plane_filename,path,1023);

	//Load physics
	vessels[new_index].weight.chassis = 100.0;
	vessels[new_index].weight.hull = 100.0;
	vessels[new_index].jxx = 10;
	vessels[new_index].jyy = 10;
	vessels[new_index].jzz = 10;

	//Load geometry
	//...

	/*vessels[new_index].geometry.obj_ref = (void*)XPLMLoadObject(path);
	if (!vessels[new_index].geometry.obj_ref) { //Try to load from networked objects
		strncpy(path,FROM_PLUGINS("obj/network"),MAX_FILENAME-1);
		strncat(path,filename,MAX_FILENAME-1);
		vessels[new_index].geometry.obj_ref = (void*)XPLMLoadObject(path);
	}
	if (!vessels[new_index].geometry.obj_ref) {
		log_write("X-Space: Error loading model %s!\n",path);
	}*/

	//Load interesting stuff
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	vessels_update_aircraft(); //FIXME: sad hack
	xspace_reload_aircraft(new_index);
#endif

	return new_index;
}


//==============================================================================
// Update x-plane list
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void vessels_update_aircraft()
{
	int i,num_planes,total_planes;
	char* plane_filenames[20];

	num_planes = 0;
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].is_plane) {
			vessels[i].plane_index = num_planes;
			if (num_planes < 20) {
				plane_filenames[num_planes] = vessels[i].plane_filename;
				num_planes++;
			}
		}
	}
	total_planes = num_planes;

	//Unload other models
	while (num_planes < 20) {
		plane_filenames[num_planes] = 0;
		num_planes++;
	} //FIXME: don't do this if there are no planes
	xplane_load_aircraft(plane_filenames,total_planes);

	//Reload aircraft data
	//for (i = 0; i < total_planes; i++) {
		//xspace_reload_aircraft(i+1);
	//}
}
#endif




//==============================================================================
// Draw vessels
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void vessel_draw()
{
	/*int i;
	XPLMDrawInfo_t drawinfo;
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].geometry.obj_ref) {
			double minY = 0.0;
			double r,p,q;

			//Probe terrain under it
			{
				extern XPLMProbeRef lpad_probe;
				XPLMProbeInfo_t probeinfo;
				double lat,lon,elev;
				double x,y,z;

				probeinfo.structSize = sizeof(XPLMProbeInfo_t);
				XPLMLocalToWorld(vessels[i].sim.x,vessels[i].sim.y,vessels[i].sim.z,
				                 &lat,&lon,&elev);
				XPLMWorldToLocal(lat,lon,0,&x,&y,&z);
				if (XPLMProbeTerrainXYZ(lpad_probe,(float)x,(float)y,(float)z,&probeinfo) == xplm_ProbeHitTerrain) {
					minY = probeinfo.locationY;
				}
			}

			//Get orientation
			qeuler_to(vessels[i].sim.q,&r,&p,&q);

			//Setup draw info structure
			drawinfo.structSize = sizeof(XPLMDrawInfo_t);
			drawinfo.x = (float)vessels[i].sim.x;
			drawinfo.y = (float)vessels[i].sim.y;
			drawinfo.z = (float)vessels[i].sim.z;
			drawinfo.pitch = DEGf(p);
			drawinfo.heading = DEGf(q);
			drawinfo.roll = DEGf(r);

			//Check correct Y
			if (drawinfo.y < minY) drawinfo.y = (float)minY;

			//Draw vessel
			XPLMDrawObjects(vessels[i].geometry.obj_ref,1,&drawinfo,0,0);
		}
	}*/
}
#endif




//==============================================================================
// Apply force onto vessel
//==============================================================================
void vessels_addforce(vessel* v, double dt, double lx, double ly, double lz, double fx, double fy, double fz)
{
	double  x, y, z; //Force apply point
	double gx,gy,gz; //Global-intermediate force
	double tx,ty,tz; //Torque
	double ax,ay,az; //Linear acceleration
	double wx,wy,wz; //Angular acceleration
	vessel* root = v;

	//Calculate correct apply point and mass
	if ((v->attached != 0) && (v->attached != VESSEL_MOUNT_LAUNCHPAD)) {
		if (v->index > 0) {
			root = &vessels[v->mount.root_body];
			x = lx+v->mount.rx;
			y = ly+v->mount.ry;
			z = lz+v->mount.rz;
		} else {
			x = lx+v->mount.x;
			y = ly+v->mount.y;
			z = lz+v->mount.z;
		}
	} else {
		x = lx;
		y = ly;
		z = lz;
	}

	//Calculate torque
	tx = y*fz - z*fy; //FIXME: reference point
	ty = z*fx - x*fz;
	tz = x*fy - y*fx;

	//Apply torque
	wx = tx/root->ixx;
	wy = ty/root->iyy;
	wz = tz/root->izz;

	//Turn local coordinate system coords into global
	if (root->physics_type == VESSEL_PHYSICS_INERTIAL) {
		vec_l2i(root,fx,fy,fz,&gx,&gy,&gz);
	} else {
		vec_l2sim(root,fx,fy,fz,&gx,&gy,&gz);
	}

	//Apply force
	ax = gx/root->mass;
	ay = gy/root->mass;
	az = gz/root->mass;
	
	//Add angular acceleration
	root->accumulated.Pd += wy;
	root->accumulated.Qd += wz;
	root->accumulated.Rd += wx;

	//Add linear acceleration
	root->accumulated.ax += ax;
	root->accumulated.ay += ay;
	root->accumulated.az += az;
}


//==============================================================================
// Update mount physics
//==============================================================================
void vessels_update_mount_physics(vessel* v)
{
	//Do not update non-existant vessel
	if (!v->exists) return;

	//Update mount joints and force physics type same as of the parent
	if ((v->attached != 0) && (v->attached != VESSEL_MOUNT_LAUNCHPAD)) {
		vessel* root = &vessels[v->mount.root_body];
		if (root->physics_type == VESSEL_PHYSICS_SIM) {
			double sx,sy,sz;
			coord_l2sim(root,v->mount.rx,v->mount.ry,v->mount.rz,&sx,&sy,&sz);
			v->sim.x = sx;
			v->sim.y = sy;
			v->sim.z = sz;
			v->sim.vx = root->sim.vx;
			v->sim.vy = root->sim.vy;
			v->sim.vz = root->sim.vz;
			v->sim.ax = root->sim.ax;
			v->sim.ay = root->sim.ay;
			v->sim.az = root->sim.az;
			v->sim.P = root->sim.P;
			v->sim.Q = root->sim.Q;
			v->sim.R = root->sim.R;

			//Fix for accelerometers
			//v->inertial.ax = root->inertial.ax;
			//v->inertial.ay = root->inertial.ay;
			//v->inertial.az = root->inertial.az;

			qmul(v->sim.q,root->sim.q,v->mount.rattitude);
			//if (v->index == 0) {
				v->physics_type = VESSEL_PHYSICS_SIM;
			//} else {
			//	v->physics_type = VESSEL_PHYSICS_INERTIAL;
			//}
		} else {
			double ix,iy,iz;
			quaternion q;
			coord_l2i(root,v->mount.rx,v->mount.ry,v->mount.rz,&ix,&iy,&iz);
			v->inertial.x = ix;
			v->inertial.y = iy;
			v->inertial.z = iz;
			v->inertial.vx = root->inertial.vx;
			v->inertial.vy = root->inertial.vy;
			v->inertial.vz = root->inertial.vz;
			v->inertial.ax = root->inertial.ax;
			v->inertial.ay = root->inertial.ay;
			v->inertial.az = root->inertial.az;
			v->inertial.P = root->inertial.P;
			v->inertial.Q = root->inertial.Q;
			v->inertial.R = root->inertial.R;

			quat_i2sim(root->inertial.q,q);
			qmul(q,q,v->mount.rattitude);
			quat_sim2i(q,v->inertial.q);
			v->physics_type = VESSEL_PHYSICS_INERTIAL;
		}
	}
}


//==============================================================================
// Update physics (weights, moments of inertia)
//==============================================================================
void vessels_update_physics(vessel* v)
{
	int i;
	//if (v->mass == 0) v->mass = 1000;
	//v->ixx = v->jxx*v->mass;
	//v->iyy = v->jyy*v->mass;
	//v->izz = v->jzz*v->mass;
	//if (v->ixx == 0) v->ixx = 10000;
	//if (v->iyy == 0) v->iyy = 10000;
	//if (v->izz == 0) v->izz = 10000;

	//Exit if this was already checked
	if (v->mount.checked) return;
	v->mount.checked = 1;

	//Do not update non-existant vessel (FIXME: check if root vessel stopped existing)
	if (!v->exists) return;

	//Calculate MI/mass for this vessel
	v->mass = 0.0;
	v->attached_mass = 0.0;
	for (i = 0; i < VESSEL_MAX_FUELTANKS; i++) v->mass += v->weight.fuel[i];
	v->mass += v->weight.chassis;
	v->mass += v->weight.hull;

	v->ixx = v->mass*v->jxx;
	v->iyy = v->mass*v->jyy;
	v->izz = v->mass*v->jzz;

	//Add this mass to the root body
	if ((v->attached != 0) && (v->attached != VESSEL_MOUNT_LAUNCHPAD)) {
		vessel* root = &vessels[v->mount.body];
		if ((root->attached == 0) || (root->attached == VESSEL_MOUNT_LAUNCHPAD)) { //If root body not mounted to anything
			//Update physics of root body
			vessels_update_physics(root);

			//Set root body for this one
			v->mount.root_body = root->index;
			v->mount.rx = v->mount.x; //Offset of this one from the root body
			v->mount.ry = v->mount.y;
			v->mount.rz = v->mount.z;
			memcpy(&v->mount.rattitude,&v->mount.attitude,sizeof(quaternion));

			//Add own mass to mass of root body
			root->mass += v->mass;
			root->attached_mass += v->mass;

			//Add own moments of inertia to MI of the root mass
			root->ixx += v->ixx + v->mass*(v->mount.ry*v->mount.ry+v->mount.rz*v->mount.rz);
			root->iyy += v->iyy + v->mass*(v->mount.rx*v->mount.rx+v->mount.rz*v->mount.rz);
			root->izz += v->izz + v->mass*(v->mount.rx*v->mount.rx+v->mount.ry*v->mount.ry);
		} else { //Root body is recursively mounted to some other body
			//Correctly rotated centerpoint of this body (rotated by attitude of the mount body)
			quaternion q;
			double x,y,z;

			//Update physics of root body
			vessels_update_physics(root);

			//Rotate mount coordinates by attitude of mount body
			qset_vec(q,v->mount.x,v->mount.y,v->mount.z);
			qdiv(q,q,root->mount.attitude); //NOT rattitude! must rotate around the mount body, not root body
			qmul(q,root->mount.attitude,q);
			qget_vec(q,&x,&y,&z);

			//Set root body
			v->mount.root_body = root->mount.root_body;
			v->mount.rx = x + root->mount.rx; //Offset of this one from the root body
			v->mount.ry = y + root->mount.ry;
			v->mount.rz = z + root->mount.rz;
			qmul(v->mount.rattitude,root->mount.rattitude,v->mount.attitude);

			//Add own mass to the root body (the most root there is)
			vessels[root->mount.root_body].mass += v->mass;
			vessels[root->mount.root_body].attached_mass += v->mass;

			//Add own moments of inertia to MI of the root mass
			vessels[root->mount.root_body].ixx += v->ixx + v->mass*(v->mount.ry*v->mount.ry+v->mount.rz*v->mount.rz);
			vessels[root->mount.root_body].iyy += v->iyy + v->mass*(v->mount.rx*v->mount.rx+v->mount.rz*v->mount.rz);
			vessels[root->mount.root_body].izz += v->izz + v->mass*(v->mount.rx*v->mount.rx+v->mount.ry*v->mount.ry);
		}
	}
}

//==============================================================================
// Reset physics calculations
//==============================================================================
void vessels_reset_physics(vessel* v)
{
	v->accumulated.ax = 0;	v->accumulated.ay = 0;	v->accumulated.az = 0;
	v->accumulated.Pd = 0;	v->accumulated.Qd = 0;	v->accumulated.Rd = 0;

	if (v->physics_type != VESSEL_PHYSICS_SIM) {
		v->inertial.ax = 0; v->inertial.ay = 0; v->inertial.az = 0;
	}
	v->inertial.Pd = 0; v->inertial.Qd = 0; v->inertial.Rd = 0;
}


//==============================================================================
// Compute moments based on geometry, weight
//==============================================================================
void vessels_compute_moments(vessel* v)
{
	int i;
	double jxx,jyy,jzz;
	face* faces = v->geometry.faces;

	if (!faces) {
		v->jxx = 1.0;
		v->jyy = 1.0;
		v->jzz = 1.0;
		return;
	}

	//Calculate radius of gyration
	jxx = 0.0;
	jyy = 0.0;
	jzz = 0.0;
	for (i = 0; i < v->geometry.num_faces; i++) {
		double m1 = faces[i].area;
		double m3 = 0.0*faces[i].area*(1.0/3.0);

		double jxx_tri = m3*(pow(faces[i].ay-faces[i].y[0],2) + pow(faces[i].az-faces[i].z[0],2))+
						 m3*(pow(faces[i].ay-faces[i].y[1],2) + pow(faces[i].az-faces[i].z[1],2))+
						 m3*(pow(faces[i].ay-faces[i].y[2],2) + pow(faces[i].az-faces[i].z[2],2));
		double jyy_tri = m3*(pow(faces[i].ax-faces[i].y[0],2) + pow(faces[i].az-faces[i].z[0],2))+
						 m3*(pow(faces[i].ax-faces[i].y[1],2) + pow(faces[i].az-faces[i].z[1],2))+
						 m3*(pow(faces[i].ax-faces[i].y[2],2) + pow(faces[i].az-faces[i].z[2],2));
		double jzz_tri = m3*(pow(faces[i].ax-faces[i].y[0],2) + pow(faces[i].ay-faces[i].z[0],2))+
						 m3*(pow(faces[i].ax-faces[i].y[1],2) + pow(faces[i].ay-faces[i].z[1],2))+
						 m3*(pow(faces[i].ax-faces[i].y[2],2) + pow(faces[i].ay-faces[i].z[2],2));

		jxx += jxx_tri + m1*(pow(faces[i].ay,2) + pow(faces[i].az,2));
		jyy += jyy_tri + m1*(pow(faces[i].ax,2) + pow(faces[i].az,2));
		jzz += jzz_tri + m1*(pow(faces[i].ax,2) + pow(faces[i].ay,2));
	}

	//Divide by total mass
	v->jxx = jxx/v->geometry.total_area;
	v->jyy = jyy/v->geometry.total_area;
	v->jzz = jzz/v->geometry.total_area;
}




//==============================================================================
// High-level vessel interface
//==============================================================================
#define DEFINE_VESSEL() \
	vessel* v = 0; \
	int idx = luaL_checkint(L,1); \
	if ((idx >= 0) && (idx < vessel_count)) { \
		v = &vessels[idx]; \
	}

int vessels_highlevel_getcount(lua_State* L)
{
	lua_pushnumber(L,vessel_count);
	return 1;
}

int vessels_highlevel_getparameter(lua_State* L)
{
	int paramidx = luaL_checkint(L,2);
	int arridx = lua_tointeger(L,3);
	DEFINE_VESSEL();
	if (v && (paramidx >= 0) && (paramidx < 1000)) {
		lua_pushnumber(L,vessels_read_param_d(v->index,paramidx,arridx));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int vessels_highlevel_setparameter(lua_State* L)
{
	int paramidx = luaL_checkint(L,2);
	int arridx = 0;
	DEFINE_VESSEL();
	if (v && (paramidx >= 0) && (paramidx < 1000)) {
		if (lua_isnumber(L,4)) {
			arridx = lua_tointeger(L,3);
			luaL_checknumber(L,4);
			vessels_write_param_d(v->index,paramidx,arridx,lua_tonumber(L,4));
		} else {
			luaL_checknumber(L,3);
			vessels_write_param_d(v->index,paramidx,arridx,lua_tonumber(L,3));
		}
	}
	return 0;
}

int vessels_highlevel_getnoninertial(lua_State* L)
{
	DEFINE_VESSEL();
	if (v) vessels_get_ni(v);
	return 0;
}

int vessels_highlevel_setnoninertial(lua_State* L)
{
	DEFINE_VESSEL();
	if (v) vessels_set_ni(v);
	return 0;
}

int vessels_highlevel_setinertial(lua_State* L)
{
	DEFINE_VESSEL();
	if (v) vessels_set_i(v);
	return 0;
}

int vessels_highlevel_setlocal(lua_State* L)
{
	DEFINE_VESSEL();
	if (v) vessels_set_local(v);
	return 0;
}

int vessels_highlevel_getbynetid(lua_State* L)
{
	int i;
	int net_id = lua_tointeger(L,1);
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].net_id == net_id) {
			lua_pushinteger(L,i);
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

int vessels_highlevel_create(lua_State* L)
{
	int v_idx = vessels_add();

	//Push metadata
	lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels_data);
	lua_pushvalue(L,1);
	lua_rawseti(L,-2,v_idx);
	lua_pop(L,1);

	//Return vessel index
	lua_pushnumber(L,v_idx);
	return 1;
}

int vessels_highlevel_add(lua_State* L)
{
	int v_idx = vessels_load((char*)luaL_checkstring(L,1));

	//Push metadata
	lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels_data);
	lua_pushvalue(L,2);
	lua_rawseti(L,-2,v_idx);
	lua_pop(L,1);

	//Return vessel index
	lua_pushnumber(L,v_idx);
	return 1;
}

int vessels_highlevel_mount(lua_State* L)
{
	vessel* root = 0;
	vessel* v = 0;

	int idx_root = luaL_checkint(L,1); //Parent vessel ID
	int idx_v    = luaL_checkint(L,2); //Mounted vessel ID
	if ((idx_root >= 0) && (idx_root < vessel_count)) root = &vessels[idx_root];
	if ((idx_v >= 0) && (idx_v < vessel_count)) v = &vessels[idx_v];

	if (v && root) {
		v->attached = 1;
		v->mount.body = root->index;
		qeuler_from(v->mount.attitude,0,0,0);
	}
	return 0;
}

int vessels_highlevel_setmountoffset(lua_State* L)
{
	DEFINE_VESSEL();
	if (v) {
		luaL_checknumber(L,2);
		luaL_checknumber(L,3);
		luaL_checknumber(L,4);
		v->mount.x = lua_tonumber(L,2);
		v->mount.y = lua_tonumber(L,3);
		v->mount.z = lua_tonumber(L,4);
	}
	return 0;
}

int vessels_highlevel_setmountattitude(lua_State* L)
{
	DEFINE_VESSEL();
	if (v) {
		luaL_checknumber(L,2);
		luaL_checknumber(L,3);
		luaL_checknumber(L,4);
		qeuler_from(v->mount.attitude,
		            RAD(lua_tonumber(L,2)),
		            RAD(lua_tonumber(L,3)),
		            RAD(lua_tonumber(L,4)));
	}
	return 0;
}

int vessels_highlevel_computemoments(lua_State* L)
{
	DEFINE_VESSEL();
	if (v) vessels_compute_moments(v);
	return 0;
}

int vessels_highlevel_setfueltank(lua_State* L)
{
	int tank = luaL_checkint(L,2); //Tank ID
	DEFINE_VESSEL();
	if (v && (tank >= 0) && (tank < VESSEL_MAX_FUELTANKS)) {
		luaL_checknumber(L,3);
		v->weight.fuel[tank] = lua_tonumber(L,3);
	}
	return 0;
}

int vessels_highlevel_getfueltank(lua_State* L)
{
	int tank = luaL_checkint(L,2); //Tank ID
	DEFINE_VESSEL();
	if (v && (tank >= 0) && (tank < VESSEL_MAX_FUELTANKS)) {
		lua_pushnumber(L,v->weight.fuel[tank]);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int vessels_highlevel_getfilename(lua_State* L)
{
	DEFINE_VESSEL();
	if (v) {
		if (v->index == 0) {
			lua_pushstring(L,"vessel 0");
		} else if (v->is_plane) {
			lua_pushstring(L,v->plane_filename);
		} else {
			lua_pushstring(L,"unknown vessel");
		}
	} else {
		lua_pushnil(L);
	}
	return 1;
}


//==============================================================================
// Initializes highlevel vessel functions
//==============================================================================
void vessels_highlevel_initialize()
{
	//Register API
	lua_createtable(L,0,32);
	lua_setglobal(L,"VesselAPI");

	highlevel_addfunction("VesselAPI","GetCount",vessels_highlevel_getcount);
	highlevel_addfunction("VesselAPI","GetParameter",vessels_highlevel_getparameter);
	highlevel_addfunction("VesselAPI","SetParameter",vessels_highlevel_setparameter);
	highlevel_addfunction("VesselAPI","GetNoninertialCoordinates",vessels_highlevel_getnoninertial);
	highlevel_addfunction("VesselAPI","SetNoninertialCoordinates",vessels_highlevel_setnoninertial);
	highlevel_addfunction("VesselAPI","SetInertialCoordinates",vessels_highlevel_setinertial);
	highlevel_addfunction("VesselAPI","SetLocalCoordinates",vessels_highlevel_setlocal);

	highlevel_addfunction("VesselAPI","GetByNetID",vessels_highlevel_getbynetid);
	highlevel_addfunction("VesselAPI","Create",vessels_highlevel_create);
	highlevel_addfunction("VesselAPI","Add",vessels_highlevel_add);
	highlevel_addfunction("VesselAPI","Mount",vessels_highlevel_mount);
	highlevel_addfunction("VesselAPI","SetMountOffset",vessels_highlevel_setmountoffset);
	highlevel_addfunction("VesselAPI","SetMountAttitude",vessels_highlevel_setmountattitude);

	highlevel_addfunction("VesselAPI","ComputeMoments",vessels_highlevel_computemoments);
	highlevel_addfunction("VesselAPI","SetFuelTank",vessels_highlevel_setfueltank);
	highlevel_addfunction("VesselAPI","GetFuelTank",vessels_highlevel_getfueltank);
	highlevel_addfunction("VesselAPI","GetFilename",vessels_highlevel_getfilename);
}

void vessels_highlevel_logic()
{
	int i;

	lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels);
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists) {
			lua_rawgeti(L,-1,i);
			if (lua_istable(L,-1)) {
				lua_getfield(L,-1,"Logic");
				if (lua_isfunction(L,-1)) {
					lua_pushvalue(L,-2);
					lua_setfenv(L,-2); //Set aicraft environment
					highlevel_call(0,0);
				} else lua_pop(L,1);
			}
			lua_pop(L,1);
		}
	}
	lua_pop(L,1); //Pop vessels table
}