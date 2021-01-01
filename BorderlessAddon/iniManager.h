#pragma once
char iniDir[MAX_PATH] = { 0 };
namespace BorderlessSettings 
{
	bool settingsSetUpComplete = false;
	bool Enabled = true;
	bool EnableCustomWindowParameters;
	int WindowModeOverride;
	namespace CustomWindowParameters
	{
		unsigned int CustomPositionX;
		unsigned int CustomPositionY;
		unsigned int CustomWidth;
		unsigned int CustomHeight;

	}
}



void SetUpDLLIniDir(HMODULE hModule)
{
	BorderlessSettings::settingsSetUpComplete = true;
	GetModuleFileNameA(hModule, iniDir, MAX_PATH);
	strcpy_s((char*)(strrchr(iniDir, '.')), 5, ".ini");
	BorderlessSettings::Enabled = GetPrivateProfileIntA("Main", "Enabled", 1, iniDir);
	BorderlessSettings::EnableCustomWindowParameters = (bool) GetPrivateProfileIntA("Main", "EnableCustomWindowParameters", 0, iniDir);
	BorderlessSettings::WindowModeOverride = GetPrivateProfileIntA("Main", "WindowModeOverride", 0, iniDir);
	BorderlessSettings::CustomWindowParameters::CustomHeight = GetPrivateProfileIntA("WindowParams", "iHeight", 0, iniDir);
	BorderlessSettings::CustomWindowParameters::CustomWidth = GetPrivateProfileIntA("WindowParams", "iWidth", 0, iniDir);
	BorderlessSettings::CustomWindowParameters::CustomPositionX = GetPrivateProfileIntA("WindowParams", "iXPos", 0, iniDir);
	BorderlessSettings::CustomWindowParameters::CustomPositionY = GetPrivateProfileIntA("WindowParams", "iYPos", 0, iniDir);
	if (BorderlessSettings::CustomWindowParameters::CustomWidth <= 0)
		BorderlessSettings::CustomWindowParameters::CustomWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	if (BorderlessSettings::CustomWindowParameters::CustomHeight <= 0)
		BorderlessSettings::CustomWindowParameters::CustomHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

}