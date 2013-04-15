//==============================================================================
// Global plugin configuration
//==============================================================================
#include <string.h>
#include <math.h>

#include "x-space.h"
#include "highlevel.h"
#include "config.h"

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include "rendering.h"
#include <XPLMUtilities.h>
#endif

//Macro to read/write a single option
#define READ_OPTION(name,cfgname,type,defaultval) \
	lua_getglobal(L,cfgname); \
	if (!lua_isnil(L,-1)) { \
		config.name = lua_to##type(L,-1); \
	} else { \
		config.name = defaultval; \
	} \
	lua_pop(L,1);

#define WRITE_OPTION(name,cfgname,type,defaultval) \
	if (type == 0) { \
		fprintf(out,"%s = %s\n",cfgname,(config.name ? "true" : "false")); \
	} else if (type == 1) { \
		fprintf(out,"%s = %d\n",cfgname,config.name); \
	} else if (type == 2) { \
		fprintf(out,"%s = %f\n",cfgname,config.name); \
	}

//Macro that lists all options for reading/write
#define LIST_OPTIONS(config_macro) \
	config_macro(max_vessels,			"MaxVessels",				integer,128) \
	\
	config_macro(write_atmosphere,		"WriteAtmosphere",			boolean,0) \
	config_macro(draw_coordsys,			"DrawCoordinateSystems",	boolean,0) \
	\
	config_macro(use_shaders,			"UseShaders",				boolean,1) \
	config_macro(use_clouds,			"UseClouds",				boolean,0) \
	config_macro(use_scattering,		"UseScattering",			boolean,(sim_version < 10000)) \
	config_macro(use_hq_clouds,			"UseHighQualityClouds",		boolean,0) \
	config_macro(cloud_shadows,			"CloudsCastShadows",		boolean,1) \
	config_macro(max_particles,			"MaxParticles",				integer,1024) \
	config_macro(reentry_glow,			"DrawReentryGlow",			boolean,1) \
	config_macro(engine_debug_draw,		"DrawEngineDebug",			boolean,0) \
	config_macro(sensor_debug_draw,		"DrawSensorDebug",			boolean,0) \
	config_macro(use_particles,			"UseParticles",				boolean,1) \
	\
	config_macro(use_inertial_physics,	"UseInertialPhysics",		boolean,1) \
	config_macro(nonspherical_gravity,	"NonsphericalGravity",		boolean,1) \
	config_macro(planet_rotation,		"PlanetRotates",			boolean,1) \
	config_macro(staging_wait_time,		"StagingWaitTime",			number, 60.0) \

//Global configuration
global_config config;
char config_filename[MAX_FILENAME];
//X-Plane version
int sim_version;
int sim_sdk_version;
int sim_host_id;

int config_highlevel_save(lua_State* L)
{
	config_save();
	return 0;
}

int config_highlevel_get(lua_State* L)
{
	int idx = lua_tointeger(L,1);
	switch (idx) {
		case  1: lua_pushnumber(L,config.use_shaders); break;
		case  2: lua_pushnumber(L,config.use_clouds); break;
		case  3: lua_pushnumber(L,config.use_scattering); break;
		case  4: lua_pushnumber(L,config.write_atmosphere); break;
		case  5: lua_pushnumber(L,config.use_hq_clouds); break;
		case  6: lua_pushnumber(L,config.cloud_shadows); break;
		case  7: lua_pushnumber(L,config.use_inertial_physics); break;
		case  8: lua_pushnumber(L,config.planet_rotation); break;
		case  9: lua_pushnumber(L,config.draw_coordsys); break;
		case 10: lua_pushnumber(L,config.reentry_glow); break;
		case 11: lua_pushnumber(L,config.nonspherical_gravity); break;
		case 12: lua_pushnumber(L,config.engine_debug_draw); break;
		case 13: lua_pushnumber(L,config.sensor_debug_draw); break;
		case 14: lua_pushnumber(L,config.use_particles); break;
		case 15: lua_pushnumber(L,config.staging_wait_time); break;
		default: lua_pushnumber(L,0); break;
	}
	return 1;
}

int config_highlevel_set(lua_State* L)
{
	int idx = lua_tointeger(L,1);
	switch (idx) {
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
		case  1: rendering_deinitialize(); config.use_shaders = lua_tointeger(L,2);		rendering_initialize(); break;
		case  2: rendering_deinitialize(); config.use_clouds = lua_tointeger(L,2);		rendering_initialize(); break;
		case  3: rendering_deinitialize(); config.use_scattering = lua_tointeger(L,2);	rendering_initialize(); break;
		case 14: rendering_deinitialize(); config.use_particles = lua_tointeger(L,2);	rendering_initialize(); break;
#endif
		case  4: config.write_atmosphere = lua_tointeger(L,2); break;
		case  5: config.use_hq_clouds = lua_tointeger(L,2); break;
		case  6: config.cloud_shadows = lua_tointeger(L,2); break;
		case  7: config.use_inertial_physics = lua_tointeger(L,2); break;
		case  8: config.planet_rotation = lua_tointeger(L,2); break;
		case  9: config.draw_coordsys = lua_tointeger(L,2); break;
		case 10: config.reentry_glow = lua_tointeger(L,2); break;
		case 11: config.nonspherical_gravity = lua_tointeger(L,2); break;
		case 12: config.engine_debug_draw = lua_tointeger(L,2); break;
		case 13: config.sensor_debug_draw = lua_tointeger(L,2); break;
		case 15: config.staging_wait_time = lua_tonumber(L,2); break;
		default: break;
	}
	return 0;
}

void config_initialize()
{
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	char* outputpath;
	XPLMGetVersions(&sim_version,&sim_sdk_version,&sim_host_id);
	XPLMGetPrefsPath(config_filename);
	XPLMExtractFileAndPath(config_filename);
	strncat(config_filename,"/X-Space.prf",MAX_FILENAME-1);

	outputpath = strstr(config_filename,"Output");
	if (outputpath) snprintf(config_filename,MAX_FILENAME-1,"./%s",outputpath);
#ifdef APL
	{
		char* tpath;
		while (tpath = strchr(config_filename,':')) *tpath = '/';
	}
#endif
#else
	sim_version = 0;
	sim_sdk_version = 0;
	sim_host_id = 0;
	strncpy(config_filename,FROM_PLUGINS("config.lua"),MAX_FILENAME-1);
#endif
	config_load();
	config_save();

	lua_createtable(L,1,0);
	lua_setglobal(L,"Config");
	highlevel_addfunction("Config","Save",config_highlevel_save);
	highlevel_addfunction("Config","Get",config_highlevel_get);
	highlevel_addfunction("Config","Set",config_highlevel_set);
}

void config_load()
{
	//Read all options from the file
	memset(&config,0,sizeof(config));

	//Create isolated environment
	lua_pushvalue(L,LUA_GLOBALSINDEX);
	lua_createtable(L,16,16);
	lua_replace(L,LUA_GLOBALSINDEX);

	//Load plugin configuration
	highlevel_load(config_filename);
	LIST_OPTIONS(READ_OPTION);

	//Restore correct globals table
	lua_replace(L,LUA_GLOBALSINDEX);
}

void config_save()
{
	FILE* out = fopen(config_filename,"w+");
#define boolean 0
#define integer 1
#define number	2
	LIST_OPTIONS(WRITE_OPTION);
#undef boolean
#undef integer
#undef number
	fclose(out);
}
