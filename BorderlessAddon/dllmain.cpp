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

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID reserved)
{
	//In the DLLMain, we create our hooks
    if (dwReason == DLL_PROCESS_ATTACH) {
		//Process attach is where our library gets attached to the process
		//Therefore we create our hooks here

		//Since we do not need the thread library calls provided by the runtime, we disable them for our module
        DisableThreadLibraryCalls((HMODULE)hInstance); 
		//In these calls, we detour the original functions to our desired code paths.
		if (auto CreateWindowIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "CreateWindowExA"))
		{
			
			void* SetWindowAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowLongPtrA");
			if (!SetWindowAddress) SetWindowAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowLongA");
			if (SetWindowAddress) 
			{
				PatchAddressPointer((uintptr_t)CreateWindowIATAddress, (uintptr_t)&FunctionHooks::HookCreateWindowExA);
				PatchAddressPointer((uintptr_t)SetWindowAddress, (uintptr_t)&FunctionHooks::HookSetWindowLongPtrA);


			}

		}


    }
	//Detach message, which is where the process unloads all our changes.
	//Here, we return everything to its original state
    else if (dwReason == DLL_PROCESS_DETACH) {
		if (auto CreateWindowIATAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "CreateWindowExA"))
		{
			PatchAddressPointer((uintptr_t)CreateWindowIATAddress, (uintptr_t)::CreateWindowExA);
		}
		void* SetWindowAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowLongPtrA");
		if (!SetWindowAddress) SetWindowAddress = GetIATFunctionAddress((BYTE*)GetModuleHandle(NULL), "USER32.dll", "SetWindowLongA");
		if (SetWindowAddress) 
		{
			PatchAddressPointer((uintptr_t)SetWindowAddress, (uintptr_t)SetWindowLongPtrA);
		}

    }
    return TRUE;
}