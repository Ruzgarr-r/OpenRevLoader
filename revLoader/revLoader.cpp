#define _CRT_SECURE_NO_WARNINGS	

#include <Windows.h>
#include <gdiplus.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <io.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;

char g_LauncherDir[MAX_PATH] = {};
char g_RevIniName[MAX_PATH] = {};
char g_ProcName[MAX_PATH] = {};
char g_LibraryName[MAX_PATH] = {};
char g_GameAppId[256] = {};

wchar_t **g_Argv = nullptr;
char g_AdditionalProcName[MAX_PATH] = {};
int g_NumArgs = 0;

// Multi-Account Değişkenleri
std::vector<std::string> g_AccountList;
std::string g_SelectedAccount = "";
bool g_AccountSelected = false;

// steam_appid.txt okuma
bool GetSteamAppID(char *pszOut)
{
	FILE* f = fopen("steam_appid.txt", "r");
	if (!f)
	{
		*pszOut = '\0';
		return false;
	}

	int fno = _fileno(f);
	int flen = _filelength(fno);
	fread(pszOut, sizeof(pszOut[0]), flen, f);
	fclose(f);

	char *psz = strchr(pszOut, ' ');
	if (psz)
	{
		*psz = '\0';
	}

	return true;
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
	}
	
	*hMapView = MapViewOfFile(*hFileMap, SECTION_ALL_ACCESS, 0, 0, 0);

	if (!*hMapView)
	{
		sprintf(szDest, "Unable to MapViewOfFile: %i", GetLastError());
		MessageBoxA(HWND_DESKTOP, szDest, "Error", MB_OK);
		CloseHandle(*hFileMap);
	}

	*hEvent = CreateEventA(NULL, FALSE, FALSE, "Local\\SteamStart_SharedMemLock");

	if (!*hEvent)
	{
		sprintf(szDest, "Unable to CreateEvent: %i", GetLastError());
		MessageBoxA(HWND_DESKTOP, szDest, "Error", MB_OK);
		CloseHandle(*hFileMap);
		CloseHandle(*hMapView);
	}

	SetEvent(*hEvent);
}

void SetActiveProcess(int pid)
{
	DWORD dwD;
	HKEY phkResult;

	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, KEY_WRITE, &phkResult))
		RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, NULL, 0, KEY_WRITE, NULL, &phkResult, &dwD);

	RegSetValueExA(phkResult, "pid", 0, REG_DWORD, (BYTE *)&pid, sizeof(pid));
	RegCloseKey(phkResult);
}

void SetSteamClientDll(char *pszLib)
{
	DWORD dwD;
	HKEY phkResult;

	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, KEY_WRITE, &phkResult))
		RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, NULL, 0, KEY_WRITE, NULL, &phkResult, &dwD);

	RegSetValueExA(phkResult, "SteamClientDll", 0, REG_SZ, (BYTE *)pszLib, strlen(pszLib) + 1);
	RegCloseKey(phkResult);
}

// Oyuncu Seçim Penceresi WndProc
LRESULT CALLBACK AccountPickerProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		Graphics graphics(hdc);

		// Arka planı koyu gri yap
		SolidBrush bgBrush(Color(255, 30, 30, 30));
		graphics.FillRectangle(&bgBrush, 0, 0, 800, 600);

		FontFamily fontFamily(L"Segoe UI");
		Gdiplus::Font font(&fontFamily, 11, FontStyleBold, UnitPixel);
		SolidBrush textBrush(Color(255, 240, 240, 240));
		SolidBrush defaultAvatarBrush(Color(255, 70, 70, 70));
		Pen borderPen(Color(255, 100, 100, 100), 2);

		int xOffset = 30;
		int yOffset = 30;
		int boxWidth = 110;
		int boxHeight = 110;
		int padding = 25;

		for (size_t i = 0; i < g_AccountList.size(); i++)
		{
			// Avatar Yolu: .\steam\OYUNCU.jpg
			std::string imgPathStr = std::string(g_LauncherDir) + "steam\\" + g_AccountList[i] + ".jpg";
			std::wstring wImgPath(imgPathStr.begin(), imgPathStr.end());

			Image image(wImgPath.c_str());

			// Kare Avatar Çizimi
			if (image.GetLastStatus() == Ok)
			{
				graphics.DrawImage(&image, xOffset, yOffset, boxWidth, boxHeight);
			}
			else
			{
				// Avatar yoksa gri varsayılan kutu
				graphics.FillRectangle(&defaultAvatarBrush, xOffset, yOffset, boxWidth, boxHeight);
			}

			// Çerçeve
			graphics.DrawRectangle(&borderPen, xOffset, yOffset, boxWidth, boxHeight);

			// Oyuncu İsmi Metni
			std::wstring wName(g_AccountList[i].begin(), g_AccountList[i].end());
			RectF textRect((REAL)xOffset - 10, (REAL)(yOffset + boxHeight + 5), (REAL)(boxWidth + 20), 25.0f);
			
			StringFormat format;
			format.SetAlignment(StringAlignmentCenter);
			graphics.DrawString(wName.c_str(), -1, &font, textRect, &format, &textBrush);

			xOffset += boxWidth + padding;
			if (xOffset + boxWidth > 580)
			{
				xOffset = 30;
				yOffset += boxHeight + padding + 25;
			}
		}

		EndPaint(hWnd, &ps);
		break;
	}
	case WM_LBUTTONDOWN:
	{
		int xPos = LOWORD(lParam);
		int yPos = HIWORD(lParam);

		int xOffset = 30;
		int yOffset = 30;
		int boxWidth = 110;
		int boxHeight = 110;
		int padding = 25;

		for (size_t i = 0; i < g_AccountList.size(); i++)
		{
			if (xPos >= xOffset && xPos <= (xOffset + boxWidth) &&
				yPos >= yOffset && yPos <= (yOffset + boxHeight + 25))
			{
				g_SelectedAccount = g_AccountList[i];
				g_AccountSelected = true;
				DestroyWindow(hWnd);
				break;
			}

			xOffset += boxWidth + padding;
			if (xOffset + boxWidth > 580)
			{
				xOffset = 30;
				yOffset += boxHeight + padding + 25;
			}
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Oyuncu Seçim Diyaloğunu Başlatma
bool ShowAccountPicker()
{
	WNDCLASSEXA wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEXA);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = AccountPickerProc;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = "OpenRevAccountPicker";

	RegisterClassExA(&wcex);

	int windowWidth = 600;
	int windowHeight = 350;

	HWND hWnd = CreateWindowExA(
		WS_EX_TOPMOST,
		"OpenRevAccountPicker",
		"Select Profile - OpenRevLoader",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		(GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2,
		(GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2,
		windowWidth, windowHeight,
		NULL, NULL, GetModuleHandle(NULL), NULL
	);

	if (!hWnd) return false;

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return g_AccountSelected;
}

void StartGameApp()
{
	HANDLE hFileMap = 0;
	HANDLE hMapView = 0;
	HANDLE hSteamMem = 0;
	CreateSharedMemFile(&hMapView, &hFileMap, &hSteamMem);

	STARTUPINFOA StartupInformation = {};
	PROCESS_INFORMATION ProcessInformation = {};

	StartupInformation.cb = sizeof(StartupInformation);

	if (CreateProcessA(NULL, g_ProcName, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInformation, &ProcessInformation))
	{
		SetActiveProcess(ProcessInformation.dwProcessId);

		WaitForSingleObject(ProcessInformation.hThread, INFINITE);
		if (hSteamMem)
			CloseHandle(hSteamMem);
		if (hMapView)
			CloseHandle(hMapView);
		if (hFileMap)
			CloseHandle(hFileMap);
	}
	else
	{
		char szDest[512];
		sprintf(szDest, "Unable to execute command %s (%d)", g_ProcName, GetLastError());
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
		MessageBoxA(HWND_DESKTOP, "Unable to initialize the process", "Error", MB_ICONWARNING | MB_SYSTEMMODAL);
		return -1;
	}

	char *psz = strrchr(g_LauncherDir, '\\') + 1;
	*psz = '\0';

	strcpy(g_RevIniName, g_LauncherDir);
	strcat(g_RevIniName, "rev.ini");

	// GDI+ Başlat
	ULONG_PTR gdiplusToken;
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// MultiAcc Kontrolü
	char szMultiAcc[16] = {};
	GetPrivateProfileStringA("steamclient", "MultiAcc", "False", szMultiAcc, sizeof(szMultiAcc), g_RevIniName);

	if (_stricmp(szMultiAcc, "True") == 0 || _stricmp(szMultiAcc, "1") == 0)
	{
		char szPlayerNames[1024] = {};
		GetPrivateProfileStringA("steamclient", "PlayerNames", "", szPlayerNames, sizeof(szPlayerNames), g_RevIniName);

		if (strlen(szPlayerNames) > 0)
		{
			std::stringstream ss(szPlayerNames);
			std::string item;
			while (std::getline(ss, item, ','))
			{
				// Boşlukları temizle
				size_t start = item.find_first_not_of(" \t");
				size_t end = item.find_last_not_of(" \t");
				if (start != std::string::npos && end != std::string::npos)
				{
					g_AccountList.push_back(item.substr(start, end - start + 1));
				}
			}
		}

		if (!g_AccountList.empty())
		{
			if (ShowAccountPicker())
			{
				// Seçilen oyuncu adını rev.ini içerisine kaydet
				WritePrivateProfileStringA("steamclient", "PlayerName", g_SelectedAccount.c_str(), g_RevIniName);
			}
			else
			{
				// Kullanıcı seçim yapmadan kapattıysa oyunu başlatma
				GdiplusShutdown(gdiplusToken);
				return 0;
			}
		}
	}

	g_Argv = CommandLineToArgvW(GetCommandLineW(), &g_NumArgs);

	for (int i = 0; i < g_NumArgs; i++)
	{
		if (_wcsicmp(g_Argv[i], L"-launch") == 0)
		{
			wcstombs(g_ProcName, g_Argv[i++ + 1], sizeof(g_ProcName) - 1);
		}
		else if (_wcsicmp(g_Argv[i], L"-appid") == 0)
		{
			wcstombs(g_GameAppId, g_Argv[i++ + 1], sizeof(g_GameAppId) - 1);
		}
		else
		{
			if (i != 0)
			{
				char szArg[128];
				wcstombs(szArg, g_Argv[i], sizeof(szArg) - 1);
				strcat(g_AdditionalProcName, szArg);
				strcat(g_AdditionalProcName, " ");
			}
		}
	}

	if (strlen(g_AdditionalProcName) != 0)
		strcat(g_ProcName, g_AdditionalProcName);

	if (!GetPrivateProfileStringA("Loader", "ProcName", "", g_ProcName, sizeof(g_ProcName), g_RevIniName))
	{
		MessageBoxA(HWND_DESKTOP, "ProcName value not found on command line or in rev.ini. Please edit the file.", 
			"Error", MB_ICONWARNING | MB_SYSTEMMODAL);
		GdiplusShutdown(gdiplusToken);
		return -1;
	}

	if (!GetSteamAppID(g_GameAppId))
	{
		MessageBoxA(HWND_DESKTOP, "No steam_appid.txt detected, the game might not launch correctly", 
			"Warning", MB_ICONWARNING | MB_SYSTEMMODAL);
	}

	if (g_GameAppId[0] != '\0')
	{
		SetEnvironmentVariableA("SteamGameId", g_GameAppId);
		SetEnvironmentVariableA("SteamAppId", g_GameAppId);
	}

	char szSteamClientDll[MAX_PATH];
	if (GetPrivateProfileStringA("Loader", "SteamClientDll", "", szSteamClientDll, sizeof(szSteamClientDll), g_RevIniName))
	{
		if (szSteamClientDll[0] != '\0')
		{
			strcpy(g_LibraryName, g_LauncherDir);
			strcat(g_LibraryName, szSteamClientDll);

			if (!LoadLibraryA(g_LibraryName))
			{
				char szDest[512];
				sprintf(szDest, "Can't find steamclient.dll relative to executable path %s", g_LauncherDir);
				MessageBoxA(HWND_DESKTOP, szDest, "Warning", MB_ICONWARNING | MB_SYSTEMMODAL);
				GdiplusShutdown(gdiplusToken);
				return -1;
			}

			SetSteamClientDll(g_LibraryName);
		}
	}

	strcpy(g_LibraryName, g_LauncherDir);
	strcat(g_LibraryName, "steam.dll");

	if (!LoadLibraryA(g_LibraryName))
	{
		char szDest[512];
		sprintf(szDest, "Can't find steam.dll relative to executable path %s", g_LauncherDir);
		MessageBoxA(HWND_DESKTOP, szDest, "Warning", MB_ICONWARNING | MB_SYSTEMMODAL);
		GdiplusShutdown(gdiplusToken);
		return -1;
	}

	StartGameApp();

	GdiplusShutdown(gdiplusToken);
	return 0;
}
