#ifndef MATERIAL_H
#define MATERIAL_H

#define MIN_TEMPERATURE		0.0
#define MAX_TEMPERATURE		5000.0
#define TEMPERATURE_STEP	5.0


//==============================================================================
// Material structure
//==============================================================================
typedef struct material {
	char name[256];
	double density; //kg/m3

	//Temperature boundaries for interpolated values
	double minTemperature,maxTemperature;

	//Specific heat
	int numCp;
	double* Cp; //J/(kg*K)

	//Thermal conductivity
	int numk;
	double* k;
} material;

//==============================================================================
// Material type
//==============================================================================
typedef int material_idx;

//All loaded materials
extern material* materials;
extern int materials_count;

//Functions to work with materials
void material_initialize();
void material_deinitialize();
material_idx material_get(char* name);
double material_getCp(material_idx midx, double temperature);
double material_getk(material_idx midx, double temperature);
double material_getDensity(material_idx midx);

//Internal functions used for loading
void material_lerp_calc(material* m, float* Cp, int numCp, float* k, int numk);
double material_lerp_y(float* Y, int numY, double X);

//Highlevel interface
void material_highlevel_initialize();

#endif