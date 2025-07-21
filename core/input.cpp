#include "input.h"


/**
 *	setup input handler
 */
Input::Input()
{
	unset_input_mode();
}

/**
 *	update peripheral state must be called in the main loop, to be able to read user input correctly
 *	\param running: reference to program running state, closing requests will force this to false
 */
void Input::update(bool& running)
{
	mouse.wheel = 0;

	// reset trigger input per frame
	keyboard.triggered_keys.reset();
	mouse.triggered_buttons.reset();
	// FIXME trigger logic utterly broken

	// reset keypress state
	keyboard.key_pressed = false;

	// process peripheral events
	while (SDL_PollEvent(&m_Event))
	{
		switch (m_Event.type)
		{
			// keyboard input
		case SDL_KEYDOWN:
			keyboard.key_pressed = true;

			// action input
			keyboard.keys.set(m_Event.key.keysym.scancode);
			keyboard.triggered_keys.set(m_Event.key.keysym.scancode);

			// text input
			if (!SDL_IsTextInputActive()) break;
			if (m_Event.key.keysym.scancode==SDL_SCANCODE_BACKSPACE&&!m_TextBuffer->empty())
				m_TextBuffer->pop_back();
			break;
		case SDL_KEYUP: keyboard.keys.unset(m_Event.key.keysym.scancode);
			break;
		case SDL_TEXTINPUT:
			(*m_TextBuffer) += m_Event.text.text;

			// mouse input
		case SDL_MOUSEMOTION:
			SDL_GetMouseState(&mouse.apos_x,&mouse.apos_y);
			mouse.position = vec2(FRAME_RESOLUTION_X_INV*mouse.apos_x,FRAME_RESOLUTION_Y_INV*mouse.apos_y);
			mouse.position *= vec2(MATH_CARTESIAN_XRANGE,MATH_CARTESIAN_YRANGE);
			mouse.position.y = MATH_CARTESIAN_YRANGE-mouse.position.y;
			break;
		case SDL_MOUSEBUTTONDOWN:
			mouse.buttons.set(m_Event.button.button-1);
			mouse.triggered_buttons.set(m_Event.button.button-1);
			break;
		case SDL_MOUSEBUTTONUP: mouse.buttons.unset(m_Event.button.button-1);
			break;
		case SDL_MOUSEWHEEL: mouse.wheel = m_Event.wheel.y;
			break;

			// system closing request
		case SDL_QUIT: running = false;
			break;
		};
	}

	// calculate mouse delta velocity
	mouse.velocity = mouse.position-mouse.last_position;
	mouse.last_position = mouse.position;
}

/**
 *	enable text input mode
 *	\param buffer: buffer memory pointer to write text input to
 */
void Input::set_input_mode(string* buffer)
{
	m_TextBuffer = buffer;
	SDL_StartTextInput();
}
