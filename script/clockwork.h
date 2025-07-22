#ifndef SCRIPT_CLOCKWORK_HEADER
#define SCRIPT_CLOCKWORK_HEADER


#include "../core/blitter.h"
#include "../core/renderer.h"
#include "../core/input.h"
#include "../core/wheel.h"


// movement
constexpr f32 CLOCKWORK_MVMT_ACCEL = .15f;
constexpr f32 CLOCKWORK_MVMT_FLOATFACTOR = .8f;

// zoom
constexpr f32 CLOCKWORK_ZOOM_ACCELLERATION = -.15f;
constexpr f32 CLOCKWORK_ZOOM_FLOATFACTOR = .9f;
constexpr f32 CLOCKWORK_ZOOM_MINDIST = 2.f;
constexpr f32 CLOCKWORK_ZOOM_MAXDIST = 50.f;
constexpr f32 CLOCKWORK_ZOOM_EASE = .25f;

// yaw
constexpr f32 CLOCKWORK_ROT_MOUSEACC = -.05f;
constexpr f32 CLOCKWORK_ROT_KEYACC = .5f;
constexpr f32 CLOCKWORK_ROT_FLOATFACTOR = .8f;


class Clockwork
{
public:
	Clockwork(Font* font);
	static inline void _update(void* cc) { Clockwork* p = (Clockwork*)cc; p->update(); }
	void update();

private:

	vec3 m_CameraMomentum = vec3(0);
	f32 m_ZoomMomentum = .3f;
	vec2 m_RotMomentum = vec2(.0f);

	// measurements
	lptr<Text> m_FPS;
};


#endif
