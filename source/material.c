#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "x-space.h"
#include "material.h"
#include "highlevel.h"

//X-Plane SDK
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include "rendering.h"
#include <XPLMPlanes.h>
#include <XPLMGraphics.h>
#endif

//Material storage
material* materials = 0;
int materials_count = 0;

void material_initialize()
{
	char filename[MAX_FILENAME] = { 0 }, buf[ARBITRARY_MAX] = { 0 };
	int units_mode = 0; //0: metric, 1: imperial
	int material_index;
	FILE* f;

	float Cp[256*2]; //Temporary storage
	int numCp = 0;
	float k[256*2]; //Temporary storage
	int numk = 0;

#define MATERIAL_LERP_CALC() \
	if (numCp == 0) { \
		Cp[numCp*2+0] = 0; \
		Cp[numCp*2+1] = 897; \
		numCp = 1; \
	} \
	if (numk == 0) { \
		k[numk*2+0] = 0; \
		k[numk*2+1] = 100; \
		numk = 1; \
	} \
	material_lerp_calc(&materials[material_index],Cp,numCp,k,numk);

	//Report state
	log_write("X-Space: Reloading materials database\n");

	//Read path to X-Space plugin
	snprintf(filename,MAX_FILENAME-1,FROM_PLUGINS("material.dat"));
	f = fopen(filename,"r");
	if (!f) log_write("X-Space: Failed to load materials database\n");

	//Scan for number of models and pads
	materials_count = 0;
	while (f && (!feof(f))) {
		char tag[256];
		tag[0] = 0; fscanf(f,"%255s",tag); tag[255] = 0;
		if (strcmp(tag,"MATERIAL") == 0) materials_count++;
		fgets(buf,ARBITRARY_MAX-1,f);
	}
	if (f) fseek(f,0,0);

	//Report info
	log_write("X-Space: Loading %d material entries\n",materials_count);

	//Load materials
	materials = (material*)malloc(sizeof(material)*materials_count);
	material_index = -1;
	while (f && (!feof(f)) && ((numk < 256) && (numCp < 256))) {
		char tag[256]; float temp;
		tag[0] = 0; fscanf(f,"%255s",tag); tag[255] = 0;
		if (strcmp(tag,"MATERIAL") == 0) { //Next material entry
			if (material_index >= 0) { //Create interpolation tables
				MATERIAL_LERP_CALC();
				numCp = 0;
				numk = 0;
			}
			material_index++;
			fscanf(f,"%255s",materials[material_index].name);
			materials[material_index].density = 1000.0;
		} else if (strcmp(tag,"SI_UNITS") == 0) {
			units_mode = 0;
		} else if (strcmp(tag,"IMPERIAL_UNITS") == 0) {
			units_mode = 1;
		} else if (strcmp(tag,"Density") == 0) { //Set density
			if ((material_index >= 0) && (material_index < materials_count)) {
				fscanf(f,"%f",&temp);
				if (units_mode) temp = (float)(temp*16.0184634); //lb/ft3
				materials[material_index].density = temp;
			}
		} else if (strcmp(tag,"Cp") == 0) { //Specific heat entry
			if ((material_index >= 0) && (material_index < materials_count) && (numCp < 256)) {
				fscanf(f,"%f %f",&Cp[numCp*2+0],&Cp[numCp*2+1]);
				if (units_mode) {
					Cp[numCp*2+0] *= (float)(5.0/9.0); //R
					Cp[numCp*2+1] *= (float)(1054.35026444/(0.45359237*5.0/9.0)); //Btu/(lb*R)
				}
				numCp++;
			}
		} else if (strcmp(tag,"k") == 0) { //Thermal conductivity entry
			if ((material_index >= 0) && (material_index < materials_count) && (numCp < 256)) {
				fscanf(f,"%f %f",&k[numk*2+0],&k[numk*2+1]);
				if (units_mode) {
					//k[numk*3+0] *= (float)(47.8); //psf
					k[numk*2+0] *= (float)(5.0/9.0); //R
					k[numk*2+1] *= (float)(1054.35026444/(0.3048*5.0/9.0)); //Btu/(ft*s*R)
				}
				numk++;
			}
		} else {
			fgets(buf,ARBITRARY_MAX-1,f);
		}
	}
	if (f) fclose(f);
	if (material_index >= 0) {
		MATERIAL_LERP_CALC();
	}
	
	//Highlevel interface
	material_highlevel_initialize();
}

void material_deinitialize()
{
	int i;
	for (i = 0; i < materials_count; i++) {
		free(materials[i].Cp);
		free(materials[i].k);
	}
	free(materials);
	materials = 0;
}

material_idx material_get(char* name)
{
	int i;
	if (strncmp(name,"None",256) == 0) return 0;
	for (i = 0; i < materials_count; i++) {
		if (strncmp(materials[i].name,name,256) == 0) {
			return i;
		}
	}
	return 0;
}

double material_getCp(material_idx midx, double temperature)
{
	int i;
	if ((midx < 0) || (midx >= materials_count)) return 897.0;

	i = (int)((temperature - MIN_TEMPERATURE)/TEMPERATURE_STEP);
	if (i < 0) i = 0;
	if (i >= materials[midx].numCp) i = materials[midx].numCp-1;
	return materials[midx].Cp[i];
}

double material_getk(material_idx midx, double temperature)
{
	int i;
	if ((midx < 0) || (midx >= materials_count)) return 1000.0;

	i = (int)((temperature - MIN_TEMPERATURE)/TEMPERATURE_STEP);
	if (i < 0) i = 0;
	if (i >= materials[midx].numk) i = materials[midx].numk-1;
	return materials[midx].k[i];
}

double material_getDensity(material_idx midx)
{
	if ((midx < 0) || (midx >= materials_count)) return 1000.0;
	return materials[midx].density;
}

void material_lerp_calc(material* m, float* Cp, int numCp, float* k, int numk)
{
	int i;

	//Create tables for Cp
	m->numCp = (int)((MAX_TEMPERATURE-MIN_TEMPERATURE)/TEMPERATURE_STEP);
	m->Cp = (double*)malloc(sizeof(double)*m->numCp);
	for (i = 0; i < m->numCp; i++) {
		m->Cp[i] = material_lerp_y(Cp,numCp,MIN_TEMPERATURE+(i+0)*TEMPERATURE_STEP);
	}

	//Create tables for k
	m->numk = (int)((MAX_TEMPERATURE-MIN_TEMPERATURE)/TEMPERATURE_STEP);
	m->k = (double*)malloc(sizeof(double)*m->numk);
	for (i = 0; i < m->numk; i++) {
		m->k[i] = material_lerp_y(k,numk,MIN_TEMPERATURE+(i+0)*TEMPERATURE_STEP);
	}
}

double material_lerp_y(float* Y, int numY, double X)
{
	int i,idx;
	if (numY == 1) return Y[0*2+1];

	if (X <= Y[0*2+0]) {
		idx = 0;
	} else if (X > Y[(numY-1)*2+0]) {
		idx = numY-2;
	} else {
		for (i = numY-1; i >= 0; i--) {
			if (X > Y[i*2+0]) {
				idx = i;
				break;
			}
		}
	}
	return Y[idx*2+1] + (X - Y[idx*2+0])*((Y[(idx+1)*2+1] - Y[idx*2+1]) / (Y[(idx+1)*2+0] - Y[idx*2+0]));
}



//==============================================================================
// Initialize highlevel material interface
//==============================================================================
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
int material_highlevel_drawgraph1(lua_State* L)
{
	material_idx m1,m2;
	float x,y,w,h,t;
	float minCp,maxCp;
	float mink,maxk;
	int mode;

	float RGB1[4] = { 0.60f,0.90f,0.75f,1.0f };
	float RGB2[4] = { 0.90f,0.75f,0.10f,1.0f };

	m1 = luaL_checkinteger(L,1);
	m2 = luaL_checkinteger(L,2);
	mode = luaL_checkinteger(L,3);
	x = (float)lua_tonumber(L,4);
	y = (float)lua_tonumber(L,5);
	w = (float)lua_tonumber(L,6);
	h = (float)lua_tonumber(L,7);

#define MIN_T 300.0f
#define MAX_T 2000.0f

	//Find mix/max for data
	minCp = 1e9f;
	maxCp = -1e9f;
	mink = 1e9f;
	maxk = -1e9f;
	for (t = MIN_T; t <= MAX_T; t += TEMPERATURE_STEP) {		
		if (mode == 0) {
			float Cp = (float)material_getCp(m1,t);
			minCp = min(minCp,Cp);
			maxCp = max(maxCp,Cp);
		} else {
			float k = (float)material_getk(m1,t);
			mink = min(mink,k);
			maxk = max(maxk,k);
		}

		if (mode == 0) {
			float Cp = (float)material_getCp(m2,t);
			minCp = min(minCp,Cp);
			maxCp = max(maxCp,Cp);
		} else {
			float k = (float)material_getk(m2,t);
			mink = min(mink,k);
			maxk = max(maxk,k);
		}
	}
	maxk = max(mink+1.0f,maxk);
	maxCp = max(minCp+1.0f,maxCp);

	//Draw curves
	XPLMSetGraphicsState(0,0,0,0,1,0,0);
	glPushMatrix();
	glTranslatef(x,y,0.0f);

	//Draw coordinate system
	glLineWidth(1.0f);
	glColor4f(0.00f,0.00f,0.00f,0.60f);
	glBegin(GL_LINES);
		for (t = MIN_T; t <= MAX_T; t += 100.0f) {
			glVertex2f(w*(t-MIN_T)/(MAX_T-MIN_T),0.0f);
			glVertex2f(w*(t-MIN_T)/(MAX_T-MIN_T),h);
		}

		if (mode == 0) {
			for (t = minCp; t < maxCp; t += (maxCp-minCp)*0.1f) {
				glVertex2f(0.0f,h*((t-minCp)/(maxCp-minCp)));
				glVertex2f(w,h*((t-minCp)/(maxCp-minCp)));
			}
		} else {
			for (t = mink; t < maxk; t += (maxk-mink)*0.1f) {
				glVertex2f(0.0f,h*((t-mink)/(maxk-mink)));
				glVertex2f(w,h*((t-mink)/(maxk-mink)));
			}
		}
	glEnd();

	//Draw Cp
	if (mode == 0) {
		glLineWidth(1.0f);
		glColor4f(0.90f,0.75f,0.10f,0.9f);
		glBegin(GL_LINES);
			glVertex2f(0.0f,h*(((float)material_getCp(m2,MIN_T)-minCp)/(maxCp-minCp)));
			for (t = MIN_T; t <= MAX_T; t += 10.0f) {
				float Cp = (float)material_getCp(m2,t);
				glVertex2f(w*(t-MIN_T)/(MAX_T-MIN_T),h*((Cp-minCp)/(maxCp-minCp)));
				glVertex2f(w*(t-MIN_T)/(MAX_T-MIN_T),h*((Cp-minCp)/(maxCp-minCp)));
			}
			glVertex2f(w,h*(((float)material_getCp(m2,MAX_T)-minCp)/(maxCp-minCp)));
		glEnd();
		glLineWidth(1.0f);
		glColor4f(0.60f,0.90f,0.75f,0.9f);
		glBegin(GL_LINES);
			glVertex2f(0.0f,h*(((float)material_getCp(m1,MIN_T)-minCp)/(maxCp-minCp)));
			for (t = MIN_T; t <= MAX_T; t += 10.0f) {
				float Cp = (float)material_getCp(m1,t);
				glVertex2f(w*(t-MIN_T)/(MAX_T-MIN_T),h*((Cp-minCp)/(maxCp-minCp)));
				glVertex2f(w*(t-MIN_T)/(MAX_T-MIN_T),h*((Cp-minCp)/(maxCp-minCp)));
			}
			glVertex2f(w,h*(((float)material_getCp(m1,MAX_T)-minCp)/(maxCp-minCp)));
		glEnd();
	}

	//Draw k
	if (mode != 0) {
		glLineWidth(1.0f);
		glColor4f(0.90f,0.75f,0.10f,0.9f);
		glBegin(GL_LINES);
			glVertex2f(0.0f,h*(((float)material_getk(m2,MIN_T)-mink)/(maxk-mink)));
			for (t = MIN_T; t <= MAX_T; t += 10.0f) {
				float k = (float)material_getk(m2,t);
				glVertex2f(w*(t-MIN_T)/(MAX_T-MIN_T),h*((k-mink)/(maxk-mink)));
				glVertex2f(w*(t-MIN_T)/(MAX_T-MIN_T),h*((k-mink)/(maxk-mink)));
			}
			glVertex2f(w,h*(((float)material_getk(m2,MAX_T)-mink)/(maxk-mink)));
		glEnd();
		glLineWidth(1.0f);
		glColor4f(0.60f,0.90f,0.75f,0.9f);
		glBegin(GL_LINES);
			glVertex2f(0.0f,h*(((float)material_getk(m1,MIN_T)-mink)/(maxk-mink)));
			for (t = MIN_T; t <= MAX_T; t += 10.0f) {
				float k = (float)material_getk(m1,t);
				glVertex2f(w*(t-MIN_T)/(MAX_T-MIN_T),h*((k-mink)/(maxk-mink)));
				glVertex2f(w*(t-MIN_T)/(MAX_T-MIN_T),h*((k-mink)/(maxk-mink)));
			}
			glVertex2f(w,h*(((float)material_getk(m1,MAX_T)-mink)/(maxk-mink)));
		glEnd();
	}

	//Draw graph values
	for (t = MIN_T; t <= MAX_T-100.0f; t += 100.0f) {
		char buf[ARBITRARY_MAX];
		snprintf(buf,ARBITRARY_MAX-1,"%.0f",t);
		XPLMDrawString(RGB2,(int)(w*(t-MIN_T)/(MAX_T-MIN_T)),10,buf,0,xplmFont_Basic);
	}
	if (mode == 0) {
		XPLMDrawString(RGB1,4,(int)h-20,"Cp [J kg-1 K-1] (specific heat capacity)",0,xplmFont_Basic);
	} else {
		XPLMDrawString(RGB1,4,(int)h-20,"k [J m-1 s-1 K-1] (thermal conductivity)",0,xplmFont_Basic);
	}

	glLineWidth(1.0f);
	glPopMatrix();
	return 0;
}
#endif

int material_highlevel_getcount(lua_State* L)
{
	lua_pushnumber(L,materials_count);
	return 1;
}

int material_highlevel_getname(lua_State* L)
{
	material_idx m = luaL_checkinteger(L,1);
	if ((m >= 0) && (m < materials_count)) {
		lua_pushstring(L,materials[m].name);
	} else {
		lua_pushstring(L,"Unknown");
	}
	return 1;
}

int material_highlevel_getparameters(lua_State* L)
{
	material_idx m = luaL_checkinteger(L,1);
	double t = luaL_checknumber(L,2);
	lua_pushnumber(L,material_getDensity(m));
	lua_pushnumber(L,material_getCp(m,t));
	lua_pushnumber(L,material_getk(m,t));
	return 3;
}


//==============================================================================
// Initialize highlevel material interface
//==============================================================================
void material_highlevel_initialize()
{
	//Register API
	lua_createtable(L,0,32);
	lua_setglobal(L,"MaterialAPI");

	highlevel_addfunction("MaterialAPI","GetCount",material_highlevel_getcount);
	highlevel_addfunction("MaterialAPI","GetName",material_highlevel_getname);
	highlevel_addfunction("MaterialAPI","GetParameters",material_highlevel_getparameters);

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	highlevel_addfunction("MaterialAPI","DrawGUIGraph1",material_highlevel_drawgraph1);
#endif
}