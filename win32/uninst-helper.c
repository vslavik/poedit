/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2024 Vaclav Slavik
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _M_IX86 
#error This file must be compiled for 32bit x86 target
#endif

// This DLL is used by the uninstaller to remove Poedit's 32bit version during upgrades.
// 
// Inno Setup treats 64bit and 32bit versions as separate applications, so it can't
// upgrade existing install seamlessly; we need to remove the old version first.
// 
// Unfortunately, the installer for older versions of Poedit uninstalled HKCU registry
// keys, i.e. deleted user settings; that wouldn't be good during a simple upgrade.
// Hence this DLL: it is loaded by the uninstaller and it removes the old version,
// while taking care to preserve user settings.

#define UNICODE
#include <windows.h>
#pragma comment(lib, "advapi32.lib")


__declspec(dllexport) int SafelyUninstall32BitVersion(LPCWSTR uninstallerExe)
{
	// check if the uninstaller file exists:
	if (GetFileAttributesW(uninstallerExe) == INVALID_FILE_ATTRIBUTES)
		return 0;

	// rename the HKCU\Software\Vaclav Slavik\Poedit key to *.backup:
	HKEY key = 0;
	if (SUCCEEDED(RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Vaclav Slavik\\Poedit", 0, KEY_ALL_ACCESS, &key)))
	{
		if (FAILED(RegRenameKey(key, NULL, L"Poedit.backup")))
		{
			RegCloseKey(key);
			key = 0;
		}
	}

	// run the uninstaller:
	wchar_t cmdline[10240];
	wcscpy_s(cmdline, 10240, uninstallerExe);
	wcscat_s(cmdline, 10240, L" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART");

	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	if (CreateProcessW(uninstallerExe, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	// Inno Setup's uninstaller spawns a subprocess in order to be able to delete itself, so wait until
	// the uninstaller is removed (or 60 seconds pass, whichever comes first):
	for (int i = 0; i < 600; i++)
	{
		if (GetFileAttributesW(uninstallerExe) == INVALID_FILE_ATTRIBUTES)
			break;
		Sleep(100);
	}

	// rename the backup key back:
	if (key)
	{
		RegRenameKey(key, NULL, L"Poedit");
		RegCloseKey(key);
	}

	return 1;
}
