;
;   This file is part of Poedit (https://poedit.net)
;
;   Copyright (C) 1999-2023 Vaclav Slavik
;
;   Permission is hereby granted, free of charge, to any person obtaining a
;   copy of this software and associated Docsumentation files (the "Software"),
;   to deal in the Software without restriction, including without limitation
;   the rights to use, copy, modify, merge, publish, distribute, sublicense,
;   and/or sell copies of the Software, and to permit persons to whom the
;   Software is furnished to do so, subject to the following conditions:
;
;   The above copyright notice and this permission notice shall be included in
;   all copies or substantial portions of the Software.
;
;   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
;   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
;   DEALINGS IN THE SOFTWARE.
;
;   Inno Setup installer script
;

#ifndef CONFIG
#define CONFIG           "Release"
#endif

#define BINDIR 		     "x64\" + CONFIG

#include "../" + BINDIR + "/git_build_number.h"

#define VERSION          "3.5"
#define VERSION_WIN      VERSION + "." + Str(POEDIT_GIT_BUILD_NUMBER)

#define APP_ID           "{68EB2C37-083A-4303-B5D8-41FA67E50B8F}"

#ifndef CRT_REDIST
#define CRT_REDIST       GetEnv("VCToolsRedistDir") + "\x64\Microsoft.VC143.CRT"
#endif

[Setup]
OutputBaseFilename=Poedit-{#VERSION}-setup
OutputDir=win32

AppName=Poedit
AppVerName=Poedit {#VERSION}

ArchitecturesInstallIn64BitMode=x64 arm64
ArchitecturesAllowed=x64 arm64

ChangesAssociations=true
AlwaysShowComponentsList=false
SourceDir=..
DefaultDirName={commonpf}\Poedit
DefaultGroupName=Poedit
AllowNoIcons=true
LicenseFile=COPYING
InfoAfterFile=
SolidCompression=true
Compression=lzma2/ultra64
WindowShowCaption=true
WindowStartMaximized=false
FlatComponentsList=true
WindowResizable=true
ShowLanguageDialog=no
DisableWelcomePage=true
AllowUNCPath=true
InternalCompressLevel=ultra
AppID={{#APP_ID}
VersionInfoVersion={#VERSION_WIN}
VersionInfoTextVersion={#VERSION}
AppCopyright=Copyright © 1999-2023 Vaclav Slavik
AppPublisher=Vaclav Slavik
AppSupportURL=https://poedit.net/support
AppUpdatesURL=https://poedit.net/download
AppVersion={#VERSION}
AppContact=help@poedit.net
UninstallDisplayIcon={app}\Poedit.exe
UninstallDisplayName=Poedit
MinVersion=6.1sp1
WizardSmallImageFile=artwork\windows\installer_wizard_image.bmp
WizardStyle=modern
AppPublisherURL=https://poedit.net/
DisableProgramGroupPage=true

#ifdef SIGNTOOL
SignTool={#SIGNTOOL}
SignedUninstaller=yes
#endif
VersionInfoCompany=Vaclav Slavik
VersionInfoDescription=Poedit Installer
VersionInfoCopyright=Copyright © 1999-2023 Vaclav Slavik
VersionInfoProductName=Poedit
VersionInfoProductVersion={#VERSION_WIN}
VersionInfoProductTextVersion={#VERSION}
DisableDirPage=auto

[Files]
Source: win32\uninst-helper.dll; Flags: dontcopy signonce
Source: {#BINDIR}\Poedit.exe; DestDir: {app}; Flags: ignoreversion signonce
Source: {#BINDIR}\*.dll; DestDir: {app}; Flags: ignoreversion signonce
Source: COPYING; DestDir: {app}\Docs; DestName: Copying.txt
Source: NEWS; DestDir: {app}\Docs; DestName: News.txt
Source: {#CRT_REDIST}\*.dll; DestDir: {app}
Source: "{#BINDIR}\Resources\*"; DestDir: "{app}\Resources"; Flags: recursesubdirs
Source: "{#BINDIR}\Translations\*"; DestDir: "{app}\Translations"; Flags: recursesubdirs
Source: "{#BINDIR}\GettextTools\bin\*"; DestDir: "{app}\GettextTools\bin"; Flags: ignoreversion signonce
Source: "{#BINDIR}\GettextTools\share\*"; DestDir: "{app}\GettextTools\share"; Flags: ignoreversion recursesubdirs

[InstallDelete]
; Remove all files from previous version to have known clean state
Type: filesandordirs; Name: {group}\*;

[Registry]
; Install global settings:
Root: "HKLM"; Subkey: "Software\Vaclav Slavik\Poedit\WinSparkle"; ValueType: string; ValueName: "CheckForUpdates"; ValueData: "1"; Flags: noerror

; Uninstall Poedit settings on uninstall:
Root: "HKLM"; Subkey: "Software\Vaclav Slavik\Poedit"; Flags: noerror uninsdeletekey
Root: "HKLM"; Subkey: "Software\Vaclav Slavik"; Flags: noerror uninsdeletekeyifempty

; Associate files with Poedit:
Root: "HKA"; Subkey: "Software\Classes\.po"; ValueType: string; ValueData: "Poedit.PO"; Flags: noerror
Root: "HKA"; Subkey: "Software\Classes\.po\OpenWithProgids"; ValueType: string; ValueName: "Poedit.PO"; ValueData: ""; Flags: uninsdeletevalue
Root: "HKA"; Subkey: "Software\Classes\Poedit.PO"; ValueType: string; ValueData: "PO Translation"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.PO"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\Poedit.exe,-222"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.PO\Shell\Open\Command"; ValueType: string; ValueData: """{app}\Poedit.exe"" ""%1"""; Flags: uninsdeletevalue noerror

Root: "HKA"; Subkey: "Software\Classes\.mo"; ValueType: string; ValueData: "Poedit.MO"; Flags: noerror
Root: "HKA"; Subkey: "Software\Classes\.mo\OpenWithProgids"; ValueType: string; ValueName: "Poedit.MO"; ValueData: ""; Flags: uninsdeletevalue noerror
Root: "HKA"; Subkey: "Software\Classes\.gmo"; ValueType: string; ValueData: "Poedit.MO"; Flags: noerror
Root: "HKA"; Subkey: "Software\Classes\.gmo\OpenWithProgids"; ValueType: string; ValueName: "Poedit.MO"; ValueData: ""; Flags: uninsdeletevalue noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.MO"; ValueType: string; ValueData: "Compiled Translation"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.MO"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\Poedit.exe,-223"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.MO\Shell\Open\Command"; ValueType: string; ValueData: """{app}\Poedit.exe"" ""%1"""; Flags: uninsdeletevalue noerror

Root: "HKA"; Subkey: "Software\Classes\.pot\OpenWithProgids"; ValueType: string; ValueName: "Poedit.POT"; ValueData: ""; Flags: uninsdeletevalue noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.POT"; ValueType: string; ValueData: "Translation Template"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.POT"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\Poedit.exe,-224"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.POT\Shell\Open\Command"; ValueType: string; ValueData: """{app}\Poedit.exe"" ""%1"""; Flags: uninsdeletevalue noerror

Root: "HKA"; Subkey: "Software\Classes\.xlf"; ValueType: string; ValueName: "Content Type"; ValueData: "application/x-xliff+xml"; Flags: noerror
Root: "HKA"; Subkey: "Software\Classes\.xlf\OpenWithProgids"; ValueType: string; ValueName: "Poedit.XLIFF"; ValueData: ""; Flags: uninsdeletevalue noerror
Root: "HKA"; Subkey: "Software\Classes\.xliff"; ValueType: string; ValueName: "Content Type"; ValueData: "application/x-xliff+xml"; Flags: noerror
Root: "HKA"; Subkey: "Software\Classes\.xliff\OpenWithProgids"; ValueType: string; ValueName: "Poedit.XLIFF"; ValueData: ""; Flags: uninsdeletevalue noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.XLIFF"; ValueType: string; ValueData: "XLIFF Translation"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.XLIFF"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\Poedit.exe,-225"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.XLIFF\Shell\Open\Command"; ValueType: string; ValueData: """{app}\Poedit.exe"" ""%1"""; Flags: uninsdeletevalue noerror

Root: "HKA"; Subkey: "Software\Classes\.json"; ValueType: string; ValueName: "Content Type"; ValueData: "application/json"; Flags: noerror
Root: "HKA"; Subkey: "Software\Classes\.json\OpenWithProgids"; ValueType: string; ValueName: "Poedit.JSON"; ValueData: ""; Flags: uninsdeletevalue noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.JSON"; ValueType: string; ValueData: "JSON Document"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.JSON"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\Poedit.exe,-226"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.JSON\Shell\Open\Command"; ValueType: string; ValueData: """{app}\Poedit.exe"" ""%1"""; Flags: uninsdeletevalue noerror

Root: "HKA"; Subkey: "Software\Classes\.arb\OpenWithProgids"; ValueType: string; ValueName: "Poedit.ARB"; ValueData: ""; Flags: uninsdeletevalue noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.ARB"; ValueType: string; ValueData: "Flutter Translation"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.ARB"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\Poedit.exe,-227"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\Poedit.ARB\Shell\Open\Command"; ValueType: string; ValueData: """{app}\Poedit.exe"" ""%1"""; Flags: uninsdeletevalue noerror

; URL protocol for poedit:// (various custom tasks such as OAuth)
Root: "HKA"; Subkey: "Software\Classes\poedit"; ValueType: "string"; ValueData: "URL:Poedit Custom Protocol"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\poedit"; ValueType: "string"; ValueName: "URL Protocol"; ValueData: ""; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\poedit\DefaultIcon"; ValueType: "string"; ValueData: "{app}\Poedit.exe,0"; Flags: uninsdeletekey noerror
Root: "HKA"; Subkey: "Software\Classes\poedit\shell\open\command"; ValueType: "string"; ValueData: """{app}\Poedit.exe"" --handle-poedit-uri ""%1"""; Flags: uninsdeletekey noerror

[Icons]
Name: {commonprograms}\Poedit; Filename: {app}\Poedit.exe; WorkingDir: {app}; IconIndex: 0; Comment: Translation editor.

[Run]
Filename: {app}\Poedit.exe; WorkingDir: {app}; Description: {cm:OpenAfterInstall}; Flags: postinstall nowait skipifsilent runasoriginaluser

[Dirs]
Name: {app}\Docs
Name: {app}\Resources
Name: {app}\Translations

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "catalan"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "corsican"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "hebrew"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"

[CustomMessages]
OpenAfterInstall=Open Poedit after installation
Uninstall32bit=Uninstalling 32-bit version of Poedit...
DownloadingUCRT=Poedit needs to download and install Microsoft Universal C Runtime support files, because they are missing on your computer.
InstallingUCRT=Installing Microsoft Universal C Runtime...
brazilianportuguese.OpenAfterInstall=Abrir o Poedit após a instalação
catalan.OpenAfterInstall=Obre el Poedit després de la instal·lació
corsican.OpenAfterInstall=Apre Poedit dopu à l'installazione
czech.OpenAfterInstall=Po instalaci otevřít Poedit
danish.OpenAfterInstall=Åbn Poedit efter installation
dutch.OpenAfterInstall=Poedit starten na installatie
finnish.OpenAfterInstall=Avaa Poedit asennuksen jälkeen
french.OpenAfterInstall=Ouvrir Poedit après l'installation
german.OpenAfterInstall=Poedit nach Abschluss der Installation öffnen
hebrew.OpenAfterInstall=פתח את Poedit לאחר ההתקנה
italian.OpenAfterInstall=Apri Poedit dopo l'installazione
japanese.OpenAfterInstall=インストール後 Poedit を開く
norwegian.OpenAfterInstall=Åpne Poedit etter installasjon
polish.OpenAfterInstall=Otwórz program Poedit po zakończeniu instalacji
portuguese.OpenAfterInstall=Abrir o Poedit após a instalação
russian.OpenAfterInstall=Открыть Poedit после окончания установки
slovenian.OpenAfterInstall=Odpri Poedit po namestitvi
spanish.OpenAfterInstall=Abrir Poedit tras la instalación
turkish.OpenAfterInstall=Kurulumdan sonra Poedit'i aç
ukrainian.OpenAfterInstall=Відкрити Poedit після встановлення

[Code]

{ -- Handle uninstallation of conflicting 32-bit version of Poedit -- }

const UninstallKey = 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{#APP_ID}_is1';

function SafelyUninstall32BitVersion(uninstallerExe: String): Integer;
external 'SafelyUninstall32BitVersion@files:uninst-helper.dll cdecl setuponly';


function Find32bitUninstallerPath(): String;
var
  uninst: String;
begin
  if (not RegKeyExists(HKCU64, UninstallKey)) and (not RegKeyExists(HKLM64, UninstallKey)) and RegKeyExists(HKLM32, UninstallKey) then begin
    RegQueryStringValue(HKLM32, UninstallKey, 'UninstallString', uninst);
    Result := RemoveQuotes(uninst);
  end
  else
    Result := '';
end;


procedure Uninstall32bitVersionIfPresent;
var
  uninstaller: String;
begin
  uninstaller := Find32bitUninstallerPath();
  if (uninstaller <> '') then begin
    Log('Migrating from 32bit install, uninstalling via ' + uninstaller);
    WizardForm.StatusLabel.Caption := CustomMessage('Uninstall32bit');
    SafelyUninstall32BitVersion(uninstaller);
  end;
end;


{ -- Install UCRT on ancient Windows 7/8 -- }

var
  DownloadPage: TDownloadWizardPage;
  UCRTFilename, UCRTSha256: String;


procedure PrepareUCRTDownloadIfNeeded;
var
  Version: TWindowsVersion;
begin
  DownloadPage := nil;
  { Windows 10 always includes UCRT, older versions may or may not have it installed }
  GetWindowsVersionEx(Version);
  if (Version.Major = 6) then begin
    if (not FileExists(ExpandConstant('{sys}\ucrtbase.dll'))) then begin
      if (Version.Minor = 1) then begin
        UCRTFilename := 'Windows6.1-KB3118401-x64.msu';
        UCRTSha256 := '145623e0b85037b90e1ef5c45aee1aaa4120c4d12a388d94c48cfbb083e914e4';
      end
      else if (Version.Minor = 2) then begin
        UCRTFilename := 'Windows8-RT-KB3118401-x64.msu';
        UCRTSha256 := 'fc2fb2dd6f25739f7e0938b9d24fe590ee03e62de3b4132193f424f0bbb8b0fd';
      end
      else if (Version.Minor = 3) then begin
        UCRTFilename := 'Windows8.1-KB3118401-x64.msu';
        UCRTSha256 := '0e44ad74aa341909865dc6a72b2bcb80564fcd0df7e1e388be81a7e04868c98f';
      end;
      if (UCRTFilename <> '') then
        DownloadPage := CreateDownloadPage(SetupMessage(msgWizardPreparing), CustomMessage('DownloadingUCRT'), nil);
	  end;
  end;
end;


function DownloadUCRTIfNeeded(): Boolean;
begin
  Result := True;
  if (DownloadPage <> nil) then begin
    DownloadPage.Clear;
    DownloadPage.Add('https://download.poedit.net/ucrt/' + UCRTFilename, UCRTFilename, UCRTSha256);
    DownloadPage.Show;
    try
      try
        DownloadPage.Download;
      except
        if DownloadPage.AbortedByUser then
          Log('UCRT download aborted by user.')
        else
          SuppressibleMsgBox(AddPeriod(GetExceptionMessage), mbCriticalError, MB_OK, IDOK);
        Result := False;
      end;
    finally
      DownloadPage.Hide;
    end;
  end;
end;


procedure InstallUCRTIfNeeded;
var
  ResultCode: Integer;
begin
  if (UCRTFilename <> '') then begin
    WizardForm.StatusLabel.Caption := CustomMessage('InstallingUCRT');
    Exec('wusa.exe', UCRTFilename + ' /quiet /norestart', ExpandConstant('{tmp}'), SW_SHOWNORMAL, ewWaitUntilTerminated, ResultCode);
  end;
end;


{ -- Inno Setup hooks -- }

procedure InitializeWizard;
begin
  PrepareUCRTDownloadIfNeeded();
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if (CurPageID = wpReady) then
    Result := DownloadUCRTIfNeeded()
  else
    Result := True;
  
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssInstall) then begin
    InstallUCRTIfNeeded();
    Uninstall32bitVersionIfPresent();
  end;
end;
