#define _CRT_SECURE_NO_WARNINGS	

#include <Windows.h>
#include <iostream>
#include <io.h>
#include <shellapi.h>

char g_LauncherDir[MAX_PATH] = {};
char g_RevIniName[MAX_PATH] = {};
char g_ProcName[MAX_PATH] = {};
char g_ProcArgs[MAX_PATH] = {};
char g_LibraryName[MAX_PATH] = {};

char g_GameAppId[256] = {};

wchar_t **g_Argv = nullptr;
char g_AdditionalProcName[MAX_PATH] = {};
int g_NumArgs = 0;

// steam_appid.txt okuma
bool GetSteamAppID(char *pszOut, size_t maxLen)
{
	FILE* f = fopen("steam_appid.txt", "r");
	if (!f)
	{
		if (pszOut && maxLen > 0) *pszOut = '\0';
		return false;
	}

	if (fgets(pszOut, (int)maxLen, f) != NULL)
	{
		char *psz = strpbrk(pszOut, "\r\n\t ");
		if (psz)
		{
			*psz = '\0';
		}
		fclose(f);
		return (strlen(pszOut) > 0);
	}

	fclose(f);
	if (pszOut && maxLen > 0) *pszOut = '\0';
	return false;
}

// Shared Memory oluşturma
void CreateSharedMemFile(HANDLE *hMapView, HANDLE *hFileMap, HANDLE *hEvent)
{
	char szDest[260];

	*hFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1024, "Local\\SteamStart_SharedMemFile");

	if (!*hFileMap)
	{
		sprintf(szDest, "Unable to CreateFileMapping: %i", GetLastError());
		MessageBoxA(HWND_DESKTOP, szDest, "Error", MB_OK);
		return;
	}
	
	*hMapView = MapViewOfFile(*hFileMap, SECTION_ALL_ACCESS, 0, 0, 0);

	if (!*hMapView)
	{
		sprintf(szDest, "Unable to MapViewOfFile: %i", GetLastError());
		MessageBoxA(HWND_DESKTOP, szDest, "Error", MB_OK);
		CloseHandle(*hFileMap);
		*hFileMap = NULL;
		return;
	}

	*hEvent = CreateEventA(NULL, FALSE, FALSE, "Local\\SteamStart_SharedMemLock");

	if (!*hEvent)
	{
		sprintf(szDest, "Unable to CreateEvent: %i", GetLastError());
		MessageBoxA(HWND_DESKTOP, szDest, "Error", MB_OK);
		UnmapViewOfFile(*hMapView);
		CloseHandle(*hFileMap);
		*hFileMap = NULL;
		*hMapView = NULL;
		return;
	}

	SetEvent(*hEvent);
}

// ActiveProcess Registry ayarları
void SetActiveProcess(int pid)
{
	DWORD dwD;
	HKEY phkResult;

	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, KEY_WRITE, &phkResult) != ERROR_SUCCESS)
		RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, NULL, 0, KEY_WRITE, NULL, &phkResult, &dwD);

	RegSetValueExA(phkResult, "pid", 0, REG_DWORD, (BYTE *)&pid, sizeof(pid));
	RegSetValueExA(phkResult, "SteamPath", 0, REG_SZ, (BYTE *)g_LauncherDir, (DWORD)strlen(g_LauncherDir) + 1);
	RegCloseKey(phkResult);
}

// Registry'e SteamClientDll kaydetme
void SetSteamClientDll(char *pszLib)
{
	DWORD dwD;
	HKEY phkResult;

	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, KEY_WRITE, &phkResult) != ERROR_SUCCESS)
		RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, NULL, 0, KEY_WRITE, NULL, &phkResult, &dwD);

	RegSetValueExA(phkResult, "SteamClientDll", 0, REG_SZ, (BYTE *)pszLib, (DWORD)strlen(pszLib) + 1);
	RegCloseKey(phkResult);
}

// Oyunu Başlatma
void StartGameApp()
{
	HANDLE hFileMap = NULL;
	HANDLE hMapView = NULL;
	HANDLE hSteamMem = NULL;
	CreateSharedMemFile(&hMapView, &hFileMap, &hSteamMem);

	STARTUPINFOA StartupInformation = {};
	PROCESS_INFORMATION ProcessInformation = {};
	StartupInformation.cb = sizeof(StartupInformation);

	char szFullCmd[2048] = {};
	char szExePath[MAX_PATH] = {};

	if (strchr(g_ProcName, '\\') == NULL && strchr(g_ProcName, '/') == NULL)
	{
		sprintf(szExePath, "%s%s", g_LauncherDir, g_ProcName);
	}
	else
	{
		strcpy(szExePath, g_ProcName);
	}

	if (g_ProcArgs[0] != '\0')
	{
		sprintf(szFullCmd, "\"%s\" %s", szExePath, g_ProcArgs);
	}
	else
	{
		sprintf(szFullCmd, "\"%s\"", szExePath);
	}

	BOOL bCreated = CreateProcessA(
		szExePath,
		szFullCmd,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		g_LauncherDir,
		&StartupInformation,
		&ProcessInformation
	);

	if (bCreated)
	{
		SetActiveProcess(ProcessInformation.dwProcessId);

		WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

		CloseHandle(ProcessInformation.hThread);
		CloseHandle(ProcessInformation.hProcess);

		if (hSteamMem) CloseHandle(hSteamMem);
		if (hMapView) UnmapViewOfFile(hMapView);
		if (hFileMap) CloseHandle(hFileMap);
	}
	else
	{
		char szDest[512];
		sprintf(szDest, "Unable to execute command: %s (Error Code: %d)\nWorking Dir: %s", szExePath, GetLastError(), g_LauncherDir);
		MessageBoxA(HWND_DESKTOP, szDest, "Error", MB_ICONWARNING | MB_SYSTEMMODAL);
	}
}

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	if (!GetModuleFileNameA(NULL, g_LauncherDir, sizeof(g_LauncherDir)))
	{
		MessageBoxA(HWND_DESKTOP, "Unable to initialize the process path", "Error", MB_ICONWARNING | MB_SYSTEMMODAL);
		return -1;
	}

	char *psz = strrchr(g_LauncherDir, '\\');
	if (psz)
	{
		*(psz + 1) = '\0';
	}

	strcpy(g_RevIniName, g_LauncherDir);
	strcat(g_RevIniName, "rev.ini");

	g_Argv = CommandLineToArgvW(GetCommandLineW(), &g_NumArgs);

	if (g_Argv)
	{
		for (int i = 0; i < g_NumArgs; i++)
		{
			if (_wcsicmp(g_Argv[i], L"-launch") == 0 && (i + 1 < g_NumArgs))
			{
				wcstombs(g_ProcName, g_Argv[++i], sizeof(g_ProcName) - 1);
			}
			else if (_wcsicmp(g_Argv[i], L"-appid") == 0 && (i + 1 < g_NumArgs))
			{
				wcstombs(g_GameAppId, g_Argv[++i], sizeof(g_GameAppId) - 1);
			}
			else
			{
				if (i != 0)
				{
					char szArg[128];
					wcstombs(szArg, g_Argv[i], sizeof(szArg) - 1);
					if (g_AdditionalProcName[0] != '\0') strcat(g_AdditionalProcName, " ");
					strcat(g_AdditionalProcName, szArg);
				}
			}
		}
		LocalFree(g_Argv);
	}

	if (g_ProcName[0] == '\0')
	{
		GetPrivateProfileStringA("Loader", "ProcName", "", g_ProcName, sizeof(g_ProcName), g_RevIniName);
	}

	GetPrivateProfileStringA("Loader", "ProcArgs", "", g_ProcArgs, sizeof(g_ProcArgs), g_RevIniName);

	if (strlen(g_AdditionalProcName) != 0)
	{
		if (g_ProcArgs[0] != '\0') strcat(g_ProcArgs, " ");
		strcat(g_ProcArgs, g_AdditionalProcName);
	}

	if (g_ProcName[0] == '\0')
	{
		MessageBoxA(HWND_DESKTOP, "ProcName value not found on command line or in rev.ini. Please edit the file.", 
			"Error", MB_ICONWARNING | MB_SYSTEMMODAL);
		return -1;
	}

	// AppID Kontrolü (steam_appid.txt öncelikli)
	if (g_GameAppId[0] == '\0')
	{
		if (!GetSteamAppID(g_GameAppId, sizeof(g_GameAppId)))
		{
			GetPrivateProfileStringA("Steam", "AppId", "", g_GameAppId, sizeof(g_GameAppId), g_RevIniName);
			if (g_GameAppId[0] == '\0')
			{
				GetPrivateProfileStringA("Loader", "AppId", "", g_GameAppId, sizeof(g_GameAppId), g_RevIniName);
			}
		}
	}

	if (g_GameAppId[0] != '\0')
	{
		SetEnvironmentVariableA("SteamGameId", g_GameAppId);
		SetEnvironmentVariableA("SteamAppId", g_GameAppId);
		SetEnvironmentVariableA("SteamOverlayGameId", g_GameAppId);
	}
	SetEnvironmentVariableA("SteamPath", g_LauncherDir);

	// 1. SteamDll (veya SteamClientDll) Yükleme
	char szSteamDll[MAX_PATH] = {};
	GetPrivateProfileStringA("Loader", "SteamDll", "", szSteamDll, sizeof(szSteamDll), g_RevIniName);
	if (szSteamDll[0] == '\0')
	{
		GetPrivateProfileStringA("Loader", "SteamClientDll", "", szSteamDll, sizeof(szSteamDll), g_RevIniName);
	}

	if (szSteamDll[0] != '\0')
	{
		char szRawPath[MAX_PATH];
		sprintf(szRawPath, "%s%s", g_LauncherDir, szSteamDll);
		GetFullPathNameA(szRawPath, MAX_PATH, g_LibraryName, NULL);

		if (!LoadLibraryA(g_LibraryName))
		{
			char szDest[512];
			sprintf(szDest, "Can't find %s relative to executable path %s", szSteamDll, g_LauncherDir);
			MessageBoxA(HWND_DESKTOP, szDest, "Warning", MB_ICONWARNING | MB_SYSTEMMODAL);
			return -1;
		}

		SetSteamClientDll(g_LibraryName);
	}

	// 2. Orijinal steam.dll Yükleme
	char szSteamDllPath[MAX_PATH];
	char szRawSteamPath[MAX_PATH];
	sprintf(szRawSteamPath, "%ssteam.dll", g_LauncherDir);
	GetFullPathNameA(szRawSteamPath, MAX_PATH, szSteamDllPath, NULL);

	if (!LoadLibraryA(szSteamDllPath))
	{
		char szDest[512];
		sprintf(szDest, "Can't find steam.dll relative to executable path %s", g_LauncherDir);
		MessageBoxA(HWND_DESKTOP, szDest, "Warning", MB_ICONWARNING | MB_SYSTEMMODAL);
		return -1;
	}

	StartGameApp();

	return 0;
}
