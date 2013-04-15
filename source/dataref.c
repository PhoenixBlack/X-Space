//==============================================================================
// Defining datarefs from X-Space
//==============================================================================
#include <stdlib.h>
#include "dataref.h"

//Include X-Plane SDK
#include <XPLMDataAccess.h>

//==============================================================================
// Create dataref linked to specific variable
//==============================================================================
static float  get_dataref_f_float(void* dataptr)
{
	return *((float*)dataptr);
}
static double get_dataref_f_double(void* dataptr)
{
	return (double)(*((float*)dataptr));
}
static float  get_dataref_d_float(void* dataptr)
{
	return (float)(*((double*)dataptr));
}
static double get_dataref_d_double(void* dataptr)
{
	return *((double*)dataptr);
}
static void set_dataref_f_float(void* dataptr, float value)
{
	if (dataptr) *((float*)dataptr) = value;
}
static void set_dataref_f_double(void* dataptr, double value)
{
	if (dataptr) *((float*)dataptr) = (float)value;
}
static void set_dataref_d_float(void* dataptr, float value)
{
	if (dataptr) *((double*)dataptr) = value;
}
static void set_dataref_d_double(void* dataptr, double value)
{
	if (dataptr) *((double*)dataptr) = value;
}


//==============================================================================
// Functions for reading/writing array
//==============================================================================
static long get_dataref_dv_float(void* dataptr, float* values, int inOffset, int inMax)
{
	double* ptr = (double*)((int*)(dataptr))[0];
	int size = ((int*)(dataptr))[1];
	int i;
	if (!values) return size;

	for (i = inOffset; i < inOffset+inMax; i++) {
		if (i < size) values[i-inOffset] = (float)ptr[i];
	}
	return size;
}

static void set_dataref_dv_float(void* dataptr, float* values, int inOffset, int inMax)
{
	double* ptr = (double*)((int*)(dataptr))[0];
	int size = ((int*)(dataptr))[1];
	int i;
	if (!values) return;

	for (i = inOffset; i < inOffset+inMax; i++) {
		if (i < size) ptr[i] = (double)values[i-inOffset];
	}
}

static long get_dataref_fv_float(void* dataptr, float* values, int inOffset, int inMax)
{
	float* ptr = (float*)((int*)(dataptr))[0];
	int size = ((int*)(dataptr))[1];
	int i;
	if (!values) return size;

	for (i = inOffset; i < inOffset+inMax; i++) {
		if (i < size) values[i-inOffset] = ptr[i];
	}
	return size;
}

static void set_dataref_fv_float(void* dataptr, float* values, int inOffset, int inMax)
{
	float* ptr = (float*)((int*)(dataptr))[0];
	int size = ((int*)(dataptr))[1];
	int i;
	if (!values) return;

	for (i = inOffset; i < inOffset+inMax; i++) {
		if (i < size) ptr[i] = values[i-inOffset];
	}
}


//==============================================================================
// Create datarefs
//==============================================================================
//#define EXPORT_DATAREFS
void* dataref_f(const char* name, float* dataptr)
{
#ifdef EXPORT_DATAREFS
	log_write("%s\tf\n",name);
#endif
	return XPLMRegisterDataAccessor(name,xplmType_Float,1,0,0,
									get_dataref_f_float, set_dataref_f_float,
									get_dataref_f_double,set_dataref_f_double,0,0,0,0,0,0,
									(void*)dataptr,(void*)dataptr);
}

void* dataref_d(const char* name, double* dataptr)
{
#ifdef EXPORT_DATAREFS
	log_write("%s\td\n",name);
#endif
	return XPLMRegisterDataAccessor(name,xplmType_Double,1,0,0,
									get_dataref_d_float, set_dataref_d_float,
									get_dataref_d_double,set_dataref_d_double,0,0,0,0,0,0,
									(void*)dataptr,(void*)dataptr);
}

void dataref_dv(const char* name, int count, double* dataptr)
{
	int* array_info = malloc(2*sizeof(int));
	array_info[0] = (int)dataptr;
	array_info[1] = count;
#ifdef EXPORT_DATAREFS
	log_write("%s\tdv\n",name);
#endif
	XPLMRegisterDataAccessor(name,xplmType_FloatArray,1,0,0,
	                         0,0,0,0,0,0,get_dataref_dv_float,set_dataref_dv_float,0,0,
	                         (void*)array_info,(void*)array_info);
}

void dataref_fv(const char* name, int count, float* dataptr)
{
	int* array_info = malloc(2*sizeof(int));
	array_info[0] = (int)dataptr;
	array_info[1] = count;
#ifdef EXPORT_DATAREFS
	log_write("%s\tfv\n",name);
#endif
	XPLMRegisterDataAccessor(name,xplmType_FloatArray,1,0,0,
	                         0,0,0,0,0,0,get_dataref_fv_float,set_dataref_fv_float,0,0,
	                         (void*)array_info,(void*)array_info);
}