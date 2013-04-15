#include <math.h>
#include "x-space.h"
#include "dataref.h"
#include "vessel.h"
#include "planet.h"
#include "coordsys.h"
#include "quaternion.h"
#include "config.h"

//X-Plane SDK
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include "rendering.h"
#include <XPLMGraphics.h>
#endif


//==============================================================================
// Coordinate system state
//==============================================================================
quaternion planet_rotation;		//Quaternion defining planet rotation
quaternion i2sim_rotation;		//Quaternion defining rotation from inertial to X-Plane coordinate systen
quaternion sim2i_rotation;		//Quaternion defining rotation from non-inertial to inertial system
quaternion i2rel_rotation;		//Quaternion defining rotation from inertial to ground-relative cooridinate system
quaternion ni2sim_rotation;		//Quaternion defining rotation from non-inertial to X-Plane coordinate systen
quaternion sim2ni_rotation;		//Quaternion defining rotation X-Plane coordinate systen to non-inertial system
quaternion i2ni_rotation;		//Quaternion defining rotation from inertial to non-inertial
quaternion ni2i_rotation;		//Quaternion defining rotation from non-inertial to inertial

//#define HISTORY_POINTS 60*60*4
//double coord_history_timeout;
//double coord_history_xyz[(MAX_BODIES-1)*HISTORY_POINTS*3];
//int coord_history_next;
//XPLMDataRef coord_history_mode;

//Other speeds
double ni_vmag,gen_vmag,gen_vgnd,gen_vvert,true_vgnd;


//==============================================================================
// Returns velocity of still object in non-inertial simulator coordinate system
//==============================================================================
void coordsys_retvel(vessel* v, double* nx, double* ny, double* nz)
{
	double wx,wy,wz;
	double rx,ry,rz;

	rx = v->sim.x; //Position
	ry = v->sim.y+current_planet.radius;
	rz = v->sim.z;

	wx = 0; //Setup earth rotation velocity
	wy = current_planet.w * sin(RAD(v->latitude));
	wz = current_planet.w * cos(RAD(v->latitude));

	*nx = wy*rz-wz*ry; //w x r
	*ny = wz*rx-wx*rz;
	*nz = wx*ry-wy*rx;
}


//==============================================================================
// Given non-inertial coordinates compute all others
//==============================================================================
void coordsys_update_datarefs(vessel* base, vessel* v)
{
	double vx,vy,vz;
	double nx,ny,nz;
	coordsys_retvel(v,&nx,&ny,&nz);

	//Non-inertial
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
	vec_sim2ni(v->sim.vx,v->sim.vy,v->sim.vz,&v->noninertial.nvx,&v->noninertial.nvy,&v->noninertial.nvz);

	//Local (FIXME: better have them rotated from inertial coordinates)
	coord_sim2l(base,v->sim.x,v->sim.y,v->sim.z,          &v->local.x, &v->local.y, &v->local.z);
	/*vec_sim2l(base,v->sim.vx+nx,v->sim.vy+ny,v->sim.vz+nz,&v->local.vx,&v->local.vy,&v->local.vz);
	vec_sim2l(base,v->sim.ax,v->sim.ay,v->sim.az,         &v->local.ax,&v->local.ay,&v->local.az);*/
	//coord_i2l(base,v->inertial.x,v->inertial.y,v->inertial.z,&v->local.x,&v->local.y,&v->local.z);
	vec_ni2l(v,v->noninertial.vx,v->noninertial.vy,v->noninertial.vz,&v->local.vx,&v->local.vy,&v->local.vz);
	vec_ni2l(v,v->noninertial.ax,v->noninertial.ay,v->noninertial.az,&v->local.ax,&v->local.ay,&v->local.az);

	if (v->physics_type == VESSEL_PHYSICS_INERTIAL) {
		double gx,gy,gz;
		vec_i2l(v,v->inertial.ax,v->inertial.ay,v->inertial.az,&gx,&gy,&gz);

		v->relative.gx = -gx/9.81;
		v->relative.gy = -gy/9.81;
		v->relative.gz = gz/9.81;
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
		v->relative.gx = -gx/9.81;
		v->relative.gy = -gy/9.81;
		v->relative.gz = gz/9.81;
	}

	//Relative to ground
	vec_i2rel(v->inertial.vx,v->inertial.vy,v->inertial.vz,&vx,&vy,&vz);
	quat_i2rel(v->inertial.q,v->relative.q);
	qeuler_to(v->relative.q,&v->relative.roll,&v->relative.pitch,&v->relative.yaw);

	v->relative.roll = DEG(v->relative.roll);
	v->relative.pitch = DEG(v->relative.pitch);
	v->relative.yaw = DEG(v->relative.yaw);
	v->relative.heading = DEG(v->relative.yaw) - v->geomagnetic.declination;
	v->relative.P = DEG(v->inertial.P);
	v->relative.Q = DEG(v->inertial.Q);
	v->relative.R = DEG(v->inertial.R);
	v->relative.hpath = DEG(atan2(vx,-vz));
	v->relative.vpath = DEG(atan2(vy,sqrt(vx*vx+vz*vz)));
	v->relative.vv = sqrt(pow(v->noninertial.x,2)+pow(v->noninertial.y,2)+pow(v->noninertial.z,2))*atan2(vy,sqrt(vx*vx+vz*vz));
	v->relative.airspeed = 0;

	//Other variables
	if (v->index == 0) {
		ni_vmag = sqrt(pow(v->noninertial.x,2)+pow(v->noninertial.y,2)+pow(v->noninertial.z,2));
		gen_vmag = sqrt(pow(v->inertial.x,2)+pow(v->inertial.y,2)+pow(v->inertial.z,2));
		gen_vvert = v->sim.y;
		gen_vgnd = sqrt(pow(v->sim.x,2)+pow(v->sim.z,2));
		true_vgnd = sqrt(nx*nx+ny*ny+nz*nz);
	}
}


//==============================================================================
// Update state
//==============================================================================
void coordsys_update(float dt)
{
	double lat,lon,alt;

	//Calculate offset of non-inertial (0,0,0)
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	XPLMLocalToWorld(0,0,0,&lat,&lon,&alt);
#else
	lat = 0;
	lon = 0;
	alt = 0;
#endif
	//FIXME: returns invalid values during initialization

	//Calculate quaternions for planet rotation, coordinate transformations
	qeuler_from(planet_rotation,0,current_planet.a,0);
	qeuler_from(i2sim_rotation,0,RAD(lon)+current_planet.a,RAD(90-lat));
	qconj(sim2i_rotation,i2sim_rotation);
	qeuler_from(i2rel_rotation,0,RAD(vessels[0].longitude)+current_planet.a,RAD(90-vessels[0].latitude));
	qeuler_from(ni2sim_rotation,0,RAD(lon),RAD(90-lat));
	qconj(sim2ni_rotation,ni2sim_rotation);
	qeuler_from(i2ni_rotation,0,0,-current_planet.a);
	qconj(ni2i_rotation,i2ni_rotation);

	//Update coordinate history
	/*coord_history_timeout += dt;
	if (coord_history_timeout > 0.25) {
		int i;
		for (i = 1; i < MAX_BODIES; i++) {
			coord_sim2ni(vessels[i]->sim.x,vessels[i]->sim.y,vessels[i]->sim.z,
			             &coord_history_xyz[HISTORY_POINTS*3*(i-1) + coord_history_next*3 + 0],
			             &coord_history_xyz[HISTORY_POINTS*3*(i-1) + coord_history_next*3 + 1],
			             &coord_history_xyz[HISTORY_POINTS*3*(i-1) + coord_history_next*3 + 2]);
		}
		coord_history_next++;
		coord_history_timeout = 0.0;
		if (coord_history_next == HISTORY_POINTS) coord_history_next = 0;
	}*/
}


//==============================================================================
// Initialize datarefs
//==============================================================================
void coordsys_initialize()
{
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	dataref_d("xsp/inertial/rx",&vessels[0].inertial.x);
	dataref_d("xsp/inertial/ry",&vessels[0].inertial.y);
	dataref_d("xsp/inertial/rz",&vessels[0].inertial.z);
	dataref_d("xsp/inertial/vx",&vessels[0].inertial.vx);
	dataref_d("xsp/inertial/vy",&vessels[0].inertial.vy);
	dataref_d("xsp/inertial/vz",&vessels[0].inertial.vz);
	dataref_d("xsp/inertial/ax",&vessels[0].inertial.ax);
	dataref_d("xsp/inertial/ay",&vessels[0].inertial.ay);
	dataref_d("xsp/inertial/az",&vessels[0].inertial.az);

	dataref_d("xsp/noninertial/rx",&vessels[0].noninertial.x);
	dataref_d("xsp/noninertial/ry",&vessels[0].noninertial.y);
	dataref_d("xsp/noninertial/rz",&vessels[0].noninertial.z);
	dataref_d("xsp/noninertial/vx",&vessels[0].noninertial.vx);
	dataref_d("xsp/noninertial/vy",&vessels[0].noninertial.vy);
	dataref_d("xsp/noninertial/vz",&vessels[0].noninertial.vz);
	dataref_d("xsp/noninertial/ax",&vessels[0].noninertial.ax);
	dataref_d("xsp/noninertial/ay",&vessels[0].noninertial.ay);
	dataref_d("xsp/noninertial/az",&vessels[0].noninertial.az);

	dataref_d("xsp/local/rx",&vessels[0].local.x);
	dataref_d("xsp/local/ry",&vessels[0].local.y);
	dataref_d("xsp/local/rz",&vessels[0].local.z);
	dataref_d("xsp/local/vx",&vessels[0].local.vx);
	dataref_d("xsp/local/vy",&vessels[0].local.vy);
	dataref_d("xsp/local/vz",&vessels[0].local.vz);
	dataref_d("xsp/local/ax",&vessels[0].local.ax);
	dataref_d("xsp/local/ay",&vessels[0].local.ay);
	dataref_d("xsp/local/az",&vessels[0].local.az);

	dataref_d("xsp/noninertial/vmag",&ni_vmag);
	dataref_d("xsp/noninertial/vvert",&gen_vvert);
	dataref_d("xsp/noninertial/vgnd",&gen_vgnd);
	dataref_d("xsp/inertial/vmag",&gen_vmag);
	dataref_d("xsp/inertial/vvert",&gen_vvert);
	dataref_d("xsp/inertial/vgnd",&gen_vgnd);
	dataref_d("xsp/local/vmag",&gen_vmag);
	dataref_d("xsp/local/vvert",&gen_vvert);
	dataref_d("xsp/local/vgnd",&gen_vgnd);

	dataref_d("xsp/planet/inertial/vgnd",&true_vgnd);
#endif

	coordsys_reinitialize();
	//coord_history_mode = XPLMFindDataRef("sim/cockpit/misc/show_path");
}


//==============================================================================
// Initialize paths and draw paths
//==============================================================================
void coordsys_reinitialize()
{
	//coord_history_timeout = 0;
	//coord_history_next = 0;
	//coordsys_update(0); //generate correct quaternions before flight loop
}


//==============================================================================
// Free memory
//==============================================================================
void coordsys_deinitialize()
{

}

//==============================================================================
// Draw path
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void coordsys_draw()
{
	if (config.draw_coordsys) {
		double sx,sy,sz,ox,oy,oz;
		XPLMSetGraphicsState(0,0,0,0,0,1,0);
		glLineWidth(4.0f);
		glBegin(GL_LINES);

		coord_i2sim(0,0,0,&ox,&oy,&oz);
		coord_i2sim(current_planet.radius*1.5,0,0,&sx,&sy,&sz);
		glColor4f(1.0f,0.0f,0.0f,1.0f);
		glVertex3d(ox,oy,oz);
		glVertex3d(sx,sy,sz);
		coord_i2sim(0,current_planet.radius*1.5,0,&sx,&sy,&sz);
		glColor4f(0.0f,1.0f,0.0f,1.0f);
		glVertex3d(ox,oy,oz);
		glVertex3d(sx,sy,sz);
		coord_i2sim(0,0,current_planet.radius*1.5,&sx,&sy,&sz);
		glColor4f(0.0f,0.0f,1.0f,1.0f);
		glVertex3d(ox,oy,oz);
		glVertex3d(sx,sy,sz);

		coord_l2sim(&vessels[0],0,0,0,&ox,&oy,&oz);
		coord_l2sim(&vessels[0],50.0,0,0,&sx,&sy,&sz);
		glColor4f(1.0f,0.0f,0.0f,1.0f);
		glVertex3d(ox,oy,oz);
		glVertex3d(sx,sy,sz);
		coord_l2sim(&vessels[0],0,50.0,0,&sx,&sy,&sz);
		glColor4f(0.0f,1.0f,0.0f,1.0f);
		glVertex3d(ox,oy,oz);
		glVertex3d(sx,sy,sz);
		coord_l2sim(&vessels[0],0,0,50.0,&sx,&sy,&sz);
		glColor4f(0.0f,0.0f,1.0f,1.0f);
		glVertex3d(ox,oy,oz);
		glVertex3d(sx,sy,sz);

		glEnd();

		glLineWidth(1.0f);
	}

	/*int mode = (int)XPLMGetDataf(coord_history_mode);
	if ((mode != 0) && (mode != 4)) {
		double px,py,pz,x,y,z;
		int i,j,isLine;
		XPLMSetGraphicsState(0,0,0,0,0,1,0);
		for (i = 1; i < MAX_BODIES; i++) {
			if (vessels[i]->has_existed && (!vessels[i]->attached)) {
				glBegin(GL_LINES);
				isLine = 0;
				for (j = 0; j < coord_history_next; j++) {
					coord_ni2sim(coord_history_xyz[HISTORY_POINTS*3*(i-1) + 3*j + 0],
					             coord_history_xyz[HISTORY_POINTS*3*(i-1) + 3*j + 1],
					             coord_history_xyz[HISTORY_POINTS*3*(i-1) + 3*j + 2],
					             &x,&y,&z);
					if (isLine && ((x-px)*(x-px)+(y-py)*(y-py)+(z-pz)*(z-pz) > 1e8)) {
						isLine = 0;
					}
					if (isLine) {
						if (i % 3 == 0) {
							if (j % 2)	glColor3d(1.0,1.0,0.0);
							else		glColor3d(0.0,0.0,0.0);
						} else if (i % 3 == 1) {
							if (j % 2)	glColor3d(0.0,1.0,0.0);
							else		glColor3d(0.0,0.0,0.0);
						} else {
							if (j % 2)	glColor3d(0.0,1.0,1.0);
							else		glColor3d(0.0,0.0,0.0);
						}
						glVertex3d(px,py,pz);
						glVertex3d(x, y, z);
					}
					px = x;
					py = y;
					pz = z;
					isLine = 1;
				}
				glEnd();
			}
		}
	}*/
	/*double sx,sy,sz,cx,cy,cz;
	coord_l2sim(&vessels[0],0,0,0,&cx,&cy,&cz);
	glBegin(GL_LINES);
		coord_l2sim(&vessels[0],-20,0,0,&sx,&sy,&sz);
		glColor3d(1.0,0.0,0.0);
		glVertex3d(cx,cy,cz);
		glColor3d(1.0,0.0,0.0);
		glVertex3d(sx,sy,sz);
		coord_l2sim(&vessels[0],0,20,0,&sx,&sy,&sz);
		glColor3d(0.0,1.0,0.0);
		glVertex3d(cx,cy,cz);
		glColor3d(0.0,1.0,0.0);
		glVertex3d(sx,sy,sz);
		coord_l2sim(&vessels[0],0,0,20,&sx,&sy,&sz);
		glColor3d(0.0,0.0,1.0);
		glVertex3d(cx,cy,cz);
		glColor3d(0.0,0.0,1.0);
		glVertex3d(sx,sy,sz);
	glEnd();*/
}
#endif

//==============================================================================
// Velocity transformations in simulator coordinates
//==============================================================================
void vel_i2ni_vessel(vessel* v)
{
	double nx,ny,nz;
	coordsys_retvel(v,&nx,&ny,&nz);
	v->sim.ivx = v->sim.vx + nx;
	v->sim.ivy = v->sim.vy + ny;
	v->sim.ivz = v->sim.vz + nz;
}

void vel_ni2i_vessel(vessel* v)
{
	double nx,ny,nz;
	coordsys_retvel(v,&nx,&ny,&nz);
	v->sim.ivx = v->sim.vx - nx;
	v->sim.ivy = v->sim.vy - ny;
	v->sim.ivz = v->sim.vz - nz;
}


//==============================================================================
// Coordinate transformations
//==============================================================================
void coord_sim2i(double sx, double sy, double sz, double* ix, double* iy, double* iz)
{
	quaternion q;
	qset_vec(q,sz,sy+current_planet.radius,sx);
	qdiv(q,q,sim2i_rotation); //transform by Q^-1
	qmul(q,sim2i_rotation,q);
	qget_vec(q,ix,iz,iy);
}

void coord_i2sim(double ix, double iy, double iz, double* sx, double* sy, double* sz)
{
	quaternion q;
	qset_vec(q,ix,iz,iy);
	qdiv(q,q,i2sim_rotation); //transform by Q
	qmul(q,i2sim_rotation,q);
	qget_vec(q,sz,sy,sx);
	*sy -= current_planet.radius;
}

void coord_sim2ni(double sx, double sy, double sz, double* x, double* y, double* z)
{
	quaternion q;
	qset_vec(q,sz,sy+current_planet.radius,sx);
	qdiv(q,q,sim2ni_rotation); //transform by Q^-1
	qmul(q,sim2ni_rotation,q);
	qget_vec(q,x,z,y);
}

void coord_ni2sim(double x, double y, double z, double* sx, double* sy, double* sz)
{
	quaternion q;
	qset_vec(q,x,z,y);
	qdiv(q,q,ni2sim_rotation); //transform by Q
	qmul(q,ni2sim_rotation,q);
	qget_vec(q,sz,sy,sx);
	*sy -= current_planet.radius;
}

void coord_sim2l(vessel* v, double sx, double sy, double sz, double* lx, double* ly, double* lz)
{
	vec_sim2l(v,sx-v->sim.x,sy-v->sim.y,sz-v->sim.z,lx,ly,lz);
}

void coord_l2sim(vessel* v, double lx, double ly, double lz, double* sx, double* sy, double* sz)
{
	vec_l2sim(v,lx,ly,lz,sx,sy,sz);
	*sx += v->sim.x;
	*sy += v->sim.y;
	*sz += v->sim.z;
}

void coord_l2i(vessel* v, double lx, double ly, double lz, double* ix, double* iy, double* iz)
{
	//double sx,sy,sz;
	//quat_i2sim(v->inertial.q,v->sim.q); //FIXME: resets sim Q
	//coord_l2sim(v,lx,ly,lz,&sx,&sy,&sz);
	//coord_sim2i(sx,sy,sz,ix,iy,iz);

	vec_l2i(v,lx,ly,lz,ix,iy,iz);
	*ix += v->inertial.x;
	*iy += v->inertial.y;
	*iz += v->inertial.z;
}


//==============================================================================
// Vector transformations
//==============================================================================
void vec_sim2i(double sx, double sy, double sz, double* ix, double* iy, double* iz)
{
	quaternion q;
	qset_vec(q,sz,sy,sx);
	qdiv(q,q,sim2i_rotation);
	qmul(q,sim2i_rotation,q);
	qget_vec(q,ix,iz,iy);
}

void vec_i2sim(double ix, double iy, double iz, double* sx, double* sy, double* sz)
{
	quaternion q;
	qset_vec(q,ix,iz,iy);
	qdiv(q,q,i2sim_rotation);
	qmul(q,i2sim_rotation,q);
	qget_vec(q,sz,sy,sx);
}

void vec_i2rel(double ix, double iy, double iz, double* rx, double* ry, double* rz)
{
	quaternion q;
	qset_vec(q,ix,iz,iy);
	qdiv(q,q,i2rel_rotation);
	qmul(q,i2rel_rotation,q);
	qget_vec(q,rz,ry,rx);
}

void vec_sim2ni(double sx, double sy, double sz, double* x, double* y, double* z)
{
	quaternion q;
	qset_vec(q,sz,sy,sx);
	qdiv(q,q,sim2ni_rotation);
	qmul(q,sim2ni_rotation,q);
	qget_vec(q,x,z,y);
}

void vec_ni2sim(double x, double y, double z, double* sx, double* sy, double* sz)
{
	quaternion q;
	qset_vec(q,x,z,y);
	qdiv(q,q,ni2sim_rotation);
	qmul(q,ni2sim_rotation,q);
	qget_vec(q,sz,sy,sx);
}

void vec_sim2l(vessel* v, double sx, double sy, double sz, double* lx, double* ly, double* lz)
{
	quaternion q,qv;
	qconj(qv,v->sim.q);

	qset_vec(q,sz,-sx,sy);
	qdiv(q,q,qv);
	qmul(q,qv,q);
	qget_vec(q,lx,ly,lz);
}

void vec_l2sim(vessel* v, double lx, double ly, double lz, double* sx, double* sy, double* sz)
{
	quaternion q;

	qset_vec(q,lx,ly,lz);
	qdiv(q,q,v->sim.q);
	qmul(q,v->sim.q,q);
	qget_vec(q,sz,sx,sy);
	*sx *= -1;
}

void vec_i2l(vessel* v, double ix, double iy, double iz, double* lx, double* ly, double* lz)
{
	double sx,sy,sz;
	quat_i2sim(v->inertial.q,v->sim.q); //FIXME: resets sim Q
	vec_i2sim(ix,iy,iz,&sx,&sy,&sz);
	vec_sim2l(v,sx,sy,sz,lx,ly,lz);
}

void vec_l2i(vessel* v, double lx, double ly, double lz, double* ix, double* iy, double* iz)
{
	double sx,sy,sz;
	quat_i2sim(v->inertial.q,v->sim.q);
	vec_l2sim(v,lx,ly,lz,&sx,&sy,&sz);
	vec_sim2i(sx,sy,sz,ix,iy,iz);
}

void vec_i2ni(double ix, double iy, double iz, double* x, double* y, double* z)
{
	quaternion q;
	qset_vec(q,ix,iy,iz);
	qdiv(q,q,i2ni_rotation);
	qmul(q,i2ni_rotation,q);
	qget_vec(q,x,y,z);
}

void vec_ni2i(double x, double y, double z, double* ix, double* iy, double* iz)
{
	quaternion q;
	qset_vec(q,x,y,z);
	qdiv(q,q,ni2i_rotation);
	qmul(q,ni2i_rotation,q);
	qget_vec(q,ix,iy,iz);
}

void vec_ni2l(vessel* v, double ix, double iy, double iz, double* lx, double* ly, double* lz)
{
	double sx,sy,sz;
	quat_i2sim(v->inertial.q,v->sim.q); //FIXME: resets sim Q
	vec_ni2sim(ix,iy,iz,&sx,&sy,&sz);
	vec_sim2l(v,sx,sy,sz,lx,ly,lz);
}

void vec_l2ni(vessel* v, double lx, double ly, double lz, double* ix, double* iy, double* iz)
{
	double sx,sy,sz;
	quat_i2sim(v->inertial.q,v->sim.q); //FIXME: reset sim Q
	vec_l2sim(v,lx,ly,lz,&sx,&sy,&sz);
	vec_sim2ni(sx,sy,sz,ix,iy,iz);
}


//==============================================================================
// Quaternion transformations
//==============================================================================
void quat_sim2i(quaternion sq, quaternion iq)
{
	qmul(iq,sq,sim2i_rotation);
}

void quat_i2sim(quaternion iq, quaternion sq)
{
	qmul(sq,iq,i2sim_rotation);
}

void quat_i2rel(quaternion iq, quaternion rq)
{
	qmul(rq,iq,i2rel_rotation);
}

void quat_ni2sim(quaternion nq, quaternion sq)
{
	qmul(sq,nq,ni2sim_rotation);
}

void quat_sim2ni(quaternion sq, quaternion nq)
{
	qmul(nq,sq,sim2ni_rotation);
}

void quat_ni2i(quaternion nq, quaternion iq)
{
	qmul(iq,nq,ni2i_rotation);
}

void quat_i2ni(quaternion iq, quaternion nq)
{
	qmul(nq,iq,i2ni_rotation);
}
