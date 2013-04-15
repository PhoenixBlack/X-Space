#ifndef HIGHLEVEL_H
#define HIGHLEVEL_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

struct vessel_tag;

extern lua_State* L;
extern int highlevel_table_datarefs;
extern int highlevel_table_vessels;
extern int highlevel_table_vessels_data;

void highlevel_initialize();
void highlevel_deinitialize();
void highlevel_load_aircraft(struct vessel_tag* v, char* acfpath, char* acfname);

int highlevel_panic(lua_State* L);
int highlevel_print(lua_State* L);
int highlevel_curtime(lua_State* L);
int highlevel_dataref(lua_State* L);
int highlevel_lua_load(lua_State* L);

void highlevel_addfunction(char* lib, char* name, void* ptr);
int highlevel_call(int nargs, int nresults);
int highlevel_load(char* filename);
int highlevel_pushcallback(char* name);
void highlevel_showerror(char* text);

void** highlevel_newptr(char* type, void* ptr);
#define highlevel_getptr(L,n) (*(void**)(lua_touserdata(L,n)))
#define highlevel_setptr(L,n,ptr) (*(void**)(lua_touserdata(L,n)) = (void*)ptr)
#define highlevel_checkzero(L,n) if (*(void**)(lua_touserdata(L,n)) == 0) { luaL_argerror(L,n,"bad reference/pointer"); }

#endif
