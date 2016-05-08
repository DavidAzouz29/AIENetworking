#pragma once

#include "BaseApplication.h"
#include "AIEntity.h"
#include <vector>

class Camera;

namespace RakNet {
	class RakPeerInterface;
	//class MessageID; //TODO: 
	//class Time;
}

class AssessmentNetworkingApplication : public BaseApplication {
public:

	AssessmentNetworkingApplication();
	virtual ~AssessmentNetworkingApplication();

	virtual bool startup();
	virtual GLvoid shutdown();

	virtual bool update(GLfloat deltaTime);

	virtual GLvoid draw();

private:

	RakNet::RakPeerInterface*	m_peerInterface;

	Camera*						m_camera;

	std::vector<AIEntity>		m_aiEntities;
	std::vector<AIEntity>		m_aiPrevEntities; // way to store our entities from the previous frame.

	uint64_t m_uiPrevTimeStamp;
	// Used for timestamping
//	RakNet::MessageID useTimeStamp; // Assign this to ID_TIMESTAMP
//	RakNet::Time timeStamp; // Put the system time in here returned by RakNet::GetTime()
// useTimeStamp = ID_TIMESTAMP; // MessageIdentifiers.h line: 139
// timeStamp = RakNet::GetTime();
};