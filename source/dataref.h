#ifndef DATAREF_H
#define DATAREF_H

//Dataref creation
void* dataref_f(const char* name, float* dataptr); //Float
void* dataref_d(const char* name, double* dataptr); //Double
void  dataref_dv(const char* name, int count, double* dataptr); //Double array
void  dataref_fv(const char* name, int count, float* dataptr); //Float array
void  dataref_dv_dyn(const char* name, int count, double* dataptr); //Double dynamic array

#endif
