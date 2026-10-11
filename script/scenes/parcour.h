#ifndef SCRIPT_PARCOUR_HEADER
#define SCRIPT_PARCOUR_HEADER


#include "../../core/renderer.h"
#include "../../core/input.h"
#include "../../core/wheel.h"


// environment
constexpr u8 TEST_LIGHTBULB_COUNT = 8;


struct BallIndex
{
	vec3 position = vec3(0);
	f32 scale = 1.f;
	vec3 colour = vec3(1);
	vec2 material = vec2(0,.4f);
};


class ParcourParcs
{
public:
	void init();
	void update();
	void vanish();

private:
};


#endif
