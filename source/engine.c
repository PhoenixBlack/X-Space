//==============================================================================
// Engine simulation (including X-Plane compatibility)
//==============================================================================
#include <math.h>
#include <stdlib.h>

#include "x-space.h"
#include "vessel.h"
#include "engine.h"
#include "quaternion.h"
#include "coordsys.h"
#include "highlevel.h"
#include "curtime.h"
#include "config.h"

//Include X-Plane SDK
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include "rendering.h"
#include "particles.h"
#include "lookups.h"

#include <XPLMDataAccess.h>
#include <XPLMGraphics.h>
#endif


#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
XPLMDataRef dataref_num_engines;
XPLMDataRef dataref_engine_sfc;
XPLMDataRef dataref_engine_location;
XPLMDataRef dataref_engine_force;
XPLMDataRef dataref_engine_throttle;
XPLMDataRef dataref_engine_mixture;
XPLMDataRef dataref_engine_vert;
XPLMDataRef dataref_engine_side;
XPLMDataRef dataref_engine_ff;
XPLMDataRef dataref_tank_selector;
XPLMDataRef dataref_fuel_weight;
XPLMDataRef dataref_fuel_x;
XPLMDataRef dataref_fuel_y;
XPLMDataRef dataref_fuel_z;
XPLMDataRef dataref_jato_theta;
XPLMDataRef dataref_jato_thrust;
XPLMDataRef dataref_jato_on;
XPLMDataRef dataref_jato_y;
XPLMDataRef dataref_jato_z;
XPLMDataRef dataref_jato_left;
XPLMDataRef dataref_jato_sfc;

//Handles
GLhandleARB engines_exhaust_program; //Program object
GLhandleARB engines_exhaust_vertex; //Vertex shader for clouds
GLhandleARB engines_exhaust_fragment; //Fragment shader 
GLint engines_perlin_perm_location; //Permutations texture
GLint engines_time_location; //Time
GLint engines_intensity_location; //Engine exhaust intensity
GLint engines_velocity_location; //Engine exhaust velocity
GLint engines_width_location; //Engine exhaust width

GLint engines_trail_width_location;
GLint engines_trail_height_location;
#endif




//==============================================================================
// Initializes datarefs
//==============================================================================
void engines_initialize()
{
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	dataref_num_engines			= XPLMFindDataRef("sim/aircraft/engine/acf_num_engines");
	dataref_engine_sfc			= XPLMFindDataRef("sim/aircraft/overflow/SFC_full_hi_JET");
	dataref_engine_location		= XPLMFindDataRef("sim/flightmodel/engine/POINT_XYZ");
	dataref_engine_force		= XPLMFindDataRef("sim/aircraft/engine/acf_fmax_vac");
	dataref_engine_throttle		= XPLMFindDataRef("sim/flightmodel/engine/ENGN_thro_use");
	dataref_engine_mixture		= XPLMFindDataRef("sim/flightmodel/engine/ENGN_mixt");
	dataref_engine_vert			= XPLMFindDataRef("sim/aircraft/prop/acf_vertcant");
	dataref_engine_side			= XPLMFindDataRef("sim/aircraft/prop/acf_sidecant");
	dataref_engine_ff			= XPLMFindDataRef("sim/flightmodel/engine/ENGN_FF_");
	dataref_tank_selector		= XPLMFindDataRef("sim/cockpit/engine/fuel_tank_selector");
	dataref_fuel_weight			= XPLMFindDataRef("sim/flightmodel/weight/m_fuel");
	dataref_fuel_x				= XPLMFindDataRef("sim/aircraft/overflow/acf_tank_X");
	dataref_fuel_y				= XPLMFindDataRef("sim/aircraft/overflow/acf_tank_Y");
	dataref_fuel_z				= XPLMFindDataRef("sim/aircraft/overflow/acf_tank_Z");
	dataref_jato_theta			= XPLMFindDataRef("sim/aircraft/specialcontrols/acf_jato_theta");
	dataref_jato_thrust			= XPLMFindDataRef("sim/aircraft/specialcontrols/acf_jato_thrust");
	dataref_jato_on				= XPLMFindDataRef("sim/cockpit/switches/jato_on");
	dataref_jato_y				= XPLMFindDataRef("sim/aircraft/specialcontrols/acf_jato_Y");
	dataref_jato_z				= XPLMFindDataRef("sim/aircraft/specialcontrols/acf_jato_Z");
	dataref_jato_left			= XPLMFindDataRef("sim/flightmodel/misc/jato_left");
	dataref_jato_sfc			= XPLMFindDataRef("sim/aircraft/specialcontrols/acf_jato_sfc");
#endif
}


//==============================================================================
// Reads position and vertical/horizontal angles from engine table
//==============================================================================
void engines_highlevel_read(double* x, double* y, double* z, double* vert, double* horiz)
{
	//Read thruster position
	lua_getfield(L,-1,"Position");
	if (lua_isfunction(L,-1)) highlevel_call(0,1);
	if (lua_istable(L,-1)) {
		lua_rawgeti(L,-1,1);
		lua_rawgeti(L,-2,2);
		lua_rawgeti(L,-3,3);
		*x = lua_tonumber(L,-3);
		*y = lua_tonumber(L,-2);
		*z = lua_tonumber(L,-1);
		lua_pop(L,3);
	} else {
		*x = 0;
		*y = 0;
		*z = 0;
	}
	lua_pop(L,1);

	//Read thruster angles
	lua_pushcfunction(L,highlevel_dataref);
	lua_getfield(L,-2,"VerticalAngle");
	highlevel_call(1,1);

	lua_pushcfunction(L,highlevel_dataref);
	lua_getfield(L,-3,"HorizontalAngle");
	highlevel_call(1,1);

	*vert = lua_tonumber(L,-2);
	*horiz = lua_tonumber(L,-1);
	lua_pop(L,2);
}


//==============================================================================
// Simulate a single engine
//==============================================================================
void engines_simulate_single(vessel* v, float dt)
{
	//Thruster position, orientation
	double x,y,z,vert,horiz,force = 0;
	engines_highlevel_read(&x,&y,&z,&vert,&horiz);

	//Simulate thruster for the force
	lua_getfield(L,-1,"Simulator");
	if (lua_isnil(L,-1)) {
		lua_getglobal(L,"EngineSimulator");
		lua_replace(L,-2);
	}
	if (lua_isfunction(L,-1)) {
		lua_pushvalue(L,-1); //push function
		lua_pushvalue(L,-3); //push engine
		lua_pushnumber(L,dt); //push dt

		//Simulator(Engine,dT)
		lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels);
		lua_rawgeti(L,-1,v->index);
		lua_setfenv(L,-3-2); //Set aicraft environment
		lua_pop(L,1); //Pop vessels table

		highlevel_call(2,1);
		force = lua_tonumber(L,-1);
		lua_pop(L,1);

		//Make sure it's not NaN
		if (!(force <= 0) && (!(force >= 0))) force = 0;
	}
	lua_pop(L,1);

	if (force > 0) engines_addforce(v,dt,x,y,z,vert,horiz,force);
}


//==============================================================================
// Simulates custom and X-Plane engines
//==============================================================================
void engines_simulate(float dt)
{
	int i;

	lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels);
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists) {
			lua_rawgeti(L,-1,i);
			if (lua_istable(L,-1)) {
				lua_getfield(L,-1,"Engine");
				if (lua_istable(L,-1)) {
					//First element must be nil for "next" to work
					lua_pushnil(L);

					//Loop through all engines
					while (lua_next(L,-2)) {
						//Simulate a single thruster
						if (lua_istable(L,-1)) {
							engines_simulate_single(&vessels[i],dt);
						}

						//Remove value, only leave key
						lua_pop(L,1);
					}
				}
				lua_pop(L,1);
			}
			lua_pop(L,1);
		}
	}
	lua_pop(L,1); //Pop vessels table
}


//==============================================================================
// Simulates X-Plane engines operation
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void engines_simulate_xplane(float dt)
{
	int i;
	//int num_engines = XPLMGetDatai(dataref_num_engines);
	int fuel_source = XPLMGetDatai(dataref_tank_selector);
	float max_force = XPLMGetDataf(dataref_engine_force);
	float jato_force = XPLMGetDataf(dataref_jato_thrust);
	float xyz[9*3];
	float throttle[9], mixture[9];
	float vert[9], side[9];
	float fuel[9],fuel_lat[9];
	float jato_fuel;
	int tank;

	//Read datarefs for engine data
	XPLMGetDatavf(dataref_engine_location,xyz,0,8*3);
	XPLMGetDatavf(dataref_engine_throttle,throttle,0,8);
	XPLMGetDatavf(dataref_engine_mixture,mixture,0,8);
	XPLMGetDatavf(dataref_engine_vert,vert,0,8);
	XPLMGetDatavf(dataref_engine_side,side,0,8);

	//Read datarefs for JATO
	vert[8] = XPLMGetDataf(dataref_jato_theta);
	side[8] = 0.0;
	xyz[8*3+0] = 0.0;
	xyz[8*3+1] = XPLMGetDataf(dataref_jato_y);
	xyz[8*3+2] = XPLMGetDataf(dataref_jato_z);
	throttle[8] = 0.0;//(float)XPLMGetDatai(dataref_jato_on);
	mixture[8] = 1.0f;
	jato_fuel = XPLMGetDataf(dataref_jato_left);

	//Read datarefs for fuel
	XPLMGetDatavf(dataref_fuel_weight,fuel,0,9);
	XPLMGetDatavf(dataref_fuel_x,fuel_lat,0,9);

	//Pick correct fuel tank
	tank = -1;
	for (i = 8; i >= 0; i--) {
		if ((fuel_source == 1) && (fuel_lat[i]  < 0)) tank = i;
		if ((fuel_source == 2) && (fuel_lat[i] == 0)) tank = i;
		if ((fuel_source == 3) && (fuel_lat[i]  > 0)) tank = i;
	}

	//Rocket engines
	for (i = 0; i < 9; i++) { //FIXME: should be num_engines?
		double x,y,z;
		double force;

		//Calculate position
		x = xyz[i*3+2]; //z: lon
		y = xyz[i*3+0]; //x: lat
		z = xyz[i*3+1]; //y: vert

		//Calculate force
		if (i == 8) {
			force = throttle[i]*jato_force;
			if (jato_fuel <= 0) force = 0;
		} else {
			force = throttle[i]*max_force;
			if (mixture[i] < 0.9f) force = 0;
			if ((tank == -1) || (fuel[tank] <= 0.0f)) force = 0;
		}

		engines_addforce(&vessels[0],dt,x,y,z,vert[i],side[i],force);
	}
}
#endif



//==============================================================================
// Initialize/deinitialize resources
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void engines_draw_initialize()
{
	//Load lookup textures for shaders
	if (config.use_shaders) {
		//Create vertex shader for clouds
		engines_exhaust_vertex = rendering_shader_compile(GL_VERTEX_SHADER,FROM_PLUGINS("shaders/exhaust.vert"),1);
		engines_exhaust_fragment = rendering_shader_compile(GL_FRAGMENT_SHADER,FROM_PLUGINS("shaders/exhaust.frag"),0);

		//Create program
		engines_exhaust_program = rendering_shader_link(engines_exhaust_vertex,engines_exhaust_fragment);
		engines_perlin_perm_location = glGetUniformLocation(engines_exhaust_program, "permTexture");
		engines_time_location = glGetUniformLocation(engines_exhaust_program, "curTime");
		engines_intensity_location = glGetUniformLocation(engines_exhaust_program, "exhaustIntensity");
		engines_velocity_location = glGetUniformLocation(engines_exhaust_program, "exhaustVelocity");
		engines_width_location = glGetUniformLocation(engines_exhaust_program, "exhaustWidth");

		engines_trail_width_location = glGetUniformLocation(engines_exhaust_program, "trailWidth");
		engines_trail_height_location = glGetUniformLocation(engines_exhaust_program, "trailHeight");
	}
}

void engines_draw_deinitialize()
{
	if (config.use_shaders) {
		glDeleteProgram(engines_exhaust_program);
		glDeleteShader(engines_exhaust_fragment);
		glDeleteShader(engines_exhaust_vertex);
	}
}
#endif


//==============================================================================
// Draw an exhaust cone
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void engines_exhaust_cone(double thruster_length, double plume_length,
                          double thruster_width, double plume_width,
						  double thruster_length_noise, double plume_length_noise,
                          double r1, double g1, double b1, double a1,
                          double r2, double g2, double b2, double a2)
{
	double w,step;
	glBegin(GL_TRIANGLES);

	step = 2*PI/16.0;
	for (w = 0; w < 2*PI; w += step) {
		double noise = 1.0*rand()/RAND_MAX;

		glColor4d(r1, g1, b1, a1);
		glVertex3d(thruster_length+noise*thruster_length_noise,0.5*thruster_width*cos(w),0.5*thruster_width*sin(w));
		glColor4d(r2, g2, b2, a2);
		glVertex3d(plume_length+noise*plume_length_noise,0.5*plume_width*cos(w+step),0.5*plume_width*sin(w+step));
		glVertex3d(plume_length+noise*plume_length_noise,0.5*plume_width*cos(w),0.5*plume_width*sin(w));

		glColor4d(r1, g1, b1, a1);
		glVertex3d(thruster_length+noise*thruster_length_noise,0.5*thruster_width*cos(w),0.5*thruster_width*sin(w));
		glVertex3d(thruster_length+noise*thruster_length_noise,0.5*thruster_width*cos(w+step),0.5*thruster_width*sin(w+step));
		glColor4d(r2, g2, b2, a2);
		glVertex3d(plume_length+noise*plume_length_noise,0.5*plume_width*cos(w+step),0.5*plume_width*sin(w+step));
	}

	glEnd();
}
#endif


//==============================================================================
// Draw a single engine
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void engines_draw_single(vessel* v)
{
	//Various variables
	quaternion q;
	double sx,sy,sz;
	double lx,ly,lz;
	double nozzle_width;

	//Thruster position, orientation
	double x,y,z,vert,horiz;

	engines_highlevel_read(&x,&y,&z,&vert,&horiz);

	//Get rendering data for the thruster
	lua_getfield(L,-1,"Renderer");
	if (lua_isnil(L,-1)) {
		lua_getglobal(L,"EngineRenderer");
		lua_replace(L,-2);
	}
	if (lua_isfunction(L,-1)) {
		lua_pushvalue(L,-1); //push function
		lua_pushvalue(L,-3); //push engine

		//Renderer(engine)
		highlevel_call(1,1);
	} else {
		lua_createtable(L,0,0);
	}
	//lua_pop(L,1);
	lua_remove(L,-2); //Renderer function

	//Check if rendering data exists
	if (!lua_istable(L,-1)) {
		lua_pop(L,1);
		return;
	}

	//Nozzle width always defined
	lua_getfield(L,-1,"NozzleWidth");
	nozzle_width = lua_tonumber(L,-1);
	lua_pop(L,1);

	//Setup RCS thruster rendering
	glDisable(GL_CULL_FACE);
	glPushMatrix();

	//Translate into correct position
	coord_l2sim(v,x,y,z,&sx,&sy,&sz);
	glTranslated(sx,sy,sz);
	//Rotate into vessel orientation
	qeuler_to(v->sim.q,&q[0],&q[1],&q[2]);
	glRotated(-DEG(q[2]),0,1,0);
	glRotated(DEG(q[1]),1,0,0);
	glRotated(-DEG(q[0]),0,0,1);
	//Add thruster orientation to it
	glRotated(vert,     1,0,0);
	glRotated(-90-horiz,0,1,0);

	//Update previous position
	lua_getfield(L,-2,"__last_x");
	lx = lua_tonumber(L,-1);
	lua_pop(L,1);
	lua_getfield(L,-2,"__last_y");
	ly = lua_tonumber(L,-1);
	lua_pop(L,1);
	lua_getfield(L,-2,"__last_z");
	lz = lua_tonumber(L,-1);
	lua_pop(L,1);

	//Draw debug marker
	if (config.engine_debug_draw) {
		XPLMSetGraphicsState(0,0,0,0,1,1,0);
		engines_exhaust_cone(
		    0.0,nozzle_width*10.0,
		    nozzle_width,nozzle_width*3,
			0,0,
		    0.0,1.0,0.0,0.5, //Green color
		    0.5,1.0,0.0,0.0);
	}

	//Draw exhaust cones (as required)
	lua_getfield(L,-1,"ExhaustCones");
	if (lua_istable(L,-1)) {
		//Additive rendering
		XPLMSetGraphicsState(0,0,0,0,1,1,0);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);

		//Loop through all cones to draw
		lua_pushnil(L);
		while (lua_next(L,-2)) {
			if (lua_istable(L,-1)) {
				double tl,tw,pl,pw;
				double r1,g1,b1,a1;
				double r2,g2,b2,a2;
				double tln,pln;

				lua_getfield(L,-1,"NozzleWidth");	tw = lua_tonumber(L,-1); lua_pop(L,1);
				lua_getfield(L,-1,"NozzleLength");	tl = lua_tonumber(L,-1); lua_pop(L,1);
				lua_getfield(L,-1,"PlumeWidth");	pw = lua_tonumber(L,-1); lua_pop(L,1);
				lua_getfield(L,-1,"PlumeLength");	pl = lua_tonumber(L,-1); lua_pop(L,1);

				lua_getfield(L,-1,"ThrusterLengthNoise");	tln = lua_tonumber(L,-1); lua_pop(L,1);
				lua_getfield(L,-1,"PlumeLengthNoise");		pln = lua_tonumber(L,-1); lua_pop(L,1);

				lua_getfield(L,-1,"R1"); r1 = lua_tonumber(L,-1); lua_pop(L,1);
				lua_getfield(L,-1,"G1"); g1 = lua_tonumber(L,-1); lua_pop(L,1);
				lua_getfield(L,-1,"B1"); b1 = lua_tonumber(L,-1); lua_pop(L,1);
				lua_getfield(L,-1,"A1"); a1 = lua_tonumber(L,-1); lua_pop(L,1);

				lua_getfield(L,-1,"R2"); r2 = lua_tonumber(L,-1); lua_pop(L,1);
				lua_getfield(L,-1,"G2"); g2 = lua_tonumber(L,-1); lua_pop(L,1);
				lua_getfield(L,-1,"B2"); b2 = lua_tonumber(L,-1); lua_pop(L,1);
				lua_getfield(L,-1,"A2"); a2 = lua_tonumber(L,-1); lua_pop(L,1);

				engines_exhaust_cone(
				    tl,pl,tw,pw,
					tln,pln,
				    r1,g1,b1,a1,
				    r2,g2,b2,a2);
			}
			lua_pop(L,1);
		}
	}
	lua_pop(L,1);

	//Draw exahust trail
	lua_getfield(L,-1,"ExhaustTrail");
	if (lua_istable(L,-1)) {
		int i;
		float max_size;
		float abc[] = { 1.0f, 0.0f, 0.0001f };
		float trail_length;
		float trail_width;
		float a1,a2;
		float texels,velocity,intensity;

		//Fetch trail length
		lua_getfield(L,-1,"Length");
		trail_length = (float)lua_tonumber(L,-1); lua_pop(L,1);
		lua_getfield(L,-1,"Width");
		trail_width = (float)lua_tonumber(L,-1); lua_pop(L,1);
		lua_getfield(L,-1,"A1");
		a1 = (float)lua_tonumber(L,-1); lua_pop(L,1);
		lua_getfield(L,-1,"A2");
		a2 = (float)lua_tonumber(L,-1); lua_pop(L,1);
		lua_getfield(L,-1,"Texels");
		texels = (float)lua_tonumber(L,-1); lua_pop(L,1);
		lua_getfield(L,-1,"Velocity");
		velocity = (float)lua_tonumber(L,-1); lua_pop(L,1);
		lua_getfield(L,-1,"Intensity");
		intensity = (float)lua_tonumber(L,-1); lua_pop(L,1);
		//lua_getfield(L,-1,"Size");
		//abc[2] = (float)lua_tonumber(L,-1); lua_pop(L,1);

		if (config.use_shaders) {
			float offset;

			XPLMSetGraphicsState(0,1,0,0,1,1,0);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);

			offset = v->index*1000.0f + (float)(lua_tonumber(L,-4))*100.0f;
			trail_width = trail_width / (4*texels); //Update trail width

			glUseProgram(engines_exhaust_program);
			glActiveTexture(GL_TEXTURE0);
			XPLMBindTexture2d(particles_perlin_perm_texture,0);
			if (engines_perlin_perm_location != -1) glUniform1i(engines_perlin_perm_location, 0);
			if (engines_time_location != -1) glUniform1f(engines_time_location, (float)curtime()+offset);
			if (engines_intensity_location != -1) glUniform1f(engines_intensity_location, (float)intensity);
			if (engines_velocity_location != -1) glUniform1f(engines_velocity_location, (float)velocity);
			if (engines_width_location != -1) glUniform1f(engines_width_location, (float)texels);

			if (engines_trail_width_location != -1) glUniform1f(engines_trail_width_location, (float)trail_width);
			if (engines_trail_height_location != -1) glUniform1f(engines_trail_height_location, (float)trail_length);

			glColor4d(1.0,1.0,1.0,a1);
			glBegin(GL_QUADS);
			glTexCoord2d(0.0f,0.0f);
			glVertex3d(0,0,0);
			glTexCoord2d(1.0f,0.0f);
			glVertex3d(0,0,0);
			glTexCoord2d(1.0f,1.0f);
			glVertex3d(0,0,0);
			glTexCoord2d(0.0f,1.0f);
			glVertex3d(0,0,0);

			/*glTexCoord2d(0.0f,0.0f);
			glVertex3d(-trail_width,0,0);
			glTexCoord2d(1.0f,0.0f);
			glVertex3d(trail_width,0,0);
			glTexCoord2d(1.0f,1.0f);
			glVertex3d(trail_width,trail_length,0);
			glTexCoord2d(0.0f,1.0f);
			glVertex3d(-trail_width,trail_length,0);*/
			glEnd();

			glUseProgram(0);
			glActiveTexture(GL_TEXTURE0);
		}

		//Setup rendering additive trail
		XPLMSetGraphicsState(0, 1, 0, 0, 1, 1, 0);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		glEnable(GL_POINT_SPRITE);
		glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
		glGetFloatv(GL_POINT_SIZE_MAX_ARB,&max_size);
		if (max_size > 256.0f) max_size = 256.0f;
		glPointSize(max_size);
		glPointParameterf(GL_POINT_SIZE_MIN, 1.0f);
		glPointParameterf(GL_POINT_SIZE_MAX, max_size);

		//Draw particles for exhaust trail (fallback for no shaders)
		if (!config.use_shaders) {
			XPLMBindTexture2d(particles_glow_texture,0);
			glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION,abc);
			glBegin(GL_POINTS);
			for (i = 0; i < 32; i++) {
				float t = i*(1.0f/32.0f);
				float dl = trail_length/32.0f;
				float dx = dl*i + (dl+0.5f*dl*t)*(1.0f*rand()/RAND_MAX);
				float dy = (1.0f+2.0f*dl*t)*(1.0f*rand()/RAND_MAX);
				float dz = (1.0f+2.0f*dl*t)*(1.0f*rand()/RAND_MAX);

				glColor4f(1.0f,0.9f,0.8f,a1*(0.40f - 0.39f*t*t)*intensity);
				glVertex3f(dx,dy,dz);
			}
			glEnd();
		}

		//Draw glow from engine
		XPLMSetGraphicsState(0, 1, 0, 0, 1, 0, 0);
		XPLMBindTexture2d(particles_glow_texture,0);
		abc[2] = 0.000003f; glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION,abc);

		//A single point
		glBegin(GL_POINTS);
		glColor4f(1.0f,0.9f,0.8f,0.4f*a2);
		glVertex3f(0,0,0);
		glEnd();

		//Disable sprites
		glDisable(GL_POINT_SPRITE);
		glPointSize(1);
	}
	lua_pop(L,1);

	//Draw smoke trail
	lua_getfield(L,-1,"ExhaustSmoke");
	if (lua_istable(L,-1)) {
		double duration,mindistance;
		int type;
		double d = sqrt((sx-lx)*(sx-lx)+
						(sy-ly)*(sy-ly)+
						(sz-lz)*(sz-lz));
		lua_getfield(L,-1,"Duration");
		duration = (float)lua_tonumber(L,-1); lua_pop(L,1);
		lua_getfield(L,-1,"MinDistance");
		mindistance = (float)lua_tonumber(L,-1); lua_pop(L,1);
		lua_getfield(L,-1,"Type");
		type = (int)lua_tonumber(L,-1); lua_pop(L,1);
		if (type < 0) type = 0;
		if (type >= PARTICLE_TYPES) type = 0;

		if (d > mindistance) {
			lua_pushnumber(L,sx); lua_setfield(L,-4,"__last_x");
			lua_pushnumber(L,sy); lua_setfield(L,-4,"__last_y");
			lua_pushnumber(L,sz); lua_setfield(L,-4,"__last_z");
			if (duration > 0.1) particles_spawn(sx,sy,sz,duration,0.0,type);
		}
	}
	lua_pop(L,1);

	//Finish drawing
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glPopMatrix();

	//Remove table with rendering parameters
	lua_pop(L,1);
}
#endif


//==============================================================================
// Draw engines
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void engines_draw()
{
	//Draw debug information for thrusters
	int i;
	extern XPLMDataRef dataref_vessel_x;
	extern XPLMDataRef dataref_vessel_y;
	extern XPLMDataRef dataref_vessel_z;
	double dx = XPLMGetDatad(dataref_vessel_x)-vessels[0].sim.x;
	double dy = XPLMGetDatad(dataref_vessel_y)-vessels[0].sim.y;
	double dz = XPLMGetDatad(dataref_vessel_z)-vessels[0].sim.z;

	lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels);
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists) {
			if (vessels[i].physics_type == VESSEL_PHYSICS_SIM) {
				glPushMatrix();
				glTranslated(dx,dy,dz);
			}

			lua_rawgeti(L,-1,i);
			if (lua_istable(L,-1)) {
				lua_getfield(L,-1,"Engine");
				if (lua_istable(L,-1)) {
					//First element must be nil for "next" to work
					lua_pushnil(L);

					//Loop through all engines
					while (lua_next(L,-2)) {
						//Simulate a single thruster
						if (lua_istable(L,-1)) {
							engines_draw_single(&vessels[i]);
						}

						//Remove value, only leave key
						lua_pop(L,1);
					}
				}
				lua_pop(L,1);
			}
			lua_pop(L,1);

			if (vessels[i].physics_type == VESSEL_PHYSICS_SIM) glPopMatrix();
		}
	}
	lua_pop(L,1); //Pop vessels table
}
#endif




//==============================================================================
// Add force for a single engine
//==============================================================================
void engines_addforce(vessel* v, double dt, double x, double y, double z, double vert, double horiz, double force)
{
	double fx,fy,fz; //Local force
	double svx,svy; //Horizontal, vertical deflection (sines)
	double cvx,cvy; //Horizontal, vertical deflection (cosines)

	//Calculate force rotation
	svx = sin(RAD(horiz));
	svy = sin(RAD(-vert));
	cvx = cos(RAD(horiz));
	cvy = cos(RAD(-vert));

	//Calculate force vector (opposite of thrust plume vector)
	//Horizontal component
	//fx = -cvx;
	//fy =  svx;
	//fz =  0.0;
	//Vertical componet
	//fx = fx*cvy-fz*svy;
	//fy = fy;
	//fz = fx*svy+fz*cvy;

	//Combined
	fx = -force*cvx*cvy;
	fy = -force*svx;
	fz = -force*cvx*svy;

	//Calculate torque
	vessels_addforce(v,dt,x,y,z,fx,fy,fz);
}




//==============================================================================
// Add weight of JATO fuel to total weight of vessel 0
//==============================================================================
void engines_update_weights(vessel* v)
{
	if (v->index == 0) {
		int i;
		float fuel[9];

		//Update normal tanks fuel
		XPLMGetDatavf(dataref_fuel_weight,fuel,0,9);
		for (i = 0; i < 9; i++) v->weight.fuel[i] = fuel[i];

		//Update weight of JATO
		{
			float sfc = XPLMGetDataf(dataref_jato_sfc);
			float thrust = XPLMGetDataf(dataref_jato_thrust);
			float left = XPLMGetDataf(dataref_jato_left);

			v->weight.fuel[9] = sfc*2.83254504e-5*thrust*left;
		}
	}
}

void engines_write_weights(vessel* v)
{
	if (v->index == 0) {
		int i;
		float fuel[9];

		//Update normal tanks fuel
		for (i = 0; i < 9; i++) fuel[i] = (float)v->weight.fuel[i];
		XPLMSetDatavf(dataref_fuel_weight,fuel,0,9);
	}
}
