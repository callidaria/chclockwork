#ifndef CORE_INPUT_HEADER
#define CORE_INPUT_HEADER


#include "base.h"


constexpr u16 KEYBOARD_INPUT_RANGE = 512;
constexpr u8 MOUSE_INPUT_RANGE = 5;


struct Keyboard
{
	BitwiseWords keys = BitwiseWords(KEYBOARD_INPUT_RANGE);
	BitwiseWords triggered_keys = BitwiseWords(KEYBOARD_INPUT_RANGE);
	bool key_pressed = false;
};

struct Mouse
{
	s32 apos_x,apos_y;
	vec2 last_position = vec2(0);
	vec2 position;
	vec2 velocity;
	s32 wheel;
	BitwiseWords buttons = BitwiseWords(MOUSE_INPUT_RANGE);
	BitwiseWords triggered_buttons = BitwiseWords(MOUSE_INPUT_RANGE);
};


class Input
{
public:
	Input();
	void update(bool& running);

	// text input
	void set_input_mode(string* buffer);
	static inline void unset_input_mode() { SDL_StopTextInput(); }

public:
	static inline Keyboard keyboard;
	static inline Mouse mouse;

private:
	SDL_Event m_Event;
	string* m_TextBuffer;
};

inline Input g_Input;


#endif
