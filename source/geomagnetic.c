#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <wmm.h>

#include "x-space.h"
#include "vessel.h"
#include "dataref.h"
#include "coordsys.h"
#include "geomagnetic.h"
#include "planet.h"

#include <soil.h>

//Models
WMMtype_MagneticModel* geomagnetic_MagneticModel;
WMMtype_MagneticModel* geomagnetic_TimedMagneticModel;
WMMtype_GravitationalModel* geomagnetic_GravitationalModel;
WMMtype_Ellipsoid geomagnetic_Ellipsoid;
WMMtype_Geoid geomagnetic_Geoid;

//Variables
WMMtype_GeoMagneticElements geomagnetic_Elements;
WMMtype_LegendreFunction* geomagnetic_LegendreFunction;


//==============================================================================
// Loads geomagnetic model
//==============================================================================
void geomagnetic_initialize()
{
	int num_terms = ((WMM_MAX_MODEL_DEGREES + 1) * (WMM_MAX_MODEL_DEGREES + 2) / 2);

	//Report info
	log_write("X-Space: Loading %d magnetic model terms\n",num_terms);

	//For storing the WMM Model parameters
	geomagnetic_MagneticModel = WMM_AllocateModelMemory(num_terms);
	//For storing the gravitational parameters
	geomagnetic_GravitationalModel = WMM_AllocateGModelMemory(num_terms);
	//For storing the time modified WMM Model parameters
	geomagnetic_TimedMagneticModel = WMM_AllocateModelMemory(num_terms);
	if ((!geomagnetic_MagneticModel) || (!geomagnetic_TimedMagneticModel)) {
		log_write("X-Space: Problem when loading/initializing magnetic model\n");
		return;
	}

	//Set default values and constants
	WMM_SetDefaults(&geomagnetic_Ellipsoid, geomagnetic_MagneticModel, &geomagnetic_Geoid);
	geomagnetic_GravitationalModel->nMax = geomagnetic_MagneticModel->nMax;

	//Read magnetic model
	//WMM_readMagneticModel_Large(filename, MagneticModel);
	WMM_readMagneticModel(FROM_PLUGINS("wmm.cof"), geomagnetic_MagneticModel);
	//Read gravitational model
	WMM_readGravitationalModel(FROM_PLUGINS("egm96.cof"), geomagnetic_GravitationalModel);
	//Read the Geoid file
	WMM_InitializeGeoid(FROM_PLUGINS("wgs84.bin"), &geomagnetic_Geoid);

	//Initialize legendre function memory
	geomagnetic_LegendreFunction = WMM_AllocateLegendreFunctionMemory(num_terms);

	//Print information
	log_write("X-Space: Magnetic model %s (epoch %.0f)\n",
	          geomagnetic_MagneticModel->ModelName,
	          geomagnetic_MagneticModel->epoch);

	//Register datarefs
#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	dataref_d("xsp/local/magnetic/decl",	&vessels[0].geomagnetic.declination);
	dataref_d("xsp/local/magnetic/incl",	&vessels[0].geomagnetic.inclination);
	dataref_d("xsp/local/magnetic/F",		&vessels[0].geomagnetic.F);
	dataref_d("xsp/local/magnetic/H",		&vessels[0].geomagnetic.H);
	dataref_d("xsp/local/magnetic/X",		&vessels[0].geomagnetic.local.x);
	dataref_d("xsp/local/magnetic/Y",		&vessels[0].geomagnetic.local.y);
	dataref_d("xsp/local/magnetic/Z",		&vessels[0].geomagnetic.local.z);
	dataref_d("xsp/local/magnetic/GV",		&vessels[0].geomagnetic.GV);
	dataref_d("xsp/local/gravitational/g",	&vessels[0].geomagnetic.g);
	dataref_d("xsp/local/gravitational/V",	&vessels[0].geomagnetic.V);
	dataref_d("xsp/noninertial/magnetic/X",	&vessels[0].geomagnetic.noninertial.x);
	dataref_d("xsp/noninertial/magnetic/Y",	&vessels[0].geomagnetic.noninertial.y);
	dataref_d("xsp/noninertial/magnetic/Z",	&vessels[0].geomagnetic.noninertial.z);
#endif

#if 0
	{
#define MUL		4
#define GEN_W	360*MUL
#define GEN_H	180*MUL
		static char texture[GEN_W*GEN_H*3];
		double ref[GEN_H];
		int i,j;

		vessel_count = 1;
		current_planet.mu = 398600.4418*1e9;
		for (j = 0; j < GEN_H; j++) {
			ref[j] = 0.0;
			for (i = 0; i < 180; i++) {
				vessels[0].longitude = 2.0*i-180;
				vessels[0].latitude = 1.0*j/(1.0*MUL)-90;
				vessels[0].elevation = 10000.0;
				geomagnetic_update();
				ref[j] += vessels[0].geomagnetic.g;
			}
			ref[j] = ref[j]/180.0;
		}

		for (j = 0; j < GEN_H; j++) {
			for (i = 0; i < GEN_W; i++) {
				vessels[0].longitude = 1.0*i/(1.0*MUL)-180;
				vessels[0].latitude = 1.0*j/(1.0*MUL)-90;
				vessels[0].elevation = 10000.0;
				geomagnetic_update();

				texture[(i+(GEN_H-j-1)*GEN_W)*3+0] = min(0xFF,max(0,(vessels[0].geomagnetic.g-ref[j])*700000));
				texture[(i+(GEN_H-j-1)*GEN_W)*3+1] = min(0xFF,max(0,(vessels[0].geomagnetic.g-ref[j])*1300000));
				texture[(i+(GEN_H-j-1)*GEN_W)*3+2] = min(0xFF,max(0,0x7F+(vessels[0].geomagnetic.g-ref[j])*1300000));

				//vessels[0].latitude = 1.0*i/4.0;
				//vessels[0].longitude = 1.0*j/4.0;
				//vessels[0].elevation = 0.0;
				//geomagnetic_update();


				//Write color
				//texture[(i+j*GEN_W)*3+0] = (color & 0xFF);
				//texture[(i+j*GEN_W)*3+1] = (color & 0xFF00) >> 8;
				//texture[(i+j*GEN_W)*3+2] = (color & 0xFF0000) >> 16;
			}
		}
		vessel_count = 0;

		SOIL_save_image(FROM_PLUGINS("geomag2.bmp"),SOIL_SAVE_TYPE_BMP,
			GEN_W,GEN_H,3,texture);
	}
#endif
}


//==============================================================================
// Unloads geomagnetic model
//==============================================================================
void geomagnetic_deinitialize()
{
	WMM_FreeMagneticModelMemory(geomagnetic_MagneticModel);
	WMM_FreeMagneticModelMemory(geomagnetic_TimedMagneticModel);
	WMM_FreeGravitationalModelMemory(geomagnetic_GravitationalModel);
	WMM_FreeLegendreMemory(geomagnetic_LegendreFunction);

	if (geomagnetic_Geoid.GeoidHeightBuffer) {
		free(geomagnetic_Geoid.GeoidHeightBuffer);
		geomagnetic_Geoid.GeoidHeightBuffer = NULL;
	}
}


//==============================================================================
// Updates the model and datarefs
//==============================================================================
void geomagnetic_update()
{
	int i;
	for (i = 0; i < vessel_count; i++) { //FIXME: can optimize out vessels which don't need magnetic data
		WMMtype_CoordSpherical CoordSpherical;
		WMMtype_CoordGeodetic CoordGeodetic;
		WMMtype_Date UserDate;
	
		//Set coordinates and time
		CoordGeodetic.phi = vessels[i].latitude;
		CoordGeodetic.lambda = vessels[i].longitude;
		CoordGeodetic.HeightAboveGeoid = vessels[i].elevation*1e-3;
		WMM_ConvertGeoidToEllipsoidHeight(&CoordGeodetic, &geomagnetic_Geoid);
		UserDate.DecimalYear = 2012;
		geomagnetic_GravitationalModel->mu = current_planet.mu;

		//Convert from geodeitic to Spherical Equations: 17-18, WMM Technical report
		WMM_GeodeticToSpherical(geomagnetic_Ellipsoid, CoordGeodetic, &CoordSpherical);
		//Time adjust the coefficients, Equation 19, WMM Technical report
		WMM_TimelyModifyMagneticModel(UserDate, geomagnetic_MagneticModel, geomagnetic_TimedMagneticModel);
		//Computes the geoMagnetic field elements and their time change
		WMM_Geomag(geomagnetic_Ellipsoid, 
			CoordSpherical, 
			CoordGeodetic, 
			geomagnetic_GravitationalModel, 
			geomagnetic_TimedMagneticModel, 
			&geomagnetic_Elements, 
			geomagnetic_LegendreFunction);
		//Computes grid variation
		WMM_CalculateGridVariation(CoordGeodetic, &geomagnetic_Elements);

		vessels[i].geomagnetic.inclination = geomagnetic_Elements.Incl;
		vessels[i].geomagnetic.declination = geomagnetic_Elements.Decl;
		vessels[i].geomagnetic.GV = geomagnetic_Elements.GV;
		vessels[i].geomagnetic.H = geomagnetic_Elements.H;
		vessels[i].geomagnetic.F = geomagnetic_Elements.F;
		vessels[i].geomagnetic.V = geomagnetic_Elements.V;
		vessels[i].geomagnetic.g = geomagnetic_Elements.g;
		vessels[i].geomagnetic.noninertial.x = geomagnetic_Elements.X;
		vessels[i].geomagnetic.noninertial.y = geomagnetic_Elements.Y;
		vessels[i].geomagnetic.noninertial.z = geomagnetic_Elements.Z;

		//Convert the result into local coordinates
		vec_ni2l(&vessels[0],
				geomagnetic_Elements.X,
				geomagnetic_Elements.Y,
				geomagnetic_Elements.Z,
				&vessels[i].geomagnetic.local.x,
				&vessels[i].geomagnetic.local.y,
				&vessels[i].geomagnetic.local.z);
	}
}


//==============================================================================
// WMM error handler
//==============================================================================
void WMM_Error(int control)
{
	switch (control) {
		case 1:
			log_write("X-Space: WMM: Error allocating in WMM_AllocateLegendreFunctionMemory.\n");
			break;
		case 2:
			log_write("X-Space: WMM: Error allocating in WMM_AllocateModelMemory.\n");
			break;
		case 3:
			log_write("X-Space: WMM: Error allocating in WMM_InitializeGeoid\n");
			break;
		case 4:
			log_write("X-Space: WMM: Error in setting default values.\n");
			break;
		case 5:
			log_write("X-Space: WMM: Error initializing Geoid.\n");
			break;
		case 6:
			log_write("X-Space: WMM: Error opening WMM.COF\n.");
			break;
		case 7:
			log_write("X-Space: WMM: Error opening WMMSV.COF\n.");
			break;
		case 8:
			log_write("X-Space: WMM: Error reading Magnetic Model.\n");
			break;
		case 9:
			log_write("X-Space: WMM: Error printing Command Prompt introduction.\n");
			break;
		case 10:
			log_write("X-Space: WMM: Error converting from geodetic co-ordinates to spherical co-ordinates.\n");
			break;
		case 11:
			log_write("X-Space: WMM: Error in time modifying the Magnetic model\n");
			break;
		case 12:
			log_write("X-Space: WMM: Error in Geomagnetic\n");
			break;
		case 13:
			log_write("X-Space: WMM: Error printing user data\n");
			break;
		case 14:
			log_write("X-Space: WMM: Error allocating in WMM_SummationSpecial\n");
			break;
		case 15:
			log_write("X-Space: WMM: Error allocating in WMM_SecVarSummationSpecial\n");
			break;
		case 16:
			log_write("X-Space: WMM: Error in opening EGM9615.BIN file\n");
			break;
		case 17:
			log_write("X-Space: WMM: Error: Latitude OR Longitude out of range in WMM_GetGeoidHeight\n");
			break;
		case 18:
			log_write("X-Space: WMM: Error allocating in WMM_PcupHigh\n");
			break;
		case 19:
			log_write("X-Space: WMM: Error allocating in WMM_PcupLow\n");
			break;
		case 20:
			log_write("X-Space: Error opening coefficient file\n");
			break;
		case 21:
			log_write("X-Space: WMM: Error: UnitDepth too large\n");
			break;
		case 22:
			log_write("X-Space: WMM: Your system needs big endian version of EGM9615.BIN\n");
			log_write("Please download this file from http://www.ngdc.noaa.gov/geomag/WMM/DoDWMM.shtml\n");
			log_write("Replace the existing EGM9615.BIN file with the downloaded one\n");
			break;
	}
}