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

	enum class UI : uint32_t {
		MENU_BEGIN = 0			,
		FILE = MENU_BEGIN		,
		EDIT					,
		MENU_END				,

		PARAMS_BEGIN = MENU_END,
		PARAM_WAVE = PARAMS_BEGIN		,
		PARAM_VOLUME					,
		PARAM_VOLUME_ENV				,
		PARAM_PERIOD					,
		PARAM_PERIOD_ENV				,
		PARAM_LENGTH					,
		PARAMS_END				,

		SELECT_BEGIN = PARAMS_END		,
		SELECT_MINUS = SELECT_BEGIN		,
		SELECT_0,
		SELECT_MAX = SELECT_0 + MAX_FX	,
		SELECT_PLUS,
		SELECT_END,

		UI_END,
	};

	static const char * _param_strings[];

	void Begin();

	void Update();

	void DrawParamEntry(uint8_t index);

	void DrawBar(int x, int y, int32_t value, int32_t min, int32_t max, bool selected);

	void Draw();

	void DrawStatusBar();

	inline Gamebuino_Meta::Sound_FX * currentFx() { return &_work_sfx[0]; };

private:
	Gamebuino_Meta::Sound_FX _work_sfx[MAX_FX];

	uint32_t _current_index;
	uint32_t _current_fx;

};

