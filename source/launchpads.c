//==============================================================================
// Launch pads and other service objects
//==============================================================================
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "x-space.h"
#include "quaternion.h"
#include "launchpads.h"
#include "planet.h"
#include "dataref.h"
#include "vessel.h"
#include "highlevel.h"

//X-Plane SDK
#include <XPLMDisplay.h>
#include <XPLMDataAccess.h>
#include <XPLMGraphics.h>
//#include <XPLMUtilities.h>
//#include <XPLMPlanes.h>
//#include <XPLMPlugin.h>
//#include <XPLMProcessing.h>
#include <XPLMScenery.h>

#define DRAWINFO_PER_MODEL 32

//==============================================================================
// Various variables
//==============================================================================
//Pads and models
launch_pad* launchpads = 0;
launch_pad_model* launchpads_models;
XPLMObjectRef* launchpads_refs = 0;
XPLMDrawInfo_t** launchpads_drawinfo;
int* launchpads_drawinfo_count;
int launchpads_count;
int launchpads_model_count;

//Variables for datarefs
double launchpads_release;	//Should pad be released
double launchpads_locked;	//Dataref indicating whether pad is locked

//Terrain probe
XPLMProbeRef launchpads_probe;
//Release hotkey
XPLMHotKeyID launchpads_hotkey = 0;


//==============================================================================
// ID-based interface to vessel internal variables
//==============================================================================
double launchpads_read_param_d(int idx, int paramidx)
{
	switch (paramidx) {
		case  0: return (double)launchpads[idx].latitude;
		case  1: return (double)launchpads[idx].longitude;
		case  2: return (double)launchpads[idx].elevation;
		case  3: return (double)launchpads[idx].heading;
		case  4: return (double)launchpads[idx].planet;
		case  5: return (double)launchpads[idx].flags;
		case  6: return (double)launchpads[idx].model;
		default: return 0.0;
	}
}

void launchpads_write_param_d(int idx, int paramidx, double val)
{
	switch (paramidx) {
		case  0: launchpads[idx].latitude = (float)val; break;
		case  1: launchpads[idx].longitude = (float)val; break;
		case  2: launchpads[idx].elevation = (float)val; break;
		case  3: launchpads[idx].heading = (float)val; break;
		case  4: launchpads[idx].planet = (int)val; break;
		case  5: launchpads[idx].flags = (int)val; break;
		case  6: launchpads[idx].model = (int)val; break;
		default: break;
	}
}


//==============================================================================
// Dataref interface to vessel internal variables
//==============================================================================
static long launchpads_get_param_fv(void* dataptr, float* values, int inOffset, int inMax)
{
	int i;
	int paramidx = (int)dataptr;
	if (!values) return launchpads_count;

	for (i = inOffset; i < inOffset+inMax; i++) {
		if ((i >= 0) && (i < launchpads_count)) {
			values[i-inOffset] = (float)(launchpads_read_param_d(i,paramidx));
		}
	}
	return launchpads_count;
}

static void launchpads_set_param_fv(void* dataptr, float* values, int inOffset, int inMax)
{
	int i;
	int paramidx = (int)dataptr;
	if (!values) return;

	for (i = inOffset; i < inOffset+inMax; i++) {
		if ((i >= 0) && (i < launchpads_count)) {
			launchpads_write_param_d(i,paramidx,(double)values[i-inOffset]);
		}
	}
}

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
void launchpads_register_dataref(char* name, int paramidx)
{
	XPLMRegisterDataAccessor(
	    name,xplmType_FloatArray,1,0,0,0,0,0,0,0,0,
	    launchpads_get_param_fv,launchpads_set_param_fv,0,0,
	    (void*)paramidx,(void*)paramidx);
}
#endif


void launchpads_key_handler(void* ref)
{
	launchpads_set(&vessels[0],0);
}


void launchpads_initialize()
{
	FILE* f;
	char path[MAX_FILENAME] = { 0 }, filename[MAX_FILENAME] = { 0 }, buf[ARBITRARY_MAX] = { 0 };
	int pad_index,model_index;

	//Report state
	log_write("X-Space: Loading launch pads\n");

	//Read path to X-Space plugin
	snprintf(path,MAX_FILENAME-1,FROM_PLUGINS(""));

	//Load launch pads
	strcpy(filename,path);
	strcat(filename,"pads.dat");
	f = fopen(filename,"r");
	if (!f) log_write("X-Space: Failed to load launch pads\n");

	//Scan for number of models and pads
	launchpads_count = 0;
	launchpads_model_count = 0;
	while (f && (!feof(f))) {
		char tag[256];
		tag[0] = 0; fscanf(f,"%255s",tag); tag[255] = 0;
		if (strcmp(tag,"PAD") == 0) { //Load pad
			launchpads_count++;
		} else if (strcmp(tag,"MODEL") == 0) { //Load model
			launchpads_model_count++;
		}
		fgets(buf,ARBITRARY_MAX-1,f);
	}
	if (f) fseek(f,0,0);

	//Report info
	log_write("X-Space: Found %d launch pads, %d models\n",launchpads_count,launchpads_model_count);

	//Make sure there's at least one model
	if (launchpads_model_count == 0) launchpads_model_count = 1;

	//Read models and pads
	launchpads = (launch_pad*)malloc(sizeof(launch_pad)*launchpads_count);
	launchpads_refs = (XPLMObjectRef*)malloc(sizeof(XPLMObjectRef)*launchpads_model_count);
	launchpads_models = (launch_pad_model*)malloc(sizeof(launch_pad_model)*launchpads_model_count);
	launchpads_drawinfo = (XPLMDrawInfo_t**)malloc(sizeof(XPLMDrawInfo_t*)*launchpads_model_count);
	launchpads_drawinfo_count = (int*)malloc(sizeof(int)*launchpads_model_count);
	for (model_index = 0; model_index < launchpads_model_count; model_index++) {
		launchpads_drawinfo[model_index] = (XPLMDrawInfo_t*)malloc(sizeof(XPLMDrawInfo_t)*DRAWINFO_PER_MODEL);
	}

	//Read actual data
	launchpads_refs[0] = 0; //Make sure first model is at least zero
	pad_index = 0;
	model_index = 0;
	while (f && (!feof(f))) {
		char tag[256]; char* bufptr;
		tag[0] = 0; fscanf(f,"%255s",tag); tag[255] = 0;
		if (strcmp(tag,"PAD") == 0) { //Load pad
			fscanf(f,"%f %f %f %d %d %7s",
			       &launchpads[pad_index].latitude,
			       &launchpads[pad_index].longitude,
			       &launchpads[pad_index].heading,
			       &launchpads[pad_index].model,
			       &launchpads[pad_index].planet,
				   launchpads[pad_index].pad_name);

			launchpads[pad_index].elevation = 0.0f;
			fgets(launchpads[pad_index].complex_name,255,f);
			launchpads[pad_index].pad_name[7] = 0;
			launchpads[pad_index].complex_name[255] = 0;
			if ((launchpads[pad_index].model < 0) || (launchpads[pad_index].model >= launchpads_model_count)) {
				launchpads[pad_index].model = 0;
			}

			//Remove leading spaces
			bufptr = launchpads[pad_index].complex_name; while (*bufptr == ' ') bufptr++;
			strncpy(launchpads[pad_index].complex_name,bufptr,255);
			bufptr = launchpads[pad_index].pad_name; while (*bufptr == ' ') bufptr++;
			strncpy(launchpads[pad_index].pad_name,bufptr,7);

			//Remove trailing newlines
			bufptr = launchpads[pad_index].complex_name; 
			while (*bufptr) {
				if ((*bufptr == '\r') || (*bufptr == '\n')) {
					*bufptr = 0;
				}
				bufptr++;
			}
			bufptr = launchpads[pad_index].pad_name; 
			while (*bufptr) {
				if ((*bufptr == '\r') || (*bufptr == '\n')) {
					*bufptr = 0;
				}
				bufptr++;
			}

			//Go on to the next pad
			launchpads[pad_index].flags = 0;
			launchpads[pad_index].index = pad_index;
			if (pad_index+1 < launchpads_count) pad_index++;
		} else if (strcmp(tag,"MODEL") == 0) { //Load model
			//Read model data
			float r,p,y;
			fscanf(f,"%f %f %f %f %f %f %d",
			       &launchpads_models[model_index].x,
			       &launchpads_models[model_index].y,
			       &launchpads_models[model_index].z,
			       &r, &p, &y,
			       &launchpads_models[model_index].flags);

			qeuler_from(launchpads_models[model_index].q,RAD(r),RAD(p),RAD(y));

			//Read filename
			fgets(buf,ARBITRARY_MAX-1,f);
			strncpy(filename,path,MAX_FILENAME);
			strncat(filename,"objects",MAX_FILENAME);
			strncat(filename,"/",MAX_FILENAME);

			//Remove leading spaces
			bufptr = buf; while (*bufptr == ' ') bufptr++;
			strncat(filename,bufptr,MAX_FILENAME);
			//Remove trailing newlines
			bufptr = strstr(filename,".obj");
			*(bufptr+4) = 0;

			//Load object
			launchpads_refs[model_index] = XPLMLoadObject(filename);
			if (!launchpads_refs[model_index]) {
				log_write("X-Space: Error loading launch pad model %s!\n",filename);
			}
			if (model_index+1 < launchpads_model_count) model_index++;
		} else {
			fgets(buf,ARBITRARY_MAX-1,f);
		}
	}
	if (f) fclose(f);

	//Create terrain probe
	launchpads_probe = XPLMCreateProbe(xplm_ProbeY);

	//Create datarefs
	dataref_d("xsp/launchpads/local_release",&launchpads_release);
	dataref_d("xsp/launchpads/local_locked",&launchpads_locked);
	launchpads_register_dataref("xsp/launchpads/latitude",0);
	launchpads_register_dataref("xsp/launchpads/longitude",1);
	launchpads_register_dataref("xsp/launchpads/elevation",2);
	launchpads_register_dataref("xsp/launchpads/heading",3);
	launchpads_register_dataref("xsp/launchpads/planet",4);
	launchpads_register_dataref("xsp/launchpads/flags",5);
	launchpads_register_dataref("xsp/launchpads/model",6);
}


void launchpads_draw()
{
	int i;
	double reflat,reflon,refalt;

	//Locate (0,0,0) point
	XPLMLocalToWorld(0,0,0,&reflat,&reflon,&refalt);

	//Prepare drawing
	for (i = 0; i < launchpads_model_count; i++) launchpads_drawinfo_count[i] = 0;

	//Draw all pads that belong to current planet, are within certain range
	for (i = 0; i < launchpads_count; i++) {
		if ((launchpads[i].planet == current_planet.index) &&
		    (pow(launchpads[i].latitude-reflat,2)+pow(launchpads[i].longitude-reflon,2) < 1.5*1.5)) {
			int model = launchpads[i].model;
			XPLMDrawInfo_t* drawinfo;
			double x,y,z;

			//Make sure model is correct
			if (model < 0) model = 0;
			if (model >= launchpads_model_count) model = 0;

			//Fetch correct position
			{
				XPLMProbeInfo_t probeinfo;
				probeinfo.structSize = sizeof(XPLMProbeInfo_t);
				XPLMWorldToLocal(launchpads[i].latitude,launchpads[i].longitude,0,&x,&y,&z);
				if (XPLMProbeTerrainXYZ(launchpads_probe,(float)x,(float)y,(float)z,&probeinfo) == xplm_ProbeHitTerrain) {
					XPLMLocalToWorld(x,probeinfo.locationY,z,&x,&y,&launchpads[i].elevation);
					launchpads[i].flags |= LAUNCHPADS_FLAG_KNOWNPOSITION;
				}
			}

			//Add to drawing list
			drawinfo = &launchpads_drawinfo[model][launchpads_drawinfo_count[model]];
			if (launchpads_drawinfo_count[model] < DRAWINFO_PER_MODEL) launchpads_drawinfo_count[model]++;

			//Setup draw info structure
			XPLMWorldToLocal(launchpads[i].latitude,launchpads[i].longitude,launchpads[i].elevation,&x,&y,&z);
			drawinfo->structSize = sizeof(XPLMDrawInfo_t);
			drawinfo->x = (float)x;
			drawinfo->y = (float)y;
			drawinfo->z = (float)z;
			drawinfo->pitch = 0;
			drawinfo->heading = launchpads[i].heading;
			drawinfo->roll = 0;
		}
	}

	//Draw all pads
	for (i = 0; i < launchpads_model_count; i++) {
		if (launchpads_refs[i] && launchpads_drawinfo_count[i]) {
			XPLMDrawObjects(launchpads_refs[i],launchpads_drawinfo_count[i],launchpads_drawinfo[i],0,1);
		}
	}
}



void launchpads_simulate(float dt)
{
	int i;

	//Release launch pad for local vessel if commanded
	if (launchpads_release >= 1.0) {
		launchpads_set(&vessels[0],0);
	}

	//Apply launch pad physics to all vessels which need it
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].attached == VESSEL_MOUNT_LAUNCHPAD) {
			launch_pad* pad;
			quaternion q,p;
			double x,y,z,lx,ly,lz;

			//Try to fetch pad
			if ((vessels[i].mount.body >= 0) && (vessels[i].mount.body < launchpads_count)) {
				pad = &launchpads[vessels[i].mount.body];
			} else {
				continue;
			}

			//Rotate aircraft around offset point
			memcpy(vessels[i].sim.q,vessels[i].mount.attitude,sizeof(quaternion));
			qeuler_from(q,RAD(0),RAD(89.9999),RAD(pad->heading));
			qmul(vessels[i].sim.q,q,vessels[i].sim.q);
			qmul(vessels[i].sim.q,vessels[i].sim.q,launchpads_models[pad->model].q);

			//Convert local pad coordinates into global ones
			qeuler_from(p,RAD(0),RAD(0),RAD(pad->heading));
			qset_vec(q,launchpads_models[pad->model].x+vessels[i].mount.x,
					   launchpads_models[pad->model].y+vessels[i].mount.y,
					   launchpads_models[pad->model].z+vessels[i].mount.z);
			qdiv(q,q,p);
			qmul(q,p,q);
			qget_vec(q,&z,&x,&y);
			x *= -1;

			//Set offset point
			if (pad->flags & LAUNCHPADS_FLAG_KNOWNPOSITION) {
				XPLMWorldToLocal(pad->latitude,pad->longitude,pad->elevation,&lx,&ly,&lz);
			} else {
				XPLMWorldToLocal(pad->latitude,pad->longitude,0,&lx,&ly,&lz);
			}

			//Set vessel location
			vessels[i].sim.x = lx+x;
			vessels[i].sim.y = ly+y;
			vessels[i].sim.z = lz+z;

			//Set zero velocity
			vessels[i].sim.P = 0;	vessels[i].sim.Q = 0;	vessels[i].sim.R = 0;
			vessels[i].sim.vx = 0;	vessels[i].sim.vy = 0;	vessels[i].sim.vz = 0;
		}
	}
}



void launchpads_set(struct vessel_tag* v, launch_pad* pad)
{
	if ((pad) && (pad->planet == current_planet.index)) {
		if (v->index == 0) {
			launchpads_locked = 1.0;
			launchpads_release = 0.0;
			launchpads_hotkey = XPLMRegisterHotKey(XPLM_VK_SPACE,xplm_UpFlag,"",launchpads_key_handler,0);
		}

		v->attached = VESSEL_MOUNT_LAUNCHPAD;
		v->mount.body = pad->index;
	} else {
		if (v->index == 0) {
			launchpads_locked = 0.0;
			if (launchpads_hotkey) XPLMUnregisterHotKey(launchpads_hotkey);
		}
		v->attached = 0;
	}
}

launch_pad* launchpads_get_nearest(struct vessel_tag* v) 
{
	int i;
	launch_pad* pad = 0;
	double distance = 1e99;
	for (i = 0; i < launchpads_count; i++) {
		double d = pow(launchpads[i].latitude-v->latitude,2)+pow(launchpads[i].longitude-v->longitude,2);
		if ((d < distance) && (launchpads[i].planet == current_planet.index)) {
			distance = d;
			pad = &launchpads[i];
		}
	}
	return pad;
}



//==============================================================================
// High-level launch pads interface
//==============================================================================
int launchpads_highlevel_getcount(lua_State* L)
{
	lua_pushnumber(L,launchpads_count);
	return 1;
}

int launchpads_highlevel_setvessel(lua_State* L)
{
	launch_pad* pad = 0;
	vessel* v = 0;
	int idxpad = luaL_checkint(L,1);
	int idxv = luaL_checkint(L,2);
	if ((idxpad >= 0) && (idxpad < launchpads_count)) {
		pad = &launchpads[idxpad];
	} 

	if ((idxv >= 0) && (idxv < vessel_count)) {
		v = &vessels[idxv];
		launchpads_set(v,pad);
	}
	return 0;
}

int launchpads_highlevel_getnearest(lua_State* L)
{
	launch_pad* pad = 0;
	vessel* v = 0;
	int idxv = luaL_checkint(L,1);
	if ((idxv >= 0) && (idxv < vessel_count)) {
		v = &vessels[idxv];
		pad = launchpads_get_nearest(v);
	}

	if (pad) {
		lua_pushnumber(L,pad->index);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int launchpads_highlevel_getname(lua_State* L)
{
	launch_pad* pad = 0;
	int idxpad = luaL_checkint(L,1);
	if ((idxpad >= 0) && (idxpad < launchpads_count)) {
		pad = &launchpads[idxpad];
	} 

	if (pad) {
		lua_pushstring(L,pad->pad_name);
		lua_pushstring(L,pad->complex_name);
	} else {
		lua_pushnil(L);
		lua_pushnil(L);
	}
	return 2;
}

int launchpads_highlevel_getparameter(lua_State* L)
{
	int idxpad = luaL_checkint(L,1);
	int idxparam = luaL_checkint(L,2);
	lua_pushnumber(L,launchpads_read_param_d(idxpad,idxparam));
	return 1;
}



void launchpads_highlevel_initialize() {
	//Register API
	lua_createtable(L,0,32);
	lua_setglobal(L,"LaunchPadsAPI");

	highlevel_addfunction("LaunchPadsAPI","GetCount",launchpads_highlevel_getcount);
	highlevel_addfunction("LaunchPadsAPI","SetVessel",launchpads_highlevel_setvessel);
	highlevel_addfunction("LaunchPadsAPI","GetNearest",launchpads_highlevel_getnearest);
	highlevel_addfunction("LaunchPadsAPI","GetName",launchpads_highlevel_getname);
	highlevel_addfunction("LaunchPadsAPI","GetParameter",launchpads_highlevel_getparameter);
}
