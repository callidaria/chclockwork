#ifndef SCRIPT_CLOCKWORK_HEADER
#define SCRIPT_CLOCKWORK_HEADER


#include "../core/blitter.h"
#include "../core/renderer.h"
#include "../core/input.h"
#include "../core/wheel.h"


// movement
constexpr f32 CLOCKWORK_MVMT_ACCELERATION = 25;
constexpr f32 CLOCKWORK_MVMT_FLOATFACTOR = .1f;

// zoom
constexpr f32 CLOCKWORK_ZOOM_ACCELLERATION = -25;
constexpr f32 CLOCKWORK_ZOOM_FLOATFACTOR = .0005f;

// yaw
constexpr f32 CLOCKWORK_ROT_MOUSEACC = -10;
constexpr f32 CLOCKWORK_ROT_FLOATFACTOR = .0001f;


class Clockwork
{
public:
	Clockwork(Font* font);
	static inline void _update(void* cc) { Clockwork* p = (Clockwork*)cc; p->update(); }
	void update();

private:

	TargetMomentumSnap m_CameraPosition = TargetMomentumSnap(CLOCKWORK_MVMT_FLOATFACTOR);
	f32 m_ZoomMomentum = .0f;
	vec2 m_RotMomentum = vec2(0);

	// measurements
	lptr<Text> m_FPS;
};


#endif
