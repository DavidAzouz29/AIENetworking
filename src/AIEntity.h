#pragma once

#include <MessageIdentifiers.h>
#include <cmath>

enum GameMessages {
	// this ID is used for sending the AI entities
	// the structure of the bitstream is:
	// [ message ID, unsigned int bytecount, AIEntity array of size (bytecount / sizeof(AIEntity)) ]
	ID_ENTITY_LIST = ID_USER_PACKET_ENUM + 1,
};

static const unsigned short SERVER_PORT = 5456;

// very basic 2D vector struct with helper methods
struct AIVector 
{
	float x, y;

	void normalise() 
	{
		float m = length(); // sqrt(x * x + y * y);
		x /= m; y /= m;
	}

	float length() const 
	{
		return sqrt(lengthSqr()); //return sqrt(x * x + y * y);
	}

	inline float lengthSqr() const 
	{
		return x * x + y * y;
	}
};

//TODO: pragma needed?
// basic AI entity data that is broadcast by the server
//#pragma pack(push, 1)
struct AIEntity
{
	//unsigned int uiSequence; //TODO: add a sequence? or timestamp. Defines the order in which entites data should be read.
	unsigned char typeId; 
	unsigned int id;
	AIVector position;
	AIVector velocity;
	bool teleported;
};
//#pragma pack(pop)

// server's entity data that it uses to update entities
struct AIServerEntity 
{
	AIEntity* data;
	float wanderAngle;
};