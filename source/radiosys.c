//==============================================================================
// Radio transmission simulation
//==============================================================================
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "x-space.h"
#include "vessel.h"
#include "highlevel.h"
#include "radiosys.h"
#include "planet.h"

#define RADIOSYS_QUEUE_SIZE				256*1024
#define RADIOSYS_MAX_CHANNELS			128

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
#include "dataref.h"
#include <XPLMDataAccess.h>

/*double radiosys_serial_data = 0.0;
double radiosys_serial_channel = 0.0;
//double radiosys_serial_rxbps;
double radiosys_serial_txbps = 0.0;//RADIOSYS_TRANSMISSION_SPEED;

static float radiosys_send_dataref_get(void* dataptr)
{
	return 0.0;
}
static void radiosys_send_dataref_set(void* dataptr, float value)
{
	int channel = (int)radiosys_serial_channel;
	char data = (char)radiosys_serial_data;
	radiosys_transmit(&vessels[0],channel,data);
}

static float radiosys_recv_dataref_get(void* dataptr)
{
	return 0.0;
}
static void radiosys_recv_dataref_set(void* dataptr, float value)
{
	int channel = (int)radiosys_serial_channel;
	radiosys_serial_data = radiosys_receive(&vessels[0],channel);	
}*/

static long radiosys_recv_fv(void* dataptr, float* values, int inOffset, int inMax)
{
	vessel* v = 0;
	if (!values) return vessel_count;
	if ((inOffset >= 0) && (inOffset < vessel_count)) v = &vessels[inOffset];

	if (v) values[0] = (float)radiosys_receive(v,v->radiosys.channel);
	return vessel_count;
}

static void radiosys_send_fv(void* dataptr, float* values, int inOffset, int inMax)
{
	vessel* v = 0;
	if (!values) return;
	if ((inOffset >= 0) && (inOffset < vessel_count)) v = &vessels[inOffset];

	if (v) radiosys_transmit(v,v->radiosys.channel,(char)(values[0]));
}

static long radiosys_read_channel_fv(void* dataptr, float* values, int inOffset, int inMax)
{
	vessel* v = 0;
	if (!values) return vessel_count;
	if ((inOffset >= 0) && (inOffset < vessel_count)) v = &vessels[inOffset];

	if (v) values[0] = (float)v->radiosys.channel;
	return vessel_count;
}

static void radiosys_write_channel_fv(void* dataptr, float* values, int inOffset, int inMax)
{
	vessel* v = 0;
	if (!values) return;
	if ((inOffset >= 0) && (inOffset < vessel_count)) v = &vessels[inOffset];

	if (v) v->radiosys.channel = (int)values[0];
}


#endif


//==============================================================================
// Radio channels allocation
//==============================================================================
double radiosys_channel_allocation[128*2] = {
	137.000,8,	137.050,4,	137.100,8,	137.150,4,	137.200,8,	137.250,4,	137.300,8,	137.350,4,
	137.400,8,	137.450,4,	137.500,8,	137.600,16,	137.700,8,	137.800,16,	137.900,8,	138.000,16,
	148.000,32,	148.200,16,	148.400,32,	148.600,16,	148.800,32,	149.000,16,	149.200,32,	149.400,16,
	149.600,32,	149.800,16,	149.900,6.2,149.950,6.2,150.000,6.2,150.050,6.2,235.000,96,	236.000,64,
	237.000,96,	238.000,64,	239.000,96,	240.000,64,	242.000,96,	244.000,128,246.000,96,	248.000,128,
	250.000,96,252.000,128,	254.000,96,256.000,128,	258.000,96,	260.000,128,265.000,256,270.000,256,
	275.000,256,280.000,256,285.000,256,290.000,256,295.000,256,300.000,256,304.000,96,	306.000,96,
	308.000,96,	310.000,96,	312.000,96,	314.000,96,	316.000,96,	318.000,96,	320.000,96,	322.000,96,
	335.000,256,340.000,256,345.000,256,350.000,256,355.000,256,360.000,256,365.000,256,370.000,256,
	375.000,256,380.000,256,385.000,256,390.000,256,395.000,256,400.100,16,	1525,256,	1530,256,
	1535,256,	1540,256,	1545,256,	1550,256,	1555,256,	1560,256,	1565,256,	1570,256,
	1575,256,	1580,256,	1585,256,	1590,256,	1595,256,	1610,384,	1620,384,	1630,384,
	1640,384,	1650,384,	1660,384,	2000,256,	2005,256,	2010,256,	2015,256,	2020,256,
	2180,256,	2185,256,	2190,256,	2195,256,	2200,256,	3700,1024,	3720,1024,	3740,1024,
	3760,1024,	3780,1024,	3800,1024,	3820,1024,	3840,1024,	3860,1024,	3880,1024,	3900,1024,
	3920,1024,	3940,1024,	3960,1024,	3980,1024,	4000,1024,	4020,1024,	4040,1024,	4060,1024,
};


//==============================================================================
// Radio transmission data structures
//==============================================================================
typedef struct radiosys_channel_tag {
	int bps;					//Channel speed (bits per second)
	double frequency;			//Frequency (MHz)
	double prev_time;			//Last byte at which anything was sent from this channels buffers
} radiosys_channel;

//All channels
radiosys_channel radiosys_channels[RADIOSYS_MAX_CHANNELS];
//Global timing for radiosystem
double radiosys_time;


//==============================================================================
// Initialize send/receive buffers
//==============================================================================
void radiosys_initialize_buffers(radio_buffers* buffers)
{
	buffers->num_channels = RADIOSYS_MAX_CHANNELS;
	buffers->buffer_size = RADIOSYS_QUEUE_SIZE;

	buffers->channels_recv_used = (int*)malloc(buffers->num_channels*sizeof(int));
	buffers->send_buffer = (int**)malloc(buffers->num_channels*sizeof(int*));
	buffers->recv_buffer = (int**)malloc(buffers->num_channels*sizeof(int*));
	buffers->send_buffer_sys_position  = (int*)malloc(buffers->num_channels*sizeof(int));
	buffers->send_buffer_user_position = (int*)malloc(buffers->num_channels*sizeof(int));
	buffers->recv_buffer_sys_position  = (int*)malloc(buffers->num_channels*sizeof(int));
	buffers->recv_buffer_user_position = (int*)malloc(buffers->num_channels*sizeof(int));

	memset(buffers->channels_recv_used,0,buffers->num_channels*sizeof(int*));
	memset(buffers->send_buffer,0,buffers->num_channels*sizeof(int*));
	memset(buffers->recv_buffer,0,buffers->num_channels*sizeof(int*));
	memset(buffers->send_buffer_sys_position, 0,buffers->num_channels*sizeof(int));
	memset(buffers->send_buffer_user_position,0,buffers->num_channels*sizeof(int));
	memset(buffers->recv_buffer_sys_position, 0,buffers->num_channels*sizeof(int));
	memset(buffers->recv_buffer_user_position,0,buffers->num_channels*sizeof(int));
}


//==============================================================================
// Deinitialize send/receive buffers
//==============================================================================
void radiosys_deinitialize_buffers(radio_buffers* buffers)
{
	free(buffers->channels_recv_used);
	free(buffers->send_buffer);
	free(buffers->recv_buffer);
	free(buffers->send_buffer_sys_position);
	free(buffers->send_buffer_user_position);
	free(buffers->recv_buffer_sys_position);
	free(buffers->recv_buffer_user_position);
}


//==============================================================================
// Initialize radio transmission simulation
//==============================================================================
void radiosys_initialize()
{
	int i;

#if (!defined(DEDICATED_SERVER)) && (!defined(ORBITER_MODULE))
	XPLMRegisterDataAccessor(
	    "xsp/radio/serial/channel",xplmType_FloatArray,1,0,0,0,0,0,0,0,0,
	    radiosys_read_channel_fv,radiosys_write_channel_fv,0,0,
	    0,0);
	XPLMRegisterDataAccessor(
	    "xsp/radio/serial/send",xplmType_FloatArray,1,0,0,0,0,0,0,0,0,
	    0,radiosys_send_fv,0,0,
	    0,0);
	XPLMRegisterDataAccessor(
	    "xsp/radio/serial/recv",xplmType_FloatArray,1,0,0,0,0,0,0,0,0,
	    radiosys_recv_fv,0,0,0,
	    0,0);
#endif

	//Function to transmit arbitrary data
	lua_createtable(L,0,32);
	lua_setglobal(L,"RadioAPI");
	//Transmit from a vessel to all other vessels
	highlevel_addfunction("RadioAPI","Transmit",radiosys_highlevel_transmit);
	//Read data received by this vessel
	highlevel_addfunction("RadioAPI","Receive",radiosys_highlevel_receive);
	//Writes data directly into vessels receive buffer. Used by networking when receiving data from remote server
	highlevel_addfunction("RadioAPI","WriteReceiveBuffer",radiosys_highlevel_write_vessel_recvbuffer);
	//Mark channel as receiveable for the vessel
	highlevel_addfunction("RadioAPI","SetCanChannelReceive",radiosys_highlevel_set_canreceive);

	//Setup channels
	for (i = 0; i < RADIOSYS_MAX_CHANNELS; i++) {
		radiosys_channels[i].prev_time = 0.0;
		radiosys_channels[i].frequency = radiosys_channel_allocation[i*2+0];
		radiosys_channels[i].bps = (int)(radiosys_channel_allocation[i*2+1]*1000);
	}

	//Reset radio system timing
	radiosys_time = 0.0;

}


//==============================================================================
// Free up all resources
//==============================================================================
void radiosys_deinitialize()
{

}


//==============================================================================
// Initialize buffers for a vessel
//==============================================================================
void radiosys_initialize_vessel(vessel* v)
{
	radiosys_initialize_buffers(&v->radiosys.buffers);
}


//==============================================================================
// Deinitialize buffers for a vessel
//==============================================================================
void radiosys_deinitialize_vessel(vessel* v)
{
	radiosys_deinitialize_buffers(&v->radiosys.buffers);
}


//==============================================================================
// Advances the send buffer by a single byte
//==============================================================================
int radiosys_buffer_advance_send(radio_buffers* buffers, int channel)
{
	//Buffer empty, return error state
	if (buffers->send_buffer_sys_position[channel] == buffers->send_buffer_user_position[channel]) {
		//buffers->send_buffer_sys_position[channel]++;
		//if (buffers->send_buffer_sys_position[channel] >= buffers->buffer_size) {
			//buffers->send_buffer_sys_position[channel] = 0;
		//}
		//buffers->send_buffer_user_position[channel] = buffers->send_buffer_sys_position[channel];
		return 0;
	} else {
		buffers->send_buffer_sys_position[channel]++;
		if (buffers->send_buffer_sys_position[channel] >= buffers->buffer_size) {
			buffers->send_buffer_sys_position[channel] = 0;
		}
		return (buffers->send_buffer_sys_position[channel] != buffers->send_buffer_user_position[channel]);

		//If the buffer is suddenly empty, reset
		//if (buffers->send_buffer_sys_position[channel] == buffers->send_buffer_user_position[channel]) {
			//buffers->send_buffer_user_position[channel]++;
			//if (buffers->send_buffer_user_position[channel] >= buffers->buffer_size) {
				//buffers->send_buffer_user_position[channel] = 0;
			//}
		//}
	}
}


//==============================================================================
// Advances the receive buffer by a single byte
//==============================================================================
void radiosys_buffer_advance_recv(radio_buffers* buffers, int channel, unsigned char data)
{
	if (!buffers->recv_buffer[channel]) {
		buffers->recv_buffer[channel] = (int*)malloc(sizeof(int)*buffers->buffer_size);
	}

	buffers->recv_buffer[channel][buffers->recv_buffer_sys_position[channel]] = data;
	buffers->recv_buffer_sys_position[channel]++;
	if (buffers->recv_buffer_sys_position[channel] >= buffers->buffer_size) {
		buffers->recv_buffer_sys_position[channel] = 0;
	}
}


//==============================================================================
// Update radio transmission system
//==============================================================================
void radiosys_update(float dt)
{
	int ch,i,j;
	radiosys_time += dt;

	for (ch = 0; ch < RADIOSYS_MAX_CHANNELS; ch++) {
		//How many bytes must be sent in this frame
		double bits_transferred = (radiosys_time - radiosys_channels[ch].prev_time) * radiosys_channels[ch].bps;
		int bytes_transferred = (int)(bits_transferred/8.0);
		if (bytes_transferred == 0) continue;

		//Update channel information
		radiosys_channels[ch].prev_time = radiosys_time;

		//Update all vessels
		for (i = 0; i < vessel_count; i++) {
			vessel* src = &vessels[i]; //Source
			if (!src->exists) continue;

			//Transmit from every vessel to every other vessel
			for (j = 0; j < vessel_count; j++) {
				double error_probability;
				int threshold;
				int bytes_left = bytes_transferred;
				int initial_send_buffer_pos = src->radiosys.buffers.send_buffer_sys_position[ch];

				vessel* tgt = &vessels[j]; //Target
				if (j == i) continue;
				if (!tgt->exists) continue;
				if (!tgt->radiosys.buffers.channels_recv_used[ch]) continue;
				if (src->radiosys.buffers.send_buffer_sys_position[ch] == 
					src->radiosys.buffers.send_buffer_user_position[ch]) continue;

				//Compute radio transmission model
				error_probability = radiosys_transmission_model(
					src->noninertial.x,
					src->noninertial.y,
					src->noninertial.z,
					tgt->noninertial.x,
					tgt->noninertial.y,
					tgt->noninertial.z,
					radiosys_channels[ch].frequency);
				threshold = (int)(RAND_MAX * error_probability)+1;

				if (error_probability > 0.95) continue;

				//Transfer bytes
				while (bytes_left > 0) {
					unsigned char data;
					bytes_left--;

					//Transmit a single byte
					if (src->radiosys.buffers.send_buffer[ch]) {
						int bit;
						data = src->radiosys.buffers.send_buffer[ch][src->radiosys.buffers.send_buffer_sys_position[ch]];
						for (bit = 0; bit < 8; bit++) {
							int mask = rand() < threshold;
							data = data ^ (mask << bit);
						}

						//Callback called when byte is received. This callback is used by server to transmit received data
						//to clients (if this vessel has an assigned client)
						if (highlevel_pushcallback("OnRadioReceive")) {
							lua_pushnumber(L,tgt->index);
							lua_pushnumber(L,ch);
							lua_pushnumber(L,data);
							highlevel_call(3,1);
							if (lua_isnil(L,-1) || (!lua_isboolean(L,-1))) { //If not processed
								radiosys_buffer_advance_recv(&tgt->radiosys.buffers,ch,data);
							}
							lua_pop(L,1);
						} else {
							radiosys_buffer_advance_recv(&tgt->radiosys.buffers,ch,data);
						}
					}

					//Advance position
					if (!radiosys_buffer_advance_send(&src->radiosys.buffers,ch)) break;
				}

				//Reset position for next send attempt
				src->radiosys.buffers.send_buffer_sys_position[ch] = initial_send_buffer_pos;
			}

			//Update this vessels buffers state
			while (bytes_transferred > 0) {
				bytes_transferred--;

				//Advance to clear
				if (!radiosys_buffer_advance_send(&src->radiosys.buffers,ch)) break;
			}
		}
	}
}


//==============================================================================
// Transmission model for various frequencies
//==============================================================================
double radiosys_transmission_model(double x1, double y1, double z1,
								   double x2, double y2, double z2, double frequency)
{
	double error_probability;

	double x3 = 0;
	double y3 = 0;
	double z3 = 0;

	double a = (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1) + (z2 - z1)*(z2 - z1);
	double b = 2*((x2 - x1)*(x1 - x3) + (y2 - y1)*(y1 - y3) + (z2 - z1)*(z1 - z3));
	double c = x3*x3 + y3*y3 + z3*z3 + x1*x1 + y1*y1 + z1*z1 - 2*(x3*x1 + y3*y1 + z3*z1) - current_planet.radius*current_planet.radius;
	double D = b*b - 4*a*c;

	double t0,t1;

	if (D < 0) D = 0;

	t0 = (-b - sqrt(D))/(2.0*a);
	t1 = (-b + sqrt(D))/(2.0*a);

	if (t0 > t1) {
		double temp = t1;
		t1 = t0;
		t0 = temp;
	}

	if (t1 < 0) D = 0;
	if (t0 < 0) {
		if (t1 > 1.0) D = 0;
	} else {
		if (t0 > 1.0) D = 0;
	}

	error_probability = 8.0*sqrt(D)/(current_planet.radius*current_planet.radius);
	if (error_probability < 0) error_probability = 0;
	if (error_probability > 1) error_probability = 1;
	return error_probability;
}


//==============================================================================
// Transmit data
//==============================================================================
void radiosys_transmit_buffer(radio_buffers* buffers, int channel, unsigned char data)
{
	if (channel < 0) return;
	if (channel >= buffers->num_channels) return;

	if (!buffers->send_buffer[channel]) {
		buffers->send_buffer[channel] = (int*)malloc(sizeof(int)*buffers->buffer_size);
	}

	buffers->send_buffer[channel][buffers->send_buffer_user_position[channel]] = data;
	buffers->send_buffer_user_position[channel]++;
	if (buffers->send_buffer_user_position[channel] >= buffers->buffer_size) {
		buffers->send_buffer_user_position[channel] = 0;
	}
}

void radiosys_transmit(vessel* v, int channel, unsigned char data)
{
	if (channel < 0) return;
	if (channel >= v->radiosys.buffers.num_channels) return;

	//Callback called when a byte is transmitted. This callback is used by client
	//to send data to server. Is not used by server
	if (highlevel_pushcallback("OnRadioTransmit")) {
		lua_pushnumber(L,v->index);
		lua_pushnumber(L,channel);
		lua_pushnumber(L,data);
		highlevel_call(3,1);
		if (lua_toboolean(L,-1)) { //Processed
			lua_pop(L,1);
			return;
		}
		lua_pop(L,1);
	}

	radiosys_transmit_buffer(&v->radiosys.buffers,channel,data);
}


//==============================================================================
// Receive data. Returns -1 if no data/noise
//==============================================================================
int radiosys_receive_buffer(radio_buffers* buffers, int channel)
{
	int return_byte;

	if (channel < 0) return -1;
	if (channel >= buffers->num_channels) return -1;
	if (!buffers->recv_buffer[channel]) return -1;
	if (buffers->recv_buffer_user_position[channel] == buffers->recv_buffer_sys_position[channel]) return -1;

	return_byte = buffers->recv_buffer[channel][buffers->recv_buffer_user_position[channel]];
	buffers->recv_buffer_user_position[channel]++;
	if (buffers->recv_buffer_user_position[channel] >= buffers->buffer_size) {
		buffers->recv_buffer_user_position[channel] = 0;
	}
	return return_byte;
}

int radiosys_receive(vessel* v, int channel)
{
	return radiosys_receive_buffer(&v->radiosys.buffers,channel);
}




//==============================================================================
// Highlevel interface
//==============================================================================
int radiosys_highlevel_transmit(lua_State* L)
{
	int v_idx = lua_tointeger(L,1);
	if (v_idx < 0) return 0;
	if (v_idx >= vessel_count) return 0;

	radiosys_transmit(
		&vessels[v_idx],
		lua_tointeger(L,2),
		lua_tointeger(L,3));
	return 0;
}

int radiosys_highlevel_receive(lua_State* L)
{
	int v_idx = lua_tointeger(L,1);
	if (v_idx < 0) return 0;
	if (v_idx >= vessel_count) return 0;

	lua_pushnumber(L,radiosys_receive(
		&vessels[v_idx],
		lua_tointeger(L,2)));
	return 1;
}

int radiosys_highlevel_write_vessel_recvbuffer(lua_State* L)
{
	int v_idx = lua_tointeger(L,1);
	int channel = lua_tointeger(L,2);
	vessel* v;
	radio_buffers* buffers;

	if (v_idx < 0) return 0;
	if (v_idx >= vessel_count) return 0;
	v = &vessels[v_idx];
	buffers = &v->radiosys.buffers;

	if (channel < 0) return 0;
	if (channel >= buffers->num_channels) return 0;

	if (!buffers->recv_buffer[channel]) {
		buffers->recv_buffer[channel] = (int*)malloc(sizeof(int)*buffers->buffer_size);
	}

	buffers->recv_buffer[channel][buffers->recv_buffer_sys_position[channel]] = lua_tointeger(L,3);
	buffers->recv_buffer_sys_position[channel]++;
	if (buffers->recv_buffer_sys_position[channel] >= buffers->buffer_size) {
		buffers->recv_buffer_sys_position[channel] = 0;
	}
	return 0;
}

int radiosys_highlevel_set_canreceive(lua_State* L)
{
	int v_idx = lua_tointeger(L,1);
	int channel = lua_tointeger(L,2);
	vessel* v;
	radio_buffers* buffers;

	if (v_idx < 0) return 0;
	if (v_idx >= vessel_count) return 0;
	v = &vessels[v_idx];
	buffers = &v->radiosys.buffers;

	if (channel < 0) return 0;
	if (channel >= buffers->num_channels) return 0;

	buffers->channels_recv_used[channel] = lua_tointeger(L,3);
	return 0;
}