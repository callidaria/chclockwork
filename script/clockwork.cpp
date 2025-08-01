#include "clockwork.h"
#ifdef DEBUG


/**
 *	setup origin camera projection
 */
Clockwork::Clockwork(Font* font)
{
	// camera setup
	m_TargetingVector = vec3(g_Camera.yaw,g_Camera.pitch,g_Camera.distance);
	m_CameraRotation.target = m_TargetingVector;

	// fps display
	m_FPS = g_Renderer.write_text(font,"",vec3(-10,-10,0),15,vec4(1),Alignment{ .align=SCREEN_ALIGN_TOPRIGHT });

	g_Wheel.call(UpdateRoutine{ &Clockwork::_update,(void*)this });
}

/**
 *	update camera math based on input
 */
void Clockwork::update()
{
	// update camera position
	/*
	vec3 __Attitude = glm::normalize(vec3(g_Camera.target.x-g_Camera.position.x,
										  g_Camera.target.y-g_Camera.position.y,0));
	vec3 __OrthoAttitude = vec3(-__Attitude.y,__Attitude.x,0);
	m_CameraPosition.target += (f32(g_Input.keyboard.keys[SDL_SCANCODE_W]
							  - g_Input.keyboard.keys[SDL_SCANCODE_S])*__Attitude
						 + f32(g_Input.keyboard.keys[SDL_SCANCODE_A]
								- g_Input.keyboard.keys[SDL_SCANCODE_D])*__OrthoAttitude)
			* CLOCKWORK_MVMT_ACCELERATION*g_Frame.delta_time_real;
	m_CameraPosition.update(g_Camera.target,g_Frame.delta_time_real);
	*/

	// rotation & zoom
	vec2 __Rotation = vec2(g_Input.mouse.buttons[1]*CLOCKWORK_ROTATION_MOUSEACC)*g_Input.mouse.velocity;
	m_CameraRotation.target += vec3(__Rotation.x,__Rotation.y,0);
	m_CameraRotation.target.z += g_Input.mouse.wheel*CLOCKWORK_ZOOM_ACCELLERATION;

	// update & extract
	m_CameraRotation.update(m_TargetingVector,g_Frame.delta_time_real);
	g_Camera.yaw = glm::radians(-m_TargetingVector.x);
	g_Camera.pitch = glm::radians(m_TargetingVector.y);
	g_Camera.distance = m_TargetingVector.z;

	// fps display
	m_FPS->data = "FPS "+std::to_string(g_Frame.fps);
	m_FPS->align();
	m_FPS->load_buffer();
}


#endif
