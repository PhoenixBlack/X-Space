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
#include "config.h"
#include "planet.h"
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include "lookups.h"
#include "rendering.h"
#include "particles.h"
#include "dataref.h"
#endif

#include "vessel.h"
#include "coordsys.h"
#include "quaternion.h"

#include "curtime.h"

//X-Plane SDK
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include <XPLMDataAccess.h>
#include <XPLMGraphics.h>
#endif

//Current planet
planet current_planet;

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
//Planet state
XPLMDataRef dataref_current_planet;
XPLMDataRef dataref_planet_radius;
XPLMDataRef dataref_planet_mu;
XPLMDataRef dataref_sun_pitch;
XPLMDataRef dataref_sun_heading;

//Textures
int planet_texture_scattering; //Scattering lookup texture
int planet_texture_clouds; //Clouds layer texture

//Handles
GLhandleARB planet_clouds_program_hq; //Program object (HQ)
GLhandleARB planet_clouds_program_lq; //Program object (LQ)
GLhandleARB planet_clouds_vertex; //Vertex shader for clouds
GLhandleARB planet_clouds_fragment_hq; //Fragment shader for clouds (HQ)
GLhandleARB planet_clouds_fragment_lq; //Fragment shader for clouds (LQ)
GLint planet_perlin_perm_location_hq; //Permutations texture
GLint planet_clouds_texture_location_hq; //Permutations texture
GLint planet_perlin_perm_location_lq; //Permutations texture
GLint planet_clouds_texture_location_lq; //Permutations texture


GLhandleARB planet_scaterring_space_program;
GLhandleARB planet_scaterring_space_fragment;
GLhandleARB planet_scaterring_space_vertex;

GLhandleARB planet_scaterring_ground_program;
GLhandleARB planet_scaterring_ground_fragment;
GLhandleARB planet_scaterring_ground_vertex;
#endif


//==============================================================================
// Initialize planet variables
//==============================================================================
void planet_initialize()
{
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	dataref_current_planet     = XPLMFindDataRef("sim/graphics/scenery/current_planet");
	dataref_planet_radius      = XPLMFindDataRef("sim/physics/earth_radius_m");
	dataref_planet_mu          = XPLMFindDataRef("sim/physics/earth_mu");
	dataref_sun_pitch          = XPLMFindDataRef("sim/graphics/scenery/sun_pitch_degrees");
	dataref_sun_heading        = XPLMFindDataRef("sim/graphics/scenery/sun_heading_degrees");

	dataref_d("xsp/planet/w",&current_planet.w);
	dataref_d("xsp/planet/a",&current_planet.a);
	dataref_d("xsp/planet/mu",&current_planet.mu);
	dataref_d("xsp/planet/radius",&current_planet.radius);
#endif

	memset(&current_planet,0,sizeof(current_planet));
#if (!defined(DEDICATED_SERVER))
	current_planet.a = fmod((curtime_mjd()-51544.5)*2.0*PI,2.0*PI);
#endif
}


//==============================================================================
// Update planet state
//==============================================================================
void planet_update(float dt)
{
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	//Update parameters
	current_planet.index = XPLMGetDatai(dataref_current_planet);
	current_planet.radius = XPLMGetDataf(dataref_planet_radius);
	current_planet.mu = XPLMGetDataf(dataref_planet_mu);
	current_planet.sim_sun_pitch = XPLMGetDataf(dataref_sun_pitch);
	current_planet.sim_sun_heading = XPLMGetDataf(dataref_sun_heading);
	if (current_planet.index == 0) current_planet.w = 2*PI/86164.10; //Earth day
	else						   current_planet.w = 2*PI/88642.66; //Martian day
#else
	current_planet.radius = 6378.145e3;
	current_planet.mu = 3.9860044e14;
	current_planet.w = 2*PI/86164.10;
#endif

	//Check if planet should not rotate
	if (!config.planet_rotation) current_planet.w = 0.0;

	//Update rotation from MJD
#if (defined(DEDICATED_SERVER))
	current_planet.a = fmod((curtime_mjd()-51544.5)*2.0*PI,2.0*PI);
#else
	current_planet.a += current_planet.w * dt;
#endif
}


#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
//==============================================================================
// Atmospheric scattering
//==============================================================================
void planet_scattering_initialize()
{
	//Do not initialize if no scattering
	if (!config.use_scattering) return;

	//if (config.use_shaders) {
		planet_scaterring_space_vertex = rendering_shader_compile(GL_VERTEX_SHADER,FROM_PLUGINS("shaders/scattering_space.vert"),0);
		planet_scaterring_space_fragment = rendering_shader_compile(GL_FRAGMENT_SHADER,FROM_PLUGINS("shaders/scattering_space.frag"),0);
		planet_scaterring_space_program = rendering_shader_link(planet_scaterring_space_vertex,planet_scaterring_space_fragment);

		planet_scaterring_ground_vertex = rendering_shader_compile(GL_VERTEX_SHADER,FROM_PLUGINS("shaders/scattering_ground.vert"),0);
		planet_scaterring_ground_fragment = rendering_shader_compile(GL_FRAGMENT_SHADER,FROM_PLUGINS("shaders/scattering_ground.frag"),0);
		planet_scaterring_ground_program = rendering_shader_link(planet_scaterring_ground_vertex,planet_scaterring_ground_fragment);
	//} else {
		//Generate scattering lookup texture
		XPLMGenerateTextureNumbers(&planet_texture_scattering,1);
		XPLMBindTexture2d(planet_texture_scattering,0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 512, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, lookup_scattering);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	//}
}

void planet_scattering_deinitialize()
{
	if (!config.use_scattering) return;

	//FIXME: how this should be done in X-Plane?
	//glDeleteTextures(1,&planet_texture_scattering);
}


//==============================================================================
// Draw a single scattering ring
//==============================================================================
void planet_scattering_ring(double S, double E, double AL, int vindex, double P)
{
	double STEP = 2*PI/128.0;
	double Q;

	if ((vindex < 0) || (vindex > 15)) return;

	for (Q = 0; Q < 2*PI-STEP*0.5; Q += STEP) {
		float u1,v1,u2,v2;

		if (current_planet.index == 1) { //Atmosphere color modulation
			glColor4d(1.0f,0.6f,0.2f,1.1f*0.55f*AL);
		} else {
			glColor4d(1.0f,1.0f,1.0f,1.1f*0.72f*AL);
		}
		u1 = (float)((Q)/(2*PI));
		u2 = (float)((Q+STEP)/(2*PI));
		v1 = (float)((1.0*vindex/16.0)+0.005);
		v2 = (float)((1.0*(vindex+1)/16.0)-0.001);

		glBegin(GL_QUADS);
		glTexCoord2f(u1,v1);
		glVertex3d(S * cos(P+Q),     0, S * sin(P+Q));
		glTexCoord2f(u1,v2);
		glVertex3d(E * cos(P+Q),     0, E * sin(P+Q));
		glTexCoord2f(u2,v2);
		glVertex3d(E * cos(P+Q+STEP),0, E * sin(P+Q+STEP));
		glTexCoord2f(u2,v1);
		glVertex3d(S * cos(P+Q+STEP),0, S * sin(P+Q+STEP));

		/*glTexCoord2f(u1,v1);
		glVertex3d(S * cos(P+Q),     0, S * sin(P+Q));
		glTexCoord2f(u1,v2);
		glVertex3d(E * cos(P+Q),     0, E * sin(P+Q));
		glTexCoord2f(u2,v2);
		glVertex3d(E * cos(P+Q+STEP),0, E * sin(P+Q+STEP));
		glTexCoord2f(u2,v1);
		glVertex3d(S * cos(P+Q+STEP),0, S * sin(P+Q+STEP));*/

		glTexCoord2f(u1,v2);
		glVertex3d(0.9*S * cos(P+Q),     0, 0.9*S * sin(P+Q));
		glTexCoord2f(u1,v1);
		glVertex3d(S * cos(P+Q),     0, S * sin(P+Q));
		glTexCoord2f(u2,v1);
		glVertex3d(S * cos(P+Q+STEP),0, S * sin(P+Q+STEP));
		glTexCoord2f(u2,v2);
		glVertex3d(0.9*S * cos(P+Q+STEP),0, 0.9*S * sin(P+Q+STEP));

		/*glTexCoord2f(u1,v2);
		glVertex3d(0.7*S * cos(P+Q),     0, 0.7*S * sin(P+Q));
		glTexCoord2f(u1,v1);
		glVertex3d(S * cos(P+Q),     0, S * sin(P+Q));
		glTexCoord2f(u2,v1);
		glVertex3d(S * cos(P+Q+STEP),0, S * sin(P+Q+STEP));
		glTexCoord2f(u2,v2);
		glVertex3d(0.7*S * cos(P+Q+STEP),0, 0.7*S * sin(P+Q+STEP));*/
		glEnd();
	}
}


//==============================================================================
// Draw a scattering halo
//==============================================================================
void planet_scattering_halo(double camPos[3], double camUp[3], double sunPos[3])
{
	//Calculate world center and radius-to-camera vector
	double R  = current_planet.radius; //Planet radius
	double AR = current_planet.radius+200000; //Atmosphere radius

	double wX = 0;
	double wY = -R;
	double wZ = 0;
	double cX = camPos[0] - wX;
	double cY = camPos[1] - wY;
	double cZ = camPos[2] - wZ;

	//Calculate location and size
	double D  = sqrt(cX*cX + cY*cY + cZ*cZ); //Distance from camera to planet surface
	double K2 = (D * D - R * R < 0 ? R * R - D * D : D * D - R * R); //Distance from camera to earth edge
	double K  = (D * D - R * R < 0 ? -sqrt(K2) - 1e-15 : sqrt(K2) + 1e-15);
	double L2 = (D * D - AR * AR < 0 ? AR * AR - D * D : D * D - AR * AR); //Distance from camera to atmosphere edge
	double L  = (D * D - AR * AR < 0 ? -sqrt(L2) - 1e-15 : sqrt(L2) + 1e-15);

	double Phi1 = atan(K / R); //D line and radius to planet edge
	double Phi2 = atan(L / AR); //D line and radius to atmosphere edge

	double C = R * sin(Phi1); //Halo radius
	double A = R * cos(Phi1); //Distance to halo center from earth center
	//double B = D - A; //Distance to halo center from camera center

	//Vector from radius to camera (normalized)
	double nX = cX / D;
	double nY = cY / D;
	double nZ = cZ / D;

	//Sun coordinates
	double sX = sunPos[0];
	double sY = sunPos[1];
	double sZ = sunPos[2];

	//Camera view coordinates
	//camPos[0]*camPos[0]+camPos[1]*camPos[1]+camPos[2]*camPos[2]
	double vD = sqrt(cX*cX+cY*cY+cZ*cZ)+1e-12;
	double vX = -camPos[0]/vD;
	double vY = -camPos[1]/vD;
	double vZ = -camPos[2]/vD;

	//Camera up vector coordinates
	double uX = camUp[0];
	double uY = camUp[1];
	double uZ = camUp[2];

	//Sine of angle between camera up and sun coordinates
	double cameraCrossX = uY*sZ-uZ*sX;
	double cameraCrossY = uZ*sX-uX*sZ;
	double cameraCrossZ = uX*sY-uY*sX;
	double cameraSineSign = (vX*cameraCrossX+vY*cameraCrossY+vZ*cameraCrossZ > 0 ? 1 : -1);
	double cameraSine = cameraSineSign*sqrt(cameraCrossX*cameraCrossX+cameraCrossY*cameraCrossY+cameraCrossZ*cameraCrossZ);
	double cameraCosine = fabs(uX*sX+uY*sY+uZ*sZ);
	double cameraTangent = cameraSine/cameraCosine;
	double cameraAngle = atan(cameraTangent);

	//Angle between camera view and sun coordinates
	double viewCosine = vX*sX+vY*sY+vZ*sZ;
	double viewAngle =
	    (viewCosine > 1 ? acos(1) : (viewCosine < -1 ? acos(-1) : acos(viewCosine)));

	//Position of halo in local OpenGL coordinates
	double hX = wX + nX * A;
	double hY = wY + nY * A;
	double hZ = wZ + nZ * A;

	//Visible atmosphere height
	double AH = sqrt(R*R + AR*AR - 2 * AR * R * cos(Phi2 - Phi1));

	//Move halo into its position in local coordinates
	glTranslated(hX, hY, hZ);

	//Rotate halo so it always faces camera
	glRotated(180 - atan2(nZ,nX) / 3.1415 * 180.0, 0, 1, 0); //Heading
	glRotated(90 - atan2(nY,sqrt(nZ*nZ + nX * nX)) / 3.1415 * 180.0, 0, 0, 1); //Pitch

	//Draw the actual halo
	{
		double lookup_index = 7.5+8.5*((PI-viewAngle)/PI);
		double lerp_time = lookup_index - (int)lookup_index;

		double angle_lerp_time = (fabs(cameraAngle)-1.35)/(0.5*PI-1.35);

		//Compute alpha for the scattering
		double AL = 1.0f;
		if (D-R < 100000) {
			AL = ((D-R)-50000)/50000;
			if (AL < 0) AL = 0;
		}

		//Draw scattering rings
		if (angle_lerp_time > 0) {
			planet_scattering_ring(C,C+AH,AL*(1-lerp_time)*(1-angle_lerp_time),(int)lookup_index,  cameraAngle+PI);
			planet_scattering_ring(C,C+AH,AL*(  lerp_time)*(1-angle_lerp_time),(int)lookup_index+1,cameraAngle+PI);

			planet_scattering_ring(C,C+AH,AL*(1-lerp_time)*(angle_lerp_time),  (int)lookup_index,  -cameraAngle+PI);
			planet_scattering_ring(C,C+AH,AL*(  lerp_time)*(angle_lerp_time),  (int)lookup_index+1,-cameraAngle+PI);
		} else {
			planet_scattering_ring(C,C+AH,AL*(1-lerp_time),(int)lookup_index,  cameraAngle+PI);
			planet_scattering_ring(C,C+AH,AL*(  lerp_time),(int)lookup_index+1,cameraAngle+PI);
			//planet_scattering_ring(C,C+AH,AL*0.1f,8,0);
		}
	}
}


//==============================================================================
// Draw scattering
//==============================================================================
void planet_scattering_draw()
{
	float m[16];
	double camPos[3];
	double camUp[3];
	double sunPos[3];
	double camAlt;
	int use_shaders = config.use_shaders;

	//Exit if not using scattering
	if (!config.use_scattering) return;
	if (current_planet.index == 1) use_shaders = 0;

	//Fetch camera
	rendering_get_camera(camPos,camUp);

	//Fetch sun position
	sunPos[0] = sin(RAD(current_planet.sim_sun_heading))*cos(RAD(current_planet.sim_sun_pitch));
	sunPos[1] = sin(RAD(current_planet.sim_sun_pitch));
	sunPos[2] = -cos(RAD(current_planet.sim_sun_heading))*cos(RAD(current_planet.sim_sun_pitch));

	//Fix projection matrix so scattering does not clip
	glMatrixMode(GL_PROJECTION);
	glGetFloatv(GL_PROJECTION_MATRIX,m);
	{
		float zNear = 100.0f;//1e3f;
		float zFar = 1e12f;
		if (!use_shaders) {
			zNear = 100.0f;
		} else {
			double dist = sqrt(camPos[0]*camPos[0]+camPos[2]*camPos[2]+
				(camPos[1]+current_planet.radius)*(camPos[1]+current_planet.radius));
			if (dist > current_planet.radius + 900e3) {
				zNear = dist*0.1f;
				zFar = (dist + current_planet.radius*2)*1.00f;
			} else {
				zFar = 1e10f;
			}
		}

		m[2*4+2] = -(zFar + zNear) / (zFar - zNear);
		m[3*4+2] = -2 * zNear * zFar / (zFar - zNear);
	}
	glPushMatrix();
	glLoadMatrixf(m);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	if (use_shaders) {
		glTranslated(0.0,-current_planet.radius,0.0);
		camPos[1] += current_planet.radius;
		camAlt = sqrt(camPos[0]*camPos[0]+camPos[1]*camPos[1]+camPos[2]*camPos[2]);
		if (camAlt < current_planet.radius+15e3) {
			glPopMatrix();
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);	
			return;
		}

		XPLMSetGraphicsState(0, 0, 0, 0, 1, 1, 1);
		//glEnable(GL_DEPTH_TEST);
		glClearDepth(1);
		glClear(GL_DEPTH_BUFFER_BIT);
		glBlendFunc(GL_ONE, GL_ONE);
		glUseProgram(planet_scaterring_ground_program);
		glUniform3f(glGetUniformLocation(planet_scaterring_ground_program,"v3CameraPos"), (float)camPos[0], (float)camPos[1], (float)camPos[2]);
		glUniform3f(glGetUniformLocation(planet_scaterring_ground_program,"v3LightPos"),  (float)sunPos[0], (float)sunPos[1], (float)sunPos[2]);
		glUniform1f(glGetUniformLocation(planet_scaterring_ground_program,"fCameraHeight"), (float)camAlt);
		glUniform1f(glGetUniformLocation(planet_scaterring_ground_program,"fCameraHeight2"), (float)(camAlt*camAlt));

		//glClearStencil(0);
		//glEnable(GL_STENCIL_TEST);
		//glClear(GL_STENCIL_BUFFER_BIT);
		//glStencilFunc(GL_NEVER, 0x0, 0x0);
		//glStencilOp(GL_INCR, GL_INCR, GL_INCR);
		rendering_texture_sphere((float)(current_planet.radius+0),96);
	            
		//Now, allow drawing, except where the stencil pattern is 0x1 and do not make any further changes to the stencil buffer
		//glStencilFunc(GL_NOTEQUAL, 0x1, 0x1);
		//glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		XPLMSetGraphicsState(0, 0, 0, 0, 1, 1, 0);
		glBlendFunc(GL_ONE, GL_ONE);
		glUseProgram(planet_scaterring_space_program);
		glUniform3f(glGetUniformLocation(planet_scaterring_space_program,"v3CameraPos"), (float)camPos[0], (float)camPos[1], (float)camPos[2]);
		glUniform3f(glGetUniformLocation(planet_scaterring_space_program,"v3LightPos"),  (float)sunPos[0], (float)sunPos[1], (float)sunPos[2]);
		glUniform1f(glGetUniformLocation(planet_scaterring_space_program,"fCameraHeight"), (float)camAlt);
		glUniform1f(glGetUniformLocation(planet_scaterring_space_program,"fCameraHeight2"), (float)(camAlt*camAlt));
	
		glFrontFace(GL_CCW);
		rendering_texture_sphere((float)(current_planet.radius+100e3),64);
		glFrontFace(GL_CW);
		glUseProgram(0);

		XPLMSetGraphicsState(0, 0, 0, 0, 1, 1, 1);
		glClearDepth(1);
		glClear(GL_DEPTH_BUFFER_BIT);
	} else {
		XPLMSetGraphicsState(0, 1, 0, 0, 1, 0, 0);
		XPLMBindTexture2d(planet_texture_scattering,0);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		planet_scattering_halo(camPos,camUp,sunPos);
	}
	/////////////

	//Restore state
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}




//==============================================================================
// Orbital clouds rendering
//==============================================================================
void planet_clouds_initialize()
{
	//Generate texture number
	int texture_number;

	if (!config.use_clouds) return;
	XPLMGenerateTextureNumbers(&texture_number,1);
	XPLMBindTexture2d(texture_number,0);

	//Load clouds texture
	planet_texture_clouds = SOIL_load_OGL_texture(
	                            FROM_PLUGINS("clouds_2048.jpg"),
	                            SOIL_LOAD_AUTO,texture_number,0);
	if (!planet_texture_clouds) {
		log_write("X-Space: Unable to load cloud map: %s\n",SOIL_last_result());
	} else {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	}

	//Load lookup textures for shaders
	if (config.use_shaders) {
		//Create vertex shader for clouds
		planet_clouds_vertex = rendering_shader_compile(GL_VERTEX_SHADER,FROM_PLUGINS("shaders/clouds.vert"),1);
		planet_clouds_fragment_hq = rendering_shader_compile(GL_FRAGMENT_SHADER,FROM_PLUGINS("shaders/clouds.frag"),1);
		planet_clouds_fragment_lq = rendering_shader_compile(GL_FRAGMENT_SHADER,FROM_PLUGINS("shaders/clouds.frag"),0);

		//Create program
		planet_clouds_program_hq = rendering_shader_link(planet_clouds_vertex,planet_clouds_fragment_hq);
		planet_clouds_program_lq = rendering_shader_link(planet_clouds_vertex,planet_clouds_fragment_lq);
		planet_clouds_texture_location_hq = glGetUniformLocation(planet_clouds_program_hq, "cloudTexture");
		planet_perlin_perm_location_hq = glGetUniformLocation(planet_clouds_program_hq, "permTexture");
		planet_clouds_texture_location_lq = glGetUniformLocation(planet_clouds_program_lq, "cloudTexture");
		planet_perlin_perm_location_lq = glGetUniformLocation(planet_clouds_program_lq, "permTexture");
	}
}

void planet_clouds_deinitialize()
{
	if (!config.use_clouds) return;
	if (config.use_shaders) {
		glDeleteProgram(planet_clouds_program_lq);
		glDeleteShader(planet_clouds_fragment_lq);
		glDeleteProgram(planet_clouds_program_hq);
		glDeleteShader(planet_clouds_fragment_hq);
		glDeleteShader(planet_clouds_vertex);
	}
}


//==============================================================================
// Draw clouds layer
//==============================================================================
void planet_clouds_draw()
{
	double camPos[3];
	double camUp[3];
	double d,R,A;

	//Do not draw if not enabled
	if (!config.use_clouds) return;

	//Do not draw on Mars
	//if (current_planet.index != 0) return;

	//Fetch camera and distance from surface
	rendering_get_camera(camPos,camUp);
	R = current_planet.radius;
	d = sqrt(camPos[0]*camPos[0]+(camPos[1]+R)*(camPos[1]+R)+camPos[2]*camPos[2])-R;
	A = (d-18000)/(30000-18000);
	if (A >  1.0) A = 1.0;
	if (A <= 0.0) return; //Skip drawing if clouds are not visible

	//Setup correct mode
	glMatrixMode(GL_MODELVIEW);
	XPLMSetGraphicsState(0, 4, 1, 0, 1, 0, 0);

	//Draw around planet center
	glPushMatrix();
	glTranslated(0.0,-current_planet.radius,0.0);

	//Align with the planet
	{
		double lat,lon,alt;
		XPLMLocalToWorld(0,0,0,&lat,&lon,&alt);
		glRotated(lat,1,0,0);
		glRotated(lon-90,0,0,1);
	}

#define BIND_CLOUDS_PROGRAM(quality) \
	glUseProgram(planet_clouds_program_##quality); \
	\
	glActiveTexture(GL_TEXTURE0); \
	XPLMBindTexture2d(planet_texture_clouds,0); \
	if (planet_clouds_texture_location_##quality != -1) glUniform1i(planet_clouds_texture_location_##quality, 0); \
	\
	glActiveTexture(GL_TEXTURE1); \
	XPLMBindTexture2d(particles_perlin_perm_texture,1); \
	if (planet_perlin_perm_location_##quality != -1) glUniform1i(planet_perlin_perm_location_##quality, 1);

	//Draw cloud shadows
	if (config.cloud_shadows) {
		if (config.use_shaders) {
			BIND_CLOUDS_PROGRAM(lq);
		} else {
			XPLMBindTexture2d(planet_texture_clouds,0);
		}
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glColor4d(0.0,0.0,0.0,A*1.0);
		rendering_texture_sphere((float)(current_planet.radius+0),64);
	}

	//Draw clouds layer
	if (config.use_shaders) {
		if (config.use_hq_clouds) {
			BIND_CLOUDS_PROGRAM(hq);
		} else {
			BIND_CLOUDS_PROGRAM(lq);
		}
	} else {
		XPLMBindTexture2d(planet_texture_clouds,0);
	}
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	if (config.use_shaders) {
		glColor4d(1.0,1.0,1.0,A*1.0);
	} else {
		glColor4d(1.0,1.0,1.0,A*0.7);
	}
	rendering_texture_sphere((float)(current_planet.radius+7000),64);//128);

	//Disable shaders
	if (config.use_shaders) {
		glUseProgram(0);
		glActiveTexture(GL_TEXTURE0);
	}

	//Restore state
	glPopMatrix();
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}
#endif