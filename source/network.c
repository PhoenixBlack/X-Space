#include <string.h>
#include <enet/enet.h>
#include "x-space.h"
#include "network.h"
#include "highlevel.h"

//==============================================================================
// API for working with ENet
//==============================================================================
int network_highlevel_newmessage(lua_State* L)
{
	network_message* msg = (network_message*)malloc(sizeof(network_message));
	if (lua_toboolean(L,1)) {
		msg->packet = enet_packet_create(0,NETWORK_MAX_PACKET_SIZE,ENET_PACKET_FLAG_RELIABLE);
	} else {
		msg->packet = enet_packet_create(0,NETWORK_MAX_PACKET_SIZE,0);
	}
	msg->offset = 0;
	msg->size = NETWORK_MAX_PACKET_SIZE;

	highlevel_newptr("NetworkMessage",msg);
	return 1;
}

int network_highlevel_sendmessage(lua_State* L)
{
	network_message* msg;

	luaL_checkudata(L,1,"Host");
	luaL_checkudata(L,2,"Peer");
	luaL_checkudata(L,3,"NetworkMessage");
	highlevel_checkzero(L,1);
	highlevel_checkzero(L,2);
	highlevel_checkzero(L,3);

	//Get message and shrink the packet to correct size
	msg = highlevel_getptr(L,3);
	enet_packet_resize(msg->packet,msg->offset);

	//Send packet
	if (enet_peer_send(highlevel_getptr(L,2),0,msg->packet)) {
		lua_pushboolean(L,0);
	} else {
		lua_pushboolean(L,1);
	}

	//Message no longer needed, free it up and reset pointer
	free(msg);
	highlevel_setptr(L,3,0);
	enet_host_flush(highlevel_getptr(L,1));
	return 1;
}

int network_highlevel_messagewrite(lua_State* L)
{
	network_message* msg;
	luaL_checkudata(L,1,"NetworkMessage");
	luaL_checktype(L,2,LUA_TNUMBER); //Type to write
	highlevel_checkzero(L,1);

	//Write data to message
	msg = highlevel_getptr(L,1);
	switch (lua_tointeger(L,2)) {
		case 0: { //int8
			unsigned char value = lua_tointeger(L,3);
			network_message_write(msg,&value,1);
		}
		break;
		case 1: { //int16
			unsigned short value = lua_tointeger(L,3);
			network_message_write(msg,&value,2);
		}
		break;
		case 2: { //int32
			unsigned int value = lua_tointeger(L,3);
			network_message_write(msg,&value,4);
		}
		break;
		case 3: { //single
			float value = (float)lua_tonumber(L,3);
			network_message_write(msg,&value,4);
		}
		break;
		case 4: { //double
			double value = lua_tonumber(L,3);
			network_message_write(msg,&value,8);
		}
		break;
		case 5: { //string256
			char* str = (char*)lua_tostring(L,3); //FIXME: can return 0?
			unsigned char len = strlen(str);
			if (len <= 255) {
				network_message_write(msg,&len,1);
				network_message_write(msg,str,len);
			}
		}
		break;
		default: break;
	}
	return 0;
}

int network_highlevel_messageread(lua_State* L)
{
	network_message* msg;
	luaL_checkudata(L,1,"NetworkMessage");
	luaL_checktype(L,2,LUA_TNUMBER); //Type to read
	highlevel_checkzero(L,1);

	//Write data to message
	msg = highlevel_getptr(L,1);
	switch (lua_tointeger(L,2)) {
		case 0: { //int8
			unsigned char value;
			network_message_read(msg,&value,1);
			lua_pushnumber(L,value);
		}
		break;
		case 1: { //int16
			unsigned short value;
			network_message_read(msg,&value,2);
			lua_pushnumber(L,value);
		}
		break;
		case 2: { //int32
			unsigned int value;
			network_message_read(msg,&value,4);
			lua_pushnumber(L,value);
		}
		break;
		case 3: { //single
			float value;
			network_message_read(msg,&value,4);
			lua_pushnumber(L,value);
		}
		break;
		case 4: { //double
			double value;
			network_message_read(msg,&value,8);
			lua_pushnumber(L,value);
		}
		break;
		case 5: { //string256
			//ERR
		}
		break;
		default: lua_pushnil(L); break;
	}
	return 1;
}

int network_highlevel_host(lua_State* L)
{
	ENetAddress address;
	ENetHost* host;
	//luaL_checktype(L,1,LUA_TSTRING); //IP
	luaL_checktype(L,2,LUA_TNUMBER); //Port
	luaL_checktype(L,3,LUA_TNUMBER); //Max channels

	//Set address and create host
	if (lua_isstring(L,1)) {
		if (!(lua_tostring(L,1)[0])) {
			address.host = ENET_HOST_ANY;
		} else {
			enet_address_set_host(&address,lua_tostring(L,1));
		}
		address.port = lua_tointeger(L,2);
		host = enet_host_create(&address,lua_tointeger(L,3),0,0,0);
	} else {
		host = enet_host_create(0,lua_tointeger(L,3),0,0,0);
	}

	//Return host
	if (host) {
		highlevel_newptr("Host",host);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int network_highlevel_connect(lua_State* L)
{
	ENetAddress address;
	ENetPeer* peer;
	luaL_checkudata(L,1,"Host");
	luaL_checktype(L,2,LUA_TSTRING); //IP
	luaL_checktype(L,3,LUA_TNUMBER); //Port
	luaL_checktype(L,4,LUA_TNUMBER); //Max channels
	luaL_checktype(L,5,LUA_TNUMBER); //Data
	highlevel_checkzero(L,1);

	//Set address and create host
	enet_address_set_host(&address,lua_tostring(L,2));
	address.port = lua_tointeger(L,3);

	//Create peer
	peer = enet_host_connect(highlevel_getptr(L,1),&address,lua_tointeger(L,4),lua_tointeger(L,5));
	if (peer) {
		highlevel_newptr("Peer",peer);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int network_highlevel_destroyhost(lua_State* L)
{
	luaL_checkudata(L,1,"Host");
	highlevel_checkzero(L,1);

	//Destroy host
	enet_host_destroy(highlevel_getptr(L,1));
	highlevel_setptr(L,1,0);
	return 1;
}

int network_highlevel_resetpeer(lua_State* L)
{
	luaL_checkudata(L,1,"Peer");
	highlevel_checkzero(L,1);

	//Reset peer
	enet_peer_reset(highlevel_getptr(L,1));
	return 1;
}

int network_highlevel_update(lua_State* L)
{
	ENetHost* host;
	ENetEvent enet_event;
	luaL_checkudata(L,1,"Host"); //host
	luaL_checktype(L,2,LUA_TFUNCTION); //callback
	highlevel_checkzero(L,1);

	//Service all messages
	host = highlevel_getptr(L,1);
	while (host && (enet_host_service(host, &enet_event, 0) > 0)) {
		//Temporary object
		network_message msg;

		//Store the packet
		if (enet_event.packet) {
			highlevel_newptr("NetworkMessage",&msg);
			msg.offset = 0;
			msg.size = enet_event.packet->dataLength;
			msg.packet = enet_event.packet;
		} else {
			lua_pushnil(L);
		}

		//Call the callback
		lua_pushvalue(L,2); //function
		highlevel_newptr("Peer",enet_event.peer);
		lua_pushnumber(L,enet_event.type);
		lua_pushnumber(L,enet_event.data);
		lua_pushvalue(L,-5);
		highlevel_call(4,0);

		//Reset the packet and remove it
		if (enet_event.packet) {
			highlevel_setptr(L,-1,0);
		}
		lua_pop(L,1);
	}
	return 0;
}

int network_highlevel_getpeerhostport(lua_State* L)
{
	char buf[ARBITRARY_MAX] = { 0 };
	ENetPeer* peer;
	luaL_checkudata(L,1,"Peer"); //host
	highlevel_checkzero(L,1);

	//Return the information
	peer = highlevel_getptr(L,1);
	snprintf(buf,ARBITRARY_MAX-1,"%d.%d.%d.%d:%d",
	         (peer->address.host & 0xFF) >> 0,
	         (peer->address.host & 0xFF00) >> 8,
	         (peer->address.host & 0xFF0000) >> 16,
	         (peer->address.host & 0xFF000000) >> 24,
	         peer->address.port);
	lua_pushstring(L,buf);
	return 1;
}

int network_highlevel_setpeerid(lua_State* L)
{
	ENetPeer* peer;
	luaL_checkudata(L,1,"Peer"); //peer
	luaL_checktype(L,2,LUA_TNUMBER); //id
	highlevel_checkzero(L,1);

	//Return the information
	peer = highlevel_getptr(L,1);
	peer->data = (void*)lua_tointeger(L,2);
	return 0;
}

int network_highlevel_getpeerid(lua_State* L)
{
	ENetPeer* peer;
	luaL_checkudata(L,1,"Peer"); //peer
	highlevel_checkzero(L,1);

	//Return the information
	peer = highlevel_getptr(L,1);
	lua_pushinteger(L,(int)peer->data);
	return 1;
}


//==============================================================================
// Initialize networking
//==============================================================================
void network_initialize()
{
	//Register API
	lua_createtable(L,0,32);
	//highlevel_newptr("Peer",0);
	//lua_setfield(L,-2,"BadPeer");
	//highlevel_newptr("Host",0);
	//lua_setfield(L,-2,"BadHost");
	lua_setglobal(L,"NetAPI");

	highlevel_addfunction("NetAPI","NewMessage",network_highlevel_newmessage);
	highlevel_addfunction("NetAPI","SendMessage",network_highlevel_sendmessage);
	highlevel_addfunction("NetAPI","MessageWrite",network_highlevel_messagewrite);
	highlevel_addfunction("NetAPI","MessageRead",network_highlevel_messageread);
	highlevel_addfunction("NetAPI","Host",network_highlevel_host);
	highlevel_addfunction("NetAPI","Connect",network_highlevel_connect);
	highlevel_addfunction("NetAPI","DestroyHost",network_highlevel_destroyhost);
	highlevel_addfunction("NetAPI","ResetPeer",network_highlevel_resetpeer);
	highlevel_addfunction("NetAPI","Update",network_highlevel_update);
	highlevel_addfunction("NetAPI","GetPeerHostPort",network_highlevel_getpeerhostport);
	highlevel_addfunction("NetAPI","SetPeerID",network_highlevel_setpeerid);
	highlevel_addfunction("NetAPI","GetPeerID",network_highlevel_getpeerid);

	//Initialize ENet
	enet_initialize();

	//Initialize X-Net
	highlevel_load(FROM_PLUGINS("lua/network.lua"));
}

//==============================================================================
// Clear up resources
//==============================================================================
void network_deinitialize()
{
	enet_deinitialize();
}



//==============================================================================
// Read bytes from message
//==============================================================================
int network_message_read(network_message* msg, void* ptr, int size)
{
	char* new_ptr = msg->packet->data + msg->offset;
	if (msg->offset + size <= msg->size) {
		memcpy(ptr,new_ptr,size);
		msg->offset += size;
		return 1;
	} else {
		return 0;
	}
}

//==============================================================================
// Writes bytes to message
//==============================================================================
int network_message_write(network_message* msg, void* ptr, int size)
{
	char* new_ptr = msg->packet->data + msg->offset;
	if (msg->offset + size <= msg->size) {
		memcpy(new_ptr,ptr,size);
		msg->offset += size;
		return 1;
	} else {
		return 0;
	}
}