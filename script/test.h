#ifndef SCRIPT_TEST_HEADER
#define SCRIPT_TEST_HEADER


#include "../core/renderer.h"
#include "../core/input.h"
#include "../core/wheel.h"


// boundaries
constexpr vec2 TEST_FIELD_DIMENSION = vec2(10.f,10.f);
constexpr vec2 TEST_CHAR_DIMENSION = vec2(.5f,.25f);

// movement
constexpr f32 TEST_MOVEMENT_SPEED = .75f;


class TestScene
{
public:
	TestScene();
	static inline void _update(void* cc) { TestScene* p = (TestScene*)cc; p->update(); }
	void update();

private:

	lptr<GeometryBatch> m_AnimationBatch;
	u32 m_DudeID;
	//AnimatedMesh m_Dude = AnimatedMesh("./res/test/dude.dae");
	AnimatedMesh m_Dude = AnimatedMesh("./res/test/me.dae");
	vec3 m_PlayerPosition = vec3(0,0,.8f);
	TargetMomentumSnap m_PlayerMomentum = TargetMomentumSnap(.1f);
};


#endif
