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
	2697,2546,2403,2267,2141,2020,1907,1800,1699,1604,1513,1429,1349,1273,1201,1134,1070,1010,954,900,850,802,757,714,674,636,601,567,535,505,477,450,425,401,378,357,337,318,300,283,268,253,238,225,212,200,189,179,169,159,150,142,134,126,119,113,106,100,95,89,84,80,75,71,67,63,60,56,53,50,47,45,42,40,38,35,33,32,30,28,27,25,24,22,21,20,19,18,17,16,15,14,13,13,12,11,11,10,9,9,8,8,7,7,7,6,6,6 
};

Gamebuino_Meta::Sound_FX sfx_test[8] =
{
	{ Gamebuino_Meta::Sound_FX_Wave::SQUARE,1, 0xFF ,0,100,0,5 },
	{ Gamebuino_Meta::Sound_FX_Wave::SQUARE,1, 0xFF ,0,200,0,5 },
	{ Gamebuino_Meta::Sound_FX_Wave::SQUARE,0, 0xFF ,0,100,0,5 },
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
		{ 0xFF , 0, 0xFF , 4 },
		{ 0xFF  , 0, 0xFF , 4 },
	},
	{ // Volume_env
		{ 0 , INT8_MIN  , INT8_MAX , 1},
		{ 0 , INT8_MIN , INT8_MAX , 1},
	},
	{ // Period
		{ 0, 0, 256, 16},
		{ 50, 0, 2697, 1},
	},
	{ // Period_env
		{ 0 , INT8_MIN  , INT8_MAX , 1 },
		{ 0 , INT8_MIN , INT8_MAX , 1 },
	},
};

constexpr const value & getParamValue(const Gui::UI param, const uint8_t wave)
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
	_current_fx = 0;
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
	_current_fx = 0;
	if (newSave.version != SAVE_VERSION)
	{
		memset(&newSave, 0, sizeof(Save));
		ResetPattern(_current_fx);
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
	while ((int32_t)_work_sfx_ram[r].continue_flag)
	{
		r++;
	}
	return r+1;
}

uint8_t Gui::FindCurrentToneId()
{
	uint32_t current_tone = currentFx()->period_start;
	int index = 0;
	while (current_tone < note_table[index]  && index < sizeof(note_table) / sizeof(note_table[0]))
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
	uint8_t wave = fx->type;

	fx->volume_start = getParamValue(UI::PARAM_VOLUME, wave).def;
	fx->volume_sweep = getParamValue(UI::PARAM_VOLUME_ENV, wave).def;
	fx->period_start = getParamValue(UI::PARAM_PERIOD, wave).def;
	fx->period_sweep = getParamValue(UI::PARAM_PERIOD_ENV, wave).def;
	fx->length = 10;
}

void Gui::FixHeader(int id)
{
	if (id < _pattern_length - 1)
	{
		Gamebuino_Meta::Sound_FX * fx = getFx(id);
		fx->continue_flag = 1;
	}
	else if (id == _pattern_length - 1)
	{
		Gamebuino_Meta::Sound_FX * fx = getFx(id);
		fx->continue_flag = 0;
	}
}

void Gui::FixAllHeaders()
{
	for (int i = 0; i < _pattern_length; i++)
	{
		FixHeader(i);
	}
}

void Gui::OutputPattern(const Gamebuino_Meta::Sound_FX *fx, uint16_t id)
{
	if (SerialUSB)
	{
		char buffer[16];
		sprintf(buffer, "sfx_%d", id);

		SerialUSB.print("//Put this in your .h file :\n");
		SerialUSB.print("extern const Gamebuino_Meta::Sound_FX ");
		SerialUSB.print(buffer);
		SerialUSB.print("[];\n\n");

		SerialUSB.print("//Put this in your .cpp file :\n");
		SerialUSB.print("const Gamebuino_Meta::Sound_FX ");
		SerialUSB.print(buffer);
		SerialUSB.print("[] = {\n");
		int i = 0;
		const Gamebuino_Meta::Sound_FX * sfx = &fx[i];
		do {
			sfx = &fx[i];
			OutputFX(sfx);
			SerialUSB.print(",\n");
			i++;
		} while (sfx->continue_flag);

		SerialUSB.print("};\n");
	}
}

void Gui::OutputFX(const Gamebuino_Meta::Sound_FX * fx)
{
	if (SerialUSB)
	{
		SerialUSB.print("\t{");

		SerialUSB.print(_output_wave_names[fx->type]);
		
		SerialUSB.print(",");
		SerialUSB.print(fx->continue_flag);

		SerialUSB.print(",");
		SerialUSB.print(fx->volume_start);

		SerialUSB.print(",");
		SerialUSB.print(fx->volume_sweep);

		SerialUSB.print(",");
		SerialUSB.print(fx->period_sweep);

		SerialUSB.print(",");
		SerialUSB.print(fx->period_start);

		SerialUSB.print(",");
		SerialUSB.print(fx->length);

		SerialUSB.print("}");
	}
}

void Gui::Update()
{
#ifdef BENCH
	gb.lights.clear(RED);
	if (gb.buttons.pressed(BUTTON_B))
	{
		gb.sound.fx(sfx_test);
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
	case State::LOAD:
		UpdateChooseFile();
		break;
	default:
		assert(false,"STATE IS NOT IMPLEMENTED");
		break;
	}
	
#endif // !BENCH
}

void Gui::UpdateChooseFile()
{
	if (gb.buttons.pressed(BUTTON_LEFT))
	{
		if (_temp_save_id > 0)
		{
			_temp_save_id--;
		}
		else
		{
			_temp_save_id = FILES_NUMBER - 1;
		}
	}

	if (gb.buttons.pressed(BUTTON_RIGHT))
	{
		if (_temp_save_id < FILES_NUMBER-1)
		{
			_temp_save_id++;
		}
		else
		{
			_temp_save_id = 0;
		}
	}

	if (gb.buttons.pressed(BUTTON_A))
	{
		Load(_temp_save_id);
		PopState();
		PopState();
		DisplayNotification("FX  loaded");
	}

	if (gb.buttons.pressed(BUTTON_B))
	{
		PopState();
	}

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
					DisplayNotification("FX Saved");
					PopState();
					break;
				case 1: // Load
					PushState(State::LOAD);
					break;
				case 2: // Export
					OutputPattern(_work_sfx_ram, _current_save_id);
					PopState();
					break;
			}
			break;
		case Menu::EDIT:
			switch (_current_menu_item)
			{
			case 0: // Copy
				Copy();
				DisplayNotification("Current FX Copied");
				PopState();
				break;
			case 1: // Pase
				Paste();
				DisplayNotification("Current FX Copied");
				PopState();
				break;
			case 2: // Clear
				ResetPattern(_current_fx);
				DisplayNotification("Current FX Cleared");
				PopState();
				break;
			}
			break;
		default:
			break;
		}
	}
}

void Gui::OpenLoadState()
{
	_temp_save_id = _current_save_id;
	PushState(State::LOAD);
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
			currentFx()->type = (uint8_t) ((currentFx()->type + Gamebuino_Meta::Sound_FX_Wave::MAX + dir) % Gamebuino_Meta::Sound_FX_Wave::MAX);
			ResetPattern(_current_fx);
			FixHeader(_current_fx);
			break;
		}
		case Gui::UI::PARAM_LENGTH:
		{
			uint16_t & param = currentFx()->length;
			param = clamp((int32_t) param + dir * scale, (int32_t)0, (int32_t) UINT16_MAX);
		}
		break;
		case Gui::UI::PARAM_VOLUME:
		{
			const value & v = getParamValue(ui_param, currentFx()->type);
			uint8_t & param = currentFx()->volume_start;
			param = clamp((int32_t)param + dir * scale, (int32_t)0, (int32_t)UINT8_MAX);
			break;
		}
		case Gui::UI::PARAM_VOLUME_ENV:
		{
			const value & v = getParamValue(ui_param, currentFx()->type);
			int8_t & param = currentFx()->volume_sweep;
			param = clamp((int32_t)param + dir * scale, (int32_t)INT8_MIN, (int32_t)INT8_MAX);
			break;
		}
		case Gui::UI::PARAM_PERIOD_ENV:
		{
			const value & v = getParamValue(ui_param, currentFx()->type);
			int8_t & param = currentFx()->period_sweep;
			param = clamp((int32_t)param + dir * scale, (int32_t)INT8_MIN, (int32_t)INT8_MAX);
			break;
		}
		break;
		case Gui::UI::PARAM_PERIOD:
		{

			if (currentFx()->type == Gamebuino_Meta::Sound_FX_Wave::NOISE)
			{
				const value & v = getParamValue(ui_param, currentFx()->type);
				uint16_t & param = currentFx()->period_start;
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
				currentFx()->period_start = note_table[note_index];
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
				if (_current_fx > _pattern_length - 1)
				{
					_current_fx = _pattern_length - 1;
				}
				FixAllHeaders();
			}
			else if (_select_fx_index == _pattern_length + 1)
			{
				_pattern_length = min(8, _pattern_length + 1);
				_select_fx_index = _pattern_length + 1; // Move the cursor to follow the plus button
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
		gb.display.printf("%5d ms", currentFx()->length * 20);
		break;
	case Gui::UI::PARAM_VOLUME:
		DrawBar(BAR_X, y, currentFx()->volume_start, getParamValue(ui_param, currentFx()->type).min, getParamValue(ui_param, currentFx()->type).max, selected);
		break;
	case Gui::UI::PARAM_VOLUME_ENV:
		DrawBar(BAR_X, y, currentFx()->volume_sweep, getParamValue(ui_param, currentFx()->type).min, getParamValue(ui_param, currentFx()->type).max, selected);
		break;
	case Gui::UI::PARAM_PERIOD:
		DrawBar(BAR_X, y, currentFx()->period_start, getParamValue(ui_param, currentFx()->type).min, getParamValue(ui_param, currentFx()->type).max, selected);
		break;
	case Gui::UI::PARAM_PERIOD_ENV:
		DrawBar(BAR_X, y, currentFx()->period_sweep, getParamValue(ui_param, currentFx()->type).min, getParamValue(ui_param, currentFx()->type).max, selected);
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
	case State::LOAD:
		DrawChooseFile();
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

void Gui::DrawChooseFile()
{
	gb.display.clear(DARKGRAY);
	DrawMenuBar();

	gb.display.setColor(GRAY);
	gb.display.setCursor(2, 9);
	gb.display.print("Choose file to load");
	gb.display.setCursor(27, 16);
	gb.display.printf("< %02d >", _temp_save_id);
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
	gb.display.clear(DARKGRAY);
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
			gb.display.setColor(BLACK);
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
	const int VALUE_X = 37+12;
	const int VALUE_Y = 58;

	if (_current_index < (uint32_t)Gui::UI::PARAMS_END)
	{
		int index = _current_index - (uint32_t)(Gui::UI::PARAMS_BEGIN);

		int32_t value = 0;
		switch ((Gui::UI)_current_index)
		{
		case Gui::UI::PARAM_WAVE:
			value = currentFx()->type;
			break;
		case Gui::UI::PARAM_VOLUME:
			value = currentFx()->volume_start;
			break;
		case Gui::UI::PARAM_VOLUME_ENV:
			value = currentFx()->volume_sweep;
			break;
		case Gui::UI::PARAM_PERIOD:
			value = currentFx()->period_start;
			break;
		case Gui::UI::PARAM_PERIOD_ENV:
			value = currentFx()->period_sweep;
			break;
		case Gui::UI::PARAM_LENGTH:
			value = currentFx()->length;
			break;

		default:
			break;
		}
		gb.display.setColor(GRAY);
		gb.display.setCursor(VALUE_X, VALUE_Y);
		gb.display.printf("v:%5d", value);
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
	const int START_X = 1;
	const int START_Y = 58;


	if (_notification_timer > 0)
	{
		_notification_timer--;

		gb.display.setColor(BLACK);
		gb.display.fillRect(START_X, START_Y - 1, 80, 7);
		gb.display.setColor(WHITE);
		gb.display.setCursor(START_X, START_Y);
		gb.display.printf(_notification_string);
	}
}
