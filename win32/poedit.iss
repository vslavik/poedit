;
;   This file is part of Poedit (http://www.poedit.net)
;
;   Copyright (C) 1999-2013 Vaclav Slavik
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

#define VERSION          "1.5.5"
#define VERSION_FULL     "1.5.5"

#ifndef CONFIG
#define Config           "Release"
#endif

#ifndef CRT_REDIST
#define CRT_REDIST       GetEnv("PROGRAMFILES") + "\Microsoft Visual Studio 11.0\VC\redist\x86\Microsoft.VC110.CRT"
#endif

[Setup]
OutputBaseFilename=Poedit-{#VERSION_FULL}-setup
AppName=Poedit
AppVerName=Poedit {#VERSION_FULL}

PrivilegesRequired=none
ChangesAssociations=true
AlwaysShowComponentsList=false
SourceDir=..
DefaultDirName={pf}\Poedit
DefaultGroupName=Poedit
AllowNoIcons=true
LicenseFile=COPYING
OutputDir={#CONFIG}
InfoAfterFile=
Compression=lzma2
WindowShowCaption=true
WindowStartMaximized=false
FlatComponentsList=true
WindowResizable=true
SolidCompression=true
ShowLanguageDialog=no
DisableWelcomePage=true
AllowUNCPath=true
InternalCompressLevel=ultra
AppID={{68EB2C37-083A-4303-B5D8-41FA67E50B8F}
VersionInfoVersion={#VERSION}
VersionInfoTextVersion={#VERSION_FULL}
AppCopyright=© 1999-2013 Vaclav Slavik
AppPublisher=Vaclav Slavik
AppSupportURL=http://www.poedit.net/support.php
AppUpdatesURL=http://www.poedit.net/download.php
AppVersion={#VERSION_FULL}
AppContact=support@poedit.net
UninstallDisplayIcon={app}\bin\poedit.exe
UninstallDisplayName=Poedit
MinVersion=0,5.01.2600sp3
WizardSmallImageFile=icons\installer_wizard_image.bmp
AppPublisherURL=http://www.poedit.net/
DisableProgramGroupPage=true

#ifdef SIGNTOOL
SignTool={#SIGNTOOL}
#endif

[Files]
Source: {#CONFIG}\*.exe; DestDir: {app}; Flags: ignoreversion
Source: {#CONFIG}\*.dll; DestDir: {app}; Flags: ignoreversion
Source: deps\gettext\COPYING; DestDir: {app}\Docs; DestName: GNU_Gettext_COPYING.txt
Source: COPYING; DestDir: {app}\Docs; DestName: Copying.txt
Source: NEWS; DestDir: {app}\Docs; DestName: News.txt
Source: icons\mime-win32\xp\*.ico; DestDir: {app}\Resources; OnlyBelowVersion: 0,6.0.6000
Source: icons\mime-win32\vista\*.ico; DestDir: {app}\Resources; MinVersion: 0,6.0.6000
Source: {#CRT_REDIST}\*.dll; DestDir: {app}
Source: "{#CONFIG}\Resources\*"; DestDir: "{app}\Resources"; Flags: recursesubdirs
Source: "{#CONFIG}\Translations\*"; DestDir: "{app}\Translations"; Flags: recursesubdirs

[InstallDelete]
; delete files from previous versions that are no longer needed (and in case of
; poedit.exe.manifest, actually harmful):
Type: filesandordirs; Name: "{app}\bin"
Type: filesandordirs; Name: "{app}\doc"
Type: filesandordirs; Name: "{app}\share"

[Registry]
Root: HKCR; SubKey: .po; ValueType: string; ValueData: GettextFile; Flags: uninsdeletekey noerror
Root: HKCR; SubKey: GettextFile; ValueType: string; ValueData: Gettext message catalog; Flags: uninsdeletekey noerror
Root: HKCR; SubKey: GettextFile\Shell\Open\Command; ValueType: string; ValueData: """{app}\Poedit.exe"" ""%1"""; Flags: uninsdeletevalue noerror
Root: HKCR; Subkey: GettextFile\DefaultIcon; ValueType: string; ValueData: {app}\Resources\poedit-translation-generic.ico; Flags: uninsdeletekey noerror
Root: HKCU; Subkey: Software\Vaclav Slavik; Flags: uninsdeletekeyifempty dontcreatekey
Root: HKCU; Subkey: Software\Vaclav Slavik\poedit; Flags: uninsdeletekey dontcreatekey

[Icons]
Name: {commonprograms}\Poedit; Filename: {app}\Poedit.exe; WorkingDir: {app}; IconIndex: 0; Comment: Translate applications and other message catalogs.

[Run]
Filename: {app}\Poedit.exe; WorkingDir: {app}; Description: Open Poedit after installation; Flags: postinstall nowait skipifsilent runasoriginaluser

[_ISTool]
UseAbsolutePaths=false

[Dirs]
Name: {app}\Docs
Name: {app}\Resources
Name: {app}\Translations

[Messages]
BeveledLabel=http://www.poedit.net


[ThirdParty]
CompileLogMethod=append
