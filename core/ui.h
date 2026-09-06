#ifndef SYSTEM_UI_HEADER
#define SYSTEM_UI_HEADER


#include "input.h"
#include "renderer.h"


constexpr f32 UI_TEXT_DOWNSCALE = .6f;
constexpr f32 UI_TEXT_BORDER_X = .95f;
constexpr f32 UI_TEXT_BORDER_Y = .4f;
constexpr f32 UI_CURSOR_BLINK_DELTA = .04f;
constexpr f32 UI_DEPTH_OFFSET = .01f;


struct Button
{
	// utility
	void update();
	void remove();

	// data
	Sprite* canvas;
	Rect* idle;
	Rect* hover;
	Rect* action;
	lptr<Text> label;
	Rect bounds;
	bool holding = false;
	bool confirm = false;
};
// TODO add hover status
// TODO enumerate bitwise, then use this as status component

struct TextField
{
	// utility
	void update(Font* font,Sprite* cursor,bool& field_switch,bool& tynext,bool& cnf_input);
	void remove();

	// data
	Sprite* canvas;
	Rect* idle;
	Rect* hover;
	Rect* select;
	string buffer;
	string buffer_head = "";
	string buffer_tail = "";
	lptr<Text> content;
	Rect bounds;
	s32 cursor_rev = 0;
	bool active = false;
	bool hidden = false;
};
// TODO utilize pointer arithmetics instead of head tail buffer appendix (\0 to end current and point to cursor)

struct UIBatch
{
	// utility
	lptr<Button> add_button(const char* label,Rect* tidle,Rect* thover,Rect* taction,
							vec3 position,vec2 scale,Alignment alignment={});
	lptr<TextField> add_text_field(Rect* tidle,Rect* thover,Rect* tselect,
								   vec3 position,vec2 scale,Alignment alignment={});

	// data
	Font* font;
	f32 alpha;
	list<Button> buttons;
	list<TextField> tfields;
};


class UI
{
public:
	UI(const char* cursor_path);
	void update();

	// registration/deregistration
	lptr<UIBatch> add_batch(Font* font,f32 alpha=1.f);
	void remove_batch(lptr<UIBatch> batch);

private:

	// data
	Font* m_Font;
	list<UIBatch> m_Batches;

	// cursor
	Sprite* m_CursorSprite;
	f32 m_CursorAnim = .0f;
};

inline UI g_UI = UI("./res/ui/cursor.png");


#endif
