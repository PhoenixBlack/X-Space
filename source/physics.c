#include <math.h>
#include <kost.h>

#include "x-space.h"
#include "dataref.h"
#include "planet.h"
#include "vessel.h"
#include "physics.h"
#include "quaternion.h"
#include "coordsys.h"
#include "config.h"

//Current orbit
orbit current_orbit;

/*******************************************************************************
 * Initailize datarefs
 ******************************************************************************/
void physics_initialize()
{
	current_orbit.count = 0.0;
	current_orbit.previous_latitude = 0.0;
	current_orbit.previous_longitude = 0.0;

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	dataref_d("xsp/orbital/count",&current_orbit.count);
#endif
}


/*******************************************************************************
 * Update various physics-related parameters
 ******************************************************************************/
void physics_update(float dt)
{
	int i;
	double w;

	//Update number of orbits around Earth
	w = sin(RAD(vessels[0].latitude))*sin(RAD(current_orbit.previous_latitude))+
	    cos(RAD(vessels[0].latitude))*cos(RAD(current_orbit.previous_latitude))*cos(RAD(vessels[0].longitude-current_orbit.previous_longitude));
	if (w >  1.0) w =  1.0; //Clamp for numerical stability
	if (w < -1.0) w = -1.0;
	w = acos(w);
	current_orbit.previous_latitude = vessels[0].latitude;
	current_orbit.previous_longitude = vessels[0].longitude;

	if (w > PI/900) { //Erroneously fast change
		current_orbit.count = 0;
	} else {
		current_orbit.count += w/(2.0*PI);
	}

	//Update orbital elements for all vessels
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists) {
			kostStateVector state;
			kostOrbitParam params;
			kostElements elements;
			double r,latitude,longitude;

			state.pos.x = vessels[i].noninertial.x;
			state.pos.y = vessels[i].noninertial.y;
			state.pos.z = vessels[i].noninertial.z;
			state.vel.x = vessels[i].noninertial.vx;
			state.vel.y = vessels[i].noninertial.vy;
			state.vel.z = vessels[i].noninertial.vz;

			kostStateVector2Elements(current_planet.mu,&state,&elements,&params);

			//Main six
			vessels[i].orbit.smA = elements.a;		
			vessels[i].orbit.e = elements.e;
			vessels[i].orbit.i = elements.i;
			vessels[i].orbit.MnA = params.MnA;
			vessels[i].orbit.AgP = params.AgP;
			vessels[i].orbit.AsN = elements.theta;

			//Extra information
			vessels[i].orbit.BSTAR = 
				(vessels[i].geometry.total_area * 0.5 * vessels[i].geometry.Cd / (1e-12+vessels[i].mass)) *
				(vessels[i].air.density / 2);
			vessels[i].orbit.BSTAR *= current_planet.radius;
			vessels[i].orbit.period = params.T;

			//Update statistics
			if (vessels[i].elevation > 100e3) vessels[i].statistics.time_in_space += dt;
			//vessels[i].statistics.time_alive += dt;

			vessels[i].statistics.total_distance +=	sqrt(vessels[i].noninertial.vx*vessels[i].noninertial.vx+
														 vessels[i].noninertial.vy*vessels[i].noninertial.vy+
														 vessels[i].noninertial.vz*vessels[i].noninertial.vz)*dt;

			longitude = fmod(vessels[i].longitude+360,360);
			if (fabs(vessels[i].statistics.prev_longitude - longitude) < 180.0/900.0) {
				vessels[i].statistics.total_orbits += fabs(vessels[i].statistics.prev_longitude - longitude)/360.0;
			}
			vessels[i].statistics.prev_longitude = vessels[i].longitude;

			r = sqrt(vessels[i].inertial.x*vessels[i].inertial.x+
					 vessels[i].inertial.y*vessels[i].inertial.y+
					 vessels[i].inertial.z*vessels[i].inertial.z);
			latitude = 90-DEG(acos(vessels[i].inertial.z/r));
			longitude = DEG(atan2(vessels[i].inertial.y/r,vessels[i].inertial.x/r));

			w = sin(RAD(latitude))*sin(RAD(vessels[i].statistics.prev_latitude))+
				cos(RAD(latitude))*cos(RAD(vessels[i].statistics.prev_latitude))*cos(RAD(longitude-vessels[i].statistics.prev_inertial_longitude));
			if (w >  1.0) w =  1.0; //Clamp for numerical stability
			if (w < -1.0) w = -1.0;
			w = acos(w);
			if (fabs(w) < PI/900) {
				vessels[i].statistics.total_true_orbits += fabs(w)/(2.0*PI);
			}
			vessels[i].statistics.prev_latitude = latitude;
			vessels[i].statistics.prev_inertial_longitude = longitude;
		}
	}
}


/*******************************************************************************
 * Helper functions for the integrator
 ******************************************************************************/
void physics_setvec(double v[6], double a, double b, double c, double d, double e, double f)
{
	v[0] = a; v[1] = b; v[2] = c; v[3] = d; v[4] = e; v[5] = f;
}

void physics_scalevec(double v[6], double f)
{
	v[0] *= f; v[1] *= f; v[2] *= f; v[3] *= f; v[4] *= f; v[5] *= f;
}


/*******************************************************************************
 * Long-term forces description
 ******************************************************************************/
void physics_force_gravity(vessel* v, double t, double rv[6], double va[6])
{
	double R,R2,Gmag,nx,ny,nz;

	//Compute radius-vector
	R2 = rv[0]*rv[0]+rv[1]*rv[1]+rv[2]*rv[2];
	R = sqrt(R2);

	//Compute gravity force
	if (config.nonspherical_gravity) {
		double Rref,Rref2;
		Rref2 = v->inertial.x*v->inertial.x+v->inertial.y*v->inertial.y+v->inertial.z*v->inertial.z;
		Rref = sqrt(Rref2);

		//Gmag(R) = G
		//Gmag(R2) = G*(Rref2/R2)
		Gmag = v->geomagnetic.g*(Rref2/R2);
		nx = rv[0]/R;
		ny = rv[1]/R;
		nz = rv[2]/R;
	} else {
		Gmag = current_planet.mu/R2;
		nx = rv[0]/R;
		ny = rv[1]/R;
		nz = rv[2]/R;
	}

	//Apply gravity
	va[3] += -Gmag*nx;
	va[4] += -Gmag*ny;
	va[5] += -Gmag*nz;
}

void physics_force_bodygravity(vessel* v, vessel* b, double t, double rv[6], double va[6])
{
	double R,R2,Gmag,nx,ny,nz;

	//Compute radius-vector
	R2 = (rv[0]-b->inertial.x)*(rv[0]-b->inertial.x)+
		 (rv[1]-b->inertial.y)*(rv[1]-b->inertial.y)+
		 (rv[2]-b->inertial.z)*(rv[2]-b->inertial.z);
	R = sqrt(R2);

	//Compute gravity force
	Gmag = 6.67384e-11*b->weight.chassis/R2;
	nx = (rv[0]-b->inertial.x)/R;
	ny = (rv[1]-b->inertial.y)/R;
	nz = (rv[2]-b->inertial.z)/R;

	//Apply gravity
	va[3] += -Gmag*nx;
	va[4] += -Gmag*ny;
	va[5] += -Gmag*nz;
}

void physics_getforces(vessel* v, double t, double rv[6], double va[6])
{
	int i;
	double ax,ay,az; //Acceleration vector
	double rx,ry,rz; //Position vector
	double nx,ny,nz; //Temporary vector
	double wx,wy,wz; //Rotation rate
	double cx,cy,cz; //Center of mass

	//Compute acceleration of the reference point
	coord_l2i(v,v->cx,v->cy,v->cz,&cx,&cy,&cz);
	cx = v->inertial.x-cx;
	cy = v->inertial.y-cy;
	cz = v->inertial.z-cz;

	wy = v->inertial.P; wz = v->inertial.Q; wx = v->inertial.R;
	ry = v->accumulated.Pd; rz = v->accumulated.Qd; rx = v->accumulated.Rd;

	nx = (wy*cz-wz*cy); //w x c
	ny = (wz*cx-wx*cz);
	nz = (wx*cy-wy*cx);
	ax = v->accumulated.ax + (ry*cz-rz*cy) + (wy*nz-wz*ny); //A = a' + a + w' x c + 2*(w x v') + w x (w x c)
	ay = v->accumulated.ay + (rz*cx-rx*cz) + (wz*nx-wx*nz); //v' = 0 (center of mass does not move)
	az = v->accumulated.az + (rx*cy-ry*cx) + (wx*ny-wy*nx); //a' = 0 (center of mass does not accelerate)

	//Reset output
	physics_setvec(va,rv[3],rv[4],rv[5],0,0,0);

	//Apply long-term forces
	physics_force_gravity(v,t,rv,va);
	for (i = 0; i < vessel_count; i++) {
		if ((vessels[i].exists) && (vessels[i].net_id >= 1000000)) {
			physics_force_bodygravity(v,&vessels[i],t,rv,va);
		}
	}

	//Apply short-term forces
	va[3] += ax;
	va[4] += ay;
	va[5] += az;

	//Compute new velocity
	/*va[0] += va[3]*t;
	va[1] += va[4]*t;
	va[2] += va[5]*t;*/
}


/*******************************************************************************
 * Simulate and perform integration if required
 ******************************************************************************/
void physics_integrate(float dt1, vessel* v)
{
	double dt = dt1;
	if (!v->exists) return;

	if (v->physics_type == VESSEL_PHYSICS_INERTIAL) { //Perform full integration
		//State vector
		double rv[6],k1[6],k2[6],k3[6],k4[6];
		quaternion dq,lq;

		//Short-term forces integration
		v->inertial.P  += v->accumulated.Pd*dt;
		v->inertial.Q  += v->accumulated.Qd*dt;
		v->inertial.R  += v->accumulated.Rd*dt;

		//Long-term forces integration
		physics_setvec(rv,v->inertial.x, v->inertial.y, v->inertial.z,
		                  v->inertial.vx,v->inertial.vy,v->inertial.vz);
		physics_getforces(v,0,rv,k1);
		physics_scalevec(k1,dt);

		physics_setvec(rv,v->inertial.x +(1.0/2.0)*k1[0], v->inertial.y+(1.0/2.0)*k1[1], v->inertial.z+(1.0/2.0)*k1[2],
						  v->inertial.vx+(1.0/2.0)*k1[3],v->inertial.vy+(1.0/2.0)*k1[4],v->inertial.vz+(1.0/2.0)*k1[5]);
		physics_getforces(v,dt/2,rv,k2);
		physics_scalevec(k2,dt);

		physics_setvec(rv,v->inertial.x +(1.0/2.0)*k2[0], v->inertial.y+(1.0/2.0)*k2[1], v->inertial.z+(1.0/2.0)*k2[2],
						  v->inertial.vx+(1.0/2.0)*k2[3],v->inertial.vy+(1.0/2.0)*k2[4],v->inertial.vz+(1.0/2.0)*k2[5]);
		physics_getforces(v,dt/2,rv,k3);
		physics_scalevec(k3,dt);

		physics_setvec(rv,v->inertial.x +k3[0], v->inertial.y+k3[1], v->inertial.z+k3[2],
						  v->inertial.vx+k3[3],v->inertial.vy+k3[4],v->inertial.vz+k3[5]);
		physics_getforces(v,dt,rv,k4);
		physics_scalevec(k4,dt);

		//RK4 integration for position and velocity
		v->inertial.x  += (1.0/6.0)*(k1[0]+2*k2[0]+2*k3[0]+k4[0]);
		v->inertial.y  += (1.0/6.0)*(k1[1]+2*k2[1]+2*k3[1]+k4[1]);
		v->inertial.z  += (1.0/6.0)*(k1[2]+2*k2[2]+2*k3[2]+k4[2]);
		v->inertial.vx += (1.0/6.0)*(k1[3]+2*k2[3]+2*k3[3]+k4[3]);
		v->inertial.vy += (1.0/6.0)*(k1[4]+2*k2[4]+2*k3[4]+k4[4]);
		v->inertial.vz += (1.0/6.0)*(k1[5]+2*k2[5]+2*k3[5]+k4[5]);

		//Integration for attitude (a little bit of a hack!)
		qeuler_from(dq,v->inertial.R*dt,v->inertial.P*dt,v->inertial.Q*dt);
		quat_i2sim(v->inertial.q,lq);
		qmul(lq,lq,dq);
		quat_sim2i(lq,v->inertial.q);

		//Update the information variables
		v->inertial.ax += v->accumulated.ax;
		v->inertial.ay += v->accumulated.ay;
		v->inertial.az += v->accumulated.az;
		v->inertial.Pd += v->accumulated.Pd;
		v->inertial.Qd += v->accumulated.Qd;
		v->inertial.Rd += v->accumulated.Rd;
	} else { //Calculate only fictious forces caused by rotating frame of reference
		double ax,ay,az; //Acceleration vector
		double rx,ry,rz; //Position vector
		double nx,ny,nz; //Temporary vector
		double wx,wy,wz; //Rotation rate
		double cx,cy,cz; //Center of mass

		//== Simulate rotation of Simulator coordinate system ==================
		//Angular velocity vector
		wx = 0;
		wy = -current_planet.w * sin(RAD(v->latitude));
		wz = -current_planet.w * cos(RAD(v->latitude));

		//Spacecraft position
		rx = v->sim.x;
		ry = v->sim.y + current_planet.radius;
		rz = v->sim.z;

		//Fictious accelerations due to Earth rotation (coriolis, centrifugal)
		nx = (wy*rz-wz*ry); //w x r
		ny = (wz*rx-wx*rz);
		nz = (wx*ry-wy*rx);
		ax = -2*(wy*v->sim.vz-wz*v->sim.vy) - (wy*nz-wz*ny); //A = - 2*(w x v) - w x (w x r)
		ay = -2*(wz*v->sim.vx-wx*v->sim.vz) - (wz*nx-wx*nz);
		az = -2*(wx*v->sim.vy-wy*v->sim.vx) - (wx*ny-wy*nx);


		//== Account for non-zero center of mass ===============================
		coord_l2sim(v,v->cx,v->cy,v->cz,&cx,&cy,&cz);
		cx = v->sim.x-cx;
		cy = v->sim.y-cy;
		cz = v->sim.z-cz;

		wy = v->sim.P; wz = v->sim.Q; wx = v->sim.R;
		ry = v->accumulated.Pd; rz = v->accumulated.Qd; rx = v->accumulated.Rd;

		nx = (wy*cz-wz*cy); //w x c
		ny = (wz*cx-wx*cz);
		nz = (wx*cy-wy*cx);
		ax += v->accumulated.ax + (ry*cz-rz*cy) + (wy*nz-wz*ny); //A = a' + a + w' x c + 2*(w x v') + w x (w x c)
		ay += v->accumulated.ay + (rz*cx-rx*cz) + (wz*nx-wx*nz); //v' = 0 (center of mass does not move)
		az += v->accumulated.az + (rx*cy-ry*cx) + (wx*ny-wy*nx); //a' = 0 (center of mass does not accelerate)

		wy = v->accumulated.Pd;
		wz = v->accumulated.Qd;
		wx = v->accumulated.Rd;


		//== Integrate what's left =============================================
		//Add angular acceleration
		v->sim.P += wy*dt;
		v->sim.Q += wz*dt;
		v->sim.R += wx*dt;

		//Add linear acceleration
		v->sim.vx += ax*dt;
		v->sim.vy += ay*dt;
		v->sim.vz += az*dt;

		//Update acceleration values for display
		v->sim.ax += ax; v->sim.ay += ay; v->sim.az += az;
		v->sim.Pd += wy; v->sim.Qd += wz; v->sim.Rd += wx;
	}
}