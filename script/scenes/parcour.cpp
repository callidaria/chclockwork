#include "parcour.h"


/**
 *	setup parcour test scene
 */
void ParcourParcs::init()
{
	// pipelines
	lptr<ShaderPipeline> __BulbPipeline = g_Renderer.register_pipeline(
		"./shader/vulkan/bin/bulb.vert","./shader/vulkan/bin/bulb.frag",
			TARGET_RESOLUTION_FULLSCREEN,RENDER_STAGE_GEOMETRY_PASS,SPF_DEFAULT);
	lptr<ShaderPipeline> __AnimationPipeline = g_Renderer.register_pipeline(
			"./shader/vulkan/bin/animation.vert","./shader/vulkan/bin/gpass.frag",
			TARGET_RESOLUTION_FULLSCREEN,RENDER_STAGE_GEOMETRY_PASS,SPF_DEFAULT);
	lptr<ShaderPipeline> __BulbShadowPipeline = g_Renderer.register_pipeline(
			"./shader/vulkan/bin/bulb_shadow.vert","./shader/vulkan/bin/shadow.frag",
			TARGET_RESOLUTION_FULLSCREEN,RENDER_STAGE_SHADOW_PROJECTION,SPF_FLIPPED);
	lptr<ShaderPipeline> __AnimationShadowPipeline = g_Renderer.register_pipeline(
			"./shader/vulkan/bin/animation_shadow.vert","./shader/vulkan/bin/shadow.frag",
			TARGET_RESOLUTION_FULLSCREEN,RENDER_STAGE_SHADOW_PROJECTION,SPF_FLIPPED);
	lptr<ShaderPipeline> __CubemapPipeline = g_Renderer.register_pipeline(
			"./shader/vulkan/bin/cubemap_hdr.vert","./shader/vulkan/bin/skybox.frag",
			TARGET_RESOLUTION_FULLSCREEN,RENDER_STAGE_FORWARD_DRAW,SPF_FLIPPED);
	// TODO find out what else in the signature is obsolete

	// batches
	lptr<GeometryBatch> __EnviroBatch = g_Renderer.register_deferred_geometry_batch(4);
	lptr<ParticleBatch> __BulbBatch = g_Renderer.register_deferred_particle_batch(__BulbPipeline);
	m_AnimBatch = g_Renderer.register_deferred_geometry_batch(__AnimationPipeline,1);
	lptr<GeometryBatch> __CubemapBatch = g_Renderer.register_geometry_batch(__CubemapPipeline,1);

	// setup geometry batch
	GeometryTuple* __Floor = __EnviroBatch->add_geometry(Mesh::CUBE);
	GeometryTuple* __Box15 = __EnviroBatch->add_geometry(Mesh::CUBE);
	GeometryTuple* __Box16 = __EnviroBatch->add_geometry(Mesh::CUBE);
	GeometryTuple* __Sphere = __EnviroBatch->add_geometry(Mesh::SPHERE);
	__EnviroBatch->load();

	// setup animated character batch
	m_Dude = m_AnimBatch->add_geometry(m_DudeMesh);
	m_AnimBatch->load();
	g_Frame.time_factor = 2.f;  // FIXME yes, this is bad
	m_DudeMesh.set_default_animation(3,.4f);
	m_PlayerMomentum.target = m_PlayerPosition;
	g_Renderer.animate(&m_DudeMesh);
	m_SpineJointTransform = &m_DudeMesh.find_joint("metarig_spine")->local_transform;

	// cubemap
	GeometryTuple* __Cubemap = __CubemapBatch->add_geometry(Mesh::CUBEMAP_CUBE,CUBEMAP_CUBE_VERTEX_LENGTH,
															CUBEMAP_CUBE_FLOAT_STRIDE*sizeof(f32));
	__CubemapBatch->load();

	// transform objects
	__Floor->transform.scale(vec3(10,10,.1f));
	__Floor->transform.translate(vec3(0,0,-1));
	__Floor->object_uniform_data.texel = 10.f;
	__Floor->object_uniform_data.texel = 10.f;
	__Box15->transform.transform(vec3(4,4,.75f),vec3(.75f),vec3(.0f));
	__Box16->transform.transform(vec3(-4,4,1),vec3(.5f),vec3(.0f));
	__Box16->object_uniform_data.texel = 4.f;
	__Sphere->transform.transform(vec3(0,-4,.5f),vec3(.5f),vec3(.0f));

	// materials
	Material* __FabricMaterial = g_Renderer.register_material(
			g_Renderer.register_texture("./res/test/fabric_colour.png",TEXTURE_FORMAT_SRGB),
			g_Renderer.register_texture("./res/test/fabric_normal.png"),
			g_Renderer.register_texture("./res/test/fabric_material.png")
		);
	Material* __FloorMaterial = g_Renderer.register_material(
			g_Renderer.register_texture("./res/test/floor_colour.png",TEXTURE_FORMAT_SRGB),
			g_Renderer.register_texture("./res/test/floor_normal.png"),
			g_Renderer.register_texture("./res/test/floor_material.png")
		);
	Material* __GoldMaterial = g_Renderer.register_material(
			g_Renderer.register_texture("./res/test/gold_colour.png",TEXTURE_FORMAT_SRGB),
			g_Renderer.register_texture("./res/test/gold_normal.png"),
			g_Renderer.register_texture("./res/test/gold_material.png")
		);
	Material* __DudeMaterial = g_Renderer.register_material(
			g_Renderer.register_texture("./res/private/texmex.jpg",TEXTURE_FORMAT_SRGB)
		);

	// cubemap
	//Texture* __CubemapTexture = g_Renderer.register_cubemap_hdr("./res/ibl/russia.hdr");
	Cubemap* __CubemapTexture = g_Renderer.register_cubemap_classical("./res/cm/yokohama",TEXTURE_FORMAT_SRGB);
	__Cubemap->ubo.attach_texture(RENDERER_CUBEMAP_DESCRIPTOR_CHANNEL,&__CubemapTexture->cubemap);

	// lightbulbs
	__BulbBatch->load(Mesh::SPHERE,TEST_LIGHTBULB_COUNT,sizeof(BallIndex));
	BallIndex* __BulbIndices = (BallIndex*)__BulbBatch->imem;
	__BulbIndices[0] = { vec3(9,9,.2f),.2f,vec3(1,0,0),vec2(0,.4f) };
	__BulbIndices[1] = { vec3(9,-9,.7f),.2f,vec3(1,1,0),vec2(0,.4f) };
	__BulbIndices[2] = { vec3(-9,9,1.2f),.2f,vec3(0,1,0),vec2(0,.4f) };
	__BulbIndices[3] = { vec3(-9,-9,.2f),.2f,vec3(0,1,1),vec2(0,.4f) };
	__BulbIndices[4] = { vec3(5,9,.7f),.2f,vec3(0,0,1),vec2(0,.4f) };
	__BulbIndices[5] = { vec3(5,-9,1.2f),.2f,vec3(1,0,1),vec2(0,.4f) };
	__BulbIndices[6] = { vec3(-9,5,.2f),.2f,vec3(.5f,.5f,0),vec2(0,.4f) };
	__BulbIndices[7] = { vec3(-9,-5,.7f),.2f,vec3(0,.5f,.5f),vec2(0,.4f) };
	__BulbBatch->update_instances();

	// setup lighting
	g_Renderer.add_sunlight(vec3(75,-150,100),vec3(1.f),.5f,true);
	for (u8 i=0;i<TEST_LIGHTBULB_COUNT;i++)
		g_Renderer.add_pointlight(__BulbIndices[i].position,__BulbIndices[i].colour,7.5f,1.f,.8f,.24f);

	// register textures
	__Floor->ubo.attach_material(__FloorMaterial);
	__Box15->ubo.attach_material(__FloorMaterial);
	__Box16->ubo.attach_material(__GoldMaterial);
	__Sphere->ubo.attach_material(__FabricMaterial);
	m_Dude->ubo.attach_material(__DudeMaterial);

	// shadows
	g_Renderer.register_shadow_batch(__EnviroBatch);
	g_Renderer.register_shadow_batch(__BulbBatch,__BulbShadowPipeline);
	g_Renderer.register_shadow_batch(m_AnimBatch,__AnimationShadowPipeline);

	g_Wheel.call(this);
}

/**
 *	process movement state controller & focus camera on character
 */
void ParcourParcs::update()
{
	vec3 __CenteredPosition = vec3((*m_SpineJointTransform)[3]);
	vec3 __Attitude = glm::normalize(vec3(g_Camera.target.x-g_Camera.position.x,
										  g_Camera.target.y-g_Camera.position.y,
										  0));
	vec3 __OrthoAttitude = vec3(-__Attitude.y,__Attitude.x,0);

	// player state
	switch (m_MoveState)
	{
	case MOVE_JUMPING:
		// jumping movement
		m_PosDelta = vec3(m_MoveDirection.x,m_MoveDirection.y,0)
				*vec3(m_DudeMesh.get_progress()*(m_DudeMesh.get_progress()<.9f)*TEST_JUMP_SPEED);
		m_PosDelta.z = glm::sin((m_DudeMesh.get_progress()-.5f)*2.5f*MATH_PI*2)
				*(m_DudeMesh.get_progress()<.9f&&m_DudeMesh.get_progress()>.5f)*TEST_JUMP_HEIGHT;

		// state machine flow
		if (m_DudeMesh.current_animation!=DANIM_JUMPING) m_MoveState = MOVE_STANDARD;
		break;

	case MOVE_ROLLING:
		// rolling movement
		m_PosDelta = vec3(m_MoveDirection.x,m_MoveDirection.y,0)
				*vec3((m_DudeMesh.get_progress()<.65f&&m_DudeMesh.get_progress()>.25f)*TEST_ROLL_SPEED);

		// state machine flow
		if (m_DudeMesh.current_animation!=DANIM_ROLLING) m_MoveState = MOVE_STANDARD;
		break;

	case MOVE_CELEBRATING:
		if (m_DudeMesh.current_animation!=DANIM_CELEBRATE) m_MoveState = MOVE_STANDARD;
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
		m_DudeMesh.current_animation = 3-(glm::length(m_PosDelta)>.01f);
		m_MoveDirection = m_PosDelta;

		// state machine flow
		if (g_Input.keyboard.keys[SDL_SCANCODE_SPACE])
		{
			m_MoveState = MOVE_JUMPING;
			m_DudeMesh.set_animation(DANIM_JUMPING,.5f);
		}
		else if (g_Input.keyboard.keys[SDL_SCANCODE_LSHIFT])
		{
			m_MoveState = MOVE_ROLLING;
			m_DudeMesh.set_animation(DANIM_ROLLING,.5f);
		}
		else if (g_Input.keyboard.keys[SDL_SCANCODE_E])
		{
			m_MoveState = MOVE_CELEBRATING;
			m_PosDelta = vec3(.0f);
			m_DudeMesh.set_animation(DANIM_CELEBRATE,.5f);
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

	// player transformation
	m_Dude->transform.reset();
	m_Dude->transform.rotate_x(90.f);
	m_Dude->transform.translate(-__CenteredPosition);
	m_Dude->object_uniform_data.model = glm::translate(mat4(1.f),m_PlayerPosition)
		* glm::rotate(mat4(1.f),m_PlayerRotation,vec3(0,0,1))
		* m_Dude->object_uniform_data.model;

	// camera
	g_Camera.target = m_PlayerPosition;
}

/**
 *	unload parcour test scene
 */
void ParcourParcs::vanish()
{
}
