#ifndef GEOMAG_H
#define GEOMAG_H

#include "wmm.h"

void geomagnetic_initialize();
void geomagnetic_deinitialize();
void geomagnetic_update();

WMMtype_GeoMagneticElements geomagnetic_inertial_elements;

#endif
