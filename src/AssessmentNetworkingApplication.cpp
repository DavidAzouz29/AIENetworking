/// <summary>
/// Author: David Azouz
/// Date modified: 26/04/16
/// viewed: http://gafferongames.com/networking-for-game-programmers/
/// https://en.wikipedia.org/wiki/Low-pass_filter
/// https://devblogs.nvidia.com/parallelforall/lerp-faster-cuda/
/// http://glm.g-truc.net/0.9.4/api/a00129.html#ga3f64b3986efe205cf30300700667e761
/// http://www.cplusplus.com/reference/cmath/fma/
/// </summary>

#include "AssessmentNetworkingApplication.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>

#include "Gizmos.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::vec3;
using glm::vec4;

AssessmentNetworkingApplication::AssessmentNetworkingApplication() 
: m_camera(nullptr),
m_peerInterface(nullptr) {}

AssessmentNetworkingApplication::~AssessmentNetworkingApplication() {}

bool AssessmentNetworkingApplication::startup() 
{
	// setup the basic window
	createWindow("Client Application", 1280, 720);

	Gizmos::create();

	// set up basic camera
	m_camera = new Camera(glm::pi<float>() * 0.25f, 16 / 9.f, 0.1f, 1000.f);
	m_camera->setLookAtFrom(vec3(10, 10, 10), vec3(0));

	// start client connection
	m_peerInterface = RakNet::RakPeerInterface::GetInstance();
	
	RakNet::SocketDescriptor sd;
	m_peerInterface->Startup(1, &sd, 1);

	// request access to server
	std::string ipAddress = "";
	std::cout << "Press 1 to connect to the local host \nOr 2 to input your own: ";
	int isIPAddress = -1;
	std::cin >> isIPAddress;
	// if the user inputs '1' then input the IP address below
	if (isIPAddress == 1)
	{
		ipAddress = "localhost";
		std::cout << "Connecting to server at: " << ipAddress << "\n";
	}
	else
	{
		std::cout << "Connecting to server at: ";
		std::cin >> ipAddress;
	}
	RakNet::ConnectionAttemptResult res = m_peerInterface->Connect(ipAddress.c_str(), SERVER_PORT, nullptr, 0);

	if (res != RakNet::CONNECTION_ATTEMPT_STARTED) {
		std::cout << "Unable to start connection, Error number: " << res << std::endl;
		return false;
	}

	return true;
}

void AssessmentNetworkingApplication::shutdown() 
{
	// delete our camera and cleanup gizmos
	delete m_camera;
	Gizmos::destroy();

	// destroy our window properly
	destroyWindow();
}

bool AssessmentNetworkingApplication::update(float deltaTime) 
{
	// close the application if the window closes
	if (glfwWindowShouldClose(m_window) ||
		glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		return false;

	// update camera
	m_camera->update(deltaTime);

	// handle network messages
	RakNet::Packet* packet;
	for (packet = m_peerInterface->Receive(); packet;
		m_peerInterface->DeallocatePacket(packet),
		packet = m_peerInterface->Receive()) {

		switch (packet->data[0]) {
		case ID_CONNECTION_REQUEST_ACCEPTED:
			std::cout << "Our connection request has been accepted." << std::endl;
			break;
		case ID_CONNECTION_ATTEMPT_FAILED:
			std::cout << "Our connection request failed!" << std::endl;
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			std::cout << "The server is full." << std::endl;
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			std::cout << "We have been disconnected." << std::endl;
			break;
		case ID_CONNECTION_LOST:
			std::cout << "Connection lost." << std::endl;
			break;
		case ID_ENTITY_LIST: {

			// receive list of entities
			RakNet::BitStream stream(packet->data, packet->length, false);
			stream.IgnoreBytes(sizeof(RakNet::MessageID));
			unsigned int size = 0;
			stream.Read(size);

			bool isFirstRun = false;
			// if first time receiving entities...
			if (m_aiEntities.size() == 0) 
			{
				// ... resize our vector
				m_aiEntities.resize(size / sizeof(AIEntity));
				isFirstRun = true;
			}
			// ... the second time, set the current entites to the previous.
			else
			{
				m_aiPrevEntities = m_aiEntities; // TODO: m_aiPrevEntities is 0?
			}

			// Setting current entites.
			stream.Read((char*)m_aiEntities.data(), size);

			// our range //USHORT?
			//short sRange = 0.1f;
			float sRange = 25.0f;

			// if it's the first time running, assign prev to current
			if (isFirstRun)
			{
				m_aiPrevEntities = m_aiEntities;
				continue;
			}
			/// <summary>
			/// To account for packet loss/ stuttering
			/// Check for lost packets.
			/// </summary>
			//for (auto& ai : m_aiEntities)
			for (int i = 0; i < m_aiEntities.size(); ++i)
			{
				AIEntity& ai = m_aiEntities[i];
				AIEntity& pAI = m_aiPrevEntities[i];
				// if our past position with our current velocity is more than our allocated distance...
				glm::vec2 v2ExpectedPos(pAI.position.x + pAI.velocity.x * deltaTime, pAI.position.y + pAI.velocity.y * deltaTime);
				glm::vec2 v2CurrentPos(ai.position.x, ai.position.y);
				// if our difference is outside our range
				if (glm::distance(v2ExpectedPos, v2CurrentPos) > sRange && !ai.teleported)
				{
					//... set our current pos to our expected pos
					//ai.position.x = fExpectedPosX;
					//ai.position.y = fExpectedPosY;
					std::cout << "Entity " << ai.id << " moved." << std::endl;

					/// <summary>
					/// TODO: lerp from our previous position to our current position over time.
					/// <example> Lerp = fma(t, v1, fma(-t, v0, v0)) </example> 
					/// </summary>
					//fmaf(glfwGetTime(), ai.position.x, fmaf(-glfwGetTime(), pAI.position.x, pAI.position.x));
					//fmaf(glfwGetTime(), ai.position.y, fmaf(-glfwGetTime(), pAI.position.y, pAI.position.y));
					
					//glm::mix(pAI.position.x, ai.position.x, deltaTime); //glfwGetTime()); //TODO: deltaTime?
					//glm::mix(pAI.position.y, ai.position.y, deltaTime); //glfwGetTime());
				}


				// TODO: Do predictive movement even without packets.

			}

			break;
		}
		default:
			std::cout << "Received unhandled message." << std::endl;
			break;
		}
	}

	Gizmos::clear();

	// add a grid
	for (int i = 0; i < 21; ++i) {
		Gizmos::addLine(vec3(-10 + i, 0, 10), vec3(-10 + i, 0, -10),
						i == 10 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));

		Gizmos::addLine(vec3(10, 0, -10 + i), vec3(-10, 0, -10 + i),
						i == 10 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));
	}

	return true;
}

void AssessmentNetworkingApplication::draw() 
{
	// clear the screen for this frame
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// draw entities
	for (auto& ai : m_aiEntities) {
		vec3 p1 = vec3(ai.position.x + ai.velocity.x * 0.25f, 0, ai.position.y + ai.velocity.y * 0.25f);
		vec3 p2 = vec3(ai.position.x, 0, ai.position.y) - glm::cross(vec3(ai.velocity.x, 0, ai.velocity.y), vec3(0, 1, 0)) * 0.1f;
		vec3 p3 = vec3(ai.position.x, 0, ai.position.y) + glm::cross(vec3(ai.velocity.x, 0, ai.velocity.y), vec3(0, 1, 0)) * 0.1f;
		if (ai.id != 1)
		{
			Gizmos::addTri(p1, p2, p3, glm::vec4(1, 0, 0, 1));
		}
		else
		{
			Gizmos::addTri(p1, p2, p3, glm::vec4(1, 0, 1, 1));
		}
	}

	// display the 3D gizmos
	Gizmos::draw(m_camera->getProjectionView());
}

float lerp(float v0, float v1, float t) 
{
	return (1 - t)*v0 + t*v1;
}