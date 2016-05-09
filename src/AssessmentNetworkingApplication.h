#pragma once

#include "AIEntity.h"
#include "BaseApplication.h"
#include <RakNetTime.h>
#include <vector>

class Camera;

namespace RakNet {
	class RakPeerInterface;
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

	// Used for timestamping
	RakNet::Time m_uiPrevTimeStamp;
	RakNet::Time m_uiCurrentTimeStamp;
};