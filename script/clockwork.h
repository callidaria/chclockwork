#ifndef SCRIPT_CLOCKWORK_HEADER
#define SCRIPT_CLOCKWORK_HEADER
//#ifdef DEBUG



#include "../core/blitter.h"
#include "../core/renderer.h"
#include "../core/input.h"
#include "../core/wheel.h"


// movement
constexpr f32 CLOCKWORK_MVMT_ACCELERATION = 25;
constexpr f32 CLOCKWORK_MVMT_FLOATFACTOR = .1f;

// rotation
constexpr f32 CLOCKWORK_ZOOM_ACCELLERATION = -1.5f;
constexpr f32 CLOCKWORK_ROTATION_MOUSEACC = -.4f;
constexpr f32 CLOCKWORK_ROTATION_FLOATFACTOR = .1f;


class Clockwork
{
public:
	Clockwork(Font* font);
	void update();
	void vanish();

private:

	TargetMomentumSnap m_CameraPosition = TargetMomentumSnap(CLOCKWORK_MVMT_FLOATFACTOR);
	TargetMomentumSnap m_CameraRotation = TargetMomentumSnap(CLOCKWORK_ROTATION_FLOATFACTOR);

	vec3 m_TargetingVector =
#ifdef VKBUILD
		vec3(0);
#else
		vec3(0,25.f,0);
#endif

	// measurements
	lptr<Text> m_FPS;
};


#endif
