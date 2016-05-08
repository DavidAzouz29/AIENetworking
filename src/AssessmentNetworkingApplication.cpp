/// <summary>
/// Author: David Azouz
/// Date modified: 26/04/16
/// ------------------------------------------------------------------
/// viewed: http://gafferongames.com/networking-for-game-programmers/
/// https://en.wikipedia.org/wiki/Low-pass_filter
/// https://devblogs.nvidia.com/parallelforall/lerp-faster-cuda/
/// http://glm.g-truc.net/0.9.4/api/a00129.html#ga3f64b3986efe205cf30300700667e761
/// http://www.cplusplus.com/reference/cmath/fma/
/// https://en.wikipedia.org/wiki/Dead_reckoning
/// AIE only - Networking P3 Dead Reckoning 
/// http://aieportal.aie.edu.au/pluginfile.php/51756/mod_resource/content/0/Exercise%20-%20Networking%20Part%203.pdf
/// http://www.jenkinssoftware.com/raknet/manual/timestamping.html
/// ------------------------------------------------------------------
/// ***Edit***
/// Timestamping - David Azouz 8/05/2016
/// </summary>
/// ------------------------------------------------------------------

#include "AssessmentNetworkingApplication.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>
//#include <GetTime.h> //TODO: 

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
	m_camera = new Camera(glm::pi<GLfloat>() * 0.25f, 16 / 9.f, 0.1f, 1000.f);
	m_camera->setLookAtFrom(vec3(10, 10, 10), vec3(0));

	// start client connection
	m_peerInterface = RakNet::RakPeerInterface::GetInstance();
	
	RakNet::SocketDescriptor sd;
	m_peerInterface->Startup(1, &sd, 1);

	// request access to server
	std::string ipAddress = "";
	std::cout << "Press 1 to connect to the local host \nOr 2 to input your own: ";
	GLint isIPAddress = -1;
	std::cin >> isIPAddress;
	// if the user inputs '1'… 
	if (isIPAddress == 1)
	{
		// …connect to the local host, else…
		ipAddress = "localhost";
		std::cout << "Connecting to server at: " << ipAddress << "\n";
	}
	// ... allow the user to input their own IP address to connect to
	else
	{
		std::cout << "Connecting to server at: ";
		std::cin >> ipAddress;
	}
	RakNet::ConnectionAttemptResult res = m_peerInterface->Connect(ipAddress.c_str(), SERVER_PORT, nullptr, 0);

	// Timestamping 
	m_uiPrevTimeStamp = 0;

	if (res != RakNet::CONNECTION_ATTEMPT_STARTED) 
	{
		std::cout << "Unable to start connection, Error number: " << res << std::endl;
		return false;
	}

	return true;
}

GLvoid AssessmentNetworkingApplication::shutdown() 
{
	// delete our camera and cleanup gizmos
	delete m_camera;
	Gizmos::destroy();

	// destroy our window properly
	destroyWindow();
}

/// --------------------------------------
/// <summary>
/// Timestamping: Check the packets against the timestamp.
/// <para>http://www.jenkinssoftware.com/raknet/manual/receivingpackets.html </para>
/// </summary> //unsigned char GLubyte
unsigned char GetPacketIdentifier(RakNet::Packet *p)
{
	if ((unsigned char)p->data[0] == ID_TIMESTAMP)
	{
		//return (unsigned char)p->data[sizeof(unsigned char) + sizeof(unsigned long)];
		// Read from past the message data and timestamp TODO: correct method or should we read the timestamp?
		// Return the message: ID_ENTITY_LIST
		return (unsigned char)p->data[sizeof(RakNet::MessageID)]; //RakNet::Time // + sizeof(uint64_t)
	}
	else
	{
		return (unsigned char)p->data[0];
	}
}

bool AssessmentNetworkingApplication::update(GLfloat deltaTime)
{
	// close the application if the window closes
	if (glfwWindowShouldClose(m_window) ||
		glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		return false;
	}

	// update camera
	m_camera->update(deltaTime);

	// handle network messages
	RakNet::Packet* packet;
	for (packet = m_peerInterface->Receive(); packet;
		m_peerInterface->DeallocatePacket(packet),
		packet = m_peerInterface->Receive()) 
	{
		unsigned char abc = GetPacketIdentifier(packet);
		switch (abc) //->data[0]) //TODO: remove
		{
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
		case ID_ENTITY_LIST:
		{
			// receive list of entities
			RakNet::BitStream stream(packet->data, packet->length, false);
			stream.IgnoreBytes(sizeof(RakNet::MessageID)); // Ignore the ID_TIMESTAMP message.
			//GLubyte useTimeStamp = 0; // TODO: UCHAR?
			uint64_t timeStamp = 0; // sizeof(useTimeStamp); // <- use this if useTimeStamp is needed
			GLuint size = sizeof(stream.Read(timeStamp));// +sizeof(RakNet::MessageID)); //some reason must be seperate?
			//stream.Read(useTimeStamp);
			stream.Read(size);

			//bool isSecondRun = false;

			// if first time receiving entities...
			if (m_aiEntities.size() == 0)
			{
				// ... resize our vector, otherwise...
				m_aiEntities.resize(size / sizeof(AIEntity));
				//isSecondRun = true;
			}
			// ... the second time through, 
			else
			{
				// set the current entites to the previous,
				m_aiPrevEntities = m_aiEntities;
				// and the current time stamp to the previous.
				m_uiPrevTimeStamp = timeStamp;
			}

			// Setting current entites.
			stream.Read(timeStamp);
			stream.Read((char*)m_aiEntities.data(), size);

			// Will help determine if a packet is lost if data is out of our defined range.
			GLfloat fRange = 25.0f;

			// if it's the second time running, assign our current entites data to our previous.
			/*if (isSecondRun)
			{
				m_aiPrevEntities = m_aiEntities;
				continue;
			}*/

			/// --------------------------------------
			/// <summary>
			/// To account for packet loss/ stuttering.
			/// Check for lost packets by identifying whether data has exceeded our range.
			/// If so, adjusts the entites position.
			/// </summary> for (auto& ai : m_aiEntities)
			/// --------------------------------------
			for (GLuint i = 0; i < m_aiEntities.size(); ++i)
			{
				// Our current data
				AIEntity& ai = m_aiEntities[i];
				AIEntity& pAI = m_aiPrevEntities[i];
				// if our past position with our current velocity is more than our allocated distance...
				//glm::vec2 v2ExpectedPos(pAI.position.x + ai.velocity.x * deltaTime, pAI.position.y + ai.velocity.y * deltaTime);
				glm::vec2 v2ExpectedPos(pAI.position.x + pAI.velocity.x * deltaTime, pAI.position.y + pAI.velocity.y * deltaTime);
				// Position data
				glm::vec2 v2PreviousPos(pAI.position.x, pAI.position.y);
				glm::vec2 v2CurrentPos(ai.position.x, ai.position.y);
				// Velocity data
				glm::vec2 v2PreviousVel(pAI.velocity.x, pAI.velocity.y);
				glm::vec2 v2CurrentVel(ai.velocity.x, ai.velocity.y);

				glm::vec2 v2Heading(v2ExpectedPos - v2CurrentPos);
				//glm::vec2 v2Heading(abs(v2ExpectedPos - v2CurrentPos));
				GLfloat fDot = glm::dot(v2Heading, v2CurrentPos);

				// if our current timestamp is before our previous time stamp, or
				// our distance from our expected position is outside our range, and we haven't teleported
				if (timeStamp < m_uiPrevTimeStamp ||
					glm::distance(v2ExpectedPos, v2CurrentPos) > fRange && !ai.teleported) // && pAI.position.x < v2CurrentPos.x)
				{
					//... set our current pos to our expected pos
					//ai.position.x = fExpectedPosX;
					//ai.position.y = fExpectedPosY;
					std::cout << "Entity " << ai.id << " moved." << std::endl;

					/// <summary>
					/// TODO: lerp from our previous position to our current position over time.
					/// adjusts position and velocity to our current data.
					/// <example> Lerp = fma(t, v1, fma(-t, v0, v0)) </example> 
					/// </summary> //old//glm::mix(v2CurrentPos, v2ExpectedPos, deltaTime); //glfwGetTime());
					glm::mix(v2PreviousPos, v2CurrentPos, deltaTime); //glfwGetTime());
					glm::mix(v2PreviousVel, v2CurrentVel, deltaTime); // TODO: needed?

					/*glm::mix(v2CurrentVel, v2ExpectedPos, deltaTime); //glfwGetTime());
					//fmaf(glfwGetTime(), ai.position.x, fmaf(-glfwGetTime(), pAI.position.x, pAI.position.x));
					//fmaf(glfwGetTime(), ai.position.y, fmaf(-glfwGetTime(), pAI.position.y, pAI.position.y));
					//glm::mix(pAI.position.x, ai.position.x, deltaTime); //glfwGetTime()); //TODO: deltaTime?
					//glm::mix(pAI.position.y, ai.position.y, deltaTime); //glfwGetTime()); */
				}
				// ... if new packet data is behind us
				else if (v2ExpectedPos.x < v2Heading.x && v2ExpectedPos.y < v2Heading.y)
				{
					glm::mix(v2PreviousVel, v2CurrentVel, deltaTime); // TODO: needed?
				}
				// TODO: if packet is null
				// continue at current velocity
			}

			break;
		}
		default:
			std::cout << "Received unhandled message." << std::endl;
			break;
		}
	}

	// TODO:
	// Predictive movement: predict movement without receiving packets.
	// Dead Reckoning: adjusts the entites positions
	for (GLuint i = 0; i < m_aiEntities.size(); ++i)
	{
		AIEntity& ai = m_aiEntities[i];
		AIEntity& pAI = m_aiPrevEntities[i];
		//glm::vec2 v2ExpectedPos(pAI.position.x + ai.velocity.x * deltaTime, pAI.position.y + ai.velocity.y * deltaTime);
		glm::vec2 v2ExpectedPos(pAI.position.x + pAI.velocity.x * deltaTime, pAI.position.y + pAI.velocity.y * deltaTime);
		glm::vec2 v2CurrentPos(ai.position.x, ai.position.y);

		v2CurrentPos = v2ExpectedPos;
	}

	Gizmos::clear();

	// add a grid
	for (GLint i = 0; i < 21; ++i)
	{
		Gizmos::addLine(vec3(-10 + i, 0, 10), vec3(-10 + i, 0, -10),
						i == 10 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));

		Gizmos::addLine(vec3(10, 0, -10 + i), vec3(-10, 0, -10 + i),
						i == 10 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));
	}

	return true;
}

GLvoid AssessmentNetworkingApplication::draw()
{
	// clear the screen for this frame
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// draw entities
	for (auto& ai : m_aiEntities) 
	{
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
/*
float lerp(float v0, float v1, float t) 
{
	return (1 - t)*v0 + t*v1;
} //*/