#include "test.h"


/**
 *	setup test scene
 */
TestScene::TestScene()
{
	// resources
	vector<Texture*> __DudeTexture = {
		g_Renderer.register_texture("./res/test/texmex.jpg",TEXTURE_FORMAT_SRGB),
		g_Renderer.register_texture("./res/standard/normal.png"),
		g_Renderer.register_texture("./res/standard/material.png"),
	};
	vector<Texture*> __FloorTexture = {
		g_Renderer.register_texture("./res/test/floor_colour.png",TEXTURE_FORMAT_SRGB),
		g_Renderer.register_texture("./res/test/floor_normal.png"),
		g_Renderer.register_texture("./res/test/floor_material.png"),
	};
	// TODO pre-store standard texture fallbacks

	// geometry
	Mesh __Cube = Mesh::cube();

	// shaders
	VertexShader __AnimationVertexShader = VertexShader("./core/shader/animation.vert");
	VertexShader __AnimationShadowShader = VertexShader("./core/shader/animation_shadow.vert");
	FragmentShader __AnimationFragmentShader = FragmentShader("./core/shader/gpass.frag");
	FragmentShader __ShadowShader = FragmentShader("./core/shader/shadow.frag");
	lptr<ShaderPipeline> __AnimationShader = g_Renderer.register_pipeline(__AnimationVertexShader,
																		  __AnimationFragmentShader);
	lptr<ShaderPipeline> __AnimationShadowPipeline = g_Renderer.register_pipeline(__AnimationShadowShader,
																				  __ShadowShader);

	// animation batch
	m_AnimationBatch = g_Renderer.register_deferred_geometry_batch(__AnimationShader);
	m_DudeID = m_AnimationBatch->add_geometry(m_Dude,__DudeTexture);
	m_AnimationBatch->load();
	g_Renderer.register_shadow_batch(m_AnimationBatch,__AnimationShadowPipeline);

	// geometry batch
	lptr<GeometryBatch> __PhysicalBatch = g_Renderer.register_deferred_geometry_batch();
	u32 __FloorID = __PhysicalBatch->add_geometry(__Cube,__FloorTexture);
	u32 __BoxID = __PhysicalBatch->add_geometry(__Cube,__FloorTexture);
	__PhysicalBatch->load();
	g_Renderer.register_shadow_batch(__PhysicalBatch);

	// environment scale
	__PhysicalBatch->objects[__FloorID].transform.scale(glm::vec3(10,10,.1f));
	__PhysicalBatch->objects[__FloorID].transform.translate(glm::vec3(0,0,-.1f));
	__PhysicalBatch->objects[__FloorID].texel = 10.f;
	__PhysicalBatch->objects[__BoxID].transform.translate(glm::vec3(4,4,1));

	// lighting
	g_Renderer.add_sunlight(vec3(75,-150,100),vec3(1),4.f);
	g_Renderer.upload_lighting();

	// standard setup
	m_PlayerMomentum.target = m_PlayerPosition;
	g_Frame.time_factor = 2.f;
	m_Dude.standard_animation = 3;

	g_Wheel.call(UpdateRoutine{ &TestScene::_update,(void*)this });
}

/**
 *	update test scene
 */
void TestScene::update()
{
	// player input
	/*
	else if (g_Input.keyboard.keys[SDL_SCANCODE_U]) m_Dude.current_animation = 4;	// fall
	else if (g_Input.keyboard.keys[SDL_SCANCODE_I]) m_Dude.current_animation = 5;	// dab
	*/
	// animation update
	m_Dude.update();

	// camera view geometry
	vec3 __CenteredPosition = vec3(m_Dude.joints[2].transform[3]);
	vec3 __Attitude = glm::normalize(vec3(g_Camera.target.x-g_Camera.position.x,
										  g_Camera.target.y-g_Camera.position.y,0));
	vec3 __OrthoAttitude = vec3(-__Attitude.y,__Attitude.x,0);

	switch (m_MoveState)
	{
	case MOVE_JUMPING:

		// jumping movement
		m_PosDelta = vec3(m_MoveDirection.x,m_MoveDirection.y,0)
				*vec3(m_Dude.get_progress()*(m_Dude.get_progress()<.9f)*TEST_JUMP_SPEED);
		m_PosDelta.z = glm::sin((m_Dude.get_progress()-.5f)*2.5f*MATH_PI*2)
				*(m_Dude.get_progress()<.9f&&m_Dude.get_progress()>.5f)*TEST_JUMP_HEIGHT;

		// state machine flow
		if (m_Dude.current_animation!=0) m_MoveState = MOVE_STANDARD;
		break;
	case MOVE_ROLLING:

		// rolling movement
		m_PosDelta = vec3(m_MoveDirection.x,m_MoveDirection.y,0)
				*vec3((m_Dude.get_progress()<.65f&&m_Dude.get_progress()>.25f)*TEST_ROLL_SPEED);

		// state machine flow
		if (m_Dude.current_animation!=1) m_MoveState = MOVE_STANDARD;
		break;
	default:

		// walking movement
		m_PlayerAttitude.target = (
				f32(g_Input.keyboard.keys[SDL_SCANCODE_W]-g_Input.keyboard.keys[SDL_SCANCODE_S])*__Attitude
				+f32(g_Input.keyboard.keys[SDL_SCANCODE_A]-g_Input.keyboard.keys[SDL_SCANCODE_D])*__OrthoAttitude
			)*TEST_MOVEMENT_SPEED;
		m_PlayerAttitude.update(m_PosDelta,g_Frame.delta_time);
		m_PlayerRotation = relationship_degrees(vec2(0,-1),vec2(m_PosDelta.x,m_PosDelta.y));

		// switch animation state
		m_Dude.current_animation = 3-(glm::length(m_PosDelta)>.01f);
		m_MoveDirection = m_PosDelta;

		// state machine flow
		if (g_Input.keyboard.keys[SDL_SCANCODE_SPACE])
		{
			m_MoveState = MOVE_JUMPING;
			m_Dude.set_animation(0);
		}
		else if (g_Input.keyboard.keys[SDL_SCANCODE_LSHIFT])
		{
			m_MoveState = MOVE_ROLLING;
			m_Dude.set_animation(1);
		}
	};

	// linear interpretation of geometric request
	m_PlayerMomentum.target += m_PosDelta*g_Frame.delta_time;
	m_PlayerMomentum.target.x = glm::clamp(m_PlayerMomentum.target.x,
										   -TEST_FIELD_DIMENSION.x+TEST_CHAR_DIMENSION.x,
										   TEST_FIELD_DIMENSION.x-TEST_CHAR_DIMENSION.x);
	m_PlayerMomentum.target.y = glm::clamp(m_PlayerMomentum.target.y,
										   -TEST_FIELD_DIMENSION.y+TEST_CHAR_DIMENSION.y,
										   TEST_FIELD_DIMENSION.y-TEST_CHAR_DIMENSION.y);
	m_PlayerMomentum.update(m_PlayerPosition,g_Frame.delta_time);

	// camera
	g_Camera.target = m_PlayerPosition;

	// player transformation
	m_AnimationBatch->objects[m_DudeID].transform.reset();
	m_AnimationBatch->objects[m_DudeID].transform.rotate_x(90.f);
	m_AnimationBatch->objects[m_DudeID].transform.translate(-__CenteredPosition);
	m_AnimationBatch->objects[m_DudeID].transform.model =
			glm::translate(mat4(1.f),m_PlayerPosition)
			* glm::rotate(mat4(1.f),glm::radians(m_PlayerRotation),vec3(0,0,1))
			* m_AnimationBatch->objects[m_DudeID].transform.model;
}
