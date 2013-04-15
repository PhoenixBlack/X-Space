#ifndef RADIOSYS_H
#define RADIOSYS_H

//void radiosys_initialize_buffers(radio_buffers* buffers);
void radiosys_initialize();
void radiosys_deinitialize();
void radiosys_initialize_vessel(vessel* v);
void radiosys_deinitialize_vessel(vessel* v);
void radiosys_update(float dt);
double radiosys_transmission_model(double x1, double y1, double z1,
								   double x2, double y2, double z2, double frequency);
void radiosys_simulate_transmission(double x, double y, double z, int channel, unsigned char data);
void radiosys_transmit(vessel* v, int channel, unsigned char data);
int radiosys_receive(vessel* v, int channel);

int radiosys_highlevel_transmit(lua_State* L);
int radiosys_highlevel_receive(lua_State* L);
int radiosys_highlevel_transmit_bypass(lua_State* L);
int radiosys_highlevel_write_vessel_recvbuffer(lua_State* L);
int radiosys_highlevel_set_canreceive(lua_State* L);

#endif