#include "test.h"


#ifdef VKBUILD


/**
 *	setup scene listing
 */
SceneListing::SceneListing(Font* font)
	: m_Font(font)
{
	//Texture* __CursedBird = g_Renderer.register_texture("./res/test/cld.jpeg",TEXTURE_FORMAT_SRGB);
	//Rect* __CursedBird = g_Renderer.register_sprite_texture("./res/test/cld.jpeg");
	/*
	Sprite* __TestSprite0 = g_Renderer.register_sprite(__CursedBird,vec3(100,100,2),vec2(100,100),.0f,1.f,
													   { .alignment=SCREEN_ALIGN_TOPLEFT });
	Sprite* __TestSprite1 = g_Renderer.register_sprite(__CursedBird,vec3(0,0,5),vec2(100,100),.0f,.25f,
													   { .alignment=SCREEN_ALIGN_CENTER });
	Sprite* __TestSprite2 = g_Renderer.register_sprite(__CursedBird,vec3(-250,-250,4),vec2(120,120),.0f,1.f,
													   { .alignment=SCREEN_ALIGN_BOTTOMRIGHT });
	*/

	// textures
	Rect* __ButtonBase = g_Renderer.register_sprite_texture("./res/ui/button_base.png");
	Rect* __ButtonHover = g_Renderer.register_sprite_texture("./res/ui/button_hover.png");
	Rect* __ButtonClick = g_Renderer.register_sprite_texture("./res/ui/button_click.png");

	// heading
	lptr<Text> __Text = g_Renderer.write_text(font,"Test Scenes",vec3(0,-10,7),15,vec4(1),
											  Alignment{ .alignment=SCREEN_ALIGN_TOPCENTER });

	// selection scene ui
	m_UI = g_UI.add_batch(font);
	m_BTNTextures = m_UI->add_button("texture playground",__ButtonBase,__ButtonHover,__ButtonClick,
					 vec3(0,25,7),vec2(250,25),Alignment{ .alignment=SCREEN_ALIGN_CENTER });
	m_BTNVoxel = m_UI->add_button("voxelmatrix projection",__ButtonBase,__ButtonHover,__ButtonClick,
					 vec3(0,0,7),vec2(250,25),Alignment{ .alignment=SCREEN_ALIGN_CENTER });
	m_BTNParcour = m_UI->add_button("parcour parcs",__ButtonBase,__ButtonHover,__ButtonClick,
					 vec3(0,-25,7),vec2(250,25),Alignment{ .alignment=SCREEN_ALIGN_CENTER });

	m_Self = g_Wheel.call(this);
}

/**
 *	check for scene requests by user
 */
void SceneListing::update()
{
	u8 __Action = m_BTNTextures->confirm+m_BTNVoxel->confirm*2+m_BTNParcour->confirm*3;
	if (!__Action) return;

	// load requested scene
	switch (__Action)
	{
	case 1:  // texture playground
		// TODO
		break;
	case 2: m_RoomVoxels.init(m_Font);
		break;
	case 3: m_ParcourParcs.init();
		break;
	};

	g_Wheel.stop(m_Self);
}

/**
 *	destruct test scene
 */
void SceneListing::vanish()
{
	g_UI.remove_batch(m_UI);
}


#else  // §§prototyping remove

/**
 *	setup test scene
 */
TestScene::TestScene()
{
	// resources
	vector<Texture*> __DudeTexture = {
		g_Renderer.register_texture("./res/private/texmex.jpg",TEXTURE_FORMAT_SRGB),
		g_Renderer.register_texture("./res/standard/normal.png"),
		g_Renderer.register_texture("./res/standard/material.png"),
	};
	vector<Texture*> __FloorTexture = {
		g_Renderer.register_texture("./res/test/floor_colour.png",TEXTURE_FORMAT_SRGB),
		g_Renderer.register_texture("./res/test/floor_normal.png"),
		g_Renderer.register_texture("./res/test/floor_material.png"),
	};
	vector<Texture*> __GoldTexture = {
		g_Renderer.register_texture("./res/test/gold_colour.png",TEXTURE_FORMAT_SRGB),
		g_Renderer.register_texture("./res/test/gold_normal.png"),
		g_Renderer.register_texture("./res/test/gold_material.png"),
	};
	vector<Texture*> __FabricTexture = {
		g_Renderer.register_texture("./res/test/fabric_colour.png",TEXTURE_FORMAT_SRGB),
		g_Renderer.register_texture("./res/test/fabric_normal.png"),
		g_Renderer.register_texture("./res/test/fabric_material.png"),
	};
	// TODO pre-store standard texture fallbacks

	// geometry
	Mesh __Sphere = Mesh::sphere();
	Mesh __Cube = Mesh::cube();

	// shaders
	VertexShader __AnimationVertexShader = VertexShader("./shader/ogl/animation.vert");
	VertexShader __AnimationShadowShader = VertexShader("./shader/ogl/animation_shadow.vert");
	VertexShader __BulbVertexShader = VertexShader("./shader/ogl/bulb.vert");
	FragmentShader __AnimationFragmentShader = FragmentShader("./shader/ogl/gpass.frag");
	FragmentShader __ShadowShader = FragmentShader("./shader/ogl/shadow.frag");
	FragmentShader __BulbFragmentShader = FragmentShader("./shader/ogl/bulb.frag");
	lptr<ShaderPipeline> __AnimationShader = g_Renderer.register_pipeline(__AnimationVertexShader,
																		  __AnimationFragmentShader);
	lptr<ShaderPipeline> __AnimationShadowPipeline = g_Renderer.register_pipeline(__AnimationShadowShader,
																				  __ShadowShader);
	lptr<ShaderPipeline> __BulbPipeline = g_Renderer.register_pipeline(__BulbVertexShader,__BulbFragmentShader);

	// animation batch
	m_AnimationBatch = g_Renderer.register_deferred_geometry_batch(__AnimationShader);
	m_DudeID = m_AnimationBatch->add_geometry(m_Dude,__DudeTexture);
	m_AnimationBatch->load();
	g_Renderer.register_shadow_batch(m_AnimationBatch,__AnimationShadowPipeline);

	// geometry batch
	lptr<GeometryBatch> __PhysicalBatch = g_Renderer.register_deferred_geometry_batch();
	u32 __FloorID = __PhysicalBatch->add_geometry(__Cube,__FloorTexture);
	u32 __BoxID = __PhysicalBatch->add_geometry(__Cube,__FloorTexture);
	u32 __BoxID0 = __PhysicalBatch->add_geometry(__Cube,__GoldTexture);
	u32 __BoxID1 = __PhysicalBatch->add_geometry(__Sphere,__FabricTexture);
	__PhysicalBatch->load();
	g_Renderer.register_shadow_batch(__PhysicalBatch);

	// environment scale
	__PhysicalBatch->objects[__FloorID].transform.scale(glm::vec3(10,10,.1f));
	__PhysicalBatch->objects[__FloorID].transform.translate(glm::vec3(0,0,-.1f));
	__PhysicalBatch->objects[__FloorID].texel = 10.f;
	__PhysicalBatch->objects[__BoxID].transform.translate(glm::vec3(4,4,.75f));
	__PhysicalBatch->objects[__BoxID].transform.scale(.75f);
	__PhysicalBatch->objects[__BoxID0].transform.translate(glm::vec3(-4,4,1));
	__PhysicalBatch->objects[__BoxID0].transform.scale(.5f);
	__PhysicalBatch->objects[__BoxID0].texel = 4.f;
	__PhysicalBatch->objects[__BoxID1].transform.translate(glm::vec3(0,-4,.5));
	__PhysicalBatch->objects[__BoxID1].transform.scale(.5f);
	__PhysicalBatch->objects[__BoxID1].texel = 2.f;

	// lightbulbs
	BallIndex __BulbIndices[TEST_LIGHTBULB_COUNT] = {
		{ vec3(9,9,.2f),.2f,vec3(1,0,0),vec2(0,.4f) },
		{ vec3(9,-9,.7f),.2f,vec3(1,1,0),vec2(0,.4f) },
		{ vec3(-9,9,1.2f),.2f,vec3(0,1,0),vec2(0,.4f) },
		{ vec3(-9,-9,.2f),.2f,vec3(0,1,1),vec2(0,.4f) },
		{ vec3(5,9,.7f),.2f,vec3(0,0,1),vec2(0,.4f) },
		{ vec3(5,-9,1.2f),.2f,vec3(1,0,1),vec2(0,.4f) },
		{ vec3(-9,5,.2f),.2f,vec3(.5f,.5f,0),vec2(0,.4f) },
		{ vec3(-9,-5,.7f),.2f,vec3(0,.5f,.5f),vec2(0,.4f) },
	};
	/*
	lptr<ParticleBatch> __BulbBatch = g_Renderer.register_deferred_particle_batch(__BulbPipeline);
	__BulbBatch->load(__Sphere,TEST_LIGHTBULB_COUNT,sizeof(BallIndex));
	g_Renderer.register_shadow_batch(__BulbBatch);
	__BulbBatch->ibo.bind();
	__BulbBatch->ibo.upload_vertices(__BulbIndices);
	*/

	// lighting
	g_Renderer.add_sunlight(vec3(75,-150,100),vec3(1),.5f);
	for (u8 i=0;i<TEST_LIGHTBULB_COUNT;i++)
		g_Renderer.add_pointlight(__BulbIndices[i].position,__BulbIndices[i].colour,10.f,1.f,.8f,.24f);
	g_Renderer.upload_lighting();

	// standard setup
	m_PlayerMomentum.target = m_PlayerPosition;
	g_Frame.time_factor = 2.f;  // FIXME yes this is bad
	// TODO add a feature to accelerate animation on load so this can be avoided
	m_Dude.set_default_animation(DANIM_IDLE,.4f);
	g_Renderer.animate(&m_Dude);

	// positional correction
	m_SpineJointTransform = &m_Dude.find_joint("metarig_spine")->transform;

	g_Wheel.call(this);
}

/**
 *	update test scene
 */
void TestScene::update()
{
	// camera view geometry
	vec3 __CenteredPosition = vec3((*m_SpineJointTransform)[3]);
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
		if (m_Dude.current_animation!=DANIM_JUMPING) m_MoveState = MOVE_STANDARD;
		break;

	case MOVE_ROLLING:
		// rolling movement
		m_PosDelta = vec3(m_MoveDirection.x,m_MoveDirection.y,0)
				*vec3((m_Dude.get_progress()<.65f&&m_Dude.get_progress()>.25f)*TEST_ROLL_SPEED);

		// state machine flow
		if (m_Dude.current_animation!=DANIM_ROLLING) m_MoveState = MOVE_STANDARD;
		break;

	case MOVE_CELEBRATING:
		if (m_Dude.current_animation!=DANIM_CELEBRATE) m_MoveState = MOVE_STANDARD;
		break;

	default:
		// walking movement
		m_PlayerAttitude.target = (
				f32(g_Input.keyboard.keys[SDL_SCANCODE_W]-g_Input.keyboard.keys[SDL_SCANCODE_S])*__Attitude
				+f32(g_Input.keyboard.keys[SDL_SCANCODE_A]-g_Input.keyboard.keys[SDL_SCANCODE_D])*__OrthoAttitude
			)*TEST_MOVEMENT_SPEED;
		m_PlayerAttitude.update(m_PosDelta,g_Frame.delta_time);
		m_PlayerRotation = angular_relationship(vec2(0,-1),vec2(m_PosDelta.x,m_PosDelta.y));

		// switch animation state
		m_Dude.current_animation = 3-(glm::length(m_PosDelta)>.01f);
		m_MoveDirection = m_PosDelta;

		// state machine flow
		if (g_Input.keyboard.keys[SDL_SCANCODE_SPACE])
		{
			m_MoveState = MOVE_JUMPING;
			m_Dude.set_animation(DANIM_JUMPING,.5f);
		}
		else if (g_Input.keyboard.keys[SDL_SCANCODE_LSHIFT])
		{
			m_MoveState = MOVE_ROLLING;
			m_Dude.set_animation(DANIM_ROLLING,.5f);
		}
		else if (g_Input.keyboard.keys[SDL_SCANCODE_E])
		{
			m_MoveState = MOVE_CELEBRATING;
			m_PosDelta = vec3(.0f);
			m_Dude.set_animation(DANIM_CELEBRATE,.5f);
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
			* glm::rotate(mat4(1.f),m_PlayerRotation,vec3(0,0,1))
			* m_AnimationBatch->objects[m_DudeID].transform.model;
}

#endif
