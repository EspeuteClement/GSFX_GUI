#pragma once
//#include "GSFX.h"
#include <Gamebuino-Meta.h>

class Gui
{
public:
	Gui();
	~Gui();

	static const uint8_t MAX_FX = 8;
	static const uint8_t NUM_PARAMS = 6;
	static const uint8_t STACK_DEPTH_MAX = 4;

	enum class UI : uint32_t {
		PARAMS_BEGIN = 0,
		PARAM_WAVE = PARAMS_BEGIN		,
		PARAM_VOLUME					,
		PARAM_VOLUME_ENV				,
		PARAM_PERIOD					,
		PARAM_PERIOD_ENV				,
		PARAM_LENGTH					,
		PARAMS_END						,

		PATTERN_SELECT = PARAMS_END,

		UI_END,
	};

	enum class Menu : uint32_t {
		FILE,
		EDIT,
		MENU_MAX
	};

	enum class State : uint8_t{
		PARAMS,
		MENU,
		MODAL_YES_NO,
		LOAD,
		STATE_MAX,
	};

	static const char * _param_strings[];

	void Begin();

	void SaveCurrentPattern();

	void SaveAll();

	// Load the pattern id from the save into _work_fx
	void Load(int id);

	uint8_t FindCurrentPatternLength();

	uint8_t FindCurrentToneId();

	void ResetPattern(int id);

	void FixHeader(int id);

	void FixAllHeaders();

	void OutputPattern(const Gamebuino_Meta::Sound_FX * fx, uint16_t id);

	void OutputFX(const Gamebuino_Meta::Sound_FX * fx);

	void Update();

	void UpdateChooseFile();

	void UpdateMenu();

	void OpenLoadState();

	void UpdateParams();

	void DrawParamEntry(uint8_t index);

	void DrawBar(int x, int y, int32_t value, int32_t min, int32_t max, bool selected);

	void Draw();

	void DrawChooseFile();

	void DrawParams();

	void DrawMenu();

	void DrawMenuBar();

	void DrawMenuItems();

	void DrawPatternSelect();

	void DrawStatusBar();

	void Copy();

	void Paste();

	void DisplayNotification(const char * message, int16_t time = 60);

	void DrawNotification();

	inline Gamebuino_Meta::Sound_FX * currentFx() { return &(_work_sfx_ram[_current_fx]); };
	inline Gamebuino_Meta::Sound_FX * getFx(int id) { return &(_work_sfx_ram[id]);};

	inline void PushState(State state) { _state_stack[++_state_id] = state; };
	inline void PopState() { _state_id--; };
	inline State CurrentState() { return _state_stack[_state_id]; };

private:
	Gamebuino_Meta::Sound_FX _work_sfx_ram[8];

	Gamebuino_Meta::Sound_FX _sfx_copy; // For copy paste


	uint32_t _current_index;
	uint32_t _current_fx;
	uint32_t _pattern_length;
	uint32_t _current_save_id;
	uint32_t _temp_save_id;

	uint8_t _current_note;
	uint8_t _select_fx_index;

	uint8_t _current_menu;
	uint8_t _current_menu_item;

	const char * _notification_string;
	int16_t _notification_timer;

	int8_t _state_id;
	State _state_stack[STACK_DEPTH_MAX];
};

