#ifndef SCRIPT_CLOCKWORK_HEADER
#define SCRIPT_CLOCKWORK_HEADER
#ifdef DEBUG


#include "../core/blitter.h"
#include "../core/renderer.h"
#include "../core/input.h"
#include "../core/wheel.h"


// movement
constexpr f32 CLOCKWORK_MVMT_ACCELERATION = 25;
constexpr f32 CLOCKWORK_MVMT_FLOATFACTOR = .1f;

// rotation
constexpr f32 CLOCKWORK_ZOOM_ACCELLERATION = -1;
constexpr f32 CLOCKWORK_ROTATION_MOUSEACC = -.5f;
constexpr f32 CLOCKWORK_ROTATION_FLOATFACTOR = .1f;


class Clockwork
{
public:
	Clockwork(Font* font);
	static inline void _update(void* cc) { Clockwork* p = (Clockwork*)cc; p->update(); }
	void update();

private:

	TargetMomentumSnap m_CameraPosition = TargetMomentumSnap(CLOCKWORK_MVMT_FLOATFACTOR);
	TargetMomentumSnap m_CameraRotation = TargetMomentumSnap(CLOCKWORK_ROTATION_FLOATFACTOR);
	vec3 m_TargetingVector = vec3(0,25.f,0);

	// measurements
	lptr<Text> m_FPS;
};


#endif
#endif
