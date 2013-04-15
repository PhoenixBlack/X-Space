#include "curtime.h"

#ifdef WIN32
#include <windows.h>
#include <time.h>

double t0_res = 0.0; //resolution
unsigned __int64 t0_64 = 0; //initial time
int t0_init = 0; //initialized
double t0_mjd; //MJD date
double t0_time; //MJD time for which date is specified

void curtime_initialize()
{
	unsigned __int64 frequency;
	if (QueryPerformanceFrequency((LARGE_INTEGER*)&frequency)) {
		t0_res = 1.0 / (double)frequency;
		t0_init = 1;
		QueryPerformanceCounter((LARGE_INTEGER*)&t0_64);
	}
	t0_mjd = (time(0) / 86400.0) + 2440587.5 - 2400000.5;;
	t0_time = curtime();
}

double curtime()
{
	unsigned __int64 t_64;
	if (!t0_init) curtime_initialize();
	if (QueryPerformanceCounter((LARGE_INTEGER*)&t_64)) {
		return (double)(t_64 - t0_64)*t0_res;
	} else {
		return 0.0;
	}
}

double curtime_mjd() {
	return (curtime() - t0_time)/86400.0 + t0_mjd;
}

#else

#include <sys/time.h>
#include <time.h>

double t0_res = 1.0; //resolution
long long int t0_64 = 0; //initial time
int t0_init = 0; //initialized
double t0_mjd; //MJD date
double t0_time; //MJD time for which date is specified

void curtime_initialize()
{
	struct timeval  tv;

	t0_res = 1e-6; //1 usec resolution

	gettimeofday(&tv, 0);
	t0_64 = (long long int)tv.tv_sec*(long long int)1000000 + (long long int) tv.tv_usec;

	t0_init = 1;
	t0_mjd = (time(0) / 86400.0) + 2440587.5 - 2400000.5;;
	t0_time = curtime();
}

double curtime()
{
	long long int t0;
	struct timeval  tv;
	if (!t0_init) curtime_initialize();

	gettimeofday(&tv, 0);
	t0 = (long long int)tv.tv_sec*(long long int)1000000 + (long long int) tv.tv_usec;

	return (double)(t0 - t0_64)*t0_res;
}

double curtime_mjd() {
	return (curtime() - t0_time)/86400.0 + t0_mjd;
}
#endif