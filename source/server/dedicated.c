//==============================================================================
// X-Space resource management and base functions (dedicated server)
//==============================================================================
#include <math.h>
#include <stdio.h>
#include <enet/enet.h>

//-- Code modules --------------------------------------------------------------
#include "x-space.h"     //X-Space resource management
#include "highlevel.h"   //Highlevel code support
#include "config.h"      //Plugin configuration
//#include "gui.h"         //Highlevel GUI
#include "vessel.h"      //Vessels system
#include "atmosphere.h"  //Atmosphere physics
#include "coordsys.h"    //Coordinate systems
#include "geomagnetic.h" //Correct geomagnetic model
#include "material.h"    //Material simulation
#include "physics.h"     //Physics simulation
#include "planet.h"      //Planet stuff
#include "network.h"     //High-level networking
//#include "rendering.h"   //OpenGL rendering and shaders
//#include "engine.h"      //Engine simulation
//#include "particles.h"   //Particles rendering
#include "dragheat.h"    //Drag and heat simulation
//#include "launchpads.h"  //Launch pads simulation
//#include "camera.h"      //Extra camera views
#include "radiosys.h"    //Radio transmission simulation
#include "sol.h"         //Solar system bodies
#include "x-ivss.h"      //Internal Vessel Systems Simulator

#include "curtime.h"     //Current time (precise)
#include "threading.h"   //Multithreading support

//Resource management
int xspace_initialized_all = 0;

//==============================================================================
// Initializes all resources (global ones)
// First thing to be called when X-Space loads
//==============================================================================
void xspace_initialize_all()
{
	if (xspace_initialized_all) xspace_deinitialize_all();

	//Initialize threading system
	thread_initialize();

	//Initialize highlevel system
	highlevel_initialize(); //allocates lua memory
	highlevel_load(FROM_PLUGINS("lua/initialize.lua")); //allocates lua memory
	config_initialize(); //allocates lua memory, loads configuration

	//Initializers with no mem alloc:
	planet_initialize(); //datarefs
	atmosphere_initialize(); //datarefs
	vessels_initialize(); //datarefs, lua memory
	coordsys_initialize(); //datarefs
	physics_initialize(); //datarefs
	solsystem_initialize(); //loads file

	//Initializers with mem alloc:
	geomagnetic_initialize(); //load model, datarefs
	material_initialize(); //load database
	radiosys_initialize(); //memory, datarefs

	//Initialize various highlevel scripts
	network_initialize(); //mem alloc, lua mem
	dragheat_highlevel_initialize(); //lua mem
	xivss_highlevel_initialize();

	//Extra dedicated server stuff:
	vessels_reinitialize();

	//All resources now initialized
	xspace_initialized_all = 1;
}

void xspace_deinitialize_all()
{
	//Do not free resources if not allocated
	if (!xspace_initialized_all) return;

	//Deinitializers with mem free:
	material_deinitialize(); //free model
	geomagnetic_deinitialize(); //free model
	vessels_deinitialize(); //free memory, lua memory
	radiosys_deinitialize(); //free memory, datarefs

	//Highlevel deinitialization
	network_deinitialize(); //free mem, lua mem
	//gui_deinitialize(); //free menu items
	highlevel_deinitialize(); //free lua memory

	//Deinitialize threading system
	thread_deinitialize();

	//Mark that X-Space is not loaded
	xspace_initialized_all = 0;
}

void xspace_update(float dt)
{
	int i;

	//Update current planet and coordinate system
	planet_update(dt);
	coordsys_update(dt);

	//Read vessels
	for (i = 0; i < vessel_count; i++) {
		vessels[i].mount.checked = 0;
	}

	//Update/reset physics state
	for (i = 0; i < vessel_count; i++) vessels_update_physics(&vessels[i]);

	//Update various states
	geomagnetic_update();
	physics_update(dt);
	//particles_update(dt);

	//Simulate physics for vessels
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists && (vessels[i].physics_type == VESSEL_PHYSICS_INERTIAL)) {
			vessels_reset_physics(&vessels[i]);
			atmosphere_simulate(&vessels[i]);
		}
	}

	//Simulate physics which are called for all vessels
	if (dt < 1.0/10.0) radiosys_update(dt);
	//engines_simulate(dt);
	dragheat_simulate(dt);
	//launchpads_simulate(dt);
	//camera_simulate();
	//FIXME
	{
		extern double current_mjd;
		solsystem_update(current_mjd);
	}

	//Update Lua
	if (highlevel_pushcallback("OnFrame")) {
		lua_pushnumber(L,dt);
		highlevel_call(1,0);
	}

	//Finish physics simulation by integration
	//if (XPLMGetDataf(dataref_vessel_agl) > 100) { //Hopefully there are no mountains higher than 395,000 ft
	for (i = 0; i < vessel_count; i++) {
		if ((vessels[i].exists) && (vessels[i].physics_type == VESSEL_PHYSICS_INERTIAL)) {
			physics_integrate(dt,&vessels[i]);
		}
	}

	//Check timeout
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists && 
			(vessels[i].physics_type == VESSEL_PHYSICS_NONINERTIAL) &&
			(curtime() - vessels[i].last_update > 3.0)) {
			vessels[i].physics_type = VESSEL_PHYSICS_INERTIAL;
			vessels_set_ni(&vessels[i]);
		}
	}

	//Synchronize all coordinates (sim, inertial)
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists) {
			double r;
			vessels_get_ni(&vessels[i]);

			r = sqrt(vessels[i].noninertial.x*vessels[i].noninertial.x+
			vessels[i].noninertial.y*vessels[i].noninertial.y+
			vessels[i].noninertial.z*vessels[i].noninertial.z);
			vessels[i].elevation = r-current_planet.radius;
			vessels[i].latitude = 90-DEG(acos(vessels[i].noninertial.z/r));
			vessels[i].longitude = DEG(atan2(vessels[i].noninertial.y/r,vessels[i].noninertial.x/r));

			xivss_simulate(&vessels[i],dt);

			//vessels_update_coordinates(&vessels[i]);
			if (vessels[i].physics_type == VESSEL_PHYSICS_INERTIAL) {
				//printf("%d %f %f %f\n",vessels[i].net_id,vessels[i].elevation,vessels[i].latitude,vessels[i].longitude);
				if (vessels[i].elevation < 100e3) {
					if (vessels[i].ivss_system) xivss_deinitialize(&vessels[i]);
					vessels[i].exists = 0;
					printf("X-Space: Vessel %05d reentered atmosphere\n",vessels[i].net_id);
				}
			}
		}
	}

	//Finish physics simulation by integration
	//if (XPLMGetDataf(dataref_vessel_agl) > 100) { //Hopefully there are no mountains higher than 395,000 ft
	//for (i = 0; i < vessel_count; i++) if (vessels[i].exists) physics_integrate(dt,&vessels[i]);
	//}
}

//==============================================================================
// Log something to console
//==============================================================================
void log_write(char* text, ...)
{
	char buf[ARBITRARY_MAX] = { 0 };
	va_list args;

	va_start(args, text);
	vsnprintf(buf,ARBITRARY_MAX-1,text,args);
	printf(buf);
	va_end(args);
}