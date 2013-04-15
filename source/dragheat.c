//==============================================================================
// Drag/heating simulation
//==============================================================================
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "x-space.h"
#include "vessel.h"
#include "coordsys.h"
#include "dragheat.h"
#include "material.h"
#include "highlevel.h"
#include "collision.h"
#include "quaternion.h"
#include "planet.h"
#include "config.h"

#include "threading.h"
#include "curtime.h"


//==============================================================================
//X-Plane SDK
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include "rendering.h"
#include "particles.h"
#include <XPLMDataAccess.h>
#include <XPLMPlanes.h>
#include <XPLMGraphics.h>
#include <XPLMDisplay.h>
#include <XPLMUtilities.h>
#endif


//==============================================================================
//Drag/heating variables
//==============================================================================
int dragheat_visual_mode = 0;
int dragheat_visual_interpolate = 1;
int dragheat_selected_triangle = -1;
int dragheat_selected_vessel = -1;
int dragheat_paint_mode = -1;
int dragheat_paint_mode_selected = -1;
double dragheat_paint_thickness = 0.05;

double dragheat_max_temperature = 0.0;
double dragheat_max_heatflux = 0.0;
double dragheat_max_Q = 0.0;

int dragheat_heating_simulate = 0;

struct {
	int enabled;
	double altitude;
	double alpha;
	double beta;
	double velocity;
} dragheat_simulation = { 0, 70000, 0, 0, 0 };


//==============================================================================
// Load drag/heating data
//==============================================================================
void dragheat_initialize(vessel* v, char* acfpath, char* acfname)
{
	int num_tris,tri_idx,i;
	double mx,my,mz;
	double total_area;
	char filename[MAX_FILENAME] = { 0 };
	char buf[ARBITRARY_MAX] = { 0 };
	face* faces;
	FILE* f;

	//Try to open file on either of the two filenames
	snprintf(filename,MAX_FILENAME-1,"%s/%s_drag.dat",acfpath,acfname);
	f = fopen(filename,"r");
	if (!f) {
		snprintf(filename,MAX_FILENAME-1,"%s/drag.dat",acfpath);
		f = fopen(filename,"r");
		if (!f) {
			v->geometry.num_faces = 0;
			v->geometry.faces = 0;
			return;
		}
	}
	log_write("X-Space: Loading drag/heating model from %s...\n",filename);

	//Set default values
	v->geometry.Cd = 1.0;
	v->geometry.K = 1.0;
	v->geometry.hull = material_get("Aluminium");
	v->geometry.trqx = 1.0;
	v->geometry.trqy = 1.0;
	v->geometry.trqz = 1.0;
	v->weight.tps = 0.0;

	//Scan number of triangles and parameters
	num_tris = 0;
	while (f && (!feof(f))) {
		char tag[256];
		tag[0] = 0; fscanf(f,"%255s",tag); tag[255] = 0;
		if (strcmp(tag,"TRI_MAT2") == 0) {
			num_tris++;
		} if (strcmp(tag,"TRI_MAT3") == 0) {
			num_tris++;
		} else if (strcmp(tag,"Cd") == 0) {
			float Cd;
			fscanf(f,"%f",&Cd);
			v->geometry.Cd = (double)Cd;
		} else if (strcmp(tag,"K") == 0) {
			float K;
			fscanf(f,"%f",&K);
			v->geometry.K = (double)K;
		} else if (strcmp(tag,"HULL") == 0) {
			fscanf(f,"%s",buf);
			v->geometry.hull = material_get(buf);
		} else if (strcmp(tag,"TRQ") == 0) {
			float x,y,z;
			fscanf(f,"%f %f %f",&x,&y,&z);
			v->geometry.trqx = (double)x;
			v->geometry.trqy = (double)y;
			v->geometry.trqz = (double)z;
		} else if (strcmp(tag,"SHOCKWAVES") == 0) {
			fscanf(f,"%d",&v->geometry.shockwave_heating);
		} else {
			fgets(buf,ARBITRARY_MAX-1,f);
		}
	}
	fseek(f,0,SEEK_SET);

	//Allocate drag model faces
	v->geometry.num_faces = num_tris;
	v->geometry.faces = (face*)malloc(sizeof(face)*num_tris);
	faces = v->geometry.faces;

	//Read face data and calculate some parameters
	total_area = 0.0;
	tri_idx = 0;
	while (f && (!feof(f))) {
		char tag[256];
		tag[0] = 0; fscanf(f,"%255s",tag); tag[255] = 0;
		if (strcmp(tag,"TRI_MAT2") == 0) {
			float x0,y0,z0;
			float x1,y1,z1;
			float x2,y2,z2;
			float nx,ny,nz;
			float area;
			int drag;
			float thickness;

			fscanf(f,"%f %f %f  %f %f %f  %f %f %f  %f %f %f  %f %f %d %s",
			       &x0,&y0,&z0,&x1,&y1,&z1,&x2,&y2,&z2,&nx,&ny,&nz,&area,&thickness,&drag,buf);

			faces[tri_idx].x[0] = x0;
			faces[tri_idx].x[1] = x1;
			faces[tri_idx].x[2] = x2;
			faces[tri_idx].y[0] = y0;
			faces[tri_idx].y[1] = y1;
			faces[tri_idx].y[2] = y2;
			faces[tri_idx].z[0] = z0;
			faces[tri_idx].z[1] = z1;
			faces[tri_idx].z[2] = z2;

			faces[tri_idx].nx   = nx;
			faces[tri_idx].ny   = ny;
			faces[tri_idx].nz   = nz;
			faces[tri_idx].area = 0.5*area;
			faces[tri_idx].thickness = thickness;
			faces[tri_idx].creates_drag = drag;

			faces[tri_idx].m    = material_get(buf);

			tri_idx++;
			total_area += area;
		} if (strcmp(tag,"TRI_MAT3") == 0) {
			float x0,y0,z0;
			float x1,y1,z1;
			float x2,y2,z2;
			float nx,ny,nz;
			float area;
			int drag;
			float thickness;

			fscanf(f,"%f %f %f  %f %f %f  %f %f %f  %f %f %f  %f %f %d %s",
			       &x0,&y0,&z0,&x1,&y1,&z1,&x2,&y2,&z2,&nx,&ny,&nz,&area,&thickness,&drag,buf);

			faces[tri_idx].x[0] = x0;
			faces[tri_idx].x[1] = x1;
			faces[tri_idx].x[2] = x2;
			faces[tri_idx].y[0] = y0;
			faces[tri_idx].y[1] = y1;
			faces[tri_idx].y[2] = y2;
			faces[tri_idx].z[0] = z0;
			faces[tri_idx].z[1] = z1;
			faces[tri_idx].z[2] = z2;

			faces[tri_idx].nx   = nx;
			faces[tri_idx].ny   = ny;
			faces[tri_idx].nz   = nz;
			faces[tri_idx].area = area;
			faces[tri_idx].thickness = thickness;
			faces[tri_idx].creates_drag = drag;

			faces[tri_idx].m    = material_get(buf);

			tri_idx++;
			total_area += area;
		} else {
			fgets(buf,ARBITRARY_MAX-1,f);
		}
	}
	v->geometry.total_area = total_area;

	//Reset all triangle edge data
	for (i = 0; i < num_tris; i++) {
		faces[i].boundary_edge[0] = -1;
		faces[i].boundary_edge[1] = -1;
		faces[i].boundary_edge[2] = -1;
	}

	//Process faces
	mx = 0; my = 0; mz = 0;
	for (i = 0; i < num_tris; i++) {
		int j,n1,n2,n3;
		int own1,own2,own3;
		int vn1,vn2,vn3;

		//Find boundary edges
		for (j = 0; j < num_tris; j++) {
			if (i != j) {
				int k,l;
				for (l = 0; l < 3; l++) { //Check every edge of triangle i...
					for (k = 0; k < 3; k++) { //Against every edge of triangle j
						double d0f,d1f,d0r,d1r;
						int l1 = (l)%3;
						int l2 = (l+1)%3;
						int k1 = (k)%3;
						int k2 = (k+1)%3;

						//Same orientation of edges
						d0f = sqrt((faces[i].x[l1]-faces[j].x[k1])*(faces[i].x[l1]-faces[j].x[k1])+
						           (faces[i].y[l1]-faces[j].y[k1])*(faces[i].y[l1]-faces[j].y[k1])+
						           (faces[i].z[l1]-faces[j].z[k1])*(faces[i].z[l1]-faces[j].z[k1]));
						d1f = sqrt((faces[i].x[l2]-faces[j].x[k2])*(faces[i].x[l2]-faces[j].x[k2])+
						           (faces[i].y[l2]-faces[j].y[k2])*(faces[i].y[l2]-faces[j].y[k2])+
						           (faces[i].z[l2]-faces[j].z[k2])*(faces[i].z[l2]-faces[j].z[k2]));

						//Different orientation of edges
						d0r = sqrt((faces[i].x[l2]-faces[j].x[k1])*(faces[i].x[l2]-faces[j].x[k1])+
						           (faces[i].y[l2]-faces[j].y[k1])*(faces[i].y[l2]-faces[j].y[k1])+
						           (faces[i].z[l2]-faces[j].z[k1])*(faces[i].z[l2]-faces[j].z[k1]));
						d1r = sqrt((faces[i].x[l1]-faces[j].x[k2])*(faces[i].x[l1]-faces[j].x[k2])+
						           (faces[i].y[l1]-faces[j].y[k2])*(faces[i].y[l1]-faces[j].y[k2])+
						           (faces[i].z[l1]-faces[j].z[k2])*(faces[i].z[l1]-faces[j].z[k2]));

						//Check if any of the conditions is true
						if (((d0f < 0.01) && (d1f < 0.01)) ||
							((d0r < 0.01) && (d1r < 0.01))) {
							faces[i].boundary_edge[l] = j;
							faces[j].boundary_edge[k] = i;
						}
					}
				}
			}
		}

		faces[i].vnx[0] = faces[i].nx; faces[i].vny[0] = faces[i].ny; faces[i].vnz[0] = faces[i].nz; vn1 = 1;
		faces[i].vnx[1] = faces[i].nx; faces[i].vny[1] = faces[i].ny; faces[i].vnz[1] = faces[i].nz; vn2 = 1;
		faces[i].vnx[2] = faces[i].nx; faces[i].vny[2] = faces[i].ny; faces[i].vnz[2] = faces[i].nz; vn3 = 1;

		//Find boundary vertexes
		n1 = 0; n2 = 0; n3 = 0;
		own1 = 0; own2 = 0; own3 = 0;
		for (j = 0; j < num_tris; j++) {
			if (i != j) {
				int k;
				for (k = 0; k < 3; k++) {
					double x,y,z,d0,d1,d2;
					x = faces[j].x[k];
					y = faces[j].y[k];
					z = faces[j].z[k];

					d0 = sqrt((faces[i].x[0]-x)*(faces[i].x[0]-x)+
					          (faces[i].y[0]-y)*(faces[i].y[0]-y)+
					          (faces[i].z[0]-z)*(faces[i].z[0]-z));
					d1 = sqrt((faces[i].x[1]-x)*(faces[i].x[1]-x)+
					          (faces[i].y[1]-y)*(faces[i].y[1]-y)+
					          (faces[i].z[1]-z)*(faces[i].z[1]-z));
					d2 = sqrt((faces[i].x[2]-x)*(faces[i].x[2]-x)+
					          (faces[i].y[2]-y)*(faces[i].y[2]-y)+
					          (faces[i].z[2]-z)*(faces[i].z[2]-z));

					if (d0 < 0.01) {
						if (n1 < 8) {
							faces[i].boundary_faces[0*8+n1] = j;
							n1++;
							if (j > i) { //One-way list
								faces[i].ow_boundary_faces[0*8+own1] = j;
								own1++;
							}
						}

						faces[i].vnx[0] += faces[j].nx;
						faces[i].vny[0] += faces[j].ny;
						faces[i].vnz[0] += faces[j].nz;
						vn1++;
					}
					if (d1 < 0.01) {
						if (n2 < 8) {
							faces[i].boundary_faces[1*8+n2] = j;
							n2++;
							if (j > i) { //One-way list
								faces[i].ow_boundary_faces[1*8+own2] = j;
								own2++;
							}
						}

						faces[i].vnx[1] += faces[j].nx;
						faces[i].vny[1] += faces[j].ny;
						faces[i].vnz[1] += faces[j].nz;
						vn2++;
					}
					if (d2 < 0.01) {
						if (n3 < 8) {
							faces[i].boundary_faces[2*8+n3] = j;
							n3++;
							if (j > i) { //One-way list
								faces[i].ow_boundary_faces[2*8+own3] = j;
								own3++;
							}
						}

						faces[i].vnx[2] += faces[j].nx;
						faces[i].vny[2] += faces[j].ny;
						faces[i].vnz[2] += faces[j].nz;
						vn3++;
					}
				}
			}
		}
		faces[i].boundary_num_faces1 = n1;
		faces[i].boundary_num_faces2 = n2;
		faces[i].boundary_num_faces3 = n3;
		faces[i].ow_boundary_num_faces1 = own1;
		faces[i].ow_boundary_num_faces2 = own2;
		faces[i].ow_boundary_num_faces3 = own3;

		//Average out normals
		faces[i].vnx[0] = faces[i].vnx[0]/((float)(vn1));
		faces[i].vny[0] = faces[i].vny[0]/((float)(vn1));
		faces[i].vnz[0] = faces[i].vnz[0]/((float)(vn1));
		faces[i].vnx[1] = faces[i].vnx[1]/((float)(vn2));
		faces[i].vny[1] = faces[i].vny[1]/((float)(vn2));
		faces[i].vnz[1] = faces[i].vnz[1]/((float)(vn2));
		faces[i].vnx[2] = faces[i].vnx[2]/((float)(vn3));
		faces[i].vny[2] = faces[i].vny[2]/((float)(vn3));
		faces[i].vnz[2] = faces[i].vnz[2]/((float)(vn3));

		//Aerodynamic center
		faces[i].ax = (faces[i].x[0]+faces[i].x[1]+faces[i].x[2])/3.0;
		faces[i].ay = (faces[i].y[0]+faces[i].y[1]+faces[i].y[2])/3.0;
		faces[i].az = (faces[i].z[0]+faces[i].z[1]+faces[i].z[2])/3.0;
		//if (faces[i].ax < v->geometry.min_ax) v->geometry.min_ax = faces[i].ax;

		//Mass-center
		mx += faces[i].ax;
		my += faces[i].ay;
		mz += faces[i].az;

		//Reset condition
		faces[i].temperature[0] = 273.15;
		faces[i].temperature[1] = 273.15;
		faces[i].heat_flux = 0.0;
		faces[i].shockwaves = 0.0;

		//Compute mass
		faces[i].mass = faces[i].thickness * faces[i].area * material_getDensity(faces[i].m);
		v->weight.tps += faces[i].mass;
		//faces[i].hull_mass = (faces[i].area / total_area)*v->weight.hull;
	}

	//Mass-center
	v->geometry.mx = mx / (1.0*num_tris);
	v->geometry.my = my / (1.0*num_tris);
	v->geometry.mz = mz / (1.0*num_tris);

	//Not invalid
	v->geometry.invalid = 0;
	fclose(f);

	//Create heat simulation thread
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	v->geometry.heat_data_lock = lock_create();
	v->geometry.heat_thread = thread_create(dragheat_simulate_vessel_heat,v);
	v->geometry.shockwave_thread = thread_create(dragheat_simulate_vessel_shockwaves,v);
#endif
}


//==============================================================================
// Initialize highlevel interface to surface sensors
//==============================================================================
void dragheat_initialize_highlevel(vessel* v)
{
	face* faces = v->geometry.faces;
	if (!faces) return;

	//Initialize sensors
	lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels);
	lua_rawgeti(L,-1,v->index);
	lua_getfield(L,-1,"SurfaceSensors");
	v->sensors.surface = 0;
	if (lua_istable(L,-1)) {
		int num_sensors = 0;

		//Count number of sensors
		lua_pushnil(L);
		while (lua_next(L,-2)) {
			num_sensors++;
			lua_pop(L,1); //Remove value, only leave key
		}

		//Read sensors
		v->sensors.surface_count = num_sensors;
		v->sensors.surface = (surface_sensor*)malloc(sizeof(surface_sensor)*num_sensors);
		num_sensors = 0;
		lua_pushnil(L);
		while (lua_next(L,-2)) {
			double x,y,z,mind;
			int j,face;

			//Read position
			lua_getfield(L,-1,"Position");
			if (lua_istable(L,-1)) {
				lua_rawgeti(L,-1,1);
				lua_rawgeti(L,-2,2);
				lua_rawgeti(L,-3,3);
				x = lua_tonumber(L,-3);
				y = lua_tonumber(L,-2);
				z = lua_tonumber(L,-1);
				lua_pop(L,3);
			} else {
				x = 0;
				y = 0;
				z = 0;
			}
			lua_pop(L,1);

			//Find nearest face
			face = 0;
			mind = 1e9;
			for (j = 1; j < v->geometry.num_faces; j++) {
				double d = (faces[j].ax-x)*(faces[j].ax-x)+
						   (faces[j].ay-y)*(faces[j].ay-y)+
						   (faces[j].az-z)*(faces[j].az-z);
				if (fabs(d) < mind) {
					mind = fabs(d);
					face = j;
				}
			}

			//Compute required lookup parameters
			v->sensors.surface[num_sensors].face = face;
			v->sensors.surface[num_sensors].x = x;
			v->sensors.surface[num_sensors].y = y;
			v->sensors.surface[num_sensors].z = z;
			v->sensors.surface[num_sensors].d0 = sqrt(pow(x-faces[face].x[0],2)+
													  pow(y-faces[face].y[0],2)+
													  pow(z-faces[face].z[0],2));
			v->sensors.surface[num_sensors].d1 = sqrt(pow(x-faces[face].x[1],2)+
													  pow(y-faces[face].y[1],2)+
													  pow(z-faces[face].z[1],2));
			v->sensors.surface[num_sensors].d2 = sqrt(pow(x-faces[face].x[2],2)+
													  pow(y-faces[face].y[2],2)+
													  pow(z-faces[face].z[2],2));
			lua_pushnumber(L,num_sensors);
			lua_setfield(L,-2,"_sensor_idx");
			num_sensors++;

			lua_pop(L,1); //Remove value, only leave key
		}
	}
	lua_pop(L,3);
}


//==============================================================================
// Clear all resources
//==============================================================================
void dragheat_deinitialize(vessel* v)
{
	if ((v->geometry.heat_thread != BAD_ID) && 
		(v->geometry.heat_thread != 0)) {
		thread_kill(v->geometry.heat_thread);
		thread_kill(v->geometry.shockwave_thread);
		lock_destroy(v->geometry.heat_data_lock);
	}
	if (v->geometry.faces) {
		free(v->geometry.faces);
		v->geometry.faces = 0;
	}
	if (v->sensors.surface) {
		free(v->sensors.surface);
		v->sensors.surface = 0;
	}
}


//==============================================================================
// Reset state of the drag/heating simulation
//==============================================================================
void dragheat_reset() {
	int i;
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists && 
			vessels[i].geometry.faces && 
			(vessels[i].geometry.heat_thread != BAD_ID) && 
			(vessels[i].geometry.heat_thread != 0)) {
			int j;
			lock_enter(vessels[i].geometry.heat_data_lock);
			for (j = 0; j < vessels[i].geometry.num_faces; j++) {
				vessels[i].geometry.faces[j].temperature[0] = 273.15;
				vessels[i].geometry.faces[j].temperature[1] = 273.15;
				vessels[i].geometry.faces[j].heat_flux = 0.0;
				vessels[i].geometry.faces[j].shockwaves = 0.0;
			}
			lock_leave(vessels[i].geometry.heat_data_lock);
		}
	}
}


//==============================================================================
// Simulate physics for all vessels
//==============================================================================
void dragheat_simulate(float dt)
{
	int i;
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	if (dragheat_simulation.enabled) {
		quaternion q;

		//Force current position
		vessels[0].noninertial.x = current_planet.radius+dragheat_simulation.altitude;
		vessels[0].noninertial.y = 0;
		vessels[0].noninertial.z = 0;
		vessels[0].noninertial.vx = 0;
		vessels[0].noninertial.vy = 0;
		vessels[0].noninertial.vz = 0;
		vessels[0].noninertial.ax = 0;
		vessels[0].noninertial.ay = 0;
		vessels[0].noninertial.az = 0;

		qeuler_from(vessels[0].sim.q,0,0,0);
		qeuler_from(q,0,RAD(dragheat_simulation.alpha),0);
		qmul(vessels[0].sim.q,vessels[0].sim.q,q);
		qeuler_from(q,0,0,RAD(dragheat_simulation.beta));
		qmul(vessels[0].sim.q,vessels[0].sim.q,q);

		vessels[0].noninertial.P = 0;
		vessels[0].noninertial.Q = 0;
		vessels[0].noninertial.R = 0;

		quat_sim2ni(vessels[0].sim.q,vessels[0].noninertial.q);
		vessels_set_ni(&vessels[0]);
	}

	//Determine maximum values for drawing them later
	dragheat_max_temperature = 0.0;
	dragheat_max_heatflux = 0.0;
	dragheat_max_Q = 0.0;
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists && vessels[i].geometry.faces) {
			dragheat_simulate_vessel(&vessels[i],dt);
		}
	}

	//Round up maximum values to nearest multiplies
	dragheat_max_temperature = ((int)(dragheat_max_temperature/500)+1)*500.0;
	dragheat_max_heatflux = ((int)(dragheat_max_heatflux/500000)+1)*500000.0;
	dragheat_max_Q = ((int)(dragheat_max_Q/5000)+1)*5000.0;

	//Update heating simulation state
	dragheat_heating_simulate = (dt > 0.0);
#else
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists && (vessels[i].physics_type == VESSEL_PHYSICS_INERTIAL) && vessels[i].geometry.faces) {
			dragheat_simulate_vessel(&vessels[i],dt);
		}
	}
#endif
}


//==============================================================================
// Simulate shockwave physics for one vessel
//==============================================================================
void dragheat_simulate_vessel_shockwaves_add(vessel* v, face* f, face* t,
											 double ax, double ay, double az,
											 double dx, double dy, double dz,
											 double mach_sin, double weight)
{
	double cx = ax - f->ax;// - 0.2*f->nx;
	double cy = ay - f->ay;// - 0.2*f->nx;
	double cz = az - f->az;// - 0.2*f->nx;
	double c = sqrt(cx*cx+cy*cy+cz*cz)+1e-12;
	double b = -(cx*dx+cy*dy+cz*dz);
	double ndot = cx*t->nx+cy*t->ny+cz*t->nz;

	if ((b > 0) && (ndot < 0.0)) {
		double cone_cos = b/c;
		double cone_sin = sqrt(1 - cone_cos*cone_cos);
		if (cone_sin < mach_sin) {
			t->_shockwaves += 1.0*weight*(0.0+(cone_sin/mach_sin));
		}
	}
}
void dragheat_simulate_vessel_shockwaves(vessel* v)
{
	face* faces = v->geometry.faces;	//Lookup for faces
	int num_faces = v->geometry.num_faces;
	int i;
	
	while (1) {
		double M = v->geometry.effective_M;
		double dx = v->geometry.vx;
		double dy = v->geometry.vy;
		double dz = v->geometry.vz;

		//Skip cycle if paused
		if (!dragheat_heating_simulate) {
			thread_sleep(1.0/10.0);
			continue;
		}

		//Reset
		for (i = 0; i < num_faces; i++) {
			faces[i]._shockwaves = 0;
		}

		//Compute faces which can generate shockwaves, shockwave induced heating coefficients
		if (v->geometry.shockwave_heating && (M > 1.0)) {
			double mach_sin = 0.2;//1/M;

			for (i = 0; i < num_faces; i++) {
				faces[i].creates_shockwave = 0;
				if (faces[i]._dot < 0.0) { //Out of airflow
					if ((faces[i]._dot0 > 0.0) ||
						(faces[i]._dot1 > 0.0) ||
						(faces[i]._dot2 > 0.0)) {
						faces[i].creates_shockwave = 1;
					}
				}
				/*if (faces[i]._dot > 0.0) { //cos(DEG(30))
					faces[i].creates_shockwave = 1;
				}*/

				if (faces[i].creates_shockwave) {
					int j;
					for (j = 0; j < num_faces; j++) {
						if ((i != j)) {// && (faces[j].creates_drag)) {
							dragheat_simulate_vessel_shockwaves_add(v,&faces[i],&faces[j],
								faces[j].ax,faces[j].ay,faces[j].az,
								dx,dy,dz,mach_sin,0.25);
							dragheat_simulate_vessel_shockwaves_add(v,&faces[i],&faces[j],
								faces[j].x[0],faces[j].y[0],faces[j].z[0],
								dx,dy,dz,mach_sin,0.25);
							dragheat_simulate_vessel_shockwaves_add(v,&faces[i],&faces[j],
								faces[j].x[1],faces[j].y[1],faces[j].z[1],
								dx,dy,dz,mach_sin,0.25);
							dragheat_simulate_vessel_shockwaves_add(v,&faces[i],&faces[j],
								faces[j].x[2],faces[j].y[2],faces[j].z[2],
								dx,dy,dz,mach_sin,0.25);

							/*double cx = faces[j].ax - faces[i].ax;
							double cy = faces[j].ay - faces[i].ay;
							double cz = faces[j].az - faces[i].az;
							double c = sqrt(cx*cx+cy*cy+cz*cz)+1e-12;
							double b = -(cx*dx+cy*dy+cz*dz);
							double ndot = cx*faces[j].nx+cy*faces[j].ny+cz*faces[j].nz;
							if ((b > 0) && (ndot < 0.0)) {
								double cone_cos = b/c;
								double cone_sin = sqrt(1 - cone_cos*cone_cos);
								if (cone_sin < mach_sin) faces[j]._shockwaves = 3.0;//+= faces[i].area*cone_sin/mach_sin; //1.0;
								//faces[j]._shockwaves += 0.01*(1.0-pow(fabs(cone_sin-mach_sin),2));
							}*/
						}
					}
				}
			}
		}

		//Write back data
		for (i = 0; i < num_faces; i++) {
			double w = faces[i]._shockwaves;
			if (w > 1.0) w = 1.0;
			faces[i].shockwaves = w;
		}

		//10 FPS
		thread_sleep(1.0/10.0);
	}
}


//==============================================================================
// Simulate physics for one vessel. Calculate heat flux, forces
//==============================================================================
void dragheat_simulate_vessel(vessel* v, float dt)
{
	double a; //Speed of sound
	double M; //Mach number
	double T; //Air temperature
	double vx,vy,vz,vmag; //Velocity
	double dx,dy,dz; //Direction
	double fx,fy,fz; //Total force
	double tx,ty,tz; //Total torque
	double Cd; //Drag coefficient
	double K; //High-velocity heating coefficient
	face* faces;
	int i;

	//Simulate only valid body
	if (!v->geometry.faces) return;
	if (v->geometry.invalid) return;

	//Initialize lookups
	Cd = v->geometry.Cd;
	K = v->geometry.K;
	faces = v->geometry.faces;

	//Rotate airflow into local coordinates	
	if ((v->physics_type == VESSEL_PHYSICS_SIM) || (dragheat_simulation.enabled)) {
		//Override by simulator
		if (dragheat_simulation.enabled) {
			v->sim.vx = 0;
			v->sim.vy = 0;
			v->sim.vz = -dragheat_simulation.velocity;
		}
		vec_sim2l(v,v->sim.vx,v->sim.vy,v->sim.vz,&vx,&vy,&vz);
	} else {
		//FIXME: non-inertial velocity is from last frame... bug, must be from current frame
		vec_ni2l(v,v->noninertial.nvx,v->noninertial.nvy,v->noninertial.nvz,&vx,&vy,&vz);
	}
	vmag = sqrt(vx*vx+vy*vy+vz*vz)+1e-12;
	dx = vx / vmag;
	dy = vy / vmag;
	dz = vz / vmag;

	//Remember this data
	v->geometry.vx = dx;
	v->geometry.vy = dy;
	v->geometry.vz = dz;
	v->geometry.vmag = vmag;

	//Compute speed of sound and mach number
	T = v->air.temperature;
	a = sqrt(1.4*286*T); //Sqrt(gamma * R[air] * T)
	M = vmag / a;
	v->geometry.effective_M = M;

	//Lock data update
	//lock_enter(v->geometry.heat_data_lock);

	//Compute dot products of the faces
	for (i = 0; i < v->geometry.num_faces; i++) {
		faces[i]._dot = faces[i].nx * dx + faces[i].ny * dy + faces[i].nz * dz; //direction . normal
	}

	//Store dot products of boundary faces
	for (i = 0; i < v->geometry.num_faces; i++) {
		faces[i]._dot0 = 1.0;
		faces[i]._dot1 = 1.0;
		faces[i]._dot2 = 1.0;

		if (faces[i].boundary_edge[0] >= 0) faces[i]._dot0 = faces[faces[i].boundary_edge[0]]._dot;
		if (faces[i].boundary_edge[1] >= 0) faces[i]._dot1 = faces[faces[i].boundary_edge[1]]._dot;
		if (faces[i].boundary_edge[2] >= 0) faces[i]._dot2 = faces[faces[i].boundary_edge[2]]._dot;
	}

	//Compute total forces acting upon the aircraft
	fx = 0; fy = 0; fz = 0;
	tx = 0; ty = 0; tz = 0;
	for (i = 0; i < v->geometry.num_faces; i++) {
		double dot = faces[i]._dot;
		double F; //Aerodynamic force
		double dfx,dfy,dfz; //Force
		double temperature; //Surface temperature

		//Only calculate faces that see airflow
		if (dot < 0.0) dot = 0.0;

		//Dynamic pressure; Cd(alpha) = Cd*cos(alpha)
		faces[i].Q = 0.5*v->air.density*vmag*vmag*Cd*dot;
		F = -faces[i].Q*faces[i].area;

		//Compute force
		dfx = F*faces[i].nx;
		dfy = F*faces[i].ny;
		dfz = F*faces[i].nz;
		faces[i].fx = dfx;
		faces[i].fy = dfy;
		faces[i].fz = dfz;

		//Compute temperature
		if (faces[i].m > 0) {
			temperature = faces[i].temperature[FACE_LAYER_TPS];
		} else {
			temperature = faces[i].temperature[FACE_LAYER_HULL];
		}
		faces[i].surface_temperature = temperature;

		//Apply force and torque (if creates drag, or using inertial physics)
		if (faces[i].creates_drag || (v->physics_type != VESSEL_PHYSICS_SIM)) {
			fx += dfx; fy += dfy; fz += dfz;

			tx += (faces[i].ay)*dfz - (faces[i].az)*dfy; //FIXME: CG
			ty += (faces[i].az)*dfx - (faces[i].ax)*dfz;
			tz += (faces[i].ax)*dfy - (faces[i].ay)*dfx;
		}

		//Calculate heat flux over this surface
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#define SHOCKWAVE_COEF	0.2
		if (vmag > 3500) { //Hypersonic flow
			double Qh = 0.01 * K * 0.5 * v->air.density * pow(vmag,3.05);
			faces[i].heat_flux = dot * Qh + SHOCKWAVE_COEF * faces[i].shockwaves * Qh;
		} else if (vmag > 1000) { //Hypersonic flow (high density model)
			double Qh = 0.01 * K * 0.5 * v->air.density * pow(vmag,3.05);
			double Qh2 = 1.7e-4 * K * sqrt(v->air.density) * pow(vmag,3.00);
			double t = (vmag-1000)/(3500-1000);
			faces[i].heat_flux = (dot*Qh +SHOCKWAVE_COEF*faces[i].shockwaves*Qh)*t + 
								 (dot*Qh2+SHOCKWAVE_COEF*faces[i].shockwaves*Qh2)*(1-t);
		} else if (vmag > 800) { //Intermediate flow
			double Tt = T * (1.0 + min(1.0,2.0*dot)*((1.4 - 1.0)/2.0)*(M*M));
			double Qf = 5.6704e-8 * pow(Tt,4);
			double Qc = 30 * (v->air.density/1.25) * faces[i].area * (Tt-temperature);
			double Qh2 = 1.7e-4 * K * sqrt(v->air.density) * pow(vmag,3.00);
			double t = (vmag-800)/(1000-800);

			faces[i].heat_flux = (dot*Qh2+SHOCKWAVE_COEF*faces[i].shockwaves*Qh2)*t + (Qf + Qc)*(1-t);
		} else { //Isoentropic flow
			double Tt = T * (1.0 + min(1.0,2.7*dot)*((1.4 - 1.0)/2.0)*(M*M));
			double Qf = 5.6704e-8 * pow(Tt,4);
			double Qc = 30 * (v->air.density/1.25) * faces[i].area * (Tt-temperature);
			faces[i].heat_flux = Qf + Qc;
		}

		//Update statistics
		dragheat_max_temperature = max(dragheat_max_temperature,faces[i].surface_temperature);
		dragheat_max_temperature = max(dragheat_max_temperature,faces[i].temperature[1]);
		dragheat_max_heatflux = max(dragheat_max_heatflux,faces[i].heat_flux);
		dragheat_max_Q = max(dragheat_max_Q,faces[i].Q);
#endif
	}

	//Lock data end
	//lock_leave(v->geometry.heat_data_lock);

	//Apply forces acting on the vessel
	if (v->physics_type == VESSEL_PHYSICS_INERTIAL) {
		//Rotate force
		vec_l2i(v,fx,fy,fz,&dx,&dy,&dz);

		//Apply force
		//v->inertial.vx += (dx/v->mass)*dt;
		//v->inertial.vy += (dy/v->mass)*dt;
		//v->inertial.vz += (dz/v->mass)*dt;
		v->accumulated.ax += dx/v->mass;
		v->accumulated.ay += dy/v->mass;
		v->accumulated.az += dz/v->mass;

		//Apply torque
		v->accumulated.Pd += v->geometry.trqy*(ty/v->iyy);
		v->accumulated.Qd += v->geometry.trqz*(tz/v->izz);
		v->accumulated.Rd += v->geometry.trqx*(tx/v->ixx);
	} else if (v->physics_type == VESSEL_PHYSICS_SIM) {
		//Rotate force
		vec_l2sim(v,fx,fy,fz,&dx,&dy,&dz);

		//Apply force
		v->sim.vx += (dx/v->mass)*dt;
		v->sim.vy += (dy/v->mass)*dt;
		v->sim.vz += (dz/v->mass)*dt;
		v->sim.ax += dx/v->mass;
		v->sim.ay += dy/v->mass;
		v->sim.az += dz/v->mass;

		//Apply torque
		v->sim.P += v->geometry.trqy*(ty/v->iyy)*dt;
		v->sim.Q += v->geometry.trqz*(tz/v->izz)*dt;
		v->sim.R += v->geometry.trqx*(tx/v->ixx)*dt;
	}

	//Override by simulator
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	if (dragheat_simulation.enabled) {
		v->sim.vx = 0;
		v->sim.vy = 0;
		v->sim.vz = 0;
	}
#endif

	//Compute data values in sensors
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	if (v->sensors.surface) {
		lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels);
		lua_rawgeti(L,-1,v->index);
		lua_getfield(L,-1,"SurfaceSensors");
		if (lua_istable(L,-1)) { //Sensor table exists
			lua_pushnil(L);
			while (lua_next(L,-2)) {
				lua_getfield(L,-1,"_sensor_idx");
				if (lua_isnumber(L,-1)) {
					int sensor = lua_tointeger(L,-1);
					int face = v->sensors.surface[sensor].face;
					int j;

					//Compute interpolated parameters for this sensor
					lua_getfield(L,-2,"Temperature");
					if (!lua_isnil(L,-1)) {
						double t1,t2,t3,temperature;
						t3 = t2 = t1 = 1.0*faces[face].temperature[FACE_LAYER_TPS];
						for (j = 0; j < faces[face].boundary_num_faces1; j++) t1 += faces[faces[face].boundary_faces[0*8+j]].temperature[FACE_LAYER_TPS];
						for (j = 0; j < faces[face].boundary_num_faces2; j++) t2 += faces[faces[face].boundary_faces[1*8+j]].temperature[FACE_LAYER_TPS];
						for (j = 0; j < faces[face].boundary_num_faces3; j++) t3 += faces[faces[face].boundary_faces[2*8+j]].temperature[FACE_LAYER_TPS];
						t1 = t1 / (faces[face].boundary_num_faces1 + 1.0);
						t2 = t2 / (faces[face].boundary_num_faces2 + 1.0);
						t3 = t3 / (faces[face].boundary_num_faces3 + 1.0);

						temperature = (t1*v->sensors.surface[sensor].d0+t2*v->sensors.surface[sensor].d1+t3*v->sensors.surface[sensor].d2) /
									  (   v->sensors.surface[sensor].d0+   v->sensors.surface[sensor].d1+   v->sensors.surface[sensor].d2);
						v->sensors.surface[sensor].temperature = temperature;

						lua_pushcfunction(L,highlevel_dataref);
						lua_pushvalue(L,-2); //Push dataref
						lua_pushnumber(L,temperature); //Push value
						highlevel_call(2,0); //Write dataref
					}
					lua_pop(L,1);

					lua_getfield(L,-2,"HeatFlux");
					if (!lua_isnil(L,-1)) {
						double h1,h2,h3,heat_flux;
						h3 = h2 = h1 = 1.0*faces[face].heat_flux;
						for (j = 0; j < faces[face].boundary_num_faces1; j++) h1 += faces[faces[face].boundary_faces[0*8+j]].heat_flux;
						for (j = 0; j < faces[face].boundary_num_faces2; j++) h2 += faces[faces[face].boundary_faces[1*8+j]].heat_flux;
						for (j = 0; j < faces[face].boundary_num_faces3; j++) h3 += faces[faces[face].boundary_faces[2*8+j]].heat_flux;
						h1 = h1 / (faces[face].boundary_num_faces1 + 1.0);
						h2 = h2 / (faces[face].boundary_num_faces2 + 1.0);
						h3 = h3 / (faces[face].boundary_num_faces3 + 1.0);

						heat_flux = (h1*v->sensors.surface[sensor].d0+h2*v->sensors.surface[sensor].d1+h3*v->sensors.surface[sensor].d2) /
									(   v->sensors.surface[sensor].d0+   v->sensors.surface[sensor].d1+   v->sensors.surface[sensor].d2);
						v->sensors.surface[sensor].heat_flux = heat_flux;

						lua_pushcfunction(L,highlevel_dataref);
						lua_pushvalue(L,-2); //Push dataref
						lua_pushnumber(L,heat_flux); //Push value
						highlevel_call(2,0); //Write dataref
					}
					lua_pop(L,1);
				}
				lua_pop(L,1);

				lua_pop(L,1); //Remove value, only leave key
			}
		}
		lua_pop(L,3); //Remove all fields
	}
#endif
}


//==============================================================================
// Heating simulation thread
//==============================================================================
//Equations:
//[1] 1/keff = dx1/k1 + dx2/k2 (effective heat conductivity)
//[2] keff = (k1 k2) / (dx1 k2 + dx2 k1)
//[3] Teq = (Cp1 m1 t1 + Cp2 m2 t2) / (Cp2 m2 + Cp1 m1)
//[3] Cp10 m10 (T10 - Teq) + Cp20 m20 (T20 - Teq) = 0
//[4] Teq = (Cp10 m10 T10 + Cp20 m20 T20) / (Cp20 m20 + Cp10 m10) (equilibrium temperature)

void dragheat_simulate_vessel_heat_conduction_l2l(face* face, double dt) //Layer-to-layer
{
	double Q01	= 0.0; //Heat transferred in face, from TPS to hull
	double mQ0 = 0.0; //Max change of heat in face TPS
	double mQ1 = 0.0; //Max change of heat in face hull
	double mQ  = 0.0;

	double dQ0 = 0.0; //Change of heat in face TPS
	double dQ1 = 0.0; //Change of heat in face hull

	if (face->m > 0) {
		//Compute heat transfer between TPS and hull
		Q01 = dt * face->_k[0] * face->area * (face->temperature[1]-face->temperature[0]) / face->thickness;

		//Compute maximum change of heat
		mQ0 = dt * 0.5 * face->_Cp[0] * face->mass *      fabs(face->temperature[1]-face->temperature[0]);
		mQ1 = dt * 0.5 * face->_Cp[1] * face->hull_mass * fabs(face->temperature[1]-face->temperature[0]);
		mQ = mQ0;
		if (mQ > mQ1) mQ = mQ1;

		//Compute change in thermal energy
		dQ0 =  Q01;
		dQ1 = -Q01;
	
		//Limit amount of heat transferred
		if (dQ0 >  mQ) dQ0 =  mQ;
		if (dQ0 < -mQ) dQ0 = -mQ;
		if (dQ1 >  mQ) dQ1 =  mQ;
		if (dQ1 < -mQ) dQ1 = -mQ;
	
		//Face temperatures
		face->_temperature[0] += dQ0/(face->_Cp[0]*face->mass);
		face->_temperature[1] += dQ1/(face->_Cp[1]*face->hull_mass);
	}
}


void dragheat_simulate_vessel_heat_conduction_f2f(face* face1, face* face2, double dt) //Face-to-face
{
	//Heat transfer parameters
	double Q1	= 0.0; //Heat transferred from face 1 to face 2 hull
	double Q0	= 0.0; //Heat transferred from face 1 to face 2 TPS

	double mQ10 = 0.0; //Max change of heat in face 1 TPS
	double mQ11 = 0.0; //Max change of heat in face 1 hull
	double mQ20 = 0.0; //Max change of heat in face 2 TPS
	double mQ21 = 0.0; //Max change of heat in face 2 hull
	double mQ = 0.0;   //Composite max change

	double dQ10 = 0.0; //Change of heat in face 1 TPS
	double dQ11 = 0.0; //Change of heat in face 1 hull
	double dQ20 = 0.0; //Change of heat in face 2 TPS
	double dQ21 = 0.0; //Change of heat in face 2 hull


	//Estimate length of one side (assume triangles are equilateral), effective thickness
	double l = sqrt((face1->area + face2->area) / (2 * 0.433));
	double h = (face1->thickness + face2->thickness)*0.5;
	double d = sqrt((face1->ax-face2->ax)*(face1->ax-face2->ax)+
					(face1->ay-face2->ay)*(face1->ay-face2->ay)+
					(face1->az-face2->az)*(face1->az-face2->az))*0.1;


	//Heat transfer between face 1 and face 2 hull
	{
		double M = face1->hull_mass + face2->hull_mass;
		double dx1 = d*face1->hull_mass / M;
		double dx2 = d*face2->hull_mass / M;
		double keff = face1->_k[1];//(    face1->_k[1] *     face2->_k[1])/
					  //(dx1*face2->_k[1] + dx2*face1->_k[1]); //Eq [2]

		//Assumes 1 meter thickness for hull
		Q1 = dt * keff * l * 1.0 * (face2->temperature[1] - face1->temperature[1]) / d;
	}
	//Heat transfer between face 1 and face 2 TPS
	if ((face1->m > 0) && (face2->m > 0)) {
		double M = face1->mass+face2->mass;
		double dx1 = d*face1->mass / M;
		double dx2 = d*face2->mass / M;
		double keff = face2->_k[0];//(    face1->_k[0] *     face2->_k[0])/
					  //(dx1*face2->_k[0] + dx2*face1->_k[0]); //Eq [2]

		//Use average thickness for TPS h
		Q0 = dt * keff * l * 1.0 * (face2->temperature[0] - face1->temperature[0]) / d;
	}


	//Compute maximum change in heat
	{
		mQ11 = dt * 0.5 * face1->_Cp[1] * face1->hull_mass * fabs(face2->temperature[1]-face1->temperature[1]);
		mQ21 = dt * 0.5 * face2->_Cp[1] * face2->hull_mass * fabs(face2->temperature[1]-face1->temperature[1]);
	}
	if ((face1->m > 0) && (face2->m > 0)) {
		mQ10 = dt * 0.5 * face1->_Cp[0] * face1->mass      * fabs(face2->temperature[0]-face1->temperature[0]);
		mQ20 = dt * 0.5 * face2->_Cp[0] * face2->mass      * fabs(face2->temperature[0]-face1->temperature[0]);
	}

	mQ = mQ11;
	if (mQ > mQ21) mQ = mQ21;
	if ((face1->m > 0) && (face2->m > 0)) {
		if (mQ > mQ10) mQ = mQ10;
		if (mQ > mQ20) mQ = mQ20;
	}

	//Compute change in thermal energy
	dQ11 =  Q1;
	dQ21 = -Q1;
	dQ10 =  Q0;
	dQ20 = -Q0;


	//Recompute change in energy
	if (dQ10 >  mQ) dQ10 =  mQ;
	if (dQ10 < -mQ) dQ10 = -mQ;
	if (dQ20 >  mQ) dQ20 =  mQ;
	if (dQ20 < -mQ) dQ20 = -mQ;
	if (dQ11 >  mQ) dQ11 =  mQ;
	if (dQ11 < -mQ) dQ11 = -mQ;
	if (dQ21 >  mQ) dQ21 =  mQ;
	if (dQ21 < -mQ) dQ21 = -mQ;


	//Integrate change in heat
	face1->_temperature[0] += dQ10/(face1->_Cp[0]*face1->mass);
	face1->_temperature[1] += dQ11/(face1->_Cp[1]*face1->hull_mass);
	face2->_temperature[0] += dQ20/(face2->_Cp[0]*face2->mass);
	face2->_temperature[1] += dQ21/(face2->_Cp[1]*face2->hull_mass);
}


void dragheat_simulate_vessel_heat(vessel* v)
{
	face* faces = v->geometry.faces;	//Lookup for faces
	int num_faces = v->geometry.num_faces;
	double prev_time = curtime();
	int i,j;

	thread_sleep(1.0);
	
	while (1) {
		//Fetch some variables
		double natm = v->air.concentration;
		double Tatm = v->air.temperature;
		//Compute delta time
		double new_time = curtime();
		double dt = new_time - prev_time;
		prev_time = new_time;
		//Limit FPS to at least 10 (for stability)
		if (dt > 0.1) dt = 0.1;
		if (dt < 0.0) dt = 0.1;
		//Skip cycle if paused
		if (!dragheat_heating_simulate) {
			thread_sleep(1.0/30.0);
			continue;
		}

		//Enter data lock
		lock_enter(v->geometry.heat_data_lock);

		//Compute new temperatures due to heat flux
		for (i = 0; i < num_faces; i++) {
			double heat_flux;		//W/m2
			double area;			//Total area, m2
			double temperature;		//Outtermost temperature, K
			double thickness;		//Total thickness, m
			double k;				//Thermal conductivity, W/m
			double dQ;				//Change in energy, J

			//Triangle parameters
			area = faces[i].area;
			thickness = faces[i].thickness;
			heat_flux = faces[i].heat_flux;

			//Get correct parameters
			if (faces[i].m > 0) { //Thermal Protection
				temperature = faces[i].temperature[FACE_LAYER_TPS];
				faces[i]._Cp[0] = material_getCp(faces[i].m,temperature);		//Specific heat
				faces[i]._k[0] = material_getk(faces[i].m,temperature);			//Thermal conductivity
				faces[i]._Cp[1] = material_getCp(v->geometry.hull,temperature);	//Specific heat
				faces[i]._k[1] = material_getk(v->geometry.hull,temperature);	//Thermal conductivity
				k = faces[i]._k[0];
			} else { //Vessels Hull
				temperature = faces[i].temperature[FACE_LAYER_HULL];
				faces[i]._Cp[0] = 0.0;
				faces[i]._k[0] = 0.0;
				faces[i]._Cp[1] = material_getCp(v->geometry.hull,temperature);	//Specific heat
				faces[i]._k[1] = material_getk(v->geometry.hull,temperature);	//Thermal conductivity
				k = faces[i]._k[1];
			}

			//Compute hull mass (FIXME)
			faces[i].hull_mass = (faces[i].area / v->geometry.total_area)*v->weight.hull;

			//Calculate total change in thermal energy due to external factors
			dQ = 
				  dt * area * heat_flux										//External heat flux
				- dt * area * 5.6704e-8   * pow(temperature,4)				//Radiative losses
				- dt * area * k * (temperature-Tatm) * (1.41*1e-24) * natm;	//Conduction to atmosphere

			//Integrate
			faces[i]._temperature[0] = faces[i].temperature[0];
			faces[i]._temperature[1] = faces[i].temperature[1];
			if (faces[i].m > 0) {
				faces[i]._temperature[0] += dQ/(faces[i]._Cp[0]*faces[i].mass);
			} else {
				faces[i]._temperature[1] += dQ/(faces[i]._Cp[1]*faces[i].hull_mass);
			}
		}

		//Compute heat conduction. The conduction is calculated for both faces at once in one iteration
		for (i = 0; i < num_faces; i++) {
			face* face1 = &faces[i];
			dragheat_simulate_vessel_heat_conduction_l2l(face1,dt);
			for (j = 0; j < face1->ow_boundary_num_faces1; j++) {
				face* face2 = &faces[face1->ow_boundary_faces[0*8+j]];
				dragheat_simulate_vessel_heat_conduction_f2f(face1,face2,dt);
			}
			for (j = 0; j < face1->ow_boundary_num_faces2; j++) {
				face* face2 = &faces[face1->ow_boundary_faces[1*8+j]];
				dragheat_simulate_vessel_heat_conduction_f2f(face1,face2,dt);
			}
			for (j = 0; j < face1->ow_boundary_num_faces3; j++) {
				face* face2 = &faces[face1->ow_boundary_faces[2*8+j]];
				dragheat_simulate_vessel_heat_conduction_f2f(face1,face2,dt);
			}
		}

		//Write back temperature
		for (i = 0; i < num_faces; i++) {
			faces[i].temperature[0] = faces[i]._temperature[0];
			faces[i].temperature[1] = faces[i]._temperature[1];
		}

		//Leave data lock
		lock_leave(v->geometry.heat_data_lock);

		//30 FPS
		v->geometry.heat_fps = 1/dt;
		thread_sleep(1.0/30.0);
	}
}






//==============================================================================
// Rendering
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
float dragheat_pallette[6*3] = {		
	1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
};

GLhandleARB dragheat_reentry_plasma_program; //Program object
GLhandleARB dragheat_reentry_plasma_vertex; //Vertex shader for clouds
GLhandleARB dragheat_reentry_plasma_fragment; //Fragment shader 
GLint dragheat_reentry_plasma_perlin_perm_location; //Permutations texture
GLint dragheat_reentry_plasma_time_location; //Time
GLint dragheat_reentry_plasma_area_location; //Total estimated area
GLint dragheat_reentry_plasma_mach_location; //Mach number estimate
GLint dragheat_reentry_plasma_flux_location; //Heat flux estimate

GLhandleARB dragheat_reentry_pp_program; //Program object
GLhandleARB dragheat_reentry_pp_vertex; //Vertex shader for clouds
GLhandleARB dragheat_reentry_pp_fragment; //Fragment shader 
GLint dragheat_reentry_pp_framebuffer_location;
GLint dragheat_reentry_pp_zbuffer_location;

GLuint dragheat_fbo; //Normal size FBO
GLuint dragheat_fbo_texture;
GLuint dragheat_fbo_depth;
//GLuint dragheat_fbo2; //Half size FBO
//GLuint dragheat_fbo2_texture;
//GLuint dragheat_fbo2_depth;
int dragheat_fbo_width;
int dragheat_fbo_height;

unsigned long dragheat_getpow2(unsigned long v)
{
    v--;
    v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
    v++;
    return v;
}

void dragheat_draw_initialize()
{
	int screen_width,screen_height;
	if (!config.use_shaders) return;

	//Create shader for plasma
	dragheat_reentry_plasma_vertex = rendering_shader_compile(GL_VERTEX_SHADER,FROM_PLUGINS("shaders/reentry_plasma.vert"),1);
	dragheat_reentry_plasma_fragment = rendering_shader_compile(GL_FRAGMENT_SHADER,FROM_PLUGINS("shaders/reentry_plasma.frag"),0);

	dragheat_reentry_plasma_program = rendering_shader_link(dragheat_reentry_plasma_vertex,dragheat_reentry_plasma_fragment);
	dragheat_reentry_plasma_perlin_perm_location = glGetUniformLocation(dragheat_reentry_plasma_program, "permTexture");
	dragheat_reentry_plasma_time_location = glGetUniformLocation(dragheat_reentry_plasma_program, "curTime");
	dragheat_reentry_plasma_area_location = glGetUniformLocation(dragheat_reentry_plasma_program, "totalArea");
	dragheat_reentry_plasma_mach_location = glGetUniformLocation(dragheat_reentry_plasma_program, "machNo");
	dragheat_reentry_plasma_flux_location = glGetUniformLocation(dragheat_reentry_plasma_program, "heatFlux");

	//Create shader for postprocessing
	dragheat_reentry_pp_vertex = rendering_shader_compile(GL_VERTEX_SHADER,FROM_PLUGINS("shaders/reentry_plasma_postprocess.vert"),1);
	dragheat_reentry_pp_fragment = rendering_shader_compile(GL_FRAGMENT_SHADER,FROM_PLUGINS("shaders/reentry_plasma_postprocess.frag"),0);

	dragheat_reentry_pp_program = rendering_shader_link(dragheat_reentry_pp_vertex,dragheat_reentry_pp_fragment);
	dragheat_reentry_pp_framebuffer_location = glGetUniformLocation(dragheat_reentry_pp_program, "frameBuffer");
	dragheat_reentry_pp_zbuffer_location = glGetUniformLocation(dragheat_reentry_pp_program, "zBuffer");

	//Get screen size
	XPLMGetScreenSize(&screen_width,&screen_height);
	dragheat_fbo_width = screen_width;//dragheat_getpow2(screen_width);
	dragheat_fbo_height = screen_height;//dragheat_getpow2(screen_height);
	//dragheat_fbo_width = dragheat_getpow2(screen_width);
	//dragheat_fbo_height = dragheat_getpow2(screen_height);

	//Create FBO texture for post-processing
	rendering_init_fbo(&dragheat_fbo,&dragheat_fbo_texture,&dragheat_fbo_depth,
		dragheat_fbo_width,dragheat_fbo_height);
	//rendering_init_fbo(&dragheat_fbo2,&dragheat_fbo2_texture,&dragheat_fbo2_depth,
		//dragheat_fbo_width/4,dragheat_fbo_height/4);
}

void dragheat_draw_deinitialize()
{
	if (!config.use_shaders) return;

	glDeleteProgram(dragheat_reentry_plasma_program);
	glDeleteShader(dragheat_reentry_plasma_vertex);
	glDeleteShader(dragheat_reentry_plasma_fragment);

	glDeleteProgram(dragheat_reentry_pp_program);
	glDeleteShader(dragheat_reentry_pp_vertex);
	glDeleteShader(dragheat_reentry_pp_fragment);

	glDeleteFramebuffers(1, &dragheat_fbo);
}

void dragheat_draw()
{
	int i;
	extern XPLMDataRef dataref_vessel_x;
	extern XPLMDataRef dataref_vessel_y;
	extern XPLMDataRef dataref_vessel_z;
	double dx = XPLMGetDatad(dataref_vessel_x)-vessels[0].sim.x;
	double dy = XPLMGetDatad(dataref_vessel_y)-vessels[0].sim.y;
	double dz = XPLMGetDatad(dataref_vessel_z)-vessels[0].sim.z;

	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists && vessels[i].geometry.faces) {
			if (vessels[i].physics_type == VESSEL_PHYSICS_SIM) {
				glPushMatrix();
				glTranslated(dx,dy,dz);
			}

			dragheat_draw_vessel(&vessels[i]);

			if (vessels[i].physics_type == VESSEL_PHYSICS_SIM) glPopMatrix();
		}
	}

	//Process picking faces (painting them)
	//XPLMSetGraphicsState(0,0,0,0,0,0,0);
	if (dragheat_visual_mode > DRAGHEAT_VISUALMODE_NORMAL) {
		float distance;
		dragheat_selected_triangle = -1;
		distance = 1e9;

		for (i = 0; i < vessel_count; i++) {
			vessel* v = &vessels[i];
			face* faces = v->geometry.faces;
			double x,y,z;
			float pointNear[4],pointFar[4];
			float localNear[4],localFar[4];

			//Get view ray endpoints
			trace_get_view_ray(pointNear, pointFar);

			//Get them in local coordinates
			coord_sim2l(v,pointNear[0],pointNear[1],pointNear[2],&x,&y,&z);
			localNear[0] = (float)x; localNear[1] = (float)y; localNear[2] = (float)z;
			coord_sim2l(v,pointFar[0],pointFar[1],pointFar[2],&x,&y,&z);
			localFar[0] = (float)x; localFar[1] = (float)y; localFar[2] = (float)z;

			//Attempt to select a triangle
			for (i = 0; i < v->geometry.num_faces; i++) {
				float p1[3], p2[3], p3[3];
				float collision_point[3];
				float tx,ty,tz;

				tx = (float)v->geometry.dx;
				ty = (float)v->geometry.dy;
				tz = (float)v->geometry.dz;

				p1[0] = (float)faces[i].x[0]+tx;
				p2[0] = (float)faces[i].x[1]+tx;
				p3[0] = (float)faces[i].x[2]+tx;
				p1[1] = (float)faces[i].y[0]+ty;
				p2[1] = (float)faces[i].y[1]+ty;
				p3[1] = (float)faces[i].y[2]+ty;
				p1[2] = (float)faces[i].z[0]+tz;
				p2[2] = (float)faces[i].z[1]+tz;
				p3[2] = (float)faces[i].z[2]+tz;

				if (trace_segment_vs_triangle(p2,p1,p3,localNear,localFar,collision_point)) {
					float d = (powf(collision_point[0]-localNear[0],2)+
							   powf(collision_point[1]-localNear[1],2)+
							   powf(collision_point[2]-localNear[2],2));
					if (d < distance) {
						dragheat_selected_triangle = i;
						dragheat_selected_vessel = v->index;
						distance = d;
					}
				}
			}
		}

		if (dragheat_visual_mode == DRAGHEAT_VISUALMODE_MATERIAL) {
			if ((dragheat_paint_mode >= 0) && 
				(dragheat_selected_triangle >= 0) &&
				(dragheat_selected_vessel >= 0) &&
				(dragheat_selected_vessel < vessel_count)) {
				face* paint_face = &vessels[dragheat_selected_vessel].geometry.faces[dragheat_selected_triangle];
				paint_face->m = dragheat_paint_mode;
				paint_face->thickness = dragheat_paint_thickness;

				vessels[dragheat_selected_vessel].weight.tps -= paint_face->mass;
				paint_face->mass = paint_face->thickness * paint_face->area * material_getDensity(paint_face->m);
				vessels[dragheat_selected_vessel].weight.tps += paint_face->mass;
			}
		}
	}
}


void dragheat_draw2d() 
{
	float RGB[4] = { 1.00f,0.85f,0.30f,1.0f };
	double mag,mag_min,mag_max;
	double cmag = -1.0;
	char buf[ARBITRARY_MAX] = { 0 };
	extern int planet_texture_scattering;

	//Fucking bubbles
	/*XPLMSetGraphicsState(0,1,0,0,1,0,0);
	glPushMatrix();
	glTranslated(32.0,32.0,0.0);
	XPLMBindTexture2d(dragheat_fbo_texture,0);
	glBegin(GL_QUADS);
	glTexCoord2d(0.0,0.0);
	glVertex2d(0.0,0.0);
	glTexCoord2d(0.0,1.0);
	glVertex2d(0.0,256.0);
	glTexCoord2d(1.0,1.0);
	glVertex2d(256.0,256.0);
	glTexCoord2d(1.0,0.0);
	glVertex2d(256.0,0.0);
	glEnd();
	glPopMatrix();*/

	//Do not draw if not in special draw mode
	if ((dragheat_visual_mode <= DRAGHEAT_VISUALMODE_NORMAL) ||
		(dragheat_visual_mode >= DRAGHEAT_VISUALMODE_FEM)) return;

	//Setup 2D drawing
	XPLMSetGraphicsState(0,0,0,0,0,0,0);
	glPushMatrix();
	glTranslated(32.0,256.0,0.0);
	glBegin(GL_QUADS);
	for (mag = 0.01; mag <= 1.0; mag += 0.01) {
		int i1 = (int)(mag*5);
		int i2 = (int)((mag-0.01)*5);
		double r1,g1,b1;
		double r2,g2,b2;
		double t1,t2;
		t1 = mag*5-i1;
		t2 = (mag-0.01)*5-i2;

		r1 = dragheat_pallette[(i1)*3+0]*(1-t1) + t1*dragheat_pallette[(i1+1)*3+0];
		g1 = dragheat_pallette[(i1)*3+1]*(1-t1) + t1*dragheat_pallette[(i1+1)*3+1];
		b1 = dragheat_pallette[(i1)*3+2]*(1-t1) + t1*dragheat_pallette[(i1+1)*3+2];
		r2 = dragheat_pallette[(i2)*3+0]*(1-t2) + t2*dragheat_pallette[(i2+1)*3+0];
		g2 = dragheat_pallette[(i2)*3+1]*(1-t2) + t2*dragheat_pallette[(i2+1)*3+1];
		b2 = dragheat_pallette[(i2)*3+2]*(1-t2) + t2*dragheat_pallette[(i2+1)*3+2];

		//Draw
		glColor4d(r2,g2,b2,1.0);
		glVertex3d(0,(mag-0.01)*256.0,0);
		glColor4d(r1,g1,b1,1.0);
		glVertex3d(0.0,mag*256.0,0);
		glVertex3d(32.0,mag*256.0,0);
		glColor4d(r2,g2,b2,1.0);
		glVertex3d(32.0,(mag-0.01)*256.0,0);		
	}
	glEnd();

	if (dragheat_selected_triangle >= 0) {
		if ((dragheat_selected_vessel >= 0) &&
			(dragheat_selected_vessel < vessel_count) &&
			(vessels[dragheat_selected_vessel].exists) &&
			(vessels[dragheat_selected_vessel].geometry.faces)) {
			switch (dragheat_visual_mode) {
				case DRAGHEAT_VISUALMODE_TEMPERATURE:
					cmag = vessels[dragheat_selected_vessel].geometry.faces[dragheat_selected_triangle].surface_temperature;
				break;
				case DRAGHEAT_VISUALMODE_FLUX:
					cmag = vessels[dragheat_selected_vessel].geometry.faces[dragheat_selected_triangle].heat_flux;
				break;
				case DRAGHEAT_VISUALMODE_HULL_TEMPERATURE:
					cmag = vessels[dragheat_selected_vessel].geometry.faces[dragheat_selected_triangle].temperature[FACE_LAYER_HULL];
				break;
				case DRAGHEAT_VISUALMODE_DYNAMIC_PRESSURE:
					cmag = vessels[dragheat_selected_vessel].geometry.faces[dragheat_selected_triangle].Q;
				break;
				case DRAGHEAT_VISUALMODE_SHOCKWAVES:
					cmag = vessels[dragheat_selected_vessel].geometry.faces[dragheat_selected_triangle].shockwaves;
				break;
				default:
					cmag = 0.0;
				break;
			}
		}
	}

	switch (dragheat_visual_mode) {
		case DRAGHEAT_VISUALMODE_TEMPERATURE:
			mag_min = 300.0; mag_max = dragheat_max_temperature;
			XPLMDrawString(RGB,4,256+16,"Temperature (K)",0,xplmFont_Basic);
			
			if (cmag >= 0.0) {
				snprintf(buf,ARBITRARY_MAX-1,"%.0f C",cmag-273.15);
				XPLMDrawString(RGB,4+32,256+48,buf,0,xplmFont_Basic);
				snprintf(buf,ARBITRARY_MAX-1,"%.0f K",cmag);
				XPLMDrawString(RGB,4+32,256+32,buf,0,xplmFont_Basic);
			}
		break;
		case DRAGHEAT_VISUALMODE_FLUX:
			mag_min = 0.0; mag_max = dragheat_max_heatflux*1e-3;
			XPLMDrawString(RGB,4,256+16,"Heat flux (KW/m2)",0,xplmFont_Basic);
			
			if (cmag >= 0.0) {
				snprintf(buf,ARBITRARY_MAX-1,"%.3f KW/m2",cmag*1e-3);
				XPLMDrawString(RGB,4+32,256+32,buf,0,xplmFont_Basic);
			}
		break;
		case DRAGHEAT_VISUALMODE_HULL_TEMPERATURE:
			mag_min = 300.0; mag_max = dragheat_max_temperature;
			XPLMDrawString(RGB,4,256+16,"Hull temperature (K)",0,xplmFont_Basic);
			
			if (cmag >= 0.0) {
				snprintf(buf,ARBITRARY_MAX-1,"%.0f C",cmag-273.15);
				XPLMDrawString(RGB,4+32,256+48,buf,0,xplmFont_Basic);
				snprintf(buf,ARBITRARY_MAX-1,"%.0f K",cmag);
				XPLMDrawString(RGB,4+32,256+32,buf,0,xplmFont_Basic);
			}
		break;
		case DRAGHEAT_VISUALMODE_DYNAMIC_PRESSURE:
			mag_min = 0.0; mag_max = dragheat_max_Q*1e-3;
			XPLMDrawString(RGB,4,256+16,"Dynamic pressure (KPa)",0,xplmFont_Basic);
			
			if (cmag >= 0.0) {
				snprintf(buf,ARBITRARY_MAX-1,"%.3f KPa",cmag*1e-3);
				XPLMDrawString(RGB,4+32,256+32,buf,0,xplmFont_Basic);
			}
		break;
		case DRAGHEAT_VISUALMODE_SHOCKWAVES:
			mag_min = 0.0; mag_max = 1.0;//dragheat_max_Q*1e-3;
			//XPLMDrawString(RGB,4,256+16,"Dynamic pressure (KPa)",0,xplmFont_Basic);
			XPLMDrawString(RGB,4,256+16,"Shockwaves",0,xplmFont_Basic);
			
			if (cmag >= 0.0) {
				snprintf(buf,ARBITRARY_MAX-1,"%.3f",cmag*1e-3);
				XPLMDrawString(RGB,4+32,256+32,buf,0,xplmFont_Basic);
			}
		break;
		default:
			mag_min = 0.0; mag_max = 1.0;
			XPLMDrawString(RGB,4,256+16,"Error",0,xplmFont_Basic);

			if (cmag >= 0.0) {
				snprintf(buf,ARBITRARY_MAX-1,"%.0f error",cmag);
				XPLMDrawString(RGB,4+32,256+32,buf,0,xplmFont_Basic);
			}
		break;
	}

	for (mag = 0.00; mag <= 1.0; mag += 0.10) {
		snprintf(buf,ARBITRARY_MAX-1,"%.0f",mag_min+mag*(mag_max-mag_min));
		XPLMDrawString(RGB,4+32,(int)(mag*256.0),buf,0,xplmFont_Basic);
	}

	glPopMatrix();
}
#endif


//Planck's constant
#define H 6.26e-34
//Speed of light
#define C 299792458

//Red wavelength
#define R_WL 700e-9
//Green wavelength
#define G_WL 540e-9
//Blue wavelength
#define B_WL 450e-9

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void dragheat_draw_vessel(vessel* v)
{
	int i,j;
	quaternion q;
	double tx,ty,tz;
	face* faces = v->geometry.faces;

	//Setup rendering
	glPushMatrix();
	//Translate into correct position
	coord_l2sim(v,0,0,0,&q[0],&q[1],&q[2]);
	glTranslated(q[0],q[1],q[2]);
	//Rotate into vessel orientation
	qeuler_to(v->sim.q,&q[0],&q[1],&q[2]);
	glRotated(-DEG(q[2]),0,1,0);
	glRotated( DEG(q[1]),1,0,0);
	glRotated(-DEG(q[0]),0,0,1);

	//Polygon offset for rendering over the old mesh
	glEnable(GL_POLYGON_OFFSET_FILL);
	//if (dragheat_visual_mode >= DRAGHEAT_VISUALMODE_FEM) {
		//glPolygonOffset(0,-8);
	//} else {
	glPolygonOffset(-2,-2);
	//}

	//Setup correct mode
	switch (dragheat_visual_mode) {
		case DRAGHEAT_VISUALMODE_NORMAL:
			XPLMSetGraphicsState(0,0,0,0,1,1,1);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		break;
		case DRAGHEAT_VISUALMODE_FLUX:
		case DRAGHEAT_VISUALMODE_TEMPERATURE:
		case DRAGHEAT_VISUALMODE_HULL_TEMPERATURE:
		case DRAGHEAT_VISUALMODE_DYNAMIC_PRESSURE:
		case DRAGHEAT_VISUALMODE_SHOCKWAVES:
			XPLMSetGraphicsState(0,0,0,0,1,1,1);
		break;
		case DRAGHEAT_VISUALMODE_FEM:
			XPLMSetGraphicsState(0,0,1,0,1,1,1);
		break;
		case DRAGHEAT_VISUALMODE_MATERIAL:
			XPLMSetGraphicsState(0,0,0,0,1,1,1);
		break;
	}


	//Draw mesh
	glBegin(GL_TRIANGLES);
	for (i = 0; i < v->geometry.num_faces; i++) {
		switch (dragheat_visual_mode) {
			case DRAGHEAT_VISUALMODE_NORMAL:
				{
					double normalization = 1/1.0e9; //Normalization coefficient
					double r1,b1,g1; //Colors for three vertices
					double r2,b2,g2;
					double r3,b3,g3;
					double t1,t2,t3; //Temperature for three vertices
					//double noise1 = (1.0 * rand() / RAND_MAX)-0.5; //Random noise
					//double noise2 = (1.0 * rand() / RAND_MAX)-0.5;
					//double noise3 = (1.0 * rand() / RAND_MAX)-0.5;
	
					//Calculate temperature in each vertex
					t3 = t2 = t1 = faces[i].surface_temperature;
					for (j = 0; j < faces[i].boundary_num_faces1; j++)
						t1 += faces[faces[i].boundary_faces[0*8+j]].surface_temperature;
					for (j = 0; j < faces[i].boundary_num_faces2; j++)
						t2 += faces[faces[i].boundary_faces[1*8+j]].surface_temperature;
					for (j = 0; j < faces[i].boundary_num_faces3; j++)
						t3 += faces[faces[i].boundary_faces[2*8+j]].surface_temperature;

					//Average it out
					t1 = t1 / (1.0*v->geometry.faces[i].boundary_num_faces1+1.0);
					t2 = t2 / (1.0*v->geometry.faces[i].boundary_num_faces2+1.0);
					t3 = t3 / (1.0*v->geometry.faces[i].boundary_num_faces3+1.0);

					//If sufficiently hot, calculate and draw radiative losses
					if ((t1 > 800) || (t2 > 800) || (t3 > 800)) {
						r1 = normalization*(2*H*C*C/pow(R_WL,5))/(exp(H*C/(R_WL*1.38e-23*t1))-1);
						g1 = normalization*(2*H*C*C/pow(G_WL,5))/(exp(H*C/(G_WL*1.38e-23*t1))-1);
						b1 = normalization*(2*H*C*C/pow(B_WL,5))/(exp(H*C/(B_WL*1.38e-23*t1))-1);

						r2 = normalization*(2*H*C*C/pow(R_WL,5))/(exp(H*C/(R_WL*1.38e-23*t2))-1);
						g2 = normalization*(2*H*C*C/pow(G_WL,5))/(exp(H*C/(G_WL*1.38e-23*t2))-1);
						b2 = normalization*(2*H*C*C/pow(B_WL,5))/(exp(H*C/(B_WL*1.38e-23*t2))-1);

						r3 = normalization*(2*H*C*C/pow(R_WL,5))/(exp(H*C/(R_WL*1.38e-23*t3))-1);
						g3 = normalization*(2*H*C*C/pow(G_WL,5))/(exp(H*C/(G_WL*1.38e-23*t3))-1);
						b3 = normalization*(2*H*C*C/pow(B_WL,5))/(exp(H*C/(B_WL*1.38e-23*t3))-1);

						glColor4d(r2,g2,b2,1.0);
						glVertex3d(-faces[i].y[1],faces[i].z[1],faces[i].x[1]);
						glColor4d(r1,g1,b1,1.0);
						glVertex3d(-faces[i].y[0],faces[i].z[0],faces[i].x[0]);
						glColor4d(r3,g3,b3,1.0);
						glVertex3d(-faces[i].y[2],faces[i].z[2],faces[i].x[2]);
					}
				}
				break;
			case DRAGHEAT_VISUALMODE_FLUX:
			case DRAGHEAT_VISUALMODE_TEMPERATURE:
			case DRAGHEAT_VISUALMODE_HULL_TEMPERATURE:
			case DRAGHEAT_VISUALMODE_DYNAMIC_PRESSURE:
			case DRAGHEAT_VISUALMODE_SHOCKWAVES:
				{
					double mag1,mag2,mag3; //Magnitudes
					double mag_min,mag_max;
					double r1,b1,g1; //Colors for three vertices
					double r2,b2,g2;
					double r3,b3,g3;
					int i1,i2,i3;
					double t1,t2,t3;

					//Min/max range
					mag_min = 0.0;
					mag_max = 1.0;

					//Calculate value in each vertex
					switch (dragheat_visual_mode) {
						case DRAGHEAT_VISUALMODE_TEMPERATURE:
							mag_min = 300.0; mag_max = dragheat_max_temperature;
							mag3 = mag2 = mag1 = faces[i].surface_temperature;
							if (dragheat_visual_interpolate) {
								for (j = 0; j < faces[i].boundary_num_faces1; j++)
									mag1 += faces[faces[i].boundary_faces[0*8+j]].surface_temperature;
								for (j = 0; j < faces[i].boundary_num_faces2; j++)
									mag2 += faces[faces[i].boundary_faces[1*8+j]].surface_temperature;
								for (j = 0; j < faces[i].boundary_num_faces3; j++)
									mag3 += faces[faces[i].boundary_faces[2*8+j]].surface_temperature;
							}
						break;
						case DRAGHEAT_VISUALMODE_FLUX:
							mag_min = 0.0; mag_max = dragheat_max_heatflux;
							mag3 = mag2 = mag1 = faces[i].heat_flux;
							if (dragheat_visual_interpolate) {
								for (j = 0; j < faces[i].boundary_num_faces1; j++)
									mag1 += faces[faces[i].boundary_faces[0*8+j]].heat_flux;
								for (j = 0; j < faces[i].boundary_num_faces2; j++)
									mag2 += faces[faces[i].boundary_faces[1*8+j]].heat_flux;
								for (j = 0; j < faces[i].boundary_num_faces3; j++)
									mag3 += faces[faces[i].boundary_faces[2*8+j]].heat_flux;
							}
						break;
						case DRAGHEAT_VISUALMODE_HULL_TEMPERATURE:
							mag_min = 300.0; mag_max = dragheat_max_temperature;
							mag3 = mag2 = mag1 = faces[i].temperature[FACE_LAYER_HULL];
							if (dragheat_visual_interpolate) {
								for (j = 0; j < faces[i].boundary_num_faces1; j++)
									mag1 += faces[faces[i].boundary_faces[0*8+j]].temperature[FACE_LAYER_HULL];
								for (j = 0; j < faces[i].boundary_num_faces2; j++)
									mag2 += faces[faces[i].boundary_faces[1*8+j]].temperature[FACE_LAYER_HULL];
								for (j = 0; j < faces[i].boundary_num_faces3; j++)
									mag3 += faces[faces[i].boundary_faces[2*8+j]].temperature[FACE_LAYER_HULL];
							}
						break;
						case DRAGHEAT_VISUALMODE_DYNAMIC_PRESSURE:
							mag_min = 0.0; mag_max = dragheat_max_Q;
							mag3 = mag2 = mag1 = faces[i].Q;
							if (dragheat_visual_interpolate) {
								for (j = 0; j < faces[i].boundary_num_faces1; j++)
									mag1 += faces[faces[i].boundary_faces[0*8+j]].Q;
								for (j = 0; j < faces[i].boundary_num_faces2; j++)
									mag2 += faces[faces[i].boundary_faces[1*8+j]].Q;
								for (j = 0; j < faces[i].boundary_num_faces3; j++)
									mag3 += faces[faces[i].boundary_faces[2*8+j]].Q;
							}
						break;
						case DRAGHEAT_VISUALMODE_SHOCKWAVES:
							mag_min = 0.0; mag_max = 1.0;
							mag3 = mag2 = mag1 = faces[i].shockwaves;
							if (dragheat_visual_interpolate) {
								for (j = 0; j < faces[i].boundary_num_faces1; j++)
									mag1 += faces[faces[i].boundary_faces[0*8+j]].shockwaves;
								for (j = 0; j < faces[i].boundary_num_faces2; j++)
									mag2 += faces[faces[i].boundary_faces[1*8+j]].shockwaves;
								for (j = 0; j < faces[i].boundary_num_faces3; j++)
									mag3 += faces[faces[i].boundary_faces[2*8+j]].shockwaves;
							}
						break;
						default:
							mag3 = mag2 = mag1 = 0;
					}

					//Average it out
					if (dragheat_visual_interpolate) {
						mag1 = mag1 / (1.0*v->geometry.faces[i].boundary_num_faces1+1.0);
						mag2 = mag2 / (1.0*v->geometry.faces[i].boundary_num_faces2+1.0);
						mag3 = mag3 / (1.0*v->geometry.faces[i].boundary_num_faces3+1.0);
					}

					//Convert magnitudes to 0..1 range
					mag1 = max(0,min(0.99,(mag1-mag_min)/(mag_max-mag_min)));
					mag2 = max(0,min(0.99,(mag2-mag_min)/(mag_max-mag_min)));
					mag3 = max(0,min(0.99,(mag3-mag_min)/(mag_max-mag_min)));

					i1 = (int)(mag1*5); i2 = (int)(mag2*5); i3 = (int)(mag3*5);
					t1 = mag1*5-i1; t2 = mag2*5-i2; t3 = mag3*5-i3;

					//Generate colors
					r1 = dragheat_pallette[(i1)*3+0]*(1-t1) + t1*dragheat_pallette[(i1+1)*3+0];
					g1 = dragheat_pallette[(i1)*3+1]*(1-t1) + t1*dragheat_pallette[(i1+1)*3+1];
					b1 = dragheat_pallette[(i1)*3+2]*(1-t1) + t1*dragheat_pallette[(i1+1)*3+2];
					r2 = dragheat_pallette[(i2)*3+0]*(1-t2) + t2*dragheat_pallette[(i2+1)*3+0];
					g2 = dragheat_pallette[(i2)*3+1]*(1-t2) + t2*dragheat_pallette[(i2+1)*3+1];
					b2 = dragheat_pallette[(i2)*3+2]*(1-t2) + t2*dragheat_pallette[(i2+1)*3+2];
					r3 = dragheat_pallette[(i3)*3+0]*(1-t3) + t3*dragheat_pallette[(i3+1)*3+0];
					g3 = dragheat_pallette[(i3)*3+1]*(1-t3) + t3*dragheat_pallette[(i3+1)*3+1];
					b3 = dragheat_pallette[(i3)*3+2]*(1-t3) + t3*dragheat_pallette[(i3+1)*3+2];

					//Draw
					glColor4d(r2,g2,b2,1.0);
					glVertex3d(-faces[i].y[1],faces[i].z[1],faces[i].x[1]);
					glColor4d(r1,g1,b1,1.0);
					glVertex3d(-faces[i].y[0],faces[i].z[0],faces[i].x[0]);
					glColor4d(r3,g3,b3,1.0);
					glVertex3d(-faces[i].y[2],faces[i].z[2],faces[i].x[2]);
				}
				break;
			case DRAGHEAT_VISUALMODE_FEM:
			case DRAGHEAT_VISUALMODE_MATERIAL:
				if (dragheat_visual_mode == DRAGHEAT_VISUALMODE_MATERIAL) {
					int r = ((faces[i].m+1) / 1) % 2;
					int g = ((faces[i].m+1) / 2) % 2;
					int b = ((faces[i].m+1) / 4) % 2;
					if ((dragheat_selected_vessel == v->index) &&
						(dragheat_selected_triangle == i)) {
						glColor4d(0.3*r,0.3*g,0.3*b,0.7);
					} else {
						glColor4d(1.0*r,1.0*g,1.0*b,0.7);
					}

					if ((dragheat_selected_vessel == v->index) &&
						(dragheat_selected_triangle > 0)) {
							if (faces[dragheat_selected_triangle].boundary_edge[0] == i) glColor4d(r,0.3*g,0.3*b,0.7);
							if (faces[dragheat_selected_triangle].boundary_edge[1] == i) glColor4d(0.3*r,g,0.3*b,0.7);
							if (faces[dragheat_selected_triangle].boundary_edge[2] == i) glColor4d(0.3*r,0.3*g,b,0.7);
					}
				} else {
					if ((dragheat_selected_vessel == v->index) &&
						(dragheat_selected_triangle == i)) {
						glColor4d(0.0,0.5*faces[i].creates_drag,0.5*(!faces[i].creates_drag),1.0);
					} else {
						glColor4d(0.0,1.0*faces[i].creates_drag,1.0*(!faces[i].creates_drag),1.0);
					}
				}

				//Translation/offset
				tx = v->geometry.dx;//+0.00*faces[i].nx;
				ty = v->geometry.dy;//+0.00*faces[i].ny;
				tz = v->geometry.dz;//+0.00*faces[i].nz;

				//Draw faces
				//glNormal3d(-faces[i].vny[1],faces[i].vnz[1],faces[i].vnx[1]);
				glNormal3d(-faces[i].ny,faces[i].nz,faces[i].nx);
				glVertex3d(-faces[i].y[1]-ty,faces[i].z[1]+tz,faces[i].x[1]+tx);
				//glNormal3d(-faces[i].vny[0],faces[i].vnz[0],faces[i].vnx[0]);
				glNormal3d(-faces[i].ny,faces[i].nz,faces[i].nx);
				glVertex3d(-faces[i].y[0]-ty,faces[i].z[0]+tz,faces[i].x[0]+tx);
				//glNormal3d(-faces[i].vny[2],faces[i].vnz[2],faces[i].vnx[2]);
				glNormal3d(-faces[i].ny,faces[i].nz,faces[i].nx);
				glVertex3d(-faces[i].y[2]-ty,faces[i].z[2]+tz,faces[i].x[2]+tx);

				//Draw thickness
				if (dragheat_visual_mode == DRAGHEAT_VISUALMODE_MATERIAL) {
					glColor4d(0.0,0.0,0.0,0.25);
					tx = v->geometry.dx+faces[i].thickness*10.0*faces[i].nx;
					ty = v->geometry.dy+faces[i].thickness*10.0*faces[i].ny;
					tz = v->geometry.dz+faces[i].thickness*10.0*faces[i].nz;
					glNormal3d(-faces[i].ny,faces[i].nz,faces[i].nx);
					glVertex3d(-faces[i].y[1]-ty,faces[i].z[1]+tz,faces[i].x[1]+tx);
					glNormal3d(-faces[i].ny,faces[i].nz,faces[i].nx);
					glVertex3d(-faces[i].y[0]-ty,faces[i].z[0]+tz,faces[i].x[0]+tx);
					glNormal3d(-faces[i].ny,faces[i].nz,faces[i].nx);
					glVertex3d(-faces[i].y[2]-ty,faces[i].z[2]+tz,faces[i].x[2]+tx);
				}
				break;
			default:
				break;
		}
	}
	glEnd();

	//Draw glow
	if ((config.reentry_glow) && (dragheat_visual_mode == DRAGHEAT_VISUALMODE_NORMAL)) {
		float max_size;
		float abc[] = { 1.0f, 0.0f, 0.000007f };
		double vx,vy,vz,vmag;

		//Setup rendering additive glow
		XPLMSetGraphicsState(0, 1, 0, 0, 1, 1, 0);
		XPLMBindTexture2d(particles_glow_texture,0);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		glEnable(GL_POINT_SPRITE);
		glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
		glGetFloatv(GL_POINT_SIZE_MAX_ARB,&max_size);
		if (max_size > 256.0f) max_size = 256.0f;
		glPointSize(max_size);
		glPointParameterf(GL_POINT_SIZE_MIN, 1.0f);
		glPointParameterf(GL_POINT_SIZE_MAX, max_size);
		glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION,abc);

		//Draw glow
		glBegin(GL_POINTS);
		for (i = 0; i < v->geometry.num_faces; i++) {
			if (faces[i].area > 0.12) {
				double t = faces[i].surface_temperature;
				if (t > 1400) {
					double d = 2.00*faces[i].area;
					if (d > 1.0) d = 1.0;

					glColor4f(1.0f,1.0f,0.6f,0.03f*(float)((t-1400)/1000));
					glVertex3d(
						-faces[i].ay-d*faces[i].ny,
						 faces[i].az+d*faces[i].nz,
						 faces[i].ax+d*faces[i].nx);
				}
			}
		}
		glEnd();
		glDisable(GL_POINT_SPRITE);
		glPointSize(1);

		//Compute velocity
		vmag = -v->geometry.vmag;
		vx = -v->geometry.vx;
		vy = -v->geometry.vy;
		vz = -v->geometry.vz;

		//Draw plasma trail
		if (config.use_shaders && dragheat_fbo && (fabs(vmag) > 3000))
		{
			double area_est,heat_flux;
			int viewport[4],msize;
			XPLMSetGraphicsState(0,0,0,0,1,1,1);
			//glBlendFunc(GL_SRC_ALPHA,GL_ONE);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_CULL_FACE);

			//Copy depth buffer to FBO
			glFinish();
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dragheat_fbo);
			glBlitFramebuffer(0,0,dragheat_fbo_width-1,dragheat_fbo_height-1,
							  0,0,dragheat_fbo_width-1,dragheat_fbo_height-1,
							  GL_DEPTH_BUFFER_BIT, GL_NEAREST);

			//Bind FBO
			glBindFramebuffer(GL_FRAMEBUFFER, dragheat_fbo);
			glClearColor(0.0f,0.0f,0.0f,1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			XPLMSetGraphicsState(0,0,0,0,1,0,0); //FIXME: had to disable z testing??

			//Estimate size
			area_est = 0.8;//v->geometry.total_area/250;//150;

			//Estimate heat flux
			heat_flux = - 1e-6 * v->air.density * pow(vmag,3);

			glUseProgram(dragheat_reentry_plasma_program);
			glActiveTexture(GL_TEXTURE0);
			XPLMBindTexture2d(particles_perlin_perm_texture,0);
			if (dragheat_reentry_plasma_perlin_perm_location != -1) glUniform1i(dragheat_reentry_plasma_perlin_perm_location, 0);
			if (dragheat_reentry_plasma_time_location != -1) glUniform1f(dragheat_reentry_plasma_time_location, (float)curtime());
			if (dragheat_reentry_plasma_area_location != -1) glUniform1f(dragheat_reentry_plasma_area_location, (float)area_est);
			if (dragheat_reentry_plasma_mach_location != -1) glUniform1f(dragheat_reentry_plasma_mach_location, (float)v->geometry.effective_M);
			if (dragheat_reentry_plasma_flux_location != -1) glUniform1f(dragheat_reentry_plasma_flux_location, (float)heat_flux);

			glBegin(GL_TRIANGLES);
			for (i = 0; i < v->geometry.num_faces; i++) {
				double dot = faces[i]._dot;
				double dot0 = 1.0;
				double dot1 = 1.0;
				double dot2 = 1.0;

				if (faces[i].boundary_edge[0] >= 0) dot0 = faces[faces[i].boundary_edge[0]]._dot;
				if (faces[i].boundary_edge[1] >= 0) dot1 = faces[faces[i].boundary_edge[1]]._dot;
				if (faces[i].boundary_edge[2] >= 0) dot2 = faces[faces[i].boundary_edge[2]]._dot;

				if (dot > 0.0) {
					//Get coordinates
					double boundary = 1.5;
					//for (boundary = 1.5-0.3; boundary <= 1.5+0.3; boundary += 0.3) {
					{
						double x_s[3] = {
							-faces[i].y[0]-faces[i].vny[0]*boundary,
							-faces[i].y[1]-faces[i].vny[1]*boundary,
							-faces[i].y[2]-faces[i].vny[2]*boundary};
						double y_s[3] = {
							 faces[i].z[0]+faces[i].vnz[0]*boundary,
							 faces[i].z[1]+faces[i].vnz[1]*boundary,
							 faces[i].z[2]+faces[i].vnz[2]*boundary};
						double z_s[3] = {
							 faces[i].x[0]+faces[i].vnx[0]*boundary,
							 faces[i].x[1]+faces[i].vnx[1]*boundary,
							 faces[i].x[2]+faces[i].vnx[2]*boundary};

						double x_e[3] = {
							-faces[i].y[0]-faces[i].vny[0]*2.5*boundary-vy*100*area_est,
							-faces[i].y[1]-faces[i].vny[1]*2.5*boundary-vy*100*area_est,
							-faces[i].y[2]-faces[i].vny[2]*2.5*boundary-vy*100*area_est};
						double y_e[3] = {  
							 faces[i].z[0]+faces[i].vnz[0]*2.5*boundary+vz*100*area_est,
							 faces[i].z[1]+faces[i].vnz[1]*2.5*boundary+vz*100*area_est,
							 faces[i].z[2]+faces[i].vnz[2]*2.5*boundary+vz*100*area_est};
						double z_e[3] = {  
							 faces[i].x[0]+faces[i].vnx[0]*2.5*boundary+vx*100*area_est,
							 faces[i].x[1]+faces[i].vnx[1]*2.5*boundary+vx*100*area_est,
							 faces[i].x[2]+faces[i].vnx[2]*2.5*boundary+vx*100*area_est};

						//Light cap
						glNormal3d(-vy,vz,vx);
						glVertex3d(x_s[1],y_s[1],z_s[1]);
						glVertex3d(x_s[0],y_s[0],z_s[0]);
						glVertex3d(x_s[2],y_s[2],z_s[2]);

						//Edge 1
						if (dot0 <= 0.0) {
							glNormal3d(-vy,vz,vx);
							glVertex3d(x_s[1],y_s[1],z_s[1]);
							glVertex3d(x_e[0],y_e[0],z_e[0]);
							glVertex3d(x_s[0],y_s[0],z_s[0]);

							glNormal3d(-vy,vz,vx);
							glVertex3d(x_s[1],y_s[1],z_s[1]);
							glVertex3d(x_e[1],y_e[1],z_e[1]);
							glVertex3d(x_e[0],y_e[0],z_e[0]);
						}

						//Edge 2
						if (dot1 <= 0.0) {
							glNormal3d(-vy,vz,vx);
							glVertex3d(x_s[2],y_s[2],z_s[2]);
							glVertex3d(x_e[1],y_e[1],z_e[1]);
							glVertex3d(x_s[1],y_s[1],z_s[1]);

							glNormal3d(-vy,vz,vx);
							glVertex3d(x_s[2],y_s[2],z_s[2]);
							glVertex3d(x_e[2],y_e[2],z_e[2]);
							glVertex3d(x_e[1],y_e[1],z_e[1]);
						}

						//Edge 3
						if (dot2 <= 0.0) {
							glNormal3d(-vy,vz,vx);
							glVertex3d(x_s[0],y_s[0],z_s[0]);
							glVertex3d(x_e[2],y_e[2],z_e[2]);
							glVertex3d(x_s[2],y_s[2],z_s[2]);

							glNormal3d(-vy,vz,vx);
							glVertex3d(x_s[0],y_s[0],z_s[0]);
							glVertex3d(x_e[0],y_e[0],z_e[0]);
							glVertex3d(x_e[2],y_e[2],z_e[2]);
						}
					}
				}
			}
			glEnd();
			glUseProgram(0);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);

			//Unbind FBO
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			//Clear the target FBO/4
			//glBindFramebuffer(GL_FRAMEBUFFER, dragheat_fbo2);
			//glClearColor(0.0f,0.0f,0.0f,1.0f);
			//glClear(GL_COLOR_BUFFER_BIT);

			//Copy data to a smaller FBO of half size
			/*glBindFramebuffer(GL_READ_FRAMEBUFFER, dragheat_fbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dragheat_fbo2);
			glBlitFramebuffer(0,0,dragheat_fbo_width-1,dragheat_fbo_height-1,
							  0,0,dragheat_fbo_width/4-1,dragheat_fbo_height/4-1,
							  GL_COLOR_BUFFER_BIT, GL_LINEAR);
			glBindFramebuffer(GL_FRAMEBUFFER, 0); FIXME */


			//Draw FBO on screen
			XPLMSetGraphicsState(0,1,0,0,1,0,0);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);

			//2D rendering
			glGetIntegerv(GL_VIEWPORT, viewport);
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();

			glOrtho(0, viewport[2], 0, viewport[3], -1, 1);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();

			//Use postprocessing shader
			glUseProgram(dragheat_reentry_pp_program);
			glActiveTexture(GL_TEXTURE0);
			XPLMBindTexture2d(dragheat_fbo_texture,0);
			if (dragheat_reentry_pp_framebuffer_location != -1) glUniform1i(dragheat_reentry_pp_framebuffer_location, 0);
			glActiveTexture(GL_TEXTURE1);
			XPLMBindTexture2d(dragheat_fbo_depth,1);
			if (dragheat_reentry_pp_zbuffer_location != -1) glUniform1i(dragheat_reentry_pp_zbuffer_location, 1);

			XPLMGetScreenSize(&viewport[2],&viewport[3]);
			//msize = viewport[2];
			//if (viewport[3] > msize) msize = viewport[3];

			//XPLMBindTexture2d(dragheat_fbo_texture,0);
			glBegin(GL_QUADS);
			//glColor3d(4.0/msize,1.0,0.0);
			glTexCoord2d(0.0,0.0);
			glVertex2d(0,0);
			glTexCoord2d(0.0,1.0);
			glVertex2d(0,viewport[3]);
			glTexCoord2d(1.0,1.0);
			glVertex2d(viewport[2],viewport[3]);
			glTexCoord2d(1.0,0.0);
			glVertex2d(viewport[2],0.0);
			glEnd();

			glUseProgram(0);

			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();

			//Create reentry trail particles
			/*if (!dragheat_simulation.enabled) {
				if ((-vmag > 4000) && (heat_flux > 0.05)) {
					particles_spawn(v->sim.x,v->sim.y,v->sim.z,20.0,(heat_flux/30)*((-vmag-4000)/3500),1);
				}
			}*/
		}
	}

	//Draw sensors
	if (config.sensor_debug_draw) {
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		XPLMSetGraphicsState(0,0,0,0,0,1,0);
		glBegin(GL_LINES);
		for (i = 0; i < v->sensors.surface_count; i++) {
			int face = v->sensors.surface[i].face;
			glColor3f(1.0f,0.0f,0.0f);
			glVertex3d(-v->sensors.surface[i].y,v->sensors.surface[i].z,v->sensors.surface[i].x);
			glColor3f(0.0f,1.0f,0.0f);
			glVertex3d(-v->geometry.faces[face].ay,v->geometry.faces[face].az,v->geometry.faces[face].ax);
		}
		glEnd();

		glPointSize(8.0f);
		glBegin(GL_POINTS);
		for (i = 0; i < v->sensors.surface_count; i++) {
			int face = v->sensors.surface[i].face;

			glColor3f(1.0f,0.0f,0.0f);
			glVertex3d(-v->sensors.surface[i].y,v->sensors.surface[i].z,v->sensors.surface[i].x);
			glColor3f(0.0f,1.0f,0.0f);
			glVertex3d(-v->geometry.faces[face].ay,v->geometry.faces[face].az,v->geometry.faces[face].ax);
		}
		glEnd();
	}

	if (dragheat_visual_mode == DRAGHEAT_VISUALMODE_SHOCKWAVES) {
		double vx,vy,vz,vmag; //FIXME: this velocity stuff is a too big hack
		double mach_sin = 0.3;

		vmag = v->geometry.vmag;
		vx = v->geometry.vx;
		vy = v->geometry.vy;
		vz = v->geometry.vz;

		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		XPLMSetGraphicsState(0,0,0,0,1,1,0);
		glBegin(GL_LINES);
		for (i = 0; i < v->geometry.num_faces; i++) {
			if (faces[i].creates_shockwave) {
				double w;
				for (w = 0.0; w < 2*PI; w += 2*PI/2.0) {
					double dx = -20*vx;
					double dy = -20*vy+20*mach_sin*sin(w);
					double dz = -20*vz+20*mach_sin*cos(w);

					glColor4f(1.0f,1.0f,1.0f,0.5f);
					glVertex3d(-faces[i].ay,faces[i].az,faces[i].ax);
					glColor4f(1.0f,1.0f,1.0f,0.0f);
					glVertex3d(-faces[i].ay-dy,faces[i].az+dz,faces[i].ax+dx);
				}
			}
		}
		glEnd();
	}

	//Restore state
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPopMatrix();
}
#endif





//==============================================================================
// Append to drag model from an obj file (V7)
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
typedef struct dragheat_obj_triangle_tag {
	float x[3];
	float y[3];
	float z[3];
	int creates_drag;
} dragheat_obj_triangle;

dragheat_obj_triangle* dragheat_obj_triangles;
int dragheat_num_triangles;

void dragheat_model_loadobj_file(char* filename, int creates_drag)
{
	int num_tris,tri_offset;
	char buf[ARBITRARY_MAX] = { 0 };

	FILE* f = fopen(filename,"r");
	if (!f) return;

	//Count triangles
	num_tris = 0;
	while (f && (!feof(f))) {
		char tag[256];
		tag[0] = 0; fscanf(f,"%255s",tag); tag[255] = 0;
		if (strcmp(tag,"tri_strip") == 0) { //Vertex
			int num_verts;
			fscanf(f,"%d",&num_verts);
			fgets(buf,ARBITRARY_MAX-1,f);

			num_tris += num_verts-2;
		} else {
			fgets(buf,ARBITRARY_MAX-1,f);
		}
	}

	//Allocate space for extra triangles
	tri_offset = dragheat_num_triangles;
	dragheat_num_triangles += num_tris;
	dragheat_obj_triangles = realloc(dragheat_obj_triangles,dragheat_num_triangles*sizeof(dragheat_obj_triangle));

	//Read triangles
	fseek(f,0,SEEK_SET);
	while (f && (!feof(f))) {
		char tag[256];
		tag[0] = 0; fscanf(f,"%255s",tag); tag[255] = 0;
		if (strcmp(tag,"tri_strip") == 0) { //Vertex
			int num_verts,i;
			float x0,y0,z0,u,v;
			float x1,y1,z1;
			float x2,y2,z2;
			fscanf(f,"%d",&num_verts);
			fgets(buf,ARBITRARY_MAX-1,f);

			fscanf(f,"%f %f %f %f %f",&x0,&y0,&z0,&u,&v);
			fgets(buf,ARBITRARY_MAX-1,f);
			fscanf(f,"%f %f %f %f %f",&x1,&y1,&z1,&u,&v);
			fgets(buf,ARBITRARY_MAX-1,f);

			for (i=1; i<num_verts-1; i++) {
				fscanf(f,"%f %f %f %f %f",&x2,&y2,&z2,&u,&v);
				fgets(buf,ARBITRARY_MAX-1,f);

				if (i % 2 == 0) {
					dragheat_obj_triangles[tri_offset].x[0] = x1;
					dragheat_obj_triangles[tri_offset].x[1] = x0;
					dragheat_obj_triangles[tri_offset].x[2] = x2;
					dragheat_obj_triangles[tri_offset].y[0] = y1;
					dragheat_obj_triangles[tri_offset].y[1] = y0;
					dragheat_obj_triangles[tri_offset].y[2] = y2;
					dragheat_obj_triangles[tri_offset].z[0] = z1;
					dragheat_obj_triangles[tri_offset].z[1] = z0;
					dragheat_obj_triangles[tri_offset].z[2] = z2;
				} else {
					dragheat_obj_triangles[tri_offset].x[0] = x0;
					dragheat_obj_triangles[tri_offset].x[1] = x1;
					dragheat_obj_triangles[tri_offset].x[2] = x2;
					dragheat_obj_triangles[tri_offset].y[0] = y0;
					dragheat_obj_triangles[tri_offset].y[1] = y1;
					dragheat_obj_triangles[tri_offset].y[2] = y2;
					dragheat_obj_triangles[tri_offset].z[0] = z0;
					dragheat_obj_triangles[tri_offset].z[1] = z1;
					dragheat_obj_triangles[tri_offset].z[2] = z2;
				}
				dragheat_obj_triangles[tri_offset].creates_drag = creates_drag;
				tri_offset++;

				x0 = x1; y0 = y1; z0 = z1;
				x1 = x2; y1 = y2; z1 = z2;
			}
		} else {
			fgets(buf,ARBITRARY_MAX-1,f);
		}
	}
	fclose(f);
}
#endif


//==============================================================================
// Load drag model from an OBJ file
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
int dragheat_model_loadobj(vessel* v)
{
	char model[MAX_FILENAME] = { 0 }, filename[MAX_FILENAME] = { 0 };
	face* faces;
	int i;

	//Search if a same plane exists, and its already with a model
	if (v->is_plane) {
		for (i = 0; i < vessel_count; i++) {
			if ((vessels[i].index != v->index) &&
				(vessels[i].is_plane) && 
				(strncmp(vessels[i].plane_filename,v->plane_filename,1024) == 0) &&
				(vessels[i].geometry.faces)) {
				dragheat_deinitialize(v); //Deinitialize this vessel anyway
				return 1;
			}
		}
	}

	//Read aircraft path and path separator
	if (v->index == 0) {
		XPLMGetNthAircraftModel(0, model, filename);
	} else if (v->is_plane) {
		XPLMGetNthAircraftModel(v->plane_index+1, model, filename);
	} else {
		return 0;
	}
	if (strlen(model) > 4) model[strlen(model)-4] = 0;
#ifdef APL
	{
		char* tpath;
		while (tpath = strchr(path,':')) *tpath = '/';
		strncpy(model,path,MAX_FILENAME-1);
		strncpy(path,"/Volumes/",MAX_FILENAME-1);
		strncat(path,model,MAX_FILENAME-1);
	}
#endif

	//Initialize resources
	dragheat_num_triangles = 0;
	dragheat_obj_triangles = malloc(1*sizeof(dragheat_obj_triangle));

	//Load all possible OBJ files
	snprintf(filename,MAX_FILENAME-1,"./%s_folder/%s_FUSELAGE.obj",model,model);
	dragheat_model_loadobj_file(filename,1);
	snprintf(filename,MAX_FILENAME-1,"./%s_folder/%s_LEFT H STAB.obj",model,model);
	dragheat_model_loadobj_file(filename,0);
	snprintf(filename,MAX_FILENAME-1,"./%s_folder/%s_RIGT H STAB.obj",model,model);
	dragheat_model_loadobj_file(filename,0);
	for (i = 1; i <= 2; i++) {
		snprintf(filename,MAX_FILENAME-1,"./%s_folder/%s_VERT STAB %d.obj",model,model,i);
		dragheat_model_loadobj_file(filename,0);
	}
	for (i = 1; i <= 4; i++) {
		snprintf(filename,MAX_FILENAME-1,"./%s_folder/%s_LEFT WING %d.obj",model,model,i);
		dragheat_model_loadobj_file(filename,0);
		snprintf(filename,MAX_FILENAME-1,"./%s_folder/%s_RIGT WING %d.obj",model,model,i);
		dragheat_model_loadobj_file(filename,0);
	}

	for (i = 1; i <= 20; i++) {
		snprintf(filename,MAX_FILENAME-1,"./%s_folder/%s_MISC WING %d.obj",model,model,i);
		dragheat_model_loadobj_file(filename,0);
	}

	//Subdivide big triangles (Catmull-Clark)
	{
		extern void subdivide_dragheat_catmull_clark();
		subdivide_dragheat_catmull_clark();
	}

	//Load all triangles as new geometric model
	dragheat_deinitialize(v);
	v->geometry.invalid = 1;
	v->geometry.num_faces = dragheat_num_triangles;
	v->geometry.faces = (face*)malloc(sizeof(face)*dragheat_num_triangles);
	v->geometry.Cd = 1.0;
	v->geometry.K = 1.0;
	v->geometry.trqx = 1.0;
	v->geometry.trqy = 1.0;
	v->geometry.trqz = 1.0;
	v->geometry.hull = material_get("Aluminium");

	faces = v->geometry.faces;
	for (i = 0; i < dragheat_num_triangles; i++) {
		double ax,ay,az;
		double bx,by,bz;

		//Copy coordinates
		faces[i].y[0] = dragheat_obj_triangles[i].x[0];
		faces[i].y[1] = dragheat_obj_triangles[i].x[1];
		faces[i].y[2] = dragheat_obj_triangles[i].x[2];
		faces[i].z[0] = dragheat_obj_triangles[i].y[0];
		faces[i].z[1] = dragheat_obj_triangles[i].y[1];
		faces[i].z[2] = dragheat_obj_triangles[i].y[2];
		faces[i].x[0] = dragheat_obj_triangles[i].z[0];
		faces[i].x[1] = dragheat_obj_triangles[i].z[1];
		faces[i].x[2] = dragheat_obj_triangles[i].z[2];

		//Calculate normal (bad length for now)
		ax = faces[i].x[0] - faces[i].x[1];
		ay = faces[i].y[0] - faces[i].y[1];
		az = faces[i].z[0] - faces[i].z[1];
		bx = faces[i].x[2] - faces[i].x[1];
		by = faces[i].y[2] - faces[i].y[1];
		bz = faces[i].z[2] - faces[i].z[1];

		faces[i].nx = ay*bz-az*by;
		faces[i].ny = az*bx-ax*bz;
		faces[i].nz = ax*by-ay*bx;

		//Aerodynamic center
		faces[i].ax = (faces[i].x[0]+faces[i].x[1]+faces[i].x[2])/3.0;
		faces[i].ay = (faces[i].y[0]+faces[i].y[1]+faces[i].y[2])/3.0;
		faces[i].az = (faces[i].z[0]+faces[i].z[1]+faces[i].z[2])/3.0;

		//Area and fix normal lengths
		faces[i].area = 0.5*sqrt(faces[i].nx*faces[i].nx+faces[i].ny*faces[i].ny+faces[i].nz*faces[i].nz);
		if (faces[i].area < 0.01) { //Ignore faces with too small area
			faces[i].area = 0.00;
		}
		if (faces[i].area > 0) {
			faces[i].nx = (ay*bz-az*by)/(2.0*faces[i].area);
			faces[i].ny = (az*bx-ax*bz)/(2.0*faces[i].area);
			faces[i].nz = (ax*by-ay*bx)/(2.0*faces[i].area);
		}

		//Material
		faces[i].m = material_get("Aluminium");
		faces[i].creates_drag = dragheat_obj_triangles[i].creates_drag;

		//Default values
		faces[i].thickness = 0.03;
		faces[i].heat_flux = 0;
		faces[i].temperature[0] = 273.15;
		faces[i].temperature[1] = 273.15;

		//No boundary faces
		faces[i].boundary_num_faces1 = 0;
		faces[i].boundary_num_faces2 = 0;
		faces[i].boundary_num_faces3 = 0;
	}

	/*subdivided = 1;
	while (subdivided) {
		subdivided = 0;
		for (i = 0; i < v->geometry.num_faces; i++) {
			if (faces[i].area > 0.5) {
				int j,k;
				j = v->geometry.num_faces;
				k = v->geometry.num_faces+1;
				v->geometry.num_faces += 2;
				faces = (face*)realloc(faces,sizeof(face)*v->geometry.num_faces);

				subdivided = 1;

				log_write("subdivided %d into %d, %d, %d\n",i,i,j,k);

				//      1
				//   i     j
				//      a
				// 0    k    2
				//Subdivide
				faces[j].x[0] = faces[i].ax;
				faces[j].x[1] = faces[i].x[1];
				faces[j].x[2] = faces[i].x[2];

				faces[k].x[0] = faces[i].x[0];
				faces[k].x[1] = faces[i].ax;
				faces[k].x[2] = faces[i].x[2];

				faces[i].x[2] = faces[i].ax;


				faces[j].y[0] = faces[i].ay;
				faces[j].y[1] = faces[i].y[1];
				faces[j].y[2] = faces[i].y[2];

				faces[k].y[0] = faces[i].y[0];
				faces[k].y[1] = faces[i].ay;
				faces[k].y[2] = faces[i].y[2];

				faces[i].y[2] = faces[i].ay;


				faces[j].z[0] = faces[i].az;
				faces[j].z[1] = faces[i].z[1];
				faces[j].z[2] = faces[i].z[2];

				faces[k].z[0] = faces[i].z[0];
				faces[k].z[1] = faces[i].az;
				faces[k].z[2] = faces[i].z[2];

				faces[i].z[2] = faces[i].az;

				//Set other parameters
				faces[j].nx = faces[i].nx;
				faces[j].ny = faces[i].ny;
				faces[j].nz = faces[i].nz;
				faces[k].nx = faces[i].nx;
				faces[k].ny = faces[i].ny;
				faces[k].nz = faces[i].nz;

				//Aerodynamic center
				faces[i].ax = (faces[i].x[0]+faces[i].x[1]+faces[i].x[2])/3.0;
				faces[i].ay = (faces[i].y[0]+faces[i].y[1]+faces[i].y[2])/3.0;
				faces[i].az = (faces[i].z[0]+faces[i].z[1]+faces[i].z[2])/3.0;
				faces[j].ax = (faces[j].x[0]+faces[j].x[1]+faces[j].x[2])/3.0;
				faces[j].ay = (faces[j].y[0]+faces[j].y[1]+faces[j].y[2])/3.0;
				faces[j].az = (faces[j].z[0]+faces[j].z[1]+faces[j].z[2])/3.0;
				faces[k].ax = (faces[k].x[0]+faces[k].x[1]+faces[k].x[2])/3.0;
				faces[k].ay = (faces[k].y[0]+faces[k].y[1]+faces[k].y[2])/3.0;
				faces[k].az = (faces[k].z[0]+faces[k].z[1]+faces[k].z[2])/3.0;

				faces[i].area = faces[i].area/3.0;
				faces[j].area = faces[i].area;
				faces[k].area = faces[i].area;

				faces[j].m = faces[i].m;
				faces[k].m = faces[i].m;
				faces[j].creates_drag = faces[i].creates_drag;
				faces[k].creates_drag = faces[i].creates_drag;

				//Default values
				faces[j].thickness = faces[i].thickness;
				faces[j].heat_flux = 0;
				faces[j].temperature[0] = 273.15;
				faces[j].temperature[1] = 273.15;
				faces[k].thickness = faces[i].thickness;
				faces[k].heat_flux = 0;
				faces[k].temperature[0] = 273.15;
				faces[k].temperature[1] = 273.15;

				//No boundary faces
				faces[j].boundary_num_faces1 = 0;
				faces[j].boundary_num_faces2 = 0;
				faces[j].boundary_num_faces3 = 0;
				faces[k].boundary_num_faces1 = 0;
				faces[k].boundary_num_faces2 = 0;
				faces[k].boundary_num_faces3 = 0;

				break;
			}
		}
	}
	v->geometry.faces = faces;*/

	//Free resources
	free(dragheat_obj_triangles);
	return 1;
}
#endif


//==============================================================================
// Save drag model
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
int dragheat_model_save(vessel* v)
{
	char model[MAX_FILENAME] = { 0 }, filename[MAX_FILENAME] = { 0 };
	face* faces;
	FILE* f;
	int i;
	double dx,dy,dz;
	char* acfpath;

	//See if vessel has geometry at all
	if ((!v->geometry.faces) || (v->geometry.num_faces == 0)) return 0;

	//Read aircraft path and path separator
	if (v->index == 0) {
		XPLMGetNthAircraftModel(0, model, filename);
	} else if (v->is_plane) {
		XPLMGetNthAircraftModel(v->plane_index+1, model, filename);
	} else {
		return 0;
	}
	if (strlen(model) > 4) model[strlen(model)-4] = 0;
#ifdef APL
	{
		char* tpath;
		while (tpath = strchr(path,':')) *tpath = '/';
		strncpy(model,path,MAX_FILENAME-1);
		strncpy(path,"/Volumes/",MAX_FILENAME-1);
		strncat(path,model,MAX_FILENAME-1);
	}
#endif

	//Fetch only aircraft model name
	acfpath = strstr(model,".acf");
	if (acfpath) *acfpath = 0;

	//Save drag model
	XPLMExtractFileAndPath(filename);
	strcat(filename,"/");
	strcat(filename,model);
	strcat(filename,"_drag.dat");
	f = fopen(filename,"w+");
	if (!f) return 0;

	fprintf(f,"# Drag model (version 3) for %s\n",model);
	fprintf(f,"\n");
	fprintf(f,"Cd %f\n",v->geometry.Cd);
	fprintf(f,"K %f\n",v->geometry.K);
	fprintf(f,"HULL %s\n",materials[v->geometry.hull].name);
	fprintf(f,"TRQ 1.0 1.0 1.0\n");
	fprintf(f,"SHOCKWAVES 0\n");

	faces = v->geometry.faces;
	dx = v->geometry.dx;
	dy = v->geometry.dy;
	dz = v->geometry.dz;
	for (i = 0; i < v->geometry.num_faces; i++) {
		if (faces[i].area > 0.0) {
			fprintf(f,"TRI_MAT3 %f %f %f  %f %f %f  %f %f %f  %f %f %f  %f %f %d %s\n",
			        faces[i].x[0]+dx,faces[i].y[0]+dy,faces[i].z[0]+dz,
			        faces[i].x[1]+dx,faces[i].y[1]+dy,faces[i].z[1]+dz,
			        faces[i].x[2]+dx,faces[i].y[2]+dy,faces[i].z[2]+dz,
			        faces[i].nx,faces[i].ny,faces[i].nz,
			        faces[i].area,
			        faces[i].thickness,
			        faces[i].creates_drag,
			        materials[faces[i].m].name);
		}
	}

	fclose(f);
	return 1;
}
#endif




//==============================================================================
// Highlevel drag/heating interface
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
int dragheat_highlevel_loadobj(lua_State* L)
{
	vessel* v = 0;
	int idx = luaL_checkint(L,1);
	if ((idx >= 0) && (idx < vessel_count)) {
		v = &vessels[idx];
	}

	if (v) {
		lua_pushboolean(L,dragheat_model_loadobj(v));
	} else {
		lua_pushboolean(L,0);
	}
	return 1;
}

int dragheat_highlevel_savemodel(lua_State* L)
{
	vessel* v = 0;
	int idx = luaL_checkint(L,1);
	if ((idx >= 0) && (idx < vessel_count)) {
		v = &vessels[idx];
	}

	if (v) {
		lua_pushboolean(L,dragheat_model_save(v));
	} else {
		lua_pushboolean(L,0);
	}
	return 1;
}

int dragheat_highlevel_reloadmodel(lua_State* L)
{
	vessel* v = 0;
	int idx = luaL_checkint(L,1);
	if ((idx >= 0) && (idx < vessel_count)) {
		v = &vessels[idx];
	}

	if (v) {
		char path[MAX_FILENAME] = { 0 }, model[MAX_FILENAME] = { 0 };
		char buf[ARBITRARY_MAX] = { 0 };
		char* acfpath;

		XPLMGetNthAircraftModel(v->plane_index, model, path);
		XPLMExtractFileAndPath(path);
	
		//acfpath = strstr(path,"Aircraft");
		//if (acfpath) snprintf(path,MAX_FILENAME-1,"./%s",acfpath);
#ifdef APL
		{
			char* tpath;
			while (tpath = strchr(path,':')) *tpath = '/';
			strncpy(model,path,MAX_FILENAME-1);
			strncpy(path,"/Volumes/",MAX_FILENAME-1);
			strncat(path,model,MAX_FILENAME-1);
		}
#endif

		//Fetch only aircraft model name
		acfpath = strstr(model,".acf");
		if (acfpath) *acfpath = 0;

		dragheat_deinitialize(v);
		dragheat_initialize(v,path,model);
	}
	return 1;
}

int dragheat_highlevel_setpaintmode(lua_State* L)
{
	dragheat_paint_mode_selected = lua_tointeger(L,1);
	return 0;
}

int dragheat_highlevel_setvisualmode(lua_State* L)
{
	dragheat_visual_mode = lua_tointeger(L,1);
	return 0;
}

int dragheat_highlevel_setvisualinterpolation(lua_State* L)
{
	dragheat_visual_interpolate = lua_tointeger(L,1);
	return 0;
}

int dragheat_highlevel_setthickness(lua_State* L)
{
	dragheat_paint_thickness = lua_tonumber(L,1);
	return 0;
}

int dragheat_highlevel_setsimulationstate(lua_State* L)
{
	dragheat_simulation.enabled = lua_tointeger(L,1);
	dragheat_simulation.altitude = lua_tonumber(L,2);
	dragheat_simulation.alpha = lua_tonumber(L,3);
	dragheat_simulation.beta = lua_tonumber(L,4);
	dragheat_simulation.velocity = lua_tonumber(L,5);
	return 0;
}

int dragheat_highlevel_resetsimulation(lua_State* L)
{
	dragheat_reset();
	return 0;
}
#endif

int dragheat_highlevel_draginitialize(lua_State* L)
{
	vessel* v = 0;
	int idx = luaL_checkint(L,1);
	if ((idx >= 0) && (idx < vessel_count)) {
		v = &vessels[idx];
	}

	if (v) {
		dragheat_deinitialize(v);
		dragheat_initialize(v,(char*)lua_tostring(L,2),"");
	}
	return 0;
}

int dragheat_highlevel_isinitialized(lua_State* L)
{
	vessel* v = 0;
	int idx = luaL_checkint(L,1);
	if ((idx >= 0) && (idx < vessel_count)) {
		v = &vessels[idx];
	}

	if (v) {
		lua_pushboolean(L,v->geometry.faces != 0);
	} else {
		lua_pushboolean(L,0);
	}
	return 1;
}

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include <XPLMDisplay.h>
int dragheat_gui_mouseclick(XPLMWindowID window, int X, int Y, XPLMMouseStatus status, void* data)
{
	if (status == xplm_MouseDown) {
		if (dragheat_visual_mode == DRAGHEAT_VISUALMODE_MATERIAL) {
			dragheat_paint_mode = dragheat_paint_mode_selected;
			return 1;
		} else {
			return 0;
		}
	}
	if (status == xplm_MouseUp) {
		dragheat_paint_mode = -1;
		return 1;
	}
	return 0;
}

void dragheat_gui_drawwindow(XPLMWindowID inWindowID, void* inRefcon)
{
}

void dragheat_gui_handlekey(XPLMWindowID inWindowID, char inKey, XPLMKeyFlags inFlags, char inVirtualKey, void* inRefcon, int losingFocus)
{
}
#endif


//==============================================================================
// Initialize highlevel drag/heating interface
//==============================================================================
void dragheat_highlevel_initialize()
{
	//Register API
	lua_createtable(L,0,32);
	lua_setglobal(L,"DragHeatAPI");

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	highlevel_addfunction("DragHeatAPI","LoadOBJ",dragheat_highlevel_loadobj);
	highlevel_addfunction("DragHeatAPI","SaveModel",dragheat_highlevel_savemodel);
	highlevel_addfunction("DragHeatAPI","ReloadModel",dragheat_highlevel_reloadmodel);
	highlevel_addfunction("DragHeatAPI","SetPaintMode",dragheat_highlevel_setpaintmode);
	highlevel_addfunction("DragHeatAPI","SetVisualMode",dragheat_highlevel_setvisualmode);
	highlevel_addfunction("DragHeatAPI","SetVisualInterpolation",dragheat_highlevel_setvisualinterpolation);
	highlevel_addfunction("DragHeatAPI","SetPaintThickness",dragheat_highlevel_setthickness);
	highlevel_addfunction("DragHeatAPI","SetSimulationState",dragheat_highlevel_setsimulationstate);
	highlevel_addfunction("DragHeatAPI","ResetSimulation",dragheat_highlevel_resetsimulation);
#endif
	highlevel_addfunction("DragHeatAPI","LoadInitialize",dragheat_highlevel_draginitialize);
	highlevel_addfunction("DragHeatAPI","IsInitialized",dragheat_highlevel_isinitialized);

	//Create fake window to capture mouse clicks
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	{
		int sx,sy;
		XPLMCreateWindow_t win;
		XPLMGetScreenSize(&sx, &sy);

		memset(&win, 0, sizeof(win));
		win.structSize = sizeof(win);
		win.left = 0;
		win.top = sx;
		win.right = sy;
		win.bottom = 0;
		win.visible = 1;
		win.drawWindowFunc = dragheat_gui_drawwindow;
		win.handleMouseClickFunc = dragheat_gui_mouseclick;
		win.handleKeyFunc = dragheat_gui_handlekey;
		//win.handleCursorFunc = 0;//handleCursor;

		XPLMCreateWindowEx(&win);
	}
#endif
}