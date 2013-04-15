//==============================================================================
// High-level support
//==============================================================================
#include <string.h>
#include "x-space.h"
#include "highlevel.h"
#include "curtime.h"
#include "vessel.h"

//X-Plane SDK
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include "dataref.h"
#include <XPLMDataAccess.h>
#endif

//LuaSocket
#if defined(DEDICATED_SERVER)
#include <luasocket.h>
#endif

//Global lua state
lua_State* L = 0;
int highlevel_table_datarefs;
int highlevel_table_vessels;
int highlevel_table_vessels_data;




//==============================================================================
// Load all scripts
//==============================================================================
void highlevel_bind_functions()
{
	//Add global functions
	highlevel_addfunction(0,"Include",highlevel_lua_load);
	highlevel_addfunction(0,"print",highlevel_print);
	highlevel_addfunction(0,"curtime",highlevel_curtime);
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	highlevel_addfunction(0,"Dataref",highlevel_dataref);
#endif

	//lua_pushcfunction(L, highlevel_addbody);
	//lua_setglobal(L,"AddBody");
	//lua_pushcfunction(L, highlevel_setnetworkedmodel);
	//lua_setglobal(L,"SetNetworkedModel");

	//Set some constants and tables
	lua_pushstring(L,FROM_PLUGINS(""));
	lua_setglobal(L,"PluginsFolder");
	lua_createtable(L,0,16);
	lua_setglobal(L,"InternalCallbacks"); //Storage for internal callbacks
#ifdef DEDICATED_SERVER
	lua_pushboolean(L,1);
	lua_setglobal(L,"DEDICATED_SERVER"); //Storage for internal callbacks
	luaopen_socket_core(L);
#endif
#ifdef ORBITER_MODULE
	lua_pushboolean(L,1);
	lua_setglobal(L,"ORBITER_MODULE"); //Storage for internal callbacks
#endif
}

void highlevel_initialize()
{
	//Create new lua state
	L = luaL_newstate();
	lua_atpanic(L,&highlevel_panic);

	//Create table for storing datarefs
	lua_createtable(L,0,32);
	highlevel_table_datarefs = luaL_ref(L,LUA_REGISTRYINDEX);
	lua_createtable(L,16,16);
	highlevel_table_vessels = luaL_ref(L,LUA_REGISTRYINDEX);
	lua_createtable(L,16,16);
	highlevel_table_vessels_data = luaL_ref(L,LUA_REGISTRYINDEX);

	//Open libraries
	luaL_openlibs(L);

	//Bind all the default functions
	highlevel_bind_functions();
}


//==============================================================================
// Unload all scripts, clear Lua state
//==============================================================================
void highlevel_deinitialize()
{
	if (L) lua_close(L);
	L = 0;
}



//==============================================================================
// Environment
//==============================================================================
void highlevel_initialize_aircraft_environment(struct vessel_tag* v, char* acf_path, char* acf_name)
{
	//Create new environment for aircraft stuff
	lua_pushvalue(L,LUA_GLOBALSINDEX); //Store correct globals table
	lua_createtable(L,16,16); //Create new environment
	lua_replace(L,LUA_GLOBALSINDEX);

	//Work with new env
	highlevel_bind_functions();

	//Constants
	lua_pushstring(L,acf_path);
	lua_setglobal(L,"AircraftFolder");
	lua_pushstring(L,acf_name);
	lua_setglobal(L,"AircraftName");

	//Tables
	lua_createtable(L,16,16); //Engines (RCS, rockets)
	lua_setglobal(L,"Engine");
	lua_createtable(L,16,16); //Surface sensors (heating, pressure)
	lua_setglobal(L,"SurfaceSensors");

	//Data passed from VesselAPI.Add() call
	{
		//int t = lua_gettop(L);
		lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels_data);
		lua_rawgeti(L,-1,v->index);
		if (lua_istable(L,-1)) { //Add all entries to global table
			lua_pushnil(L);
			while (lua_next(L,-2)) {
				lua_pushvalue(L,-2);
				lua_pushvalue(L,-2);
				lua_rawset(L,LUA_GLOBALSINDEX);
				//if (lua_istable(L,-1)) {

				//Remove value, only leave key
				lua_pop(L,1);
			}
			lua_pop(L,1);
		} else {
			lua_setglobal(L,"Parameter");
		}
		lua_pop(L,1);
		//t = lua_gettop(L);
		//t += 1;
	}

	//Shared functions/libs
	lua_getfield(L,-1,"tostring");	lua_setglobal(L,"tostring");
	lua_getfield(L,-1,"tonumber");	lua_setglobal(L,"tonumber");
	lua_getfield(L,-1,"pairs");		lua_setglobal(L,"pairs");
	lua_getfield(L,-1,"ipairs");	lua_setglobal(L,"ipairs");
	lua_getfield(L,-1,"math");		lua_setglobal(L,"math");
	lua_getfield(L,-1,"string");	lua_setglobal(L,"string");
	lua_getfield(L,-1,"VesselAPI");
	lua_setglobal(L,"VesselAPI");
	//lua_getfield(L,-1,"LaunchPadsAPI");
	//lua_setglobal(L,"LaunchPadsAPI");

	lua_pushnumber(L,v->index);
	lua_setglobal(L,"CurrentVessel");

	//Store into vessels table
	lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels);
	lua_pushvalue(L,LUA_GLOBALSINDEX);
	lua_rawseti(L,-2,v->index);
	lua_pop(L,1); //Push vessels table

	lua_replace(L,LUA_GLOBALSINDEX); //Restore correct globals table
}




//==============================================================================
// Load aircraft-specific Lua
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void highlevel_load_aircraft(struct vessel_tag* v, char* acfpath, char* acfname)
{
	char filename[MAX_FILENAME] = { 0 };
	int err;

	//Create environment for aircraft
	highlevel_initialize_aircraft_environment(v,acfpath,acfname);

	//Check whether aircraft is loaded
	if (acfpath[0]) {
		int load_failed = 0;

		//Try to load file from two paths
		snprintf(filename,MAX_FILENAME-1,"%s/%s.lua",acfpath,acfname);
		if (err = luaL_loadfile(L,filename)) {
			if (err != LUA_ERRFILE) log_write("X-Space: Error while loading config file: %s\n",lua_tostring(L,-1));
			lua_pop(L,1);

			//Load on another path
			snprintf(filename,MAX_FILENAME-1,"%s/x-space.lua",acfpath);
			if (err = luaL_loadfile(L,filename)) {
				if (err != LUA_ERRFILE) log_write("X-Space: Error while loading config file: %s\n",lua_tostring(L,-1));
				lua_pop(L,1);

				load_failed = 1;
			} else {
				log_write("X-Space: Loading aircraft specific configuration from %s\n",filename);
			}
		} else {
			log_write("X-Space: Loading aircraft specific configuration from %s\n",filename);
		}

		//Try to load and run the script
		if (!load_failed) {
			//Signal loading
			//log_write("X-Space: Loading configuration for \"%s.acf\"...\n",acfname);

			//Run script
			lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels);
			lua_rawgeti(L,-1,v->index);
			lua_setfenv(L,-3); //Set aicraft environment
			lua_pop(L,1); //Pop vessels table
			highlevel_call(0,0);

			//Run initialization for this script
			if (highlevel_pushcallback("OnVesselInitialize")) {
				lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_vessels);
				lua_rawgeti(L,-1,v->index);
				lua_remove(L,-2);
				highlevel_call(1,0);
			}
		}
	}
}
#endif




//==============================================================================
// Add a function to highlevel interface
//==============================================================================
void highlevel_addfunction(char* lib, char* name, void* ptr)
{
	if (lib) {
		lua_getglobal(L,lib);
		if (lua_istable(L,-1)) {
			lua_pushcfunction(L,ptr);
			lua_setfield(L,-2,name);
		}
		lua_pop(L,1);
	} else {
		lua_pushcfunction(L,ptr);
		lua_setglobal(L,name);
	}
}


//==============================================================================
// Fatal error in Lua code
//==============================================================================
int highlevel_panic(lua_State* L)
{
	log_write("X-Space: Catastrophic lua error: %s\n",lua_tostring(L,-1));

	//Breakpoint
#ifdef _DEBUG
	_asm {int 3}
#endif
	return 0;
}


//==============================================================================
// Lua wrapper for print
//==============================================================================
int highlevel_print(lua_State* L)
{
	int n = lua_gettop(L);
	int i;
	char buffer[ARBITRARY_MAX];
	char* buf = buffer;
	buffer[0] = 0;

	lua_getglobal(L, "tostring");
	for (i = 1; i <= n; i++) {
		const char* str;
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);
		str = lua_tostring(L, -1);
		if (strlen(str)+strlen(buffer) < ARBITRARY_MAX) {
			strcpy(buf,str);
			buf = buf + strlen(buf);
			buf[0] = '\t';
			buf = buf + 1;
			buf[0] = 0;
		}
		lua_pop(L, 1);
	}
	buffer[strlen(buffer)-1] = '\n';
	log_write(buffer);
	return 0;
}


//==============================================================================
// Lua wrapper for print
//==============================================================================
int highlevel_curtime(lua_State* L)
{
	lua_pushnumber(L,curtime());
	return 1;
}


//==============================================================================
// Read/write dataref from Lua
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
int highlevel_dataref(lua_State* L)
{
	//Check if need to write
	int writeDataref = 0;
	if (lua_isnumber(L,2)) writeDataref = 1;

	//Check if parameter is a function
	if (lua_isfunction(L,1)) {
		lua_pushvalue(L,1); //function
		if (writeDataref) {
			lua_pushvalue(L,2); //parameter
			highlevel_call(1,0);
		} else {
			highlevel_call(0,1);
		}
	} else if (lua_isnumber(L,1)) { //Or just a number
		if (!writeDataref) {
			lua_pushvalue(L,1); //Return the number
		}
	} else if (lua_isstring(L,1)) { //String (actual dataref to read/write)
		XPLMDataRef dataref = 0;
		char datarefName[ARBITRARY_MAX] = { 0 };
		char fullName[ARBITRARY_MAX] = { 0 };
		int arrayIndex = -1;
		char* arrayIndexStart;

		//Fetch full dataref name
		strncpy(fullName,lua_tostring(L,1),ARBITRARY_MAX-1);

		//Parse out array index
		arrayIndexStart = strrchr(fullName,'[');
		if (arrayIndexStart) {
			strncpy(datarefName,fullName,arrayIndexStart-fullName);
	//		snscanf(arrayIndexStart+1,ARBITRARY_MAX-1,"%d",&arrayIndex);
			sscanf(arrayIndexStart+1,"%d",&arrayIndex); //FIXME: KNOWN POSSIBLE SCANNING PROBLEM
		} else {
			strncpy(datarefName,fullName,ARBITRARY_MAX-1);
		}

		//Read datarefs global table
		lua_rawgeti(L,LUA_REGISTRYINDEX,highlevel_table_datarefs);

		//Check if the dataref exists
		lua_getfield(L,-1,datarefName);
		if (lua_islightuserdata(L,-1)) {
			dataref = (XPLMDataRef)lua_touserdata(L,-1);
			lua_pop(L,1); //Pop userdata field
		} else {
			dataref = XPLMFindDataRef(datarefName);
			lua_pop(L,1); //Pop nil userdata

			//Check if dataref must be created
			if ((!dataref) && (writeDataref)) {
				double* v = (double*)malloc(sizeof(double));
				*v = 0.0;
				dataref = dataref_d(datarefName,v);
			}

			//Remember pointer to dataref
			lua_pushlightuserdata(L,(void*)dataref);
			lua_setfield(L,-2,fullName);
		}

		//Pop datarefs table
		lua_pop(L,1);

		//Write or read from dataref
		if (dataref) {
			if (writeDataref) {
				if (arrayIndex >= 0) {
					float f = (float)lua_tonumber(L,2);
					XPLMSetDatavf(dataref,&f,arrayIndex,1);
				} else {
					XPLMSetDataf(dataref,(float)lua_tonumber(L,2));
				}
			} else {
				if (arrayIndex >= 0) {
					float f;
					XPLMGetDatavf(dataref,&f,arrayIndex,1);
					lua_pushnumber(L,f);
				} else {
					lua_pushnumber(L,XPLMGetDataf(dataref));
				}
			}
		} else {
			if (!writeDataref) lua_pushnumber(L,0);
		}
	}

	//Return if there's any result on stack
	return !writeDataref;
}
#endif


//==============================================================================
// Read/write dataref from Lua
//==============================================================================
int highlevel_call(int nargs, int nresults)
{
	if (lua_pcall(L,nargs,nresults,0)) {
		const char* error_message = lua_tostring(L,-1);
		log_write("X-Space: Error in lua script: %s\n",lua_tostring(L,-1));
		lua_pop(L,1);

		//Breakpoint
#ifdef _DEBUG
		_asm {int 3}
#endif
		return 1;
	} else {
		return 0;
	}
}

//==============================================================================
// Load and execute a file
//==============================================================================
int highlevel_load(char* filename)
{
	if (luaL_loadfile(L,filename)) {
		const char* error_message = lua_tostring(L,-1);
		log_write("X-Space: Error while loading: %s\n",lua_tostring(L,-1));
		lua_pop(L,1);

		//Breakpoint
#ifdef _DEBUG
		_asm {int 3}
#endif
		return 0;
	} else {
		highlevel_call(0,0);
		return 1;
	}
}

int highlevel_lua_load(lua_State* L)
{
	luaL_checktype(L,1,LUA_TSTRING);
	highlevel_load((char*)lua_tostring(L,1));
	return 0;
}

//==============================================================================
// Create a new pointer with type (pointer or arbitrary integer)
//==============================================================================
static const luaL_reg highlevel_type_metatable[] = {
	{ NULL,     NULL }
};

void** highlevel_newptr(char* type, void* ptr)
{
	void** p = lua_newuserdata(L,sizeof(void*));
	*p = ptr;

	//Create metatable for type if it doesn't exist yet
	//luaL_getmetatable(L,type);
	//if (lua_isnil(L,-1)) {
	//lua_pop(L,1);
//		luaL_newmetatable(L,type);
	//}
	luaL_newmetatable(L,type);
	lua_setmetatable(L,-2);
	return p;
}


//==============================================================================
// Push internal callback function
//==============================================================================
int highlevel_pushcallback(char* name)
{
	lua_getglobal(L,"InternalCallbacks");
	if (lua_istable(L,-1)) {
		lua_getfield(L,-1,name);
		lua_remove(L,-2); //Remove table from stack
		if (lua_isfunction(L,-1)) {
			return 1; //only function remains on stack
		}
	}
	lua_pop(L,1); //pop table or function
	return 0;
}


//==============================================================================
// Show error box
//==============================================================================
void highlevel_showerror(char* text) {
	if (highlevel_pushcallback("OnErrorMessage")) {
		lua_pushstring(L,text);
		highlevel_call(1,0);
	}
}