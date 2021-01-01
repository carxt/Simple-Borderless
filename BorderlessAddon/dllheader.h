
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "iniManager.h"


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
	);

	LONG_PTR  WINAPI HookSetWindowLongPtrA(
		HWND hWnd,
		int nIndex,
		LONG dwNewLong);

	HWND WINAPI HookCreateWindowExW(
		DWORD     dwExStyle,
		LPCWSTR   lpClassName,
		LPCWSTR   lpWindowName,
		DWORD     dwStyle,
		int       X,
		int       Y,
		int       nWidth,
		int       nHeight,
		HWND      hWndParent,
		HMENU     hMenu,
		HINSTANCE hInstance,
		LPVOID    lpParam
	);
	LONG_PTR  WINAPI HookSetWindowLongPtrW(
		HWND hWnd,
		int nIndex,
		LONG dwNewLong);
	BOOL WINAPI HookSetWindowPos(
		HWND hWnd,
		HWND hWndInsertAfter,
		int  X,
		int  Y,
		int  cx,
		int  cy,
		UINT uFlags
	);
	BOOL WINAPI HookMoveWindow(
		HWND hWnd,
		int  X,
		int  Y,
		int  nWidth,
		int  nHeight,
		BOOL bRepaint
	);
}