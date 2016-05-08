#pragma once

#include "gl_core_4_4.h"

struct GLFWwindow;

class BaseApplication {
public:

	BaseApplication() {}
	virtual ~BaseApplication() {}

	GLvoid run();
	
	virtual bool startup() = 0;
	virtual GLvoid shutdown() = 0;

	virtual bool update(GLfloat deltaTime) = 0;
	virtual GLvoid draw() = 0;

protected:

	virtual bool createWindow(const char* title, int width, int height);
	virtual GLvoid destroyWindow();

	GLFWwindow*	m_window;
};