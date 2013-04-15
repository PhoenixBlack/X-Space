//==============================================================================
// X-Space resource management and base functions
//==============================================================================
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <enet/enet.h>

//-- Code modules --------------------------------------------------------------
#include "x-space.h"     //X-Space resource management
#include "x-plane.h"     //Interface to X-Plane
#include "highlevel.h"   //Highlevel code support
#include "config.h"      //Plugin configuration
#include "gui.h"         //Highlevel GUI
#include "vessel.h"      //Vessels system
#include "atmosphere.h"  //Atmosphere physics
#include "coordsys.h"    //Coordinate systems
#include "geomagnetic.h" //Correct geomagnetic model
#include "material.h"    //Material simulation
#include "physics.h"     //Physics simulation
#include "planet.h"      //Planet stuff
#include "network.h"     //High-level networking
#include "rendering.h"   //OpenGL rendering and shaders
#include "engine.h"      //Engine simulation
#include "particles.h"   //Particles rendering
#include "dragheat.h"    //Drag and heat simulation
#include "launchpads.h"  //Launch pads simulation
#include "camera.h"      //Extra camera views
#include "radiosys.h"    //Radio transmission simulation
#include "solpanels.h"   //Solar panel simulation
#include "x-ivss.h"      //Internal vessels systems simulator

#include "curtime.h"     //Current time (precise)
#include "threading.h"   //Multithreading support

//X-Plane SDK
#include <XPLMPlanes.h>
#include <XPLMUtilities.h>
#include <XPLMScenery.h>

//Resource management
int xspace_initialized_all = 0;

//==============================================================================
// Initializes all resources (global ones)
// First thing to be called when X-Space loads
//==============================================================================
void xspace_initialize_all()
{
#ifdef PROFILING
	//__itt_resume();
#endif
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
	engines_initialize(); //datarefs
	camera_initialize(); //hotkeys

	//Initializers with mem alloc:
	geomagnetic_initialize(); //load model, datarefs
	material_initialize(); //load database
	rendering_initialize(); //textures
	launchpads_initialize(); //memory
	radiosys_initialize(); //memory, datarefs
	solpanels_initialize(); //textures

	//Initialize various highlevel scripts
	network_initialize(); //mem alloc, lua mem
	dragheat_highlevel_initialize(); //lua mem
	launchpads_highlevel_initialize(); //lua mem
	xivss_highlevel_initialize(); //lua mem
	gui_initialize(); //allocates lua memory, menu items, must be last

	//All resources now initialized
	xspace_initialized_all = 1;
#ifdef PROFILING
	//__itt_pause();
#endif
}

void xspace_deinitialize_all()
{
	int i;

	//Do not free resources if not allocated
	if (!xspace_initialized_all) return;

	//Deinitializers with no mem free:
	camera_deinitialize(); //hotkeys

	//Deinitializers with mem free:
	for (i = 0; i < vessel_count; i++) xspace_unload_aircraft(i); //aircraft stuff
	material_deinitialize(); //free model
	geomagnetic_deinitialize(); //free model
	rendering_deinitialize(); //free textures
	vessels_deinitialize(); //free memory, lua memory
	radiosys_deinitialize(); //free memory, datarefs

	//Highlevel deinitialization
	network_deinitialize(); //free mem, lua mem
	gui_deinitialize(); //free menu items
	highlevel_deinitialize(); //free lua memory

	//Deinitialize threading system
	thread_deinitialize();

	//Mark that X-Space is not loaded
	xspace_initialized_all = 0;
}

void xspace_reinitialize()
{
	int i;

	//Unload old resources
	for (i = 0; i < vessel_count; i++) xspace_unload_aircraft(i); //aircraft stuff

	//Reinitialize resources
	vessels_reinitialize(); //vessels reset
	coordsys_reinitialize(); //history

	//Reload users aircraft
	xspace_reload_aircraft(0);

	//Update X-Plane aircraft
	vessels_update_aircraft();
}

void xspace_reload_aircraft(int index)
{
	char path[MAX_FILENAME] = { 0 }, model[MAX_FILENAME] = { 0 };
	char buf[ARBITRARY_MAX] = { 0 };
	char* acfpath;

	//Read aircraft path and path separator
	XPLMGetNthAircraftModel(index, model, path);
	XPLMExtractFileAndPath(path);

	//If not yet loading anything, skip
	if (!model[0]) return;

	//Deinitialize old ACF resources
	xspace_unload_aircraft(index);

	//Report state
	log_write("X-Space: Reloading configuration for %s\n",model);

	//Fetch only aircraft folder and replace paths under Mac OS
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

	//Initialize aircraft
	xspace_load_aircraft(index,path,model);
}

void xspace_load_aircraft(int index, char* acfpath, char* acfname)
{
	//Load drag and heating
	dragheat_initialize(&vessels[index],acfpath,acfname);
	//Load Lua code
	highlevel_load_aircraft(&vessels[index],acfpath,acfname); //inits lua
	//Load sensors
	dragheat_initialize_highlevel(&vessels[index]);
	//Load solar panels
	solpanels_initialize_highlevel(&vessels[index]);
	//Initialize radio system
	radiosys_initialize_vessel(&vessels[index]);
	//Initialize internal vessels systems simulator
	xivss_initialize_vessel(&vessels[index],acfpath,acfname);

	{
		//extern int dragheat_model_loadobj(vessel* v);
		//dragheat_model_loadobj(&vessels[index]);
	}
}

void xspace_unload_aircraft(int index)
{
	//Deinitialize stuff
	dragheat_deinitialize(&vessels[index]); //free geometry, sensors
	radiosys_deinitialize_vessel(&vessels[index]); //free memory
	xivss_deinitialize(&vessels[index]);
}

void xspace_update(float dt)
{
	int i;
#ifdef PROFILING
	__itt_resume();
#endif

	//Update current planet and coordinate system
	planet_update(dt);
	coordsys_update(dt);

	//Read vessels
	for (i = 0; i < vessel_count; i++) {
		if ((!vessels[i].networked) && (vessels[i].physics_type != VESSEL_PHYSICS_DISABLED)) {
			vessels_read(&vessels[i],vessels[i].physics_type != VESSEL_PHYSICS_INERTIAL);
			vessels[i].mount.checked = 0;
		}
	}

	//Update/reset physics state
	for (i = 0; i < vessel_count; i++) vessels_update_physics(&vessels[i]);

	//Check if inertial physics must be enabled
	for (i = 0; i < vessel_count; i++) {
		//Only check existing vessels, and do not check if it's vessel 0 standing on launch pad
		if ((vessels[i].exists) && 
			(vessels[i].physics_type != VESSEL_PHYSICS_NONINERTIAL) &&
			(vessels[i].physics_type != VESSEL_PHYSICS_DISABLED)) {
			if (vessels[i].is_plane) { //Boosters are always in inertial physics THANKS AUSTIN
				vessels[i].physics_type = VESSEL_PHYSICS_INERTIAL;
			} else {
				if ((vessels[i].elevation > 120500) && (vessels[i].physics_type == VESSEL_PHYSICS_SIM)) {
					vessels[i].physics_type = VESSEL_PHYSICS_INERTIAL;
				}
				if ((vessels[i].elevation < 120000) && (vessels[i].physics_type == VESSEL_PHYSICS_INERTIAL)) {
					vessels[i].physics_type = VESSEL_PHYSICS_INERTIAL_TO_SIM;
				}

				if (i == 0) { //X-Plane specific hack
					if ((vessels[i].detach_time > 0.0) && ((curtime() - vessels[i].detach_time) < config.staging_wait_time)) {
						vessels[i].physics_type = VESSEL_PHYSICS_INERTIAL;
					}
				}
			}
		}

		//Disable inertial physics
		if (!config.use_inertial_physics) {
			vessels[i].physics_type = VESSEL_PHYSICS_INERTIAL;
		}

		//Allow placing the main vessel using local map if simulator is paused
		//if ((i == 0) && (dt == 0.0)) vessels_read(&vessels[i],1);
	}

	//Update various states
	geomagnetic_update();
	physics_update(dt);
	particles_update(dt);

	//Simulate physics for vessels
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists && (vessels[i].physics_type != VESSEL_PHYSICS_DISABLED)) {
			vessels_reset_physics(&vessels[i]);
			atmosphere_simulate(&vessels[i]);
		}
	}

	//Simulate physics which are called for all vessels
	//radiosys_update(dt);
	engines_simulate(dt);
	dragheat_simulate(dt);
	launchpads_simulate(dt);
	camera_simulate();

	//Update Lua
	if (highlevel_pushcallback("OnFrame")) {
		lua_pushnumber(L,dt);
		highlevel_call(1,0);
	}
	vessels_highlevel_logic();

	//Finish physics simulation by integration
	//if (XPLMGetDataf(dataref_vessel_agl) > 100) { //Hopefully there are no mountains higher than 395,000 ft
	for (i = 0; i < vessel_count; i++) {
		if ((vessels[i].exists) && 
			(vessels[i].physics_type != VESSEL_PHYSICS_DISABLED) &&
			(vessels[i].attached == 0)) {
			physics_integrate(dt,&vessels[i]);
		}
	}
	//}

	//Update mounting physics (FIXME: should not require two coordinate updates! bug!)
	for (i = 0; i < vessel_count; i++) if (vessels[i].exists) vessels_update_coordinates(&vessels[i]);
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].exists) vessels_update_mount_physics(&vessels[i]);
	}

	//Synchronize all coordinates (sim, inertial)
	for (i = 0; i < vessel_count; i++) if (vessels[i].exists) vessels_update_coordinates(&vessels[i]);

	//Write back information
	for (i = 0; i < vessel_count; i++) {
		if ((vessels[i].exists) && (!vessels[i].networked) && (vessels[i].physics_type != VESSEL_PHYSICS_DISABLED)) {
			//Do not write focused vessel if simulator is paused
			if ((i == 0) && (dt == 0.0)) {
				//FIXME: problem
				vessels_write(&vessels[i]);
			} else {
				vessels_write(&vessels[i]);
			}
		}
		coordsys_update_datarefs(&vessels[0],&vessels[i]);

		//Update internal systems
		xivss_simulate(&vessels[i],dt);

		//Transfer physics from inertial to simulator (with updating coordinates and velocities)
		if (vessels[i].physics_type == VESSEL_PHYSICS_INERTIAL_TO_SIM) {
			vessels[i].physics_type = VESSEL_PHYSICS_SIM;
		}

		//Check if vessel falls underground
		if ((i > 0) && vessels[i].exists) {
			double d = sqrt(vessels[i].inertial.x*vessels[i].inertial.x+
			                vessels[i].inertial.y*vessels[i].inertial.y+
			                vessels[i].inertial.z*vessels[i].inertial.z);
			if (d < current_planet.radius) {
				vessels_get_ni(&vessels[i]);
				vessels[i].noninertial.vx = 0;
				vessels[i].noninertial.vy = 0;
				vessels[i].noninertial.vz = 0;
				vessels[i].noninertial.P = 0;
				vessels[i].noninertial.Q = 0;
				vessels[i].noninertial.R = 0;
				vessels_set_ni(&vessels[i]);
			}
		}
	}

#ifdef PROFILING
	__itt_pause();
#endif
}