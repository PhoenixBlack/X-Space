#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "x-space.h"
#include "vessel.h"
#include "sol.h"

//==============================================================================
// DE405
#define EPH_ARRAY_SIZE 1018
#define MERCURY		0
#define VENUS		1
#define EARTH		2	/* Earth-Moon Barycenter */
#define MARS		3
#define JUPITER		4
#define SATURN		5
#define URANUS		6
#define NEPTUNE		7
#define PLUTO		8
#define MOON		9	/* Relative to geocenter */
#define SUN			10
#define SS_BARY		11
#define EM_BARY		12
#define NUTATIONS	13
#define LIBRATIONS	14


//==============================================================================
struct eph_data1 {
	char label[3][84];
	char constName[400][6];
	double timeData[3];
	long int numConst;
	double AU;
	double EMRAT;
	long int coeffPtr[12][3];
	long int DENUM;
	long int libratPtr[3];
};

struct eph_data2 {
	double constValue[400];
}; 

typedef struct eph_data1 eph_data1_t;
typedef struct eph_data2 eph_data2_t;

struct headerOne {
	eph_data1_t data;
	char pad[EPH_ARRAY_SIZE*sizeof(double) - sizeof(eph_data1_t)];
} H1;
struct headerTwo {
   eph_data2_t data;
   char pad[EPH_ARRAY_SIZE*sizeof(double) - sizeof(eph_data2_t)];
} H2;
eph_data1_t R1;
double Coeff_Array[EPH_ARRAY_SIZE], T_beg, T_end, T_span;
FILE* Ephemeris_File;

typedef struct solsystem_state_t {
	double Position[3];
	double Velocity[3];
} solsystem_state;

//==============================================================================
void DE405_Read_Coefficients( double Time )
{
  double  T_delta = 0.0;
  int     Offset  =  0 ;

  /*--------------------------------------------------------------------------*/
  /*  Find ephemeris data that record contains input time. Note that one, and */
  /*  only one, of the following conditional statements will be true (if both */
  /*  were false, this function would not have been called).                  */
  /*--------------------------------------------------------------------------*/

  if ( Time < T_beg )                    /* Compute backwards location offset */
     {
       T_delta = T_beg - Time;
       Offset  = (int) -ceil(T_delta/T_span); 
     }

  if ( Time > T_end )                    /* Compute forewards location offset */
     {
       T_delta = Time - T_end;
       Offset  = (int) ceil(T_delta/T_span);
     }

  /*--------------------------------------------------------------------------*/
  /*  Retrieve ephemeris data from new record.                                */
  /*--------------------------------------------------------------------------*/

  fseek(Ephemeris_File,(Offset-1)*EPH_ARRAY_SIZE*sizeof(double),SEEK_CUR);
  fread(&Coeff_Array,sizeof(double),EPH_ARRAY_SIZE,Ephemeris_File);
  
  T_beg  = Coeff_Array[0];
  T_end  = Coeff_Array[1];
  T_span = T_end - T_beg;
}

void DE405_Interpolate_State(double Time, int Target, solsystem_state *Planet)
{
  double    A[50]   , B[50] , Cp[50] , P_Sum[3] , V_Sum[3] , Up[50] ,
            T_break , T_seg , T_sub  , Tc;
  int       i , j;
  long int  C , G , N , offset = 0;
  solsystem_state X;

  /*--------------------------------------------------------------------------*/
  /* This function doesn't "do" nutations or librations.                      */
  /*--------------------------------------------------------------------------*/

  if ( Target >= 11 )             /* Also protects against weird input errors */
     {
       //printf("\n This function does not compute nutations or librations.\n");
       return;
     }

  /*--------------------------------------------------------------------------*/
  /* Initialize local coefficient array.                                      */
  /*--------------------------------------------------------------------------*/

  for ( i=0 ; i<50 ; i++ )
      {
        A[i] = 0.0;
        B[i] = 0.0;
      }

  /*--------------------------------------------------------------------------*/
  /* Determine if a new record needs to be input.                             */
  /*--------------------------------------------------------------------------*/
  
  if (Time < T_beg || Time > T_end)  {
	  DE405_Read_Coefficients(Time);
  }

  /*--------------------------------------------------------------------------*/
  /* Read the coefficients from the binary record.                            */
  /*--------------------------------------------------------------------------*/
  
  C = R1.coeffPtr[Target][0] - 1;               /*    Coeff array entry point */
  N = R1.coeffPtr[Target][1];                   /*          Number of coeff's */
  G = R1.coeffPtr[Target][2];                   /* Granules in current record */

  /*...................................................Debug print (optional) */

  /*--------------------------------------------------------------------------*/
  /*  Compute the normalized time, then load the Tchebeyshev coefficients     */
  /*  into array A[]. If T_span is covered by a single granule this is easy.  */
  /*  If not, the granule that contains the interpolation time is found, and  */
  /*  an offset from the array entry point for the ephemeris body is used to  */
  /*  load the coefficients.                                                  */
  /*--------------------------------------------------------------------------*/

  if ( G == 1 )
     {
       Tc = 2.0*(Time - T_beg) / T_span - 1.0;
       for (i=C ; i<(C+3*N) ; i++)  A[i-C] = Coeff_Array[i];
     }
  else if ( G > 1 )
     {
       T_sub = T_span / ((double) G);          /* Compute subgranule interval */
       
       for ( j=G ; j>0 ; j-- ) 
           {
             T_break = T_beg + ((double) j-1) * T_sub;
             if ( Time > T_break ) 
                {
                  T_seg  = T_break;
                  offset = j-1;
                  break;
                }
            }
            
       Tc = 2.0*(Time - T_seg) / T_sub - 1.0;
       C  = C + 3 * offset * N;
       
       for (i=C ; i<(C+3*N) ; i++) A[i-C] = Coeff_Array[i];
     }
  else                                   /* Something has gone terribly wrong */
     {
       //printf("\n Number of granules must be >= 1: check header data.\n");
     }

  /*--------------------------------------------------------------------------*/
  /* Compute the interpolated position & velocity                             */
  /*--------------------------------------------------------------------------*/
  
  for ( i=0 ; i<3 ; i++ )                /* Compute interpolating polynomials */
      {
        Cp[0] = 1.0;           
        Cp[1] = Tc;
        Cp[2] = 2.0 * Tc*Tc - 1.0;
        
        Up[0] = 0.0;
        Up[1] = 1.0;
        Up[2] = 4.0 * Tc;

        for ( j=3 ; j<N ; j++ )
            {
              Cp[j] = 2.0 * Tc * Cp[j-1] - Cp[j-2];
              Up[j] = 2.0 * Tc * Up[j-1] + 2.0 * Cp[j-1] - Up[j-2];
            }

        P_Sum[i] = 0.0;           /* Compute interpolated position & velocity */
        V_Sum[i] = 0.0;

        for ( j=N-1 ; j>-1 ; j-- )  P_Sum[i] = P_Sum[i] + A[j+i*N] * Cp[j];
        for ( j=N-1 ; j>0  ; j-- )  V_Sum[i] = V_Sum[i] + A[j+i*N] * Up[j];

        X.Position[i] = P_Sum[i];
        X.Velocity[i] = V_Sum[i] * 2.0 * ((double) G) / (T_span * 86400.0);
      }

  /*--------------------------------------------------------------------------*/
  /*  Return computed values.                                                 */
  /*--------------------------------------------------------------------------*/

  *Planet = X;

  return;
}

void solsystem_initialize()
{
	Ephemeris_File = fopen(FROM_PLUGINS("de405.eph"),"rb");
	if (!Ephemeris_File) {
		log_write("X-Space: Unable to open ephemeris (de405.eph)\n");
		return;
	}

	log_write("X-Space: Loading ephemeris (de405.eph)\n");
	
	fread(&H1,sizeof(double),EPH_ARRAY_SIZE,Ephemeris_File);
	fread(&H2,sizeof(double),EPH_ARRAY_SIZE,Ephemeris_File);
	fread(&Coeff_Array,sizeof(double),EPH_ARRAY_SIZE,Ephemeris_File);

	R1 = H1.data;
	T_beg  = Coeff_Array[0];
	T_end  = Coeff_Array[1];
	T_span = T_end - T_beg;

	if (R1.DENUM != 405) {
		log_write("X-Space: Invalid ephemeris file (not DE405)");
		return;
	}

	log_write("X-Space: Ephemeris from MJD%.0f to MJD%.0f\n",T_beg-2400000.5,T_end-2400000.5);
}

void solsystem_update_vessel(int net_id, solsystem_state* state, double mass) {
	vessel* v = 0;
	int i;
	for (i = 0; i < vessel_count; i++) {
		if (vessels[i].net_id == net_id) v = &vessels[i];
	}
	if (!v) {
		v = &vessels[vessels_add()];
		v->net_id = net_id;
		v->exists = 1;
	}

	v->inertial.x = state->Position[0]*1e3;
	v->inertial.y = state->Position[1]*1e3;
	v->inertial.z = state->Position[2]*1e3;
	v->inertial.vx = state->Velocity[0]*1e3;
	v->inertial.vy = state->Velocity[1]*1e3;
	v->inertial.vz = state->Velocity[2]*1e3;
	v->weight.chassis = mass;

	v->physics_type = VESSEL_PHYSICS_INERTIAL;
	vessels_get_ni(v);
	v->physics_type = VESSEL_PHYSICS_DISABLED;
}

void solsystem_update(double mjd)
{
	double jd = mjd + 2400000.5;
	solsystem_state moon;
	solsystem_state earth;
	solsystem_state sun;
	solsystem_state jupiter;

	DE405_Interpolate_State(jd, MOON, &moon);
	DE405_Interpolate_State(jd, EARTH, &earth);
	DE405_Interpolate_State(jd, SUN, &sun);
	DE405_Interpolate_State(jd, JUPITER, &jupiter);

	sun.Position[0] -= earth.Position[0];
	sun.Position[1] -= earth.Position[1];
	sun.Position[2] -= earth.Position[2];
	sun.Velocity[0] = 0;
	sun.Velocity[1] = 0;
	sun.Velocity[2] = 0;
	jupiter.Position[0] -= earth.Position[0];
	jupiter.Position[1] -= earth.Position[1];
	jupiter.Position[2] -= earth.Position[2];
	jupiter.Velocity[0] = 0;
	jupiter.Velocity[1] = 0;
	jupiter.Velocity[2] = 0;

	//Update vessel that corresponds to moon
	solsystem_update_vessel(1000000,&moon,7.3477e22);
	//solsystem_update_vessel(1000001,&sun,1.98892e30);
	//solsystem_update_vessel(1000002,&jupiter,1.8986e27);
}