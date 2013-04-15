#include <ivss.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "vessel.h"
#include "x-ivss.h"
#include "x-ivss_vsfl.h"

#include "threading.h"
#include "curtime.h"

//==============================================================================
#define IVSS_TYPE_VSFL_TIME_SATELLITE_PROTOTYPE_1 0

int IVSS_Interface_VSFL_RegisterUnit(IVSS_SYSTEM* system, IVSS_SIMULATOR* simulator, IVSS_UNIT* unit)
{
	if (strcmp(unit->model_name,"TimeSatellitePrototype1") == 0) {
		unit->type = IVSS_TYPE_VSFL_TIME_SATELLITE_PROTOTYPE_1;
		return IVSS_CLAIM_UNIT;
	}
	return IVSS_IGNORE_UNIT;
}

//==============================================================================
int IVSS_Interface_VSFL_Simulation(IVSS_SYSTEM* system, IVSS_SIMULATOR* simulator, IVSS_UNIT* unit)
{
	vessel* v = system->userdata;
	if (unit->type == IVSS_TYPE_VSFL_TIME_SATELLITE_PROTOTYPE_1) {
		int time_marker1,i;
		unsigned short time_marker2;
		unsigned short magnetic_x;
		unsigned short magnetic_y;
		unsigned short magnetic_z;
		IVSS_VARIABLE* variable;
		IVSS_Unit_GetNode(system,unit,"RadioBus",&variable);

		if (v->elevation > 250000) {
			magnetic_x = (unsigned short)(v->geomagnetic.noninertial.x + (-500.0 + 1000.0*(1.0*rand()/(1.0*RAND_MAX))));
			magnetic_y = (unsigned short)(v->geomagnetic.noninertial.y + (-500.0 + 1000.0*(1.0*rand()/(1.0*RAND_MAX))));
			magnetic_z = (unsigned short)(v->geomagnetic.noninertial.z + (-500.0 + 1000.0*(1.0*rand()/(1.0*RAND_MAX))));
			time_marker1 = (int)((curtime_mjd()-55927)*86400);
			time_marker2 = (unsigned short)(fmod((curtime_mjd()-55927)*86400,1.0)*0xFFFF);

			for (i=0;i<128;i++) { //1024 bits marker
				IVSS_Variable_WriteBuffer(system,variable,0xAA);
			}
			IVSS_Variable_WriteBuffer(system,variable,(time_marker1 >>  0) & 0xFF); //First duplicate
			IVSS_Variable_WriteBuffer(system,variable,(time_marker1 >>  8) & 0xFF);
			IVSS_Variable_WriteBuffer(system,variable,(time_marker1 >> 16) & 0xFF);
			IVSS_Variable_WriteBuffer(system,variable,(time_marker1 >> 24) & 0xFF);
			IVSS_Variable_WriteBuffer(system,variable,(time_marker2 >>  0) & 0xFF);
			IVSS_Variable_WriteBuffer(system,variable,(time_marker2 >>  8) & 0xFF);

			IVSS_Variable_WriteBuffer(system,variable,(time_marker1 >>  0) & 0xFF); //Second duplicate
			IVSS_Variable_WriteBuffer(system,variable,(time_marker1 >>  8) & 0xFF);
			IVSS_Variable_WriteBuffer(system,variable,(time_marker1 >> 16) & 0xFF);
			IVSS_Variable_WriteBuffer(system,variable,(time_marker1 >> 24) & 0xFF);
			IVSS_Variable_WriteBuffer(system,variable,(time_marker2 >>  0) & 0xFF);
			IVSS_Variable_WriteBuffer(system,variable,(time_marker2 >>  8) & 0xFF);

			IVSS_Variable_WriteBuffer(system,variable,(magnetic_x >> 0) & 0xFF); //Service data
			IVSS_Variable_WriteBuffer(system,variable,(magnetic_x >> 8) & 0xFF);
			IVSS_Variable_WriteBuffer(system,variable,(magnetic_y >> 0) & 0xFF);
			IVSS_Variable_WriteBuffer(system,variable,(magnetic_y >> 8) & 0xFF);
			IVSS_Variable_WriteBuffer(system,variable,(magnetic_z >> 0) & 0xFF);
			IVSS_Variable_WriteBuffer(system,variable,(magnetic_z >> 8) & 0xFF);
		}
		thread_sleep(1.0f);

		/*IVSS_Unit_GetParameter(system,unit,"Channel",&variable_aux);
		while (IVSS_Variable_ReadBuffer(system,variable,&value) == IVSS_OK) {
			radiosys_transmit(v,(int)variable_aux->value[0],(char)(value));
		}*/
	}
	return IVSS_OK;
}
/*
int IVSS_Interface_VSFL_Frame(IVSS_SYSTEM* system, IVSS_UNIT* unit)
{
	IVSS_VARIABLE* variable;
	IVSS_VARIABLE* variable_aux;
	vessel* v = system->userdata;
	if (unit->type == IVSS_TYPE_VSFL_RADIO) {
		double value;
		
	} else {
		int index,arr_index,vidx,is_attached;

		IVSS_Unit_GetParameter(system,unit,"Index",&variable_aux);
		index = (int)(variable_aux->value[0]);

		IVSS_Unit_GetParameter(system,unit,"ArrayIndex",&variable_aux);
		arr_index = (int)(variable_aux->value[0]);

		if (IVSS_Unit_GetParameter(system,unit,"Vessel",&variable_aux) != IVSS_ERROR_NOT_FOUND) {
			vidx = (int)(variable_aux->value[0]);
			if ((vidx >= 0) && (vidx < vessel_count)) {
				v = &vessels[vidx];
			}
		}

		is_attached = 1;
		if (IVSS_Unit_GetParameter(system,unit,"CheckAttached",&variable_aux) != IVSS_ERROR_NOT_FOUND) {
			if ((int)(variable_aux->value[0])) {
				is_attached = v->attached;
			}
		}

		//Read or write variable
		if (IVSS_Unit_GetNode(system,unit,"Set",&variable) != IVSS_ERROR_NOT_FOUND) {
			if (is_attached) {
				vessels_set_parameter(v,index,arr_index,variable->value[0]);
			}
		} else {
			IVSS_Unit_GetNode(system,unit,"Value",&variable);
			if (!variable->invalid) {
				if (is_attached) {
					variable->value[0] = vessels_get_parameter(v,index,arr_index);
				} else {
					variable->value[0] = 0.0;
				}
			}
		}
	}
	return IVSS_OK;
}*/

IVSS_SIMULATOR IVSS_Interface_VSFL = {
	"VSFL",
	IVSS_Interface_VSFL_RegisterUnit,
	0,
	IVSS_Interface_VSFL_Simulation,
	0,
	0
};


int IVSS_Interface_VSFL_Register(IVSS_SYSTEM* system) {
	return IVSS_Simulator_Register(system,&IVSS_Interface_VSFL);
}

void xivss_initialize_vsfl(IVSS_SYSTEM* system) {
	IVSS_Interface_VSFL_Register(system);
}