//==============================================================================
// Planet related functions
//==============================================================================
#include <stdlib.h>
#include <string.h>
#include <math.h>
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include <soil.h>
#endif

//X-Space stuff
#include "x-space.h"
#include "highlevel.h"
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
//#include "lookups.h"
#include "rendering.h"
//#include "particles.h"
#include "dataref.h"
#endif

#include "vessel.h"
#include "coordsys.h"
#include "quaternion.h"
#include "solpanels.h"
#include "planet.h"

#include "curtime.h"

//X-Plane SDK
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include <XPLMDataAccess.h>
#include <XPLMGraphics.h>
#endif

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
int solpanels_texture;
#endif

void solpanels_initialize_highlevel(vessel* v)
{
	//Initialize sensors
	lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels);
	lua_rawgeti(L,-1,v->index);
	lua_getfield(L,-1,"SolarPanels");
	if (lua_istable(L,-1)) {
		int i;

		//Read solar panels
		i = 0;
		lua_pushnil(L);
		while (lua_next(L,-2)) {
			double x = 0;
			double y = 0;
			double z = 0;
			double bx = 0;
			double by = 0;
			double bz = 0;
			double p = 0;
			double yw = 0;
			double r = 0;
			double w = 0;
			double h = 0;
			double Cd = 1.0;
			quaternion q;

			//Read position, base position, attitude
			lua_getfield(L,-1,"Position");
			if (lua_istable(L,-1)) {
				lua_rawgeti(L,-1,1);
				lua_rawgeti(L,-2,2);
				lua_rawgeti(L,-3,3);
				x = lua_tonumber(L,-3);
				y = lua_tonumber(L,-2);
				z = lua_tonumber(L,-1);
				lua_pop(L,3);
			}
			lua_pop(L,1);
			lua_getfield(L,-1,"BasePosition");
			if (lua_istable(L,-1)) {
				lua_rawgeti(L,-1,1);
				lua_rawgeti(L,-2,2);
				lua_rawgeti(L,-3,3);
				bx = lua_tonumber(L,-3);
				by = lua_tonumber(L,-2);
				bz = lua_tonumber(L,-1);
				lua_pop(L,3);
			}
			lua_pop(L,1);
			lua_getfield(L,-1,"Attitude");
			if (lua_istable(L,-1)) {
				lua_rawgeti(L,-1,1);
				lua_rawgeti(L,-2,2);
				lua_rawgeti(L,-3,3);
				p = lua_tonumber(L,-3);
				r = lua_tonumber(L,-2);
				yw = lua_tonumber(L,-1);
				lua_pop(L,3);
			}
			lua_pop(L,1);
			lua_getfield(L,-1,"Size");
			if (lua_istable(L,-1)) {
				lua_rawgeti(L,-1,1);
				lua_rawgeti(L,-2,2);
				w = lua_tonumber(L,-2);
				h = lua_tonumber(L,-1);
				lua_pop(L,2);
			}
			lua_pop(L,1);

			//Get attitude
			qeuler_from(q,RAD(p),RAD(r),RAD(yw));

			//Store solar panel
			v->solar_panels[i].bx = bx;
			v->solar_panels[i].by = by;
			v->solar_panels[i].bz = bz;
			v->solar_panels[i].x = x;
			v->solar_panels[i].y = y;
			v->solar_panels[i].z = z;
			v->solar_panels[i].w = w;
			v->solar_panels[i].h = h;
			memcpy(&v->solar_panels[i].q,&q,sizeof(quaternion));

			//Calculate extra stuff
			v->solar_panels[i].Cd = Cd;
			v->solar_panels[i].A = w*h;

			lua_pushnumber(L,i);
			lua_setfield(L,-2,"_solpanel_idx");

			i++;
			lua_pop(L,1); //Remove value, only leave key
		}
	}
	lua_pop(L,3);
}

void solpanels_initialize()
{
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	XPLMGenerateTextureNumbers(&solpanels_texture,1);
	XPLMBindTexture2d(solpanels_texture,0);

	//Load clouds texture
	solpanels_texture = SOIL_load_OGL_texture(
	                        FROM_PLUGINS("panels\\solar_panel0.tga"),
	                        SOIL_LOAD_AUTO,solpanels_texture,0);
	if (!solpanels_texture) {
		log_write("X-Space: Unable to load solar panel texture: %s\n",SOIL_last_result());
	} else {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
#endif
}


#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void solpanels_draw_vessel(vessel* v)
{
	int i;
	quaternion q;
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

	//Setup correct drawing mode
	XPLMSetGraphicsState(0,1,1,0,0,1,1);
	XPLMBindTexture2d(solpanels_texture,0);
	glDisable(GL_CULL_FACE);

	//Draw mesh
	for (i = 0; i < VESSEL_MAX_SOLAR_PANELS; i++) {
		if (v->solar_panels[i].A > 0.0) {
			solar_panel* p = &v->solar_panels[i];

			//Put into correct position
			glPushMatrix();
			glTranslated(-p->y,p->z,p->x);

			{
				double sx,sy,sz;
				double ax,ay,az;
				double sang2,cang2,ang;
				double cx,cy,cz;

				cx = sin(RAD(current_planet.sim_sun_heading))*cos(RAD(current_planet.sim_sun_pitch));
				cy = sin(RAD(current_planet.sim_sun_pitch));
				cz = -cos(RAD(current_planet.sim_sun_heading))*cos(RAD(current_planet.sim_sun_pitch));
				vec_sim2l(v,cx,cy,cz,&sx,&sy,&sz);

				//sx sy sz
				// 1  0  0
				// 0  1  0

				//cx = -1.0;
				//cy = 0.0;
				//cz = 0.0;
				//ang = acos(cx*sx+cy*sy+cz*sz);
				//ang = asin(sz);
				ang = atan(sz/sx);

				ax = 0.0;
				ay = 1.0;
				az = 0.0;

				sang2 = sin(-0.5*ang);
				cang2 = cos(-0.5*ang);
				
				p->q[0] = cang2;
				p->q[1] = ax*sang2;
				p->q[2] = ay*sang2;
				p->q[3] = az*sang2;
			}

			//Put into correct attitude
			qeuler_to(p->q,&q[0],&q[1],&q[2]);
			//q[0] = curtime();
			glRotated(-DEG(q[2]),0,1,0);
			glRotated( DEG(q[1]),1,0,0);
			glRotated(-DEG(q[0]),0,0,1);

			glBegin(GL_QUADS);
			glColor4d(1,1,1,1);
			glNormal3d(0.0,0.0,-1.0);

			glTexCoord2d(0.0,0.0);
			glVertex3d( p->w*0.5,-p->h*0.5,0.0);

			glTexCoord2d(0.0,p->h*1);
			glVertex3d( p->w*0.5, p->h*0.5,0.0);

			glTexCoord2d(p->w*0.5,p->h*1);
			glVertex3d(-p->w*0.5, p->h*0.5,0.0);

			glTexCoord2d(p->w*0.5,0.0);
			glVertex3d(-p->w*0.5,-p->h*0.5,0.0);
			glEnd();

			glPopMatrix();

			//Get normal
		}
	}
	/*glNormal3d(-faces[i].ny,faces[i].nz,faces[i].nx);
	glVertex3d(-faces[i].y[1]-ty,faces[i].z[1]+tz,faces[i].x[1]+tx);
	glNormal3d(-faces[i].ny,faces[i].nz,faces[i].nx);
	glVertex3d(-faces[i].y[0]-ty,faces[i].z[0]+tz,faces[i].x[0]+tx);
	glNormal3d(-faces[i].ny,faces[i].nz,faces[i].nx);
	glVertex3d(-faces[i].y[2]-ty,faces[i].z[2]+tz,faces[i].x[2]+tx);*/

	//Draw sensors
	/*XPLMSetGraphicsState(0,0,0,0,0,1,0);
	glBegin(GL_LINES);
	for (i = 0; i < v->sensors.surface_count; i++) {
		int face = v->sensors.surface[i].face;
		glColor3f(1.0f,0.0f,0.0f);
		glVertex3d(-v->sensors.surface[i].y,v->sensors.surface[i].z,v->sensors.surface[i].x);
		glColor3f(0.0f,1.0f,0.0f);
		glVertex3d(-v->geometry.faces[face].ay,v->geometry.faces[face].az,v->geometry.faces[face].ax);
	}
	glEnd();*/

	//Restore state
	glEnable(GL_CULL_FACE);
	glPopMatrix();
}
#endif

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void solpanels_draw()
{
	int i;
	extern XPLMDataRef dataref_vessel_x;
	extern XPLMDataRef dataref_vessel_y;
	extern XPLMDataRef dataref_vessel_z;
	double dx = XPLMGetDatad(dataref_vessel_x)-vessels[0].sim.x;
	double dy = XPLMGetDatad(dataref_vessel_y)-vessels[0].sim.y;
	double dz = XPLMGetDatad(dataref_vessel_z)-vessels[0].sim.z;

	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists) {
			if (vessels[i].physics_type == VESSEL_PHYSICS_SIM) {
				glPushMatrix();
				glTranslated(dx,dy,dz);
			}

			solpanels_draw_vessel(&vessels[i]);

			if (vessels[i].physics_type == VESSEL_PHYSICS_SIM) glPopMatrix();
		}
	}
}
#endif