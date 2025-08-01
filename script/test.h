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
constexpr f32 TEST_JUMP_SPEED = 2;
constexpr f32 TEST_JUMP_HEIGHT = 1.5f;
constexpr f32 TEST_ROLL_SPEED = 1.5f;

// environment
constexpr u8 TEST_LIGHTBULB_COUNT = 8;


enum MovementState : u8
{
	MOVE_STANDARD,
	MOVE_JUMPING,
	MOVE_ROLLING,
	MOVE_FALLING,
	MOVE_CELEBRATING,
};

struct BallIndex
{
	vec3 position = vec3(0);
	f32 scale = 1.f;
	vec3 colour = vec3(1);
	vec2 material = vec2(0,.4f);
};

class TestScene
{
public:
	TestScene();
	static inline void _update(void* cc) { TestScene* p = (TestScene*)cc; p->update(); }
	void update();

private:

	// rendering
	lptr<GeometryBatch> m_AnimationBatch;
	//AnimatedMesh m_Dude = AnimatedMesh("./res/test/dude.dae");
	AnimatedMesh m_Dude = AnimatedMesh("./res/test/me.dae");
	u32 m_DudeID;

	// state
	MovementState m_MoveState = MOVE_STANDARD;

	// movement
	vec3 m_PlayerPosition = vec3(0,0,.8f);
	vec3 m_MoveDirection;
	f32 m_PlayerRotation;
	vec3 m_PosDelta = vec3(.0f);
	TargetMomentumSnap m_PlayerMomentum = TargetMomentumSnap(.15f);
	TargetMomentumSnap m_PlayerAttitude = TargetMomentumSnap(.25f);
};


#endif
