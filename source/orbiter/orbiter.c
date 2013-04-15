//==============================================================================
// X-Space resource management and base functions (orbiter)
//==============================================================================
#include <stdio.h>
#include <stdarg.h>
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
//#include "physics.h"     //Physics simulation
//#include "planet.h"      //Planet stuff
#include "network.h"     //High-level networking

//Resource management
int xspace_initialized_all = 0;

//==============================================================================
// Initializes all resources (global ones)
// First thing to be called when X-Space loads
//==============================================================================
void xspace_initialize_all()
{
	if (xspace_initialized_all) xspace_deinitialize_all();

	//Initialize highlevel stuff
	highlevel_initialize(); //allocates lua memory
	highlevel_load(FROM_PLUGINS("lua/initialize.lua")); //allocates lua memory
	config_initialize(); //allocates lua memory, loads configuration
	//gui_initialize(); //allocates lua memory, menu items
	network_initialize(); //mem alloc, lua mem

	//Initializers with no mem alloc:
	//planet_initialize(); //datarefs
	atmosphere_initialize(); //datarefs
	vessels_initialize(); //datarefs, lua memory
	coordsys_initialize(); //datarefs
	//physics_initialize(); //datarefs

	//Initializers with mem alloc:
	geomagnetic_initialize(); //load model, datarefs
	material_initialize(); //load database

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

	//Highlevel deinitialization
	network_deinitialize(); //free mem, lua mem
	//gui_deinitialize(); //free menu items
	highlevel_deinitialize(); //free lua memory

	//Mark that X-Space is not loaded
	xspace_initialized_all = 0;
}

void xspace_update(float dt)
{
	//int i;

	//Update/reset physics state
	//for (i = 0; i < vessel_count; i++) {
	//vessels_update_physics(&vessels[i]);
	//}

	//Update stuff
	//planet_update(dt);
	//coordsys_update(dt);
	//geomagnetic_update();
	//physics_update(dt);

	//Simulate physics for vessels
	//for (i = 0; i < vessel_count; i++) {
	//if (vessels[i].exists) {
	//vessels_reset(&vessels[i]);
	//atmosphere_simulate(&vessels[i]);
	//dragheat_simulate(&vessels[i],dt);
	//}
	//}

	//Update Lua
	if (highlevel_pushcallback("Frame")) {
		lua_pushnumber(L,dt);
		highlevel_call(1,0);
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
	va_list args;
	FILE* log = fopen("x-orbiter_log.txt","a+");

	va_start(args, text);
	vfprintf(log,text,args);
	va_end(args);

	fclose(log);
}