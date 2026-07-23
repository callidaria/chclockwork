#include "clockwork.h"
#ifdef DEBUG


/**
 *	setup origin camera projection
 */
Clockwork::Clockwork(Font* font)
{
	// camera setup
	m_TargetingVector = vec3(g_Camera.yaw,g_Camera.pitch+25.f,g_Camera.distance);
	m_CameraRotation.target = m_TargetingVector;

	// fps display
	m_FPS = g_Renderer.write_text(font,"",vec3(-10,-10,7),15,vec4(1),
								  Alignment{ .alignment=SCREEN_ALIGN_TOPRIGHT });

	g_Wheel.call(this);
}

/**
 *	update camera math based on input
 */
void Clockwork::update()
{
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
