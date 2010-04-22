;
;   This file is part of Poedit (http://www.poedit.net)
;
;   Copyright (C) 1999-2010 Vaclav Slavik
;
;   Permission is hereby granted, free of charge, to any person obtaining a
;   copy of this software and associated documentation files (the "Software"),
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

#define VERSION          "1.5"
#define VERSION_FULL     "1.5beta1"

#ifndef CRT_REDIST
#define CRT_REDIST       "C:\Program Files\Microsoft Visual Studio 9.0\VC\redist\x86\Microsoft.VC90.CRT"
#endif

#include "poedit-locale-files.iss"

[Setup]
OutputBaseFilename=poedit-{#VERSION_FULL}-setup
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
OutputDir=distrib
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
AppCopyright=© 1999-2010 Vaclav Slavik
AppPublisher=Vaclav Slavik
AppSupportURL=http://www.poedit.net/support.php
AppUpdatesURL=http://www.poedit.net/download.php
AppVersion={#VERSION_FULL}
AppContact=poedit-users@lists.sourceforge.net
UninstallDisplayIcon={app}\bin\poedit.exe
UninstallDisplayName=Poedit
MinVersion=0,5.0.2195
WizardSmallImageFile=icons\win32\installer_wizard_image.bmp
AppPublisherURL=http://www.poedit.net/
DisableProgramGroupPage=true

#ifdef SIGNTOOL
SignTool={#SIGNTOOL}
#endif

[Files]
Source: win32\Release\poedit.exe; DestDir: {app}\bin; DestName: poedit.exe
Source: deps\gettext\COPYING; DestDir: {app}\doc; DestName: GNU_Gettext_COPYING.txt
Source: deps\bin-gettext\bin\xgettext.exe; DestDir: {app}\bin
Source: deps\bin-gettext\bin\msgmerge.exe; DestDir: {app}\bin
Source: deps\bin-gettext\bin\msgunfmt.exe; DestDir: {app}\bin
Source: deps\bin-gettext\bin\msgfmt.exe; DestDir: {app}\bin
Source: deps\bin-gettext\bin\msgcat.exe; DestDir: {app}\bin
Source: deps\bin-gettext\bin\*.dll; DestDir: {app}\bin
Source: deps\db\build_windows\Win32\Release\libdb*.dll; DestDir: {app}\bin
Source: deps\winsparkle\WinSparkle.dll; DestDir: {app}\bin
Source: COPYING; DestDir: {app}\doc; DestName: copying.txt
Source: NEWS; DestDir: {app}\doc; DestName: news.txt
Source: icons\ui\*.png; DestDir: {app}\share\poedit\icons
Source: icons\win32\xp\*.ico; DestDir: {app}\share\poedit\icons; OnlyBelowVersion: 0,6.0.6000
Source: icons\win32\vista\*.ico; DestDir: {app}\share\poedit\icons; MinVersion: 0,6.0.6000
Source: {#CRT_REDIST}\Microsoft.*.CRT.manifest; DestDir: {app}\bin
Source: {#CRT_REDIST}\*.dll; DestDir: {app}\bin
#emit LocaleFiles

[InstallDelete]
; delete files from previous versions that are no longer needed (and in case of
; poedit.exe.manifest, actually harmful):
Name: {app}\bin\poedit.exe.manifest; Type: files
Name: {app}\bin\gettextlib.dll; Type: files
Name: {app}\bin\gettextsrc.dll; Type: files
Name: {app}\bin\iconv.dll; Type: files
Name: {app}\bin\intl.dll; Type: files
Name: {app}\bin\mingwm10.dll; Type: files

[Registry]
Root: HKCR; SubKey: .po; ValueType: string; ValueData: GettextFile; Flags: uninsdeletekey noerror
Root: HKCR; SubKey: GettextFile; ValueType: string; ValueData: Gettext message catalog; Flags: uninsdeletekey noerror
Root: HKCR; SubKey: GettextFile\Shell\Open\Command; ValueType: string; ValueData: """{app}\bin\poedit.exe"" ""%1"""; Flags: uninsdeletevalue noerror
Root: HKCR; Subkey: GettextFile\DefaultIcon; ValueType: string; ValueData: {app}\share\poedit\icons\poedit-translation-generic.ico; Flags: uninsdeletekey noerror
Root: HKCU; Subkey: Software\Vaclav Slavik; Flags: uninsdeletekeyifempty dontcreatekey
Root: HKCU; Subkey: Software\Vaclav Slavik\poedit; Flags: uninsdeletekey dontcreatekey

[Icons]
Name: {commonprograms}\Poedit; Filename: {app}\bin\poedit.exe; WorkingDir: {app}; IconIndex: 0

[Run]
Filename: {app}\bin\poedit.exe; WorkingDir: {app}; Description: Open Poedit after installation; Flags: postinstall unchecked nowait skipifsilent runasoriginaluser

[_ISTool]
UseAbsolutePaths=false

[Dirs]
Name: {app}\bin
Name: {app}\doc
Name: {app}\share
Name: {app}\share\locale
Name: {app}\share\poedit
Name: {app}\share\poedit\icons
Name: {app}\share\poedit\help
Name: {app}\share\poedit\help\en
Name: {app}\share\poedit\help\en\gettext
Name: {app}\share\poedit\help\hr

[Messages]
BeveledLabel=http://www.poedit.net
