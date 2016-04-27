#pragma once

#include "BaseApplication.h"
#include "AIEntity.h"
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
	virtual void shutdown();

	virtual bool update(float deltaTime);

	virtual void draw();

private:

	RakNet::RakPeerInterface*	m_peerInterface;

	Camera*						m_camera;

	std::vector<AIEntity>		m_aiEntities;
	std::vector<AIEntity>		m_aiPrevEntities; // way to store our entities from the previous frame.
};