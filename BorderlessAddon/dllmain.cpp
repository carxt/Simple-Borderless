// dllmain.cpp : Defines the entry point for the DLL application.
#include "dllheader.h"



void MemsetOverridePerms(uintptr_t address, BYTE* data, DWORD size)
{
	//By default, we cannot edit read only values in  Windows, this function changes it.
	DWORD oPermissions;
	VirtualProtect((void*)address, size, PAGE_EXECUTE_READWRITE, &oPermissions);
	memcpy((void*)address, data, size);
	VirtualProtect((void*)address, size, oPermissions, &oPermissions);
	FlushInstructionCache(GetCurrentProcess(), (void*)address, size);
}

void PatchAddressPointer(uintptr_t address, uintptr_t data)
{
	//This architecture agnostic function is used to patch pointers to pointers or pointers to addresses.
	BYTE bytes[sizeof(uintptr_t)];
	*(uintptr_t*)bytes = data;
	MemsetOverridePerms(address, bytes, sizeof(uintptr_t));
}

void* GetIATFunctionAddress(BYTE* base, const char* dll_name, const char* search)
{
	//This function retrieves the function pointer of the desired function from the specified module.
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)base;
	IMAGE_NT_HEADERS* nt_headers = (IMAGE_NT_HEADERS*)(base + dos_header->e_lfanew);
	IMAGE_DATA_DIRECTORY section = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	IMAGE_IMPORT_DESCRIPTOR* import_descriptor = (IMAGE_IMPORT_DESCRIPTOR*)(base + section.VirtualAddress);
	for (size_t i = 0; import_descriptor[i].Name != NULL; i++)
	{
		if (!_stricmp((char*)(base + import_descriptor[i].Name), dll_name))
		{
			if (!import_descriptor[i].FirstThunk) { return nullptr; }
			IMAGE_THUNK_DATA* name_table = (IMAGE_THUNK_DATA*)(base + import_descriptor[i].OriginalFirstThunk);
			IMAGE_THUNK_DATA* import_table = (IMAGE_THUNK_DATA*)(base + import_descriptor[i].FirstThunk);
			for (; name_table->u1.Ordinal != NULL; ++name_table, ++import_table)
			{
				if (!IMAGE_SNAP_BY_ORDINAL(name_table->u1.Ordinal))
				{
					IMAGE_IMPORT_BY_NAME* import_name = (IMAGE_IMPORT_BY_NAME*)(base + name_table->u1.ForwarderString);
					char* func_name = &import_name->Name[0];
					if (!_stricmp(func_name, search)) { return &import_table->u1.AddressOfData; }
				}
			}
		}
	}
	return nullptr;
}


namespace FunctionHooks
{

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
			//We assume this is the main window
			dwStyle &= ~WS_OVERLAPPEDWINDOW;
			dwExStyle &= ~(WS_EX_OVERLAPPEDWINDOW | WS_EX_TOPMOST);
			HWND result = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y,
				nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
			if (!mainWindow || !::IsWindow(mainWindow)) mainWindow = result;
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
		return SetWindowLongPtrA(hWnd, nIndex, dwNewLong);
	}

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
	)
	{
		if (!hWndParent)
		{
			//We assume this is the main window
			dwStyle &= ~WS_OVERLAPPEDWINDOW;
			dwExStyle &= ~(WS_EX_OVERLAPPEDWINDOW | WS_EX_TOPMOST);
			HWND result = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y,
				nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
			if (!mainWindow || !::IsWindow(mainWindow)) mainWindow = result;
			return result;
		}
		else return CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y,
			nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	}
	LONG_PTR  WINAPI HookSetWindowLongPtrW(
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
		return SetWindowLongPtrW(hWnd, nIndex, dwNewLong);
	}
	BOOL WINAPI HookSetWindowPos(
		HWND hWnd,
		HWND hWndInsertAfter,
		int  X,
		int  Y,
		int  cx,
		int  cy,
		UINT uFlags
	) {
		if (hWnd == mainWindow)
		{
			if (BorderlessSettings::WindowModeOverride == 1) hWndInsertAfter = HWND_TOPMOST;
			else if (BorderlessSettings::WindowModeOverride == 2) hWndInsertAfter = HWND_TOP;
			if (BorderlessSettings::EnableCustomWindowParameters)
			{
				cx = BorderlessSettings::CustomWindowParameters::CustomWidth;
				cy = BorderlessSettings::CustomWindowParameters::CustomHeight;
				X = BorderlessSettings::CustomWindowParameters::CustomPositionX;
				Y = BorderlessSettings::CustomWindowParameters::CustomPositionY;
			}
		}

		return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
	}
	BOOL WINAPI HookMoveWindow(
		HWND hWnd,
		int  X,
		int  Y,
		int  nWidth,
		int  nHeight,
		BOOL bRepaint
	) {

		if (hWnd == mainWindow && BorderlessSettings::EnableCustomWindowParameters)
		{
			nWidth = BorderlessSettings::CustomWindowParameters::CustomWidth;
			nHeight = BorderlessSettings::CustomWindowParameters::CustomHeight;
			X = BorderlessSettings::CustomWindowParameters::CustomPositionX;
			Y = BorderlessSettings::CustomWindowParameters::CustomPositionY;
		}
		return ::MoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);

	}
}



//In the DLLMain, we create our hooks
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID reserved)
{
	if (!BorderlessSettings::settingsSetUpComplete) SetUpDLLIniDir(hInstance);
	if (!BorderlessSettings::Enabled) return FALSE;
	//Process attach is where our library gets attached to the process
	//Therefore we create our hooks here
	if (dwReason == DLL_PROCESS_ATTACH) {
		//Since we do not need the thread library calls provided by the runtime, we disable them for our module
		DisableThreadLibraryCalls((HMODULE)hInstance);
		//In these calls, we detour the original functions to our desired code paths.
		if (void* CreateWindowIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "CreateWindowExW"))
		{
			PatchAddressPointer((uintptr_t)CreateWindowIATAddress, (uintptr_t)&FunctionHooks::HookCreateWindowExW);
			void* SetWindowLPtrIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowLongPtrW");
			if (!SetWindowLPtrIATAddress) SetWindowLPtrIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowLongW");
			if (SetWindowLPtrIATAddress)
			{
				PatchAddressPointer((uintptr_t)SetWindowLPtrIATAddress, (uintptr_t)&FunctionHooks::HookSetWindowLongPtrW);
			}

		}
		if (void* CreateWindowIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "CreateWindowExA"))
		{
			PatchAddressPointer((uintptr_t)CreateWindowIATAddress, (uintptr_t)&FunctionHooks::HookCreateWindowExA);
			void* SetWindowLPtrIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowLongPtrA");
			if (!SetWindowLPtrIATAddress) SetWindowLPtrIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowLongA");
			if (SetWindowLPtrIATAddress)
			{
				PatchAddressPointer((uintptr_t)SetWindowLPtrIATAddress, (uintptr_t)&FunctionHooks::HookSetWindowLongPtrA);
			}

		}
		if (void* SetWindowsPosIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowPos"))
		{
			PatchAddressPointer((uintptr_t)SetWindowsPosIATAddress, (uintptr_t)FunctionHooks::HookSetWindowPos);
		}
		if (void* MoveWindowIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "MoveWindow"))
		{
			PatchAddressPointer((uintptr_t)MoveWindowIATAddress, (uintptr_t)FunctionHooks::HookMoveWindow);
		}
	}
	//Detach message, which is where the process unloads all our changes.
	//Here, we return everything to its original state
	else if (dwReason == DLL_PROCESS_DETACH) {
		if (void* CreateWindowIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "CreateWindowExW"))
		{
			PatchAddressPointer((uintptr_t)CreateWindowIATAddress, (uintptr_t)::CreateWindowExW);
		}
		if (void* CreateWindowIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "CreateWindowExA"))
		{
			PatchAddressPointer((uintptr_t)CreateWindowIATAddress, (uintptr_t)::CreateWindowExA);
		}
		void* SetWindowLPtrIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowLongPtrW");
		if (!SetWindowLPtrIATAddress) SetWindowLPtrIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowLongW");
		if (SetWindowLPtrIATAddress)
		{
			PatchAddressPointer((uintptr_t)SetWindowLPtrIATAddress, (uintptr_t)SetWindowLongPtrW);
		}


		SetWindowLPtrIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowLongPtrA");
		if (!SetWindowLPtrIATAddress) SetWindowLPtrIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowLongA");
		if (SetWindowLPtrIATAddress)
		{
			PatchAddressPointer((uintptr_t)SetWindowLPtrIATAddress, (uintptr_t)SetWindowLongPtrA);
		}
		if (void* SetWindowsPosIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowPos"))
		{
			PatchAddressPointer((uintptr_t)SetWindowsPosIATAddress, (uintptr_t)::SetWindowPos);
		}
		if (void* MoveWindowIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "MoveWindow"))
		{
			PatchAddressPointer((uintptr_t)MoveWindowIATAddress, (uintptr_t)::MoveWindow);
		}
	}
	return TRUE;
}