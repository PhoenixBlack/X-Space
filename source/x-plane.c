//==============================================================================
// Interface to X-Plane flight simulator
//==============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "x-space.h"
#include "x-plane.h"
#include "planet.h"

#include "vessel.h"
#include "quaternion.h"
#include "rendering.h"
#include "coordsys.h"
#include "engine.h"
#include "particles.h"
#include "dragheat.h"
#include "launchpads.h"
#include "camera.h"
#include "solpanels.h"
#include "x-ivss.h"

#include "curtime.h"
#include "dataref.h"

//Include X-Plane SDK
#include <XPLMCamera.h>
#include <XPLMDefs.h>
#include <XPLMDisplay.h>
#include <XPLMDataAccess.h>
#include <XPLMGraphics.h>
#include <XPLMUtilities.h>
#include <XPLMPlanes.h>
#include <XPLMPlugin.h>
#include <XPLMProcessing.h>
#include <XPLMScenery.h>
#include <XPLMMenus.h>
#include <XPWidgets.h>
#include <XPStandardWidgets.h>

//Is simulator paused
XPLMDataRef dataref_paused;
//Special hack: no coriolis forces on ground
XPLMDataRef dataref_vessel_agl;
//Can we control X-Plane aircraft?
int xspace_controls_aircraft = 0;

//Special sensor: 512-pixel scanline (from X-Plane OpenGL window)
unsigned char sensors_scanline512_rgb[512*3];
double sensors_scanline512[512];
double sensors_scanline512_active = 0;
double sensors_scanline512_prevtime = 0;

//==============================================================================
// Flight loop
//==============================================================================
float XPluginFlightLoop(float elapsedSinceLastCall, float elapsedTimeSinceLastFlightLoop,
                        int counter, void* refcon)
{
	//Fetch delta-time and get it to desired range
	float dt = elapsedSinceLastCall;
	if (dt < 0.0f) dt = 0.0f;
	if (dt > 1.0f/6.0f) dt = 1.0f/6.0f;

	//Only run if not pause
	if (!XPLMGetDatai(dataref_paused)) {
		xspace_update(dt);
	} else {
		xspace_update(0.0f);
	}

	return -1;
}

//==============================================================================
// Rendering callback
//==============================================================================
int XPluginDrawCallback(XPLMDrawingPhase phase, int isBefore, void* refcon)
{
	int i;
	double dx,dy,dz;
	extern XPLMDataRef dataref_vessel_x;
	extern XPLMDataRef dataref_vessel_y;
	extern XPLMDataRef dataref_vessel_z;

	switch (phase) {
		case xplm_Phase_Terrain:
			//Draw clouds visible from orbit
			planet_clouds_draw();
			//Draw atmospheric scattering
			planet_scattering_draw();
			//Draw coordinate stuff (history)
			coordsys_draw();
			//Draw launch pads
			launchpads_draw();

			//Fix near clipping plane at high altitudes
			{
				double camPos[3];
				double camUp[3];
				float m[16];
				rendering_get_camera(camPos,camUp);
				for (i = 0; i < vessel_count; i++) {
					if (vessels[i].exists) {
						if ((vessels[i].elevation > 50e3) &&
							(pow(camPos[0]-vessels[i].sim.x,2)+
							 pow(camPos[1]-vessels[i].sim.y,2)+
							 pow(camPos[2]-vessels[i].sim.z,2) < 1000*1000)) {
							glMatrixMode(GL_PROJECTION);
							glGetFloatv(GL_PROJECTION_MATRIX,m);
							{
								float zNear = 1.0f;
								float zFar = 1e9f;

								m[2*4+2] = -(zFar + zNear) / (zFar - zNear);
								m[3*4+2] = -2 * zNear * zFar / (zFar - zNear);
							}
							glLoadMatrixf(m);
							glMatrixMode(GL_MODELVIEW);
							break;
						}
					}
				}
			}
			break;
		//case xplm_Phase_Airplanes:
		case xplm_Phase_LastScene:
			//Workaround for issues with setting aircraft position
			dx = XPLMGetDatad(dataref_vessel_x)-vessels[0].sim.x;
			dy = XPLMGetDatad(dataref_vessel_y)-vessels[0].sim.y;
			dz = XPLMGetDatad(dataref_vessel_z)-vessels[0].sim.z;
			glPushMatrix();
			glTranslated(dx,dy,dz);

			//Draw other aircraft
			if (xspace_controls_aircraft) {
				for (i = 0; i < vessel_count; i++) {
					if (vessels[i].exists && vessels[i].is_plane) {// && (vessels[i].physics_type == VESSEL_PHYSICS_SIM)) {
						double p,y,r,sx,sy,sz;
						XPLMPlaneDrawState_t state;
						memset(&state,0,sizeof(state));
						state.structSize = sizeof(state);

						//Get coordinates
						qeuler_to(vessels[i].sim.q,&r,&p,&y);
						if ((vessels[i].physics_type == VESSEL_PHYSICS_INERTIAL) &&
							(vessels[0].physics_type == VESSEL_PHYSICS_SIM)) {
							sx = vessels[i].sim.x-dx;
							sy = vessels[i].sim.y-dy;
							sz = vessels[i].sim.z-dz;
						} else {
							sx = vessels[i].sim.x;
							sy = vessels[i].sim.y;
							sz = vessels[i].sim.z;
						}

						//Draw a fake aircraft image instead
						glPushMatrix();
						glTranslated(sx,sy,sz);
						glRotated(-DEG(y),0,1,0);
						glRotated( DEG(p),1,0,0);
						glRotated(-DEG(r),0,0,1);
						XPLMDrawAircraft(vessels[i].plane_index+1,
										 (float)sx,(float)sy,(float)sz,
										 (float)p,(float)y,(float)r,1,&state);
						glPopMatrix();
					}
				}
			}
			glPopMatrix();

			//Draw solar panels
			solpanels_draw();
			//Draw drag/heating mesh
			dragheat_draw();
			//Draw exhaust of custom engines
			engines_draw();
			//Draw particles
			particles_draw();

			//FIXME: 512-pixel scanline sensor simulation
			if (sensors_scanline512_active >= 1.0) {				
				if (curtime() - sensors_scanline512_prevtime > 0.10) {
					int w,h,i;
					sensors_scanline512_prevtime = curtime();
					XPLMGetScreenSize(&w,&h);

					glFinish();
					glReadPixels(w/2-256,h/2,512,1,GL_RGB,GL_UNSIGNED_BYTE,sensors_scanline512_rgb);

					for (i = 0; i < 512; i++) {
						sensors_scanline512[i] = 0.0;
						sensors_scanline512[i] += sensors_scanline512_rgb[i*3+0]*(0.30/255.0);
						sensors_scanline512[i] += sensors_scanline512_rgb[i*3+1]*(0.59/255.0);
						sensors_scanline512[i] += sensors_scanline512_rgb[i*3+2]*(0.11/255.0);
					}
				}
			}
			break;
		case xplm_Phase_Window:
			//Draw IVSS messages
			xivss_draw(&vessels[0]);
			//Draw drag/heating 2D information
			dragheat_draw2d();
			//Draw camera information
			camera_draw2d();

			//Draw critical error
			{
				extern int materials_count;
				if (materials_count == 0) {
					int w,h;
					float RGB[4] = { 1.00f,0.00f,0.00f,1.0f };
					XPLMGetScreenSize(&w,&h);
					XPLMDrawString(RGB,w/2-256,h/2,"Critical error in X-Space plugin: some important files are missing. Is X-Space install corrupted?",0,xplmFont_Basic);
				}
			}
			break;
		default:
			break;
	}
	return 1;
}

//==============================================================================
// Log something to X-Plane's Log.txt
//==============================================================================
void log_write(char* text, ...)
{
	char buf[ARBITRARY_MAX] = { 0 };
	va_list args;

	va_start(args, text);
	vsnprintf(buf,ARBITRARY_MAX-1,text,args);
	XPLMDebugString(buf);
	va_end(args);
}


//==============================================================================
// Report plugin error (from X-Plane)
//==============================================================================
#ifdef _DEBUG
void XPluginError(const char* inMessage)
{
	log_write("X-Space: plugin error [%s]\n",inMessage);

	//Breakpoint
	_asm {int 3}
}
#endif


//==============================================================================
// Command callback
//==============================================================================
XPLMCommandRef command_rcs_Xp,command_rcs_Xm;
XPLMCommandRef command_rcs_Yp,command_rcs_Ym;
XPLMCommandRef command_rcs_Zp,command_rcs_Zm;
XPLMCommandRef command_rcs_Pp,command_rcs_Pm;
XPLMCommandRef command_rcs_Qp,command_rcs_Qm;
XPLMCommandRef command_rcs_Rp,command_rcs_Rm;
double command_rcs_X = 0,command_rcs_Y = 0,command_rcs_Z = 0;
double command_rcs_P = 0,command_rcs_Q = 0,command_rcs_R = 0;

int XPluginCommandCallback_RCSp(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	if (inPhase == 1) {
		*((double*)(inRefcon)) = 1.0;
	} else {
		*((double*)(inRefcon)) = 0.0;
	}
	return 1;
}

int XPluginCommandCallback_RCSm(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	if (inPhase == 1) {
		*((double*)(inRefcon)) = -1.0;
	} else {
		*((double*)(inRefcon)) = 0.0;
	}
	return 1;
}


//==============================================================================
// Called on plugin startup
//==============================================================================
PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
	log_write("X-Space: Plugin started up\n");
	strcpy(outName, "X-Space");
	strcpy(outDesc, "Advanced spaceflight support for X-Plane");
	strcpy(outSig, "xsag.xspace");

#ifdef _DEBUG
	XPLMSetErrorCallback(XPluginError);
#endif

	//Load some datarefs
	srand((unsigned int)time(0));
	dataref_paused = XPLMFindDataRef("sim/time/paused");
	dataref_vessel_agl = XPLMFindDataRef("sim/flightmodel/position/y_agl");

	//Create RCS control datarefs early (so they can be saved in preferences)
	command_rcs_Xp = XPLMCreateCommand("xsv/rcs/x_plus", "X+ (forward) RCS manuever");
	command_rcs_Xm = XPLMCreateCommand("xsv/rcs/x_minus","X- (aft) RCS manuever");
	command_rcs_Yp = XPLMCreateCommand("xsv/rcs/y_plus", "Y+ (left) RCS manuever");
	command_rcs_Ym = XPLMCreateCommand("xsv/rcs/y_minus","Y- (right) RCS manuever");
	command_rcs_Zp = XPLMCreateCommand("xsv/rcs/z_plus", "Z+ (up) RCS manuever");
	command_rcs_Zm = XPLMCreateCommand("xsv/rcs/z_minus","Z- (down) RCS manuever");
	command_rcs_Pp = XPLMCreateCommand("xsv/rcs/p_plus", "P+ (pitch up) RCS manuever");
	command_rcs_Pm = XPLMCreateCommand("xsv/rcs/p_minus","P- (pitch down) RCS manuever");
	command_rcs_Qp = XPLMCreateCommand("xsv/rcs/q_plus", "Q+ (yaw right) RCS manuever");
	command_rcs_Qm = XPLMCreateCommand("xsv/rcs/q_minus","Q- (yaw left) RCS manuever");
	command_rcs_Rp = XPLMCreateCommand("xsv/rcs/r_plus", "R+ (roll right) RCS manuever");
	command_rcs_Rm = XPLMCreateCommand("xsv/rcs/r_minus","R- (roll left) RCS manuever");

	XPLMRegisterCommandHandler(command_rcs_Xp,XPluginCommandCallback_RCSp,1,&command_rcs_X);
	XPLMRegisterCommandHandler(command_rcs_Xm,XPluginCommandCallback_RCSm,1,&command_rcs_X);
	XPLMRegisterCommandHandler(command_rcs_Yp,XPluginCommandCallback_RCSp,1,&command_rcs_Y);
	XPLMRegisterCommandHandler(command_rcs_Ym,XPluginCommandCallback_RCSm,1,&command_rcs_Y);
	XPLMRegisterCommandHandler(command_rcs_Zp,XPluginCommandCallback_RCSp,1,&command_rcs_Z);
	XPLMRegisterCommandHandler(command_rcs_Zm,XPluginCommandCallback_RCSm,1,&command_rcs_Z);
	XPLMRegisterCommandHandler(command_rcs_Pp,XPluginCommandCallback_RCSp,1,&command_rcs_P);
	XPLMRegisterCommandHandler(command_rcs_Pm,XPluginCommandCallback_RCSm,1,&command_rcs_P);
	XPLMRegisterCommandHandler(command_rcs_Qp,XPluginCommandCallback_RCSp,1,&command_rcs_Q);
	XPLMRegisterCommandHandler(command_rcs_Qm,XPluginCommandCallback_RCSm,1,&command_rcs_Q);
	XPLMRegisterCommandHandler(command_rcs_Rp,XPluginCommandCallback_RCSp,1,&command_rcs_R);
	XPLMRegisterCommandHandler(command_rcs_Rm,XPluginCommandCallback_RCSm,1,&command_rcs_R);

	dataref_d("xsp/control/general/discrete_p",&command_rcs_P);
	dataref_d("xsp/control/general/discrete_q",&command_rcs_Q);
	dataref_d("xsp/control/general/discrete_r",&command_rcs_R);
	dataref_d("xsp/control/general/discrete_x",&command_rcs_X);
	dataref_d("xsp/control/general/discrete_y",&command_rcs_Y);
	dataref_d("xsp/control/general/discrete_z",&command_rcs_Z);

	dataref_dv("xsp/local/sensors/scanline512",512,sensors_scanline512);
	dataref_d("xsp/local/sensors/scanline512_active",&sensors_scanline512_active);

	//Initialize X-Space
	log_write("X-Space: Initializing (version "VERSION")...\n");
	xspace_initialize_all();
	log_write("X-Space: Initialized!\n");
	return 1;
}


/*******************************************************************************
 * Plugin shutdown
 ******************************************************************************/
PLUGIN_API void XPluginStop(void)
{
	xspace_deinitialize_all();
	log_write("X-Space: Plugin stopped\n");

	//Unregister RCS commands
	XPLMUnregisterCommandHandler(command_rcs_Xp,XPluginCommandCallback_RCSp,1,&command_rcs_X);
	XPLMUnregisterCommandHandler(command_rcs_Xm,XPluginCommandCallback_RCSm,1,&command_rcs_X);
	XPLMUnregisterCommandHandler(command_rcs_Yp,XPluginCommandCallback_RCSp,1,&command_rcs_Y);
	XPLMUnregisterCommandHandler(command_rcs_Ym,XPluginCommandCallback_RCSm,1,&command_rcs_Y);
	XPLMUnregisterCommandHandler(command_rcs_Zp,XPluginCommandCallback_RCSp,1,&command_rcs_Z);
	XPLMUnregisterCommandHandler(command_rcs_Zm,XPluginCommandCallback_RCSm,1,&command_rcs_Z);
	XPLMUnregisterCommandHandler(command_rcs_Pp,XPluginCommandCallback_RCSp,1,&command_rcs_P);
	XPLMUnregisterCommandHandler(command_rcs_Pm,XPluginCommandCallback_RCSm,1,&command_rcs_P);
	XPLMUnregisterCommandHandler(command_rcs_Qp,XPluginCommandCallback_RCSp,1,&command_rcs_Q);
	XPLMUnregisterCommandHandler(command_rcs_Qm,XPluginCommandCallback_RCSm,1,&command_rcs_Q);
	XPLMUnregisterCommandHandler(command_rcs_Rp,XPluginCommandCallback_RCSp,1,&command_rcs_R);
	XPLMUnregisterCommandHandler(command_rcs_Rm,XPluginCommandCallback_RCSm,1,&command_rcs_R);
}


/*******************************************************************************
 * Plugin initialization
 ******************************************************************************/
PLUGIN_API int XPluginEnable(void)
{
	log_write("X-Space: Plugin enabled\n");
	xspace_reinitialize();

	//Register callbacks
	XPLMRegisterFlightLoopCallback(XPluginFlightLoop, -1, NULL);
	XPLMRegisterDrawCallback(XPluginDrawCallback, xplm_Phase_Terrain, 0, NULL);
	//XPLMRegisterDrawCallback(XPluginDrawCallback, xplm_Phase_Airplanes, 0, NULL);
	//XPLMRegisterDrawCallback(XPluginDrawCallback, xplm_Phase_Airplanes, 1, NULL);
	XPLMRegisterDrawCallback(XPluginDrawCallback, xplm_Phase_LastScene, 0, NULL);
	XPLMRegisterDrawCallback(XPluginDrawCallback, xplm_Phase_Window, 0, NULL);
	return 1;
}


/*******************************************************************************
 * Plugin deinitialization
 ******************************************************************************/
PLUGIN_API void XPluginDisable(void)
{
	log_write("X-Space: Plugin disabled\n");

	//Unregister callbacks
	XPLMUnregisterFlightLoopCallback(XPluginFlightLoop, NULL);
	XPLMUnregisterDrawCallback(XPluginDrawCallback, xplm_Phase_FirstScene, 0, NULL);
	//XPLMUnregisterDrawCallback(XPluginDrawCallback, xplm_Phase_Airplanes, 0, NULL);
	//XPLMUnregisterDrawCallback(XPluginDrawCallback, xplm_Phase_Airplanes, 1, NULL);
	XPLMUnregisterDrawCallback(XPluginDrawCallback, xplm_Phase_LastScene, 0, NULL);
	XPLMUnregisterDrawCallback(XPluginDrawCallback, xplm_Phase_Window, 0, NULL);
}


/*******************************************************************************
 * Message handler
 ******************************************************************************/
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID fromWho, long message, void* param)
{
	if ((message == XPLM_MSG_PLANE_LOADED) && (!param)) {
		xspace_reinitialize();
	}
}


/*******************************************************************************
 *
 ******************************************************************************/
void xplane_planes_available(void* inRefcon)
{
	xspace_controls_aircraft = 1;
}

void xplane_load_aircraft(char** models, int total_planes)
{
	extern void highlevel_showerror(char* text);
	int max_total_planes,active_planes,plugin_id;

	//Stop controlling aircraft
	if (xspace_controls_aircraft) xplane_no_aircraft();

	//Check if it is possible to allocate all aircraft
	XPLMCountAircraft(&max_total_planes,&active_planes,&plugin_id);
	//if ((max_total_planes > 0) && (total_planes+1 > max_total_planes)) {
	if ((max_total_planes > 1) && (total_planes+1 >= max_total_planes)) {
		log_write("X-Space: Not enough AI plane slots in settings! Increase AI plane slots to at least %d\n",total_planes);
		highlevel_showerror("Cannot load vesels: not enough AI plane slots!");
		return;
	}

	//Try to acquire planes
	if (XPLMAcquirePlanes(models,xplane_planes_available,0)) {
		int aircraft_count = 1;
		xspace_controls_aircraft = 1;
		while ((aircraft_count < 20) && (models[aircraft_count-1])) {
			XPLMSetAircraftModel(aircraft_count, models[aircraft_count-1]);
			XPLMDisableAIForPlane(aircraft_count);
			aircraft_count++;
		}
		XPLMSetActiveAircraftCount(aircraft_count);
	} else {
		log_write("X-Space: Conflicting with another plugin! Cannot acquire control over AI planes\n");
		highlevel_showerror("Cannot load vesels: conflict with another plugin!");
	}
}

void xplane_no_aircraft()
{
	XPLMReleasePlanes();
	xspace_controls_aircraft = 0;
}