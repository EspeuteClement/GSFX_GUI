#include "Gui.h"
#include <Gamebuino-Meta.h>

#define assert(condition, reason) \
if (!(condition)) \
{ \
__assert(__func__, __FILE__, __LINE__, #condition, reason);   \
} \

void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp, const char * reason) {
	// transmit diagnostic informations through serial link. 
	if (SerialUSB)
	{
		SerialUSB.println("=== ASSERTION ERROR ===");
		SerialUSB.println(__func);
		SerialUSB.println(__file);
		SerialUSB.println(__lineno, DEC);
		SerialUSB.println(__sexp);
		gb.display.println(reason);
		SerialUSB.flush();
	}

	// abort program execution.
	//
	while (1)
	{
		if (gb.update())
		{
			gb.display.clear();
			gb.display.setColor(RED);
			gb.display.println("Assert error:");
			gb.display.println(__func);
			gb.display.println(__file);
			gb.display.println(__lineno, DEC);
			gb.display.println(__sexp);
			gb.display.println(reason);
		}
	}

}

template <typename T> T clamp(T value, T min, T max)
{
	return value > min ? (value < max ? value : max) : min;
}

const uint32_t note_table[] =
{
	690453,651674,615103,580591,548012,517231,488220,460819,434951,410531,387493,365738,345216,325837,307551,290288,273999,258615,244105,230405,217472,205265,193746,182872,172608,162918,153776,145144,136998,129309,122051,115201,108736,102633,96872,91435,86303,81460,76888,72573,68499,64655,61026,57601,54368,51316,48436,45718,43152,40730,38444,36286,34250,32327,30513,28800,27184,25658,24218,22859,21576,20365,19222,18143,17125,16164,15256,14400,13592,12829,12109,11429,10788,10182,9611,9072,8562,8082,7628,7200,6796,6415,6055,5715,5394,5091,4805,4536,4281,4041,3814,3600,3398,3207,3027,2857,2697,2546,2403,2268,2141,2020,1907,1800,1699,1604,1514,1429,1348,1273,1201,1134,1070,1010,954,900,849,802,757,714,
};

Gamebuino_Meta::Sound_FX sfx_test[8] =
{
	{ Gamebuino_Meta::Sound_FX_Wave::SQUARE_CONTINUE, 0xFF << 16,0,22859 *256,0,1000 },
	{ Gamebuino_Meta::Sound_FX_Wave::SQUARE, 0xFF << 16,0,(22859 *256)>>1,0,10000 },
	{ Gamebuino_Meta::Sound_FX_Wave::SQUARE_CONTINUE, 0xFF << 16,0,22859*256,0,1000 },
};

struct value {
	int32_t def;
	int32_t min;
	int32_t max;
	int32_t incr;
};

const uint8_t FPP = Gamebuino_Meta::Sound_Handler_FX::FPP;
const value _params_values[][2] =
{
	{ // Volume
		{ 0xFF << FPP , 0, 0xFF << FPP, 4 << FPP},
		{ 0xFF << FPP , 0, 0xFF << FPP, 4 << FPP},
	},
	{ // Volume_env
		{ 0 , -1 << (FPP - 2), 1 << (FPP - 2), 64},
		{ 0 , -1 << (FPP - 2), 1 << (FPP - 2), 64},
	},
	{ // Period
		{ 0, 0, 256 << FPP, 16 << FPP},
		{ 11046912, 0, 162918 << 8, 256},
	},
	{ // Period_env
		{ 0 , -1 << (FPP - 2), 1 << (FPP - 2), 64},
		{ 0 , -1 << (FPP - 2), 1 << (FPP - 2), 64},
	},
};

constexpr const value & getParamValue(const Gui::UI param, const Gamebuino_Meta::Sound_FX_Wave wave)
{
	return _params_values[(uint32_t) param - (uint32_t) Gui::UI::PARAM_VOLUME][(uint32_t)wave & 0xff];
}

const char * items_file[] = { "Save","Load","Export", nullptr };
const char * items_edit[] = { "Copy", "Paste", "Clear", nullptr };
const char ** items[] = { &(items_file[0]), &(items_edit[0]) };

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

const char *_output_wave_names[] =
{
	"Gamebuino_Meta::Sound_FX_Wave::NOISE",
	"Gamebuino_Meta::Sound_FX_Wave::SQUARE",
	"Gamebuino_Meta::Sound_FX_Wave::NOISE_CONTINUE",
	"Gamebuino_Meta::Sound_FX_Wave::SQUARE_CONTINUE"
};

Gui::Gui()
{
}


Gui::~Gui()
{
}

#define SAVE_VERSION 2
#define FILES_NUMBER 64

struct Save
{
	Gamebuino_Meta::Sound_FX sound_fx[8];
	uint8_t version;
};

/*const SaveDefault savefileDefaults[]{
	{ 0, SAVETYPE_BLOB, 0, sizeof(Save) },
	{ 0, SAVETYPE_BLOB, 0, sizeof(Save) },
	{ 0, SAVETYPE_BLOB, 0, sizeof(Save) }
};*/

void Gui::Begin()
{
	//gb.save.config(savefileDefaults);
	//gb.save.get(0, (char*) &theSave, sizeof(Save));
	//if (theSave.version != SAVE_VERSION)
	//{
	//	memset(&theSave, 0, sizeof(Save));
	//	theSave.version = SAVE_VERSION;
	//}
	_current_save_id = 0;
	Load(_current_save_id);
	//memset(_work_sfx, 0, sizeof(_work_sfx));
	_current_index = (uint32_t)UI::PARAM_VOLUME;
	_select_fx_index = 0;	
	_current_note = 58;

	_state_id = -1;
	PushState(State::PARAMS);

	_notification_timer = 0;
	//SerialUSB.begin(9600);
}

void Gui::SaveCurrentPattern()
{
	Save newSave;
	newSave.version = SAVE_VERSION;
	for (int i = 0; i < 8; i++)
	{
		newSave.sound_fx[i] = _work_sfx_ram[i];
	}


	gb.save.set(_current_save_id, &newSave, sizeof(Save));
}

void Gui::SaveAll()
{
	//gb.save.set(0, (char *) &theSave, sizeof(Save));
}

void Gui::Load(int id)
{
	Save newSave;
	gb.save.get(id, &newSave, sizeof(Save));
	_current_save_id = id;
	if (newSave.version != SAVE_VERSION)
	{
		memset(&newSave, 0, sizeof(Save));
	}

	for (int i = 0; i < 8; i++)
	{
		_work_sfx_ram[i] = newSave.sound_fx[i];
	}
	_pattern_length = FindCurrentPatternLength();
}

uint8_t Gui::FindCurrentPatternLength()
{
	uint8_t r = 0;
	while ((int32_t)_work_sfx_ram[r].type & (int32_t)Gamebuino_Meta::Sound_FX_Wave::CONTINUE_FLAG)
	{
		r++;
	}
	return r+1;
}

uint8_t Gui::FindCurrentToneId()
{
	uint32_t current_tone = currentFx()->period_start;
	int index = 0;
	while (current_tone / 256 < note_table[index]  && index < sizeof(note_table) / sizeof(note_table[0]))
	{
		index++;
	}
	if (index >= sizeof(note_table) / sizeof(note_table[0]))
	{
		return 50;
	}
	return index;
}

void Gui::ResetPattern(int id)
{
	Gamebuino_Meta::Sound_FX * fx = getFx(id);
	Gamebuino_Meta::Sound_FX_Wave wave = fx->type;

	fx->volume_start = getParamValue(UI::PARAM_VOLUME, wave).def;
	fx->volume_sweep = getParamValue(UI::PARAM_VOLUME_ENV, wave).def;
	fx->period_start = getParamValue(UI::PARAM_PERIOD, wave).def;
	fx->period_sweep = getParamValue(UI::PARAM_PERIOD_ENV, wave).def;
	fx->length = 50 * 441;
}

void Gui::FixHeader(int id)
{
	if (id < _pattern_length - 1)
	{
		Gamebuino_Meta::Sound_FX * fx = getFx(id);
		fx->type = (Gamebuino_Meta::Sound_FX_Wave) ((uint32_t) fx->type | (uint32_t) Gamebuino_Meta::Sound_FX_Wave::CONTINUE_FLAG);
	}
	else if (id == _pattern_length - 1)
	{
		Gamebuino_Meta::Sound_FX * fx = getFx(id);
		fx->type = (Gamebuino_Meta::Sound_FX_Wave) ((uint32_t) fx->type & ~(uint32_t) Gamebuino_Meta::Sound_FX_Wave::CONTINUE_FLAG);
	}
}

void Gui::FixAllHeaders()
{
	for (int i = 0; i < _pattern_length; i++)
	{
		FixHeader(i);
	}
}

void Gui::OutputPattern(const Gamebuino_Meta::Sound_FX *fx, const char * name)
{
	if (SerialUSB)
	{
		SerialUSB.print("//Put this in you .h file :\n");
		SerialUSB.print("extern const Gamebuino_Meta::Sound_FX ");
		SerialUSB.print(name);
		SerialUSB.print("[];\n\n");

		SerialUSB.print("//Put this in you .cpp file :\n");
		SerialUSB.print("const Gamebuino_Meta::Sound_FX ");
		SerialUSB.print(name);
		SerialUSB.print("[] = {\n");
		int i = 0;
		const Gamebuino_Meta::Sound_FX * sfx = &fx[i];
		do {
			sfx = &fx[i];
			OutputFX(sfx);
			SerialUSB.print(",\n");
			i++;
		} while ((int32_t)sfx->type & (int32_t)Gamebuino_Meta::Sound_FX_Wave::CONTINUE_FLAG);

		SerialUSB.print("};\n");
	}
}

void Gui::OutputFX(const Gamebuino_Meta::Sound_FX * fx)
{
	if (SerialUSB)
	{
		SerialUSB.print("\t{");
		int id = (int32_t)fx->type & 0xff;
		if ((int32_t)fx->type & (int32_t)Gamebuino_Meta::Sound_FX_Wave::CONTINUE_FLAG)
		{
			id += 2;
		}

		SerialUSB.print(_output_wave_names[id]);
		
		for (int i = 1; i < 6; i++)
		{
			SerialUSB.print(",");
			SerialUSB.print(fx->params[i]);
		}
		SerialUSB.print("}");
	}
}

void Gui::Update()
{
#ifdef BENCH
	gb.lights.clear(RED);
	if (gb.buttons.pressed(BUTTON_B))
	{
		//gb.sound.fx(sfx_test);
		//gb.sound.playOK();
	}
#else
	switch (CurrentState())
	{
	case State::PARAMS:
		UpdateParams();
		break;
	case State::MENU:
		UpdateMenu();
		break;
	default:
		assert(false,"STATE IS NOT IMPLEMENTED");
		break;
	}
	
#endif // !BENCH
}

void Gui::UpdateMenu()
{
	if (gb.buttons.pressed(BUTTON_B) || gb.buttons.pressed(BUTTON_MENU))
	{
		PopState();
	}

	if (gb.buttons.pressed(BUTTON_UP))
	{
		if (_current_menu_item > 0)
		{
			_current_menu_item--;
		}
		else
		{
			while (items[_current_menu][_current_menu_item+1] != nullptr)
			{
				_current_menu_item++;
			}
		}
	}

	if (gb.buttons.pressed(BUTTON_DOWN))
	{
		if (items[_current_menu][_current_menu_item+1] != nullptr)
		{
			_current_menu_item++;
		}
		else
		{
			_current_menu_item = 0;
		}
	}

	if (gb.buttons.pressed(BUTTON_LEFT))
	{
		if (_current_menu > 0)
		{
			_current_menu = _current_menu - 1;
		}
		else
		{
			_current_menu = 1;
		}
		_current_index = 0;
	}

	if (gb.buttons.pressed(BUTTON_RIGHT))
	{
		_current_menu = ((_current_menu + 1) % 2);
		_current_index = 0;
	}

	if (gb.buttons.pressed(BUTTON_A))
	{
		switch ((Menu)_current_menu)
		{
		case Menu::FILE:
			switch (_current_menu_item)
			{
				case 0: // Save
					SaveCurrentPattern();
					SaveAll();
					PopState();
					break;
				case 1: // Load
					Load(_current_save_id);
					PopState();
					break;
				case 2: // Export
					OutputPattern(_work_sfx_ram, "test");
					PopState();
					break;
			}
			break;
		case Menu::EDIT:
			break;
		default:
			break;
		}
	}
}

void Gui::UpdateParams()
{
	static int repeat = 0;

	if (gb.buttons.pressed(BUTTON_B))
	{
		gb.sound.fx(_work_sfx_ram);
		//OutputPattern(&_work_sfx[0], "_test");
	}

	if (gb.buttons.pressed(BUTTON_C))
	{
		PushState(State::MENU);
	}
	// debug move cursor
	if (gb.buttons.pressed(BUTTON_UP))
	{
		_current_index--;
	}
	if (gb.buttons.pressed(BUTTON_DOWN))
	{
		_current_index++;
	}

	int dir = 0;
	int scale = 1;

	if (gb.buttons.repeat(BUTTON_LEFT, 0) && (repeat == 0 || repeat > 12))
	{
		repeat++;
		dir = -1;
	}
	else if (gb.buttons.repeat(BUTTON_RIGHT, 0) && (repeat == 0 || repeat > 12))
	{
		repeat++;
		dir = 1;
	}
	else if (gb.buttons.repeat(BUTTON_LEFT, 0) || gb.buttons.repeat(BUTTON_RIGHT, 0))
	{
		repeat++;
	}
	else
	{
		repeat = 0;
	}

	if (gb.buttons.repeat(BUTTON_A, 0))
	{
		scale = 10;
	}

	if (dir != 0)
	{
		UI ui_param = (UI)(_current_index);
		switch (ui_param)
		{
		case Gui::UI::PARAM_WAVE:
		{
			currentFx()->type = (Gamebuino_Meta::Sound_FX_Wave) ((((uint32_t)(currentFx()->type) & 0xff) + dir + (uint32_t)(Gamebuino_Meta::Sound_FX_Wave::WAVE_COUNT)) % (uint32_t)(Gamebuino_Meta::Sound_FX_Wave::WAVE_COUNT));
			ResetPattern(_current_fx);
			FixHeader(_current_fx);
			break;
		}
		case Gui::UI::PARAM_LENGTH:
		{
			int32_t & param = currentFx()->params[_current_index - (uint32_t)Gui::UI::PARAM_WAVE];
			param = clamp(param + dir * 441 * scale, (int32_t)0, (int32_t)44100 * 100);
		}
		break;
		case Gui::UI::PARAM_VOLUME:
		case Gui::UI::PARAM_VOLUME_ENV:
		case Gui::UI::PARAM_PERIOD_ENV:
		{
			const value & v = getParamValue(ui_param, currentFx()->type);
			int32_t & param = currentFx()->params[_current_index - (uint32_t)Gui::UI::PARAM_WAVE];
			param = clamp(param + dir * v.incr * scale, v.min, v.max);
		}
		break;
		case Gui::UI::PARAM_PERIOD:
		{

			if ((uint32_t)currentFx()->type == (uint32_t)(Gamebuino_Meta::Sound_FX_Wave::NOISE) & 0xff)
			{
				const value & v = getParamValue(ui_param, currentFx()->type);
				int32_t & param = currentFx()->params[_current_index - (uint32_t)Gui::UI::PARAM_WAVE];
				param = clamp(param + dir * v.incr * scale, v.min, v.max);
			}
			else
			{
				uint8_t note_index = FindCurrentToneId();
				if (scale == 1)
				{
					note_index += -dir;
				}
				else
				{
					note_index += -dir * 12;
				}
				note_index = clamp(note_index, (uint8_t)25, (uint8_t)(sizeof(note_table) / sizeof(note_table[0])));
				currentFx()->params[_current_index - (uint32_t)Gui::UI::PARAM_WAVE] = note_table[note_index] * 256;
			}
			break;
		}
		case Gui::UI::PATTERN_SELECT:
		{
			_select_fx_index = (_select_fx_index + dir + (_pattern_length + 2)) % (_pattern_length + 2);
			break;
		}
		default:
			// Shouldn't happend
			break;
		}
	}

	// Pattern select sutff
	{
		if (gb.buttons.pressed(BUTTON_A) && _current_index == (uint32_t)Gui::UI::PATTERN_SELECT)
		{
			if (_select_fx_index == 0)
			{
				_pattern_length = max(1, (_pattern_length)-1);
				FixAllHeaders();
			}
			else if (_select_fx_index == _pattern_length + 1)
			{
				_pattern_length = min(8, _pattern_length + 1);
				_select_fx_index = _pattern_length + 1; // Move the cursor to follow the plus button
				if (_current_fx > _pattern_length - 1)
				{
					_current_fx = _pattern_length - 1;
				}
				FixAllHeaders();
			}
			else // Select the fx
			{
				_current_fx = _select_fx_index - 1;
			}
		}
	}


	if ((int32_t)_current_index < (int32_t)UI::PARAMS_BEGIN)
	{
		_current_index = (uint32_t)UI::UI_END - 1;
	}

	if (_current_index >(uint32_t)UI::UI_END - 1)
	{
		_current_index = (uint32_t)UI::PARAMS_BEGIN;
	}
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
			int id = (uint32_t)currentFx()->type & 0xff;
			gb.display.print(_wave_names[id]);
			break;
		}
	case Gui::UI::PARAM_LENGTH:
		gb.display.setCursor(BAR_X, y);
		gb.display.printf("%6d ms", 1000 * currentFx()->length/44100);
		break;
	case Gui::UI::PARAM_VOLUME:
	case Gui::UI::PARAM_VOLUME_ENV:
	case Gui::UI::PARAM_PERIOD:
	case Gui::UI::PARAM_PERIOD_ENV:
		// Draw bar
		DrawBar(BAR_X, y, currentFx()->params[index] , getParamValue(ui_param, currentFx()->type).min, getParamValue(ui_param, currentFx()->type).max, selected);
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
	gb.display.clear();

	switch (CurrentState())
	{
	case State::PARAMS:
		DrawParams();
		break;
	case State::MENU:
		DrawMenu();
		break;
	default:
		break;
	}

	DrawNotification();

#else
	gb.display.clear();
	gb.display.printf("%d\n", gb.getCpuLoad());
	gb.display.println(gb.getFreeRam());
#endif
}

void Gui::DrawParams()
{
	gb.display.clear(DARKGRAY);


	// Draw Middle Interface
	for (int index = 0; index < NUM_PARAMS; index++)
	{
		DrawParamEntry(index);
	}


	DrawMenuBar();

	// Draw Pattern List

	DrawPatternSelect();

	// Draw Status Bar
	DrawStatusBar();
}

void Gui::DrawMenu()
{
	DrawMenuBar();
	DrawMenuItems();
}

void Gui::DrawMenuBar()
{
	const char * labels[] = { "file", "edit" };
	const int STEP_X = 20;
	const int START_X = 1;
	const int START_Y = 1;

	gb.display.setColor(GRAY);
	gb.display.fillRect(0, 0, 80, 7);
	gb.display.drawRect(0, 0, 80, 57);

	// Draw Middle Inferface

	for (int i = 0; i < (uint32_t)Menu::MENU_MAX; i++)
	{
		if (_current_menu == i && CurrentState() == State::MENU)
		{
			gb.display.setColor(DARKGRAY);
			gb.display.fillRect(START_X - 2 + i * STEP_X, START_Y - 1, STEP_X, 7);
			gb.display.setColor(GRAY);
		}
		else
		{
			gb.display.setColor(DARKGRAY);
		}
		gb.display.setCursor(START_X + i * STEP_X, START_Y);
		gb.display.printf(labels[i]);
	}



	gb.display.setCursor(20, 1);
}

void Gui::DrawMenuItems()
{

	int id = _current_menu;
	const char ** current_items = items[id];
	
	const int STEP_Y = 6;
	const int START_Y = 8;
	const int START_X = 6;

	for (int i = 0; current_items[i] != nullptr; i++)
	{
		if (i == _current_menu_item)
		{
			gb.display.setColor(DARKGRAY);
			gb.display.fillRect(1, i*STEP_Y + START_Y - 1, 78, STEP_Y+1);
			gb.display.setColor(WHITE);
		}
		else
		{
			gb.display.setColor(GRAY);
		}
		gb.display.setCursor(START_X, START_Y + i * STEP_Y);
		gb.display.printf(current_items[i]);
	}

}

void Gui::DrawPatternSelect()
{
	const int START_X = 9;
	const int START_Y = 48;
	const int STEP_X = 8;
	const int SQUARE_WIDTH = 7;
	const int SIDE_OFFSET_X = 6;

	// Draw highlight bar
	if (_current_index == (uint32_t)UI::PATTERN_SELECT)
	{
		gb.display.setColor(BLACK);
		gb.display.fillRect(1, START_Y, 78, SQUARE_WIDTH);

		// Draw White around selected entry
		gb.display.setColor(WHITE);
		if (_select_fx_index == 0)
		{
			gb.display.fillRect(START_X - SIDE_OFFSET_X - 1, START_Y -1, 7, 9);
		}
		else if(_select_fx_index == _pattern_length + 1)
		{
			gb.display.fillRect(START_X + (_pattern_length * STEP_X) - 1, START_Y -1 , 7, 9);
		}
		else
		{
			gb.display.fillRect(START_X - 1 + (_select_fx_index - 1) * STEP_X, START_Y - 1, SQUARE_WIDTH + 2, SQUARE_WIDTH + 2);
		}
		
	}

	// draw minus
	gb.display.setColor(GRAY);
	gb.display.fillRect(START_X - SIDE_OFFSET_X, START_Y + 1, 5, 5);
	gb.display.setCursor(START_X - 5, START_Y + 1);
	gb.display.setColor(DARKGRAY);
	gb.display.print("-");

	// Draw fx list
	for (int i = 0; i < _pattern_length; i++)
	{
		if (i == _current_fx)
		{
			gb.display.setColor(WHITE);
		}
		else
		{
			gb.display.setColor(GRAY);
		}
		gb.display.fillRect(START_X + i * STEP_X, START_Y, SQUARE_WIDTH, SQUARE_WIDTH);
		if (i == _current_fx)
		{
			gb.display.setColor(GRAY);
		}
		else
		{
			gb.display.setColor(DARKGRAY);
		}
		
		gb.display.setCursor(START_X + i * STEP_X + 2, START_Y + 1);
		gb.display.printf("%d", i+1);
	}

	// draw plus
	gb.display.setColor(GRAY);
	gb.display.fillRect(START_X + _pattern_length * STEP_X, START_Y + 1, 5, 5);
	gb.display.setCursor(START_X + _pattern_length * STEP_X + 1, START_Y + 1);
	gb.display.setColor(DARKGRAY);
	gb.display.print("+");
}

void Gui::DrawStatusBar()
{
	const int VALUE_X = 37;
	const int VALUE_Y = 58;

	if (_current_index < (uint32_t)Gui::UI::PARAMS_END)
	{
		int index = _current_index - (uint32_t)(Gui::UI::PARAMS_BEGIN);
		gb.display.setColor(GRAY);
		gb.display.setCursor(VALUE_X, VALUE_Y);
		gb.display.printf("v:%8d", currentFx()->params[index]);
	}
	else if (_current_index == (uint32_t)Gui::UI::PATTERN_SELECT)
	{

	}
}

void Gui::Copy()
{
	_sfx_copy = *currentFx();
}

void Gui::Paste()
{
	*currentFx() = _sfx_copy;
}

void Gui::DisplayNotification(const char * message, int16_t time)
{
	_notification_string = message;
	_notification_timer = time;
}

void Gui::DrawNotification()
{
	const int START_X = 0;
	const int START_Y = 58;


	if (_notification_timer > 0)
	{
		_notification_timer--;

		gb.display.setColor(DARKGRAY);
		gb.display.fillRect(START_X, START_Y - 1, 80, 7);
		gb.display.setColor(GRAY);
		gb.display.setCursor(START_X, START_Y);
		gb.display.printf(_notification_string);
	}
}
