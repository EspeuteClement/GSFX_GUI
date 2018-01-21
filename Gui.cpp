#include "Gui.h"
#include <Gamebuino-Meta.h>

Gamebuino_Meta::Sound_FX sfx_test[8] =
{
	{ Gamebuino_Meta::Sound_FX_Wave::SQUARE_CONTINUE, 19968,0,8192,0,1000 },
	{ Gamebuino_Meta::Sound_FX_Wave::SQUARE, 19968,-5,4096,0,10000 },
};

struct value {
	int32_t def;
	int32_t min;
	int32_t max;
};

const value _params_values[][2] =
{
	{ // Volume
		{ 19968 , 0, 19968},
		{ 19968 , 0, 19968 },
	},
	{ // Volume_env
		{ 0 , -64, 64 },
		{ 0 , -64, 64 },
	},
	{ // Period
		{ 0, 0, 32 << 8},
		{ 8192, 0, 20000},
	},
	{ // Period_env
		{ 0 , -64, 64 },
		{ 0 , -64, 64 },
	},
};

constexpr const value & getParamValue(const Gui::UI param, const Gamebuino_Meta::Sound_FX_Wave wave)
{
	return _params_values[(uint32_t) param - (uint32_t) Gui::UI::PARAM_VOLUME][(uint32_t)wave & 0xff];
}

//#define BENCH

const char * Gui::_param_strings[] =
{
	"WAVE",
	"VOLUME",
	"-ENV",
	"PERIOD",
	"-ENV",
	"LENGTH"
};

const char * _wave_names[] =
{
	"noise",
	"square",
};

Gui::Gui()
{
}


Gui::~Gui()
{
}

void Gui::Begin()
{
	_current_index = (uint32_t)UI::PARAM_VOLUME;
}

void Gui::Update()
{

	if (gb.buttons.pressed(BUTTON_A))
	{
		gb.sound.fx(sfx_test);
	}
#ifndef BENCH

	// debug move cursor
	if (gb.buttons.pressed(BUTTON_UP))
	{
		_current_index--;
	}
	if (gb.buttons.pressed(BUTTON_DOWN))
	{
		_current_index++;
	}

	if (_current_index < (uint32_t)UI::PARAMS_BEGIN)
	{
		_current_index = (uint32_t)UI::PARAMS_END - 1;
	}

	if (_current_index >(uint32_t)UI::PARAMS_END - 1)
	{
		_current_index = (uint32_t)UI::PARAMS_BEGIN;
	}
#endif // !BENCH

}

void Gui::DrawParamEntry(uint8_t index)
{
	bool selected = index == (_current_index - (uint32_t)UI::PARAMS_BEGIN);

	static const int OFFSET_X = 6;
	static const int OFFSET_Y = 8;
	static const int STEP_Y = 6;
	static const int OFFSET_COLON_X = 30;
	static const int RECT_X = 1;
	static const int RECT_W = 78;
	static const int RECT_Y = -1;
	static const int RECT_H = 7;



	const int x = OFFSET_X;
	const int y = index * STEP_Y + OFFSET_Y;
	static const int BAR_X = 34;

	gb.display.setCursor(x, y);

	if (selected)
	{
		gb.display.setColor(BLACK);
		gb.display.fillRect(RECT_X, y + RECT_Y, RECT_W, RECT_H);
		gb.display.setColor(WHITE);
	}
	else
	{
		gb.display.setColor(GRAY);
	}
	
	gb.display.print(_param_strings[index]);

	gb.display.setCursor(OFFSET_COLON_X, y);
	gb.display.print(":");

	// Draw the bar background

	UI ui_param = (UI)(index + (uint32_t)UI::PARAMS_BEGIN);
	switch (ui_param)
	{
	case Gui::UI::PARAM_WAVE:
		{
			gb.display.setCursor(BAR_X, y);
			int id = (uint32_t)currentFx()->type & ~(uint32_t)Gamebuino_Meta::Sound_FX_Wave::CONTINUE_FLAG;
			gb.display.print(_wave_names[id]);
			break;
		}
	case Gui::UI::PARAM_LENGTH:
		gb.display.setCursor(BAR_X, y);
		gb.display.printf("%6d ms", 100);
		break;
	case Gui::UI::PARAM_VOLUME:
	case Gui::UI::PARAM_VOLUME_ENV:
	case Gui::UI::PARAM_PERIOD:
	case Gui::UI::PARAM_PERIOD_ENV:
		// Draw bar
		DrawBar(BAR_X, y, sfx_test[_current_fx].params[index] , getParamValue(ui_param, Gamebuino_Meta::Sound_FX_Wave::SQUARE).min, getParamValue(ui_param, Gamebuino_Meta::Sound_FX_Wave::SQUARE).max, selected);
		break;
	default:
		// Shouldn't happend
		break;
	}
}

void Gui::DrawBar(int x, int y, int32_t value, int32_t min, int32_t max, bool selected)
{
	static const int	BAR_X = x;
	const int			BAR_Y = y + 1;
	static const int	BAR_W = 44;
	static const int	BAR_H = 3;
	if (selected)
	{
		gb.display.setColor(DARKGRAY);
	}
	else
	{
		gb.display.setColor(GRAY);
	}
	gb.display.fillRect(BAR_X, BAR_Y, BAR_W, BAR_H);

	// get cursor position
	const int32_t cursor = (BAR_W * (value - min)) / (max - min);

	if (selected)
	{
		gb.display.setColor(WHITE);
	}
	else
	{
		gb.display.setColor(BLACK);
	}

	gb.display.drawFastVLine(x + cursor, BAR_Y, BAR_H);

	
}

void Gui::Draw()
{
#ifndef BENCH
	gb.display.clear(DARKGRAY);
	

	// Draw Middle Interface
	for (int index = 0; index < NUM_PARAMS; index++)
	{
		DrawParamEntry(index);
	}


	gb.display.setColor(GRAY);
	gb.display.fillRect(0, 0, 80, 7);
	gb.display.drawRect(0, 0, 80, 57);

	// Draw Middle Inferface

	gb.display.setColor(DARKGRAY);
	gb.display.setCursor(1, 1);
	gb.display.printf("file|edit");

	// Draw Status Bar
	DrawStatusBar();

#else
	gb.display.clear();
	gb.display.printf("%d", gb.getCpuLoad());
#endif
}

void Gui::DrawStatusBar()
{
	const int VALUE_X = 47;
	const int VALUE_Y = 58;
	if (_current_index >= (uint32_t)Gui::UI::MENU_BEGIN && _current_index < (uint32_t)Gui::UI::MENU_END)
	{

	}
	else if (_current_index < (uint32_t)Gui::UI::PARAMS_END)
	{
		int index = _current_index - (uint32_t)(Gui::UI::PARAMS_BEGIN);
		gb.display.setColor(GRAY);
		gb.display.setCursor(VALUE_X, VALUE_Y);
		gb.display.printf("v:%6d", sfx_test[_current_fx].params[index]);
	}
	else if (_current_index < (uint32_t)Gui::UI::SELECT_END)
	{

	}
}
