#ifndef SCRIPT_TEST_HEADER
#define SCRIPT_TEST_HEADER


#include "../core/renderer.h"
#include "../core/input.h"
#include "../core/wheel.h"


class TestScene
{
public:
	TestScene();
	void update();

private:

	AnimatedMesh m_Dude = AnimatedMesh("./res/test/dude.dae");
};


#endif
