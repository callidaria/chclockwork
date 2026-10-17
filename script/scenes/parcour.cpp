#include "parcour.h"


/**
 *	TODO
 */
void ParcourParcs::init()
{
	// pipelines
	lptr<ShaderPipeline> __BulbPipeline = g_Renderer.register_pipeline(
			g_Renderer.fd_fullscreen,"./shader/vulkan/bin/bulb.vert","./shader/vulkan/bin/bulb.frag",
			gformats,RENDERER_GBUFFER_FORMAT_SIZE,true
		);
	lptr<ShaderPipeline> __AnimationPipeline = g_Renderer.register_pipeline(
			g_Renderer.fd_fullscreen,"./shader/vulkan/bin/animation.vert","./shader/vulkan/bin/gpass.frag",
			gformats,RENDERER_GBUFFER_FORMAT_SIZE,true
		);
	lptr<ShaderPipeline> __BulbShadowPipeline = g_Renderer.register_pipeline(
			g_Renderer.fd_fullscreen,"./shader/vulkan/bin/bulb_shadow.vert","./shader/vulkan/bin/shadow.frag",
			nullptr,0,true,true
		);
	lptr<ShaderPipeline> __AnimationShadowPipeline = g_Renderer.register_pipeline(
			g_Renderer.fd_fullscreen,
			"./shader/vulkan/bin/animation_shadow.vert","./shader/vulkan/bin/shadow.frag",
			nullptr,0,true,true
		);
	// TODO maybe solve the last parameters by type (TYPE_SHADOW,TYPE_GPASS,TYPE_FORWARD, et cetera)
	// TODO namespace gformats by adding the constant inside the renderer class

	// batches
	lptr<GeometryBatch> __EnviroBatch = g_Renderer.register_deferred_geometry_batch();
	lptr<ParticleBatch> __BulbBatch = g_Renderer.register_deferred_particle_batch(__BulbPipeline);
	m_AnimBatch = g_Renderer.register_deferred_geometry_batch(__AnimationPipeline);

	// setup geometry batch
	vector<Texture*> __Textures = {  };
	Mesh __Sphere = Mesh::sphere();
	Mesh __Cube = Mesh::cube();
	u32 __FloorID = __EnviroBatch->add_geometry(__Cube,__Textures);
	u32 __BoxID = __EnviroBatch->add_geometry(__Cube,__Textures);
	u32 __BoxID0 = __EnviroBatch->add_geometry(__Cube,__Textures);
	u32 __SphereID = __EnviroBatch->add_geometry(__Sphere,__Textures);
	__EnviroBatch->load();
	// FIXME remove textures as they are passed right now

	// setup animated character batch
	m_DudeID = m_AnimBatch->add_geometry(m_Dude,__Textures);
	m_AnimBatch->load();
	g_Frame.time_factor = 2.f;  // FIXME yes, this is bad
	m_Dude.set_default_animation(3,.4f);
	m_PlayerMomentum.target = m_PlayerPosition;
	g_Renderer.animate(&m_Dude);
	m_SpineJointTransform = &m_Dude.find_joint("metarig_spine")->local_transform;

	// transform objects
	__EnviroBatch->objects[__FloorID].transform.scale(vec3(10,10,.1f));
	__EnviroBatch->objects[__FloorID].transform.translate(vec3(0,0,-.1f));
	__EnviroBatch->data[__FloorID].texel = 10.f;
	__EnviroBatch->objects[__BoxID].transform.transform(vec3(4,4,.75f),vec3(.75f),vec3(.0f));
	__EnviroBatch->objects[__BoxID0].transform.transform(vec3(-4,4,1),vec3(.5f),vec3(.0f));
	__EnviroBatch->data[__BoxID0].texel = 4.f;
	__EnviroBatch->objects[__SphereID].transform.transform(vec3(0,-4,.5f),vec3(.5f),vec3(.0f));

	// textures
	// fabric
	GPUPixelBuffer* __FabricColourTexture = g_Renderer.register_texture("./res/test/fabric_colour.png",
																		TEXTURE_FORMAT_SRGB);
	GPUPixelBuffer* __FabricNormalTexture = g_Renderer.register_texture("./res/test/fabric_normal.png");
	GPUPixelBuffer* __FabricMaterialTexture = g_Renderer.register_texture("./res/test/fabric_material.png");

	// floor
	GPUPixelBuffer* __FloorColourTexture = g_Renderer.register_texture("./res/test/floor_colour.png",
																	   TEXTURE_FORMAT_SRGB);
	GPUPixelBuffer* __FloorNormalTexture = g_Renderer.register_texture("./res/test/floor_normal.png");
	GPUPixelBuffer* __FloorMaterialTexture = g_Renderer.register_texture("./res/test/floor_material.png");

	// gold
	GPUPixelBuffer* __GoldColourTexture = g_Renderer.register_texture("./res/test/gold_colour.png",
																	  TEXTURE_FORMAT_SRGB);
	GPUPixelBuffer* __GoldNormalTexture = g_Renderer.register_texture("./res/test/gold_normal.png");
	GPUPixelBuffer* __GoldMaterialTexture = g_Renderer.register_texture("./res/test/gold_material.png");

	// player
	GPUPixelBuffer* __DudeTexture = g_Renderer.register_texture("./res/private/texmex.jpg",TEXTURE_FORMAT_SRGB);

	// standards
	GPUPixelBuffer* __NeutralNormalMap = g_Renderer.register_texture("./res/standard/normal.png");
	GPUPixelBuffer* __NeutralMaterialMap = g_Renderer.register_texture("./res/standard/material.png");
	GPUPixelBuffer* __NeutralEmissionTexture = g_Renderer.register_texture("./res/standard/none.png");

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
	__BulbBatch->load(__Sphere,TEST_LIGHTBULB_COUNT,sizeof(BallIndex));
	__BulbBatch->ibo.upload(__BulbIndices,sizeof(__BulbIndices));

	// setup lighting
	g_Renderer.add_sunlight(vec3(75,-150,100),vec3(1.f),.5f,true);
	for (u8 i=0;i<TEST_LIGHTBULB_COUNT;i++)
		g_Renderer.add_pointlight(__BulbIndices[i].position,__BulbIndices[i].colour,7.5f,1.f,.8f,.24f);

	// register textures
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__FloorID][1],0,__FloorColourTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__FloorID][1],1,__FloorNormalTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__FloorID][1],2,__FloorMaterialTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__FloorID][1],3,__NeutralEmissionTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__BoxID][1],0,__FloorColourTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__BoxID][1],1,__FloorNormalTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__BoxID][1],2,__FloorMaterialTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__BoxID][1],3,__NeutralEmissionTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__BoxID0][1],0,__GoldColourTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__BoxID0][1],1,__GoldNormalTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__BoxID0][1],2,__GoldMaterialTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__BoxID0][1],3,__NeutralEmissionTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__SphereID][1],0,__FabricColourTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__SphereID][1],1,__FabricNormalTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__SphereID][1],2,__FabricMaterialTexture);
	g_Renderer.attach_texture(&__EnviroBatch->ubo[__SphereID][1],3,__NeutralEmissionTexture);
	g_Renderer.attach_texture(&m_AnimBatch->ubo[m_DudeID][1],0,__DudeTexture);
	g_Renderer.attach_texture(&m_AnimBatch->ubo[m_DudeID][1],1,__NeutralNormalMap);
	g_Renderer.attach_texture(&m_AnimBatch->ubo[m_DudeID][1],2,__NeutralMaterialMap);
	g_Renderer.attach_texture(&m_AnimBatch->ubo[m_DudeID][1],3,__NeutralEmissionTexture);

	// shadows
	g_Renderer.register_shadow_batch(__EnviroBatch);
	g_Renderer.register_shadow_batch(__BulbBatch,__BulbShadowPipeline);
	g_Renderer.register_shadow_batch(m_AnimBatch,__AnimationShadowPipeline);

	g_Wheel.call(this);
}

/**
 *	TODO
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

	// player transformation
	m_AnimBatch->objects[m_DudeID].transform.reset();
	m_AnimBatch->objects[m_DudeID].transform.rotate_x(90.f);
	m_AnimBatch->objects[m_DudeID].transform.translate(-__CenteredPosition);
	m_AnimBatch->objects[m_DudeID].transform.model =
			glm::translate(mat4(1.f),m_PlayerPosition)
			* glm::rotate(mat4(1.f),m_PlayerRotation,vec3(0,0,1))
			* m_AnimBatch->objects[m_DudeID].transform.model;

	// camera
	g_Camera.target = m_PlayerPosition;
}

/**
 *	TODO
 */
void ParcourParcs::vanish()
{
	// TODO
}
