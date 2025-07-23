#include "clockwork.h"


/**
 *	setup origin camera projection
 */
Clockwork::Clockwork(Font* font)
{
	m_FPS = g_Renderer.write_text(font,"",vec3(-10,-10,0),15,vec4(1),Alignment{ .align=SCREEN_ALIGN_TOPRIGHT });
	g_Wheel.call(UpdateRoutine{ &Clockwork::_update,(void*)this });
}

/**
 *	update camera math based on input
 */
void Clockwork::update()
{
	// zoom input
	m_ZoomMomentum += g_Input.mouse.wheel*CLOCKWORK_ZOOM_ACCELLERATION;
	f32 __Pred = g_Camera.distance+m_ZoomMomentum;

	// camera rotational orbit
	m_RotMomentum.x += (g_Input.keyboard.keys[SDL_SCANCODE_E]-g_Input.keyboard.keys[SDL_SCANCODE_Q])
			*CLOCKWORK_ROT_KEYACC;
	m_RotMomentum += vec2(g_Input.mouse.buttons[1]*CLOCKWORK_ROT_MOUSEACC)*g_Input.mouse.velocity;
	g_Camera.yaw += glm::radians(-m_RotMomentum.x);
	g_Camera.pitch += glm::radians(m_RotMomentum.y);

	// update camera position
	vec3 __Attitude = glm::normalize(vec3(g_Camera.target.x-g_Camera.position.x,
										  g_Camera.target.y-g_Camera.position.y,0));
	vec3 __OrthoAttitude = vec3(-__Attitude.y,__Attitude.x,0);
	m_CameraMomentum += (vec3(g_Input.keyboard.keys[SDL_SCANCODE_W]
							  - g_Input.keyboard.keys[SDL_SCANCODE_S])*__Attitude
						 + vec3(g_Input.keyboard.keys[SDL_SCANCODE_A]
							   - g_Input.keyboard.keys[SDL_SCANCODE_D])*__OrthoAttitude)
			* CLOCKWORK_MVMT_ACCEL;
	g_Camera.target += m_CameraMomentum;
	g_Camera.distance += m_ZoomMomentum;
	m_CameraMomentum *= CLOCKWORK_MVMT_FLOATFACTOR;

	// haptic attenuation
	m_ZoomMomentum *= CLOCKWORK_ZOOM_FLOATFACTOR;
	m_RotMomentum *= CLOCKWORK_ROT_FLOATFACTOR;

	// fps display
#ifdef DEBUG
	m_FPS->data = "FPS "+std::to_string(g_Frame.fps);
	m_FPS->align();
	m_FPS->load_buffer();
#endif
}
