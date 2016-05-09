/// <summary>
/// Author: David Azouz
/// Date modified: 9/05/16
/// ------------------------------------------------------------------
/// viewed: http://gafferongames.com/networking-for-game-programmers/
/// https://en.wikipedia.org/wiki/Low-pass_filter
/// http://glm.g-truc.net/0.9.4/api/a00129.html#ga3f64b3986efe205cf30300700667e761
/// https://en.wikipedia.org/wiki/Dead_reckoning
/// AIE only - Networking P3 Dead Reckoning 
/// http://aieportal.aie.edu.au/pluginfile.php/51756/mod_resource/content/0/Exercise%20-%20Networking%20Part%203.pdf
/// http://www.jenkinssoftware.com/raknet/manual/timestamping.html
/// ------------------------------------------------------------------
/// ***Edit***
/// Distance checking - David Azouz 2/05/2016
/// Timestamping - David Azouz 8/05/2016
/// Predictive Movement - David Azouz 9/05/2016
/// 
/// if packet is null
/// continue at current velocity
/// </summary>
/// ------------------------------------------------------------------

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
	m_uiCurrentTimeStamp = 0;

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
	// if our message is equal to the timestamp ID
	if ((unsigned char)p->data[0] == ID_TIMESTAMP)
	{
		// Read from past the message data
		// Returns the start of message: ID_ENTITY_LIST
		return (unsigned char)p->data[sizeof(RakNet::MessageID)];
	}
	// else return the message
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
		switch (GetPacketIdentifier(packet))
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
			stream.IgnoreBytes(sizeof(RakNet::MessageID)); // Ignore the ID_ENTITY_LIST message.
			stream.Read(m_uiCurrentTimeStamp);
			unsigned int size = 0;
			stream.Read(size);

			// used to determine whether it's the first time we are running
			bool isFirstRun = false;
			// determines whether a timestamp/ packet is delayed
			bool isOutOfOrder = false;

			// if first time receiving entities...
			if (m_aiEntities.size() == 0)
			{
				// ... resize our vector, otherwise...
				m_aiEntities.resize(size / sizeof(AIEntity));
				// set our current time stamp to our previous
				m_uiPrevTimeStamp = m_uiCurrentTimeStamp;
				isFirstRun = true;
			}
			// if it's the second time running, assign our current entites data to our previous.
			else
			{
				// set the current entites to the previous,
				m_aiPrevEntities = m_aiEntities;
			}

			// Setting current entites.
			stream.Read((char*)m_aiEntities.data(), size);

			// Will help determine if a packet is lost if data is out of our defined range.
			GLfloat fRange = 25.0f;

			// Reads on the first run.
			if (isFirstRun)
			{
				// set our current data to our previous to avoid a memory fault
				m_aiPrevEntities = m_aiEntities;
				continue;
			}

			/// --------------------------------------
			/// <summary>
			/// To account for packet loss/ stuttering.
			/// Checks our timestamp sent with the packet, and
			/// for lost packets by identifying whether data has exceeded our range.
			/// If so, adjusts the entites position.
			/// </summary> 
			/// --------------------------------------
			for (GLuint i = 0; i < m_aiEntities.size(); ++i)
			{
				// Our current data
				AIEntity& ai = m_aiEntities[i];
				// Previous data
				AIEntity& pAI = m_aiPrevEntities[i];
				// Expected position based off previous position and velocity data
				glm::vec2 v2ExpectedPos(pAI.position.x + pAI.velocity.x * deltaTime, pAI.position.y + pAI.velocity.y * deltaTime);
				glm::vec2 v2CurrentPos(ai.position.x, ai.position.y); // Current Position data
				glm::vec2 v2CurrentVel(ai.velocity.x, ai.velocity.y); // Current Velocity data

				// if packet is out of order (based off the timestamp)...
				if (m_uiCurrentTimeStamp < m_uiPrevTimeStamp)
				{
					// ...use our previous data
					ai = pAI;
					isOutOfOrder = true;
				}
				// ... else if our distance from our expected position is outside our range, and we haven't teleported
				else if (glm::distance(v2ExpectedPos, v2CurrentPos) > fRange && !ai.teleported)
				{
					//std::cout << "Entity " << ai.id << " moved." << std::endl;

					/// <summary>
					/// lerp from our previous velocity to our current velocity over time.
					/// <example> Lerp = fma(t, v1, fma(-t, v0, v0)) </example> 
					/// </summary> 
					ai.velocity.x = glm::mix(pAI.velocity.x, v2CurrentVel.x, deltaTime);
					ai.velocity.y = glm::mix(pAI.velocity.y, v2CurrentVel.y, deltaTime);
				}
			}

			// if our data is valid...
			if (!isOutOfOrder)
			{
				// ...set current time stamp to the previous.
				m_uiPrevTimeStamp = m_uiCurrentTimeStamp;
			}

			break;
		}
		default:
			std::cout << "Received unhandled message." << std::endl;
			break;
		}
	}

	// Predictive movement: predict movement without receiving packets.
	// Dead Reckoning: adjusts the entites positions
	for (GLuint i = 0; i < m_aiEntities.size(); ++i)
	{
		AIEntity& ai = m_aiEntities[i];
		glm::vec2 v2ExpectedPos(ai.position.x + ai.velocity.x * deltaTime, ai.position.y + ai.velocity.y * deltaTime);

		// set our position to where we expect to be
		ai.position.x = v2ExpectedPos.x;
		ai.position.y = v2ExpectedPos.y;
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
		// if the id is not equal to 1...
		if (ai.id != 1)
		{
			// ... set the colour to red...
			Gizmos::addTri(p1, p2, p3, glm::vec4(1, 0, 0, 1));
		}
		// ... else set the colour to pink
		else
		{
			Gizmos::addTri(p1, p2, p3, glm::vec4(1, 0, 1, 1));
		}
	}

	// display the 3D gizmos
	Gizmos::draw(m_camera->getProjectionView());
}