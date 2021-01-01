#include <windows.h>



void MemsetOverridePerms(uintptr_t address, BYTE* data, DWORD size);
void* GetIATFunctionAddress(BYTE* base, const char* dll_name, const char* search);

//Since we need to hook specific WinAPI behaviour for our results, they're encapsulated in a namespace
namespace FunctionHooks
{
	
	HWND mainWindow = NULL;
	HWND WINAPI HookCreateWindowExA(
		DWORD     dwExStyle,
		LPCSTR    lpClassName,
		LPCSTR    lpWindowName,
		DWORD     dwStyle,
		int       X,
		int       Y,
		int       nWidth,
		int       nHeight,
		HWND      hWndParent,
		HMENU     hMenu,
		HINSTANCE hInstance,
		LPVOID    lpParam
	)
	{
		if (!hWndParent)
		{
			//We assume this is the main window, could be wrong.
			dwStyle &= ~WS_OVERLAPPEDWINDOW;
			dwExStyle &= ~(WS_EX_OVERLAPPEDWINDOW | WS_EX_TOPMOST);
			auto result = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y,
				nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
			if (!mainWindow) mainWindow = result;
			return result;
		}
		else return CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y,
			nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}

	LONG_PTR  WINAPI HookSetWindowLongPtrA(
			 HWND hWnd,
			 int nIndex,
			 LONG dwNewLong)
	{
		if (hWnd == mainWindow)
		{
			switch (nIndex)
			{
			case GWL_STYLE:
				dwNewLong &= ~WS_OVERLAPPEDWINDOW;
				break;
			case GWL_EXSTYLE:
				dwNewLong &= ~(WS_EX_OVERLAPPEDWINDOW | WS_EX_TOPMOST);
				break;
			default:

				break;
			}
		}
		return SetWindowLongPtr(hWnd, nIndex, dwNewLong);
	}
}