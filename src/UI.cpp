#include "UI.h"

#include <IniReader.h>

LPCSTR UI::lpWindowName = "MegaMod+";

CIniReader managerSettings("megamodplus.ini");
ImVec2 UI::vWindowSize = {
	managerSettings.ReadFloat("Settings", "WindowXSize", 960),
	managerSettings.ReadFloat("Settings", "WindowYSize", 540)
};
ImGuiWindowFlags UI::WindowFlags = 0;
bool UI::bDraw = true;

void UI::Active()
{
	bDraw = true;
}

bool UI::isActive()
{
	return bDraw == true;
}

void UI::Draw()
{
	if (isActive())
	{
		ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::Begin(lpWindowName, &bDraw, WindowFlags);
		{
			ImVec2 currentSize = ImGui::GetWindowSize();
			if (vWindowSize.x != currentSize.x)
			{
				managerSettings.WriteFloat("Settings", "WindowXSize", currentSize.x);
			}
			if (vWindowSize.y != currentSize.y)
			{
				managerSettings.WriteFloat("Settings", "WindowYSize", currentSize.y);
			}
			ImGui::Text("Imagine a MegaMix+ Mod List here.");
		}
		ImGui::End();
	}

	#ifdef _WINDLL
	if (GetAsyncKeyState(VK_INSERT) & 1)
		bDraw = !bDraw;
	#endif
}
