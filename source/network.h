#ifndef NETWORK_H
#define NETWORK_H

//Default maximum network packet size
#define NETWORK_MAX_PACKET_SIZE 65536

//Key for networking
typedef char network_key[32];

//Structure for accessing messages
typedef struct {
	ENetPacket* packet;
	int size; //total size in bytes
	int offset; //current offset into message, in bytes
} network_message;

//Network routines
void network_initialize();
void network_deinitialize();

//Network message functions
int network_message_read(network_message* msg, void* ptr, int size);
int network_message_write(network_message* msg, void* ptr, int size);

#endif