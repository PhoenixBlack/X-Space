//==============================================================================
// Atmosphere related calculations
//==============================================================================
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <nrlmsise-00.h>
#include "vessel.h"
#include "dataref.h"
#include "config.h"
#include "atmosphere.h"


//==============================================================================
// Calculate local atmosphere for a given vessel
//==============================================================================
void atmosphere_simulate(vessel* v)
{
	struct nrlmsise_output output;
	struct nrlmsise_input input;
	struct nrlmsise_flags flags;
	struct ap_array aph;
	double vmag;
	int i;
	struct tm* cur_time;
	time_t t;

	for (i = 0; i < 7; i++) aph.a[i] = 4.0;

	//Get current time
	t = time(0);
	cur_time = gmtime(&t);

	input.year = 1900+cur_time->tm_year;
	input.doy = cur_time->tm_yday;
	input.sec = cur_time->tm_sec+cur_time->tm_min*60+cur_time->tm_hour*3600;
	input.alt = v->elevation*1e-3;
	input.g_lat = v->latitude;
	input.g_long = v->longitude;
	input.lst = input.sec/3600.0 + input.g_long/15.0;
	input.f107 = 150.0;
	input.f107A = 150.0;
	input.ap = 4.0;
	input.ap_a = &aph;

	flags.switches[0] = 0;
	for (i = 1; i < 24; i++) flags.switches[i]=1;

	if (v->elevation < 200000) { //Mass density
		gtd7(&input, &flags, &output);
	} else { //Effective density
		gtd7d(&input, &flags, &output);
	}

	v->air.density_He	= 1e-3*output.d[0]*4.0/6.022e23;
	v->air.density_O	= 1e-3*output.d[1]*16.0/6.022e23;
	v->air.density_N2	= 1e-3*output.d[2]*28.0/6.022e23;
	v->air.density_O2	= 1e-3*output.d[3]*32.0/6.022e23;
	v->air.density_Ar	= 1e-3*output.d[4]*40.0/6.022e23;
	v->air.density_H	= 1e-3*output.d[6]*1.0/6.022e23;
	v->air.density_N	= 1e-3*output.d[7]*14.0/6.022e23;
	v->air.density		= output.d[5]*1e3;
	v->air.concentration_He	= output.d[0];
	v->air.concentration_O	= output.d[1];
	v->air.concentration_N2	= output.d[2];
	v->air.concentration_O2	= output.d[3];
	v->air.concentration_Ar	= output.d[4];
	v->air.concentration_H	= output.d[6];
	v->air.concentration_N	= output.d[7];
	v->air.concentration = output.d[0]+output.d[1]+output.d[2]+
	                       output.d[3]+output.d[4]+output.d[6]+
	                       output.d[7];

	v->air.temperature = output.t[1];
	v->air.exospheric_temperature = output.t[0];
	v->air.pressure = 287*output.t[1]*output.d[5]*1e3; //P = (R[air]T)/d

	vmag = sqrt(v->sim.vx*v->sim.vx+v->sim.vy*v->sim.vy+v->sim.vz*v->sim.vz);
	v->air.Q = (0.5)*vmag*vmag*v->air.density;
}


//==============================================================================
// Initialize atmospheric datarefs
//==============================================================================
void atmosphere_initialize()
{
	if (config.write_atmosphere) {
		FILE* out = fopen("./X-Space_Atmosphere.txt","w+");
		struct nrlmsise_output output;
		struct nrlmsise_input input;
		struct nrlmsise_flags flags;
		struct ap_array aph;
		double h;
		int i;
		struct tm* cur_time;
		time_t t;

		//Setup flags
		for (i = 0; i < 7; i++) aph.a[i] = 4.0;
		flags.switches[0] = 0;
		for (i = 1; i < 24; i++) flags.switches[i]=1;

		//Get current time
		t = time(0);
		cur_time = gmtime(&t);

		//Setup date/location
		input.year = 1900+cur_time->tm_year;
		input.doy = cur_time->tm_yday;
		input.sec = cur_time->tm_sec+cur_time->tm_min*60+cur_time->tm_hour*3600;
		input.g_lat = 0.0;
		input.g_long = 0.0;
		input.lst = input.sec/3600.0 + input.g_long/15.0;
		input.f107 = 150.0;
		input.f107A = 150.0;
		input.ap = 4.0;
		input.ap_a = &aph;

		//Output atmosphere
		fprintf(out,"NRLMSISE-00 ATMOSPHERE\tEPOCH %d/%d\tLAT/LON %f %f\n",
		        input.year,input.doy,input.g_lat,input.g_long);
		for (h = 0.0; h < 2500e3; h +=
		            (h >= 10000 ? (h >= 100000 ? (h >= 1000000 ? 10000 : 10000) : 1000) : 100)) {
			input.alt = h*1e-3;

			gtd7(&input, &flags, &output);
			fprintf(out,"%f m\t%e kg/m3\t%.5f degC\n",
			        h,output.d[5]*1e3,output.t[1]-273.15);
		}
		fclose(out);
	}

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	dataref_d("xsp/local/atmosphere/Q",						&vessels[0].air.Q);
	dataref_d("xsp/local/atmosphere/density",				&vessels[0].air.density);
	dataref_d("xsp/local/atmosphere/density_He",			&vessels[0].air.density_He);
	dataref_d("xsp/local/atmosphere/density_O",				&vessels[0].air.density_O);
	dataref_d("xsp/local/atmosphere/density_N2",			&vessels[0].air.density_N2);
	dataref_d("xsp/local/atmosphere/density_O2",			&vessels[0].air.density_O2);
	dataref_d("xsp/local/atmosphere/density_Ar",			&vessels[0].air.density_Ar);
	dataref_d("xsp/local/atmosphere/density_H",				&vessels[0].air.density_H);
	dataref_d("xsp/local/atmosphere/density_N",				&vessels[0].air.density_N);
	dataref_d("xsp/local/atmosphere/concentration",			&vessels[0].air.concentration);
	dataref_d("xsp/local/atmosphere/concentration_He",		&vessels[0].air.concentration_He);
	dataref_d("xsp/local/atmosphere/concentration_O",		&vessels[0].air.concentration_O);
	dataref_d("xsp/local/atmosphere/concentration_N2",		&vessels[0].air.concentration_N2);
	dataref_d("xsp/local/atmosphere/concentration_O2",		&vessels[0].air.concentration_O2);
	dataref_d("xsp/local/atmosphere/concentration_Ar",		&vessels[0].air.concentration_Ar);
	dataref_d("xsp/local/atmosphere/concentration_H",		&vessels[0].air.concentration_H);
	dataref_d("xsp/local/atmosphere/concentration_N",		&vessels[0].air.concentration_N);
	dataref_d("xsp/local/atmosphere/temperature",			&vessels[0].air.temperature);
	dataref_d("xsp/local/atmosphere/exospheric_temperature",&vessels[0].air.exospheric_temperature);
#endif
}
