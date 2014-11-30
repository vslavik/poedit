;
;   This file is part of Poedit (http://poedit.net)
;
;   Copyright (C) 1999-2014 Vaclav Slavik
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

#define VERSION          "1.7"
#define VERSION_FULL     "1.7beta3"

#ifndef CONFIG
#define Config           "Release"
#endif

#ifndef CRT_REDIST
#define CRT_REDIST       GetEnv("PROGRAMFILES") + "\Microsoft Visual Studio 12.0\VC\redist\x86\Microsoft.VC120.CRT"
#endif

[Setup]
OutputBaseFilename=Poedit-{#VERSION_FULL}-setup
OutputDir=win32\distrib-{#CONFIG}-{#VERSION_FULL}

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
AppCopyright=Copyright © 1999-2014 Vaclav Slavik
AppPublisher=Vaclav Slavik
AppSupportURL=http://poedit.net/support.php
AppUpdatesURL=http://poedit.net/download.php
AppVersion={#VERSION_FULL}
AppContact=help@poedit.net
UninstallDisplayIcon={app}\Poedit.exe
UninstallDisplayName=Poedit
MinVersion=0,5.01.2600sp3
WizardSmallImageFile=artwork\windows\installer_wizard_image.bmp
AppPublisherURL=http://poedit.net/
DisableProgramGroupPage=true

#ifdef SIGNTOOL
SignTool={#SIGNTOOL}
#endif
VersionInfoCompany=Vaclav Slavik
VersionInfoDescription=Poedit Installer
VersionInfoCopyright=Copyright © 1999-2014 Vaclav Slavik
VersionInfoProductName=Poedit
VersionInfoProductVersion={#VERSION}
VersionInfoProductTextVersion={#VERSION_FULL}
DisableDirPage=auto

[Files]
Source: {#CONFIG}\Poedit.exe; DestDir: {app}; Flags: ignoreversion
Source: {#CONFIG}\dump-legacy-tm.exe; DestDir: {app}; Flags: ignoreversion
Source: {#CONFIG}\*.dll; DestDir: {app}; Flags: ignoreversion
Source: {#CONFIG}\icudt*.dat; DestDir: {app}
Source: deps\gettext\COPYING; DestDir: {app}\Docs; DestName: GNU_Gettext_COPYING.txt
Source: COPYING; DestDir: {app}\Docs; DestName: Copying.txt
Source: NEWS; DestDir: {app}\Docs; DestName: News.txt
Source: artwork\windows\xp\*.ico; DestDir: {app}\Resources; OnlyBelowVersion: 0,6.0.6000
Source: artwork\windows\vista\*.ico; DestDir: {app}\Resources; MinVersion: 0,6.0.6000
Source: {#CRT_REDIST}\*.dll; DestDir: {app}
Source: "{#CONFIG}\Resources\*"; DestDir: "{app}\Resources"; Flags: recursesubdirs
Source: "{#CONFIG}\Translations\*"; DestDir: "{app}\Translations"; Flags: recursesubdirs
Source: "{#CONFIG}\GettextTools\*"; DestDir: "{app}\GettextTools"; Flags: ignoreversion recursesubdirs

[InstallDelete]
; delete files from previous versions that are no longer needed (and in case of
; poedit.exe.manifest, actually harmful):
Type: filesandordirs; Name: "{app}\bin"
Type: filesandordirs; Name: "{app}\doc"
Type: filesandordirs; Name: "{app}\share"

[Registry]
Root: "HKCR"; Subkey: ".po"; ValueType: string; ValueData: "Poedit.PO"; Flags: noerror
Root: "HKCR"; Subkey: "Poedit.PO"; ValueType: string; ValueData: "PO Translation"; Flags: uninsdeletekey noerror
Root: "HKCR"; Subkey: "Poedit.PO"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\Poedit.exe,-222"; Flags: uninsdeletekey noerror
Root: "HKCR"; Subkey: "Poedit.PO\Shell\Open\Command"; ValueType: string; ValueData: """{app}\Poedit.exe"" ""%1"""; Flags: uninsdeletevalue noerror
Root: "HKCR"; Subkey: "Poedit.PO\DefaultIcon"; ValueType: string; ValueData: "{app}\Resources\poedit-translation-generic.ico"; Flags: uninsdeletekey noerror

Root: "HKCR"; Subkey: ".mo"; ValueType: string; ValueData: "Poedit.MO"; Flags: noerror
Root: "HKCR"; Subkey: ".gmo"; ValueType: string; ValueData: "Poedit.MO"; Flags: noerror
Root: "HKCR"; Subkey: "Poedit.MO"; ValueType: string; ValueData: "Compiled Translation"; Flags: uninsdeletekey noerror
Root: "HKCR"; Subkey: "Poedit.MO"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "@{app}\Poedit.exe,-223"; Flags: uninsdeletekey noerror
Root: "HKCR"; Subkey: "Poedit.MO\Shell\Open\Command"; ValueType: string; ValueData: """{app}\Poedit.exe"" ""%1"""; Flags: uninsdeletevalue noerror
Root: "HKCR"; Subkey: "Poedit.MO\DefaultIcon"; ValueType: string; ValueData: "{app}\Resources\poedit-translation-generic.ico"; Flags: uninsdeletekey noerror

Root: "HKCU"; Subkey: "Software\Vaclav Slavik"; Flags: uninsdeletekeyifempty dontcreatekey
Root: "HKCU"; Subkey: "Software\Vaclav Slavik\Poedit"; Flags: uninsdeletekey dontcreatekey

[Icons]
Name: {commonprograms}\Poedit; Filename: {app}\Poedit.exe; WorkingDir: {app}; IconIndex: 0; Comment: Translations editor.

[Run]
Filename: {app}\Poedit.exe; WorkingDir: {app}; Description: {cm:OpenAfterInstall}; Flags: postinstall nowait skipifsilent runasoriginaluser

[_ISTool]
UseAbsolutePaths=false

[Dirs]
Name: {app}\Docs
Name: {app}\Resources
Name: {app}\Translations

[Messages]
BeveledLabel=http://poedit.net

[ThirdParty]
CompileLogMethod=append

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
Name: "greek"; MessagesFile: "compiler:Languages\Greek.isl"
Name: "hebrew"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "hungarian"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "nepali"; MessagesFile: "compiler:Languages\Nepali.islu"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "serbiancyrillic"; MessagesFile: "compiler:Languages\SerbianCyrillic.isl"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"

[CustomMessages]
OpenAfterInstall=Open Poedit after installation
czech.OpenAfterInstall=Po instalaci otevřít Poedit
