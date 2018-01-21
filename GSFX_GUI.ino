#include <Gamebuino-Meta.h>
#include "Gui.h"

Gui gui;

void setup()
{
	gb.begin();
	gui.Begin();
}

void loop()
{
	if (gb.update())
	{
		gui.Update();
		gui.Draw();
	}
}
