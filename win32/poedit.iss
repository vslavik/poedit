;
;   This file is part of Poedit (http://www.poedit.net)
;
;   Copyright (C) 1999-2008 Vaclav Slavik
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
;   $Id$
;
;   Inno Setup installer script
;

#define VERSION          "1.4"
#define VERSION_FULL     "1.4"

; FIXME: extract this from manifest file somehow?
#define CRT_VERSION      "8.0.50727.762"
#define CRT_HASH1        "1fc8b3b9a1e18e3b"
#define CRT_HASH2        "6b128700"
#define ASSEMBLY         "x86_Microsoft.VC80.CRT_" + CRT_HASH1 + "_" + CRT_VERSION + "_x-ww_" + CRT_HASH2
#define WINDIR           GetEnv("WINDIR")

#include "poedit-locale-files.iss"

[Setup]
OutputBaseFilename=poedit-{#VERSION_FULL}-setup
AppName=Poedit
AppVerName=Poedit {#VERSION_FULL}

PrivilegesRequired=none
ChangesAssociations=true
AlwaysShowComponentsList=true
SourceDir=..
DefaultDirName={pf}\Poedit
DefaultGroupName=Poedit
AllowNoIcons=true
LicenseFile=COPYING
OutputDir=.
InfoAfterFile=
Compression=lzma/ultra
WindowShowCaption=true
WindowStartMaximized=false
FlatComponentsList=true
WindowResizable=true
SolidCompression=true
ShowLanguageDialog=no
AllowUNCPath=true
InternalCompressLevel=ultra
AppID={{68EB2C37-083A-4303-B5D8-41FA67E50B8F}
VersionInfoTextVersion={#VERSION_FULL}
VersionInfoCopyright=© 1999-2008 Vaclav Slavik
VersionInfoVersion={#VERSION}
VersionInfoCompany=Vaclav Slavik
VersionInfoDescription=Installer for Poedit
AppCopyright=© 1999-2008 Vaclav Slavik
AppPublisher=Vaclav Slavik
AppSupportURL=http://www.poedit.net/support.php
AppUpdatesURL=http://www.poedit.net/download.php
AppVersion={#VERSION_FULL}
AppContact=poedit-users@lists.sourceforge.net
UninstallDisplayIcon={app}\bin\poedit.exe
UninstallDisplayName=Poedit
MinVersion=0,4.0.1381
WizardSmallImageFile=icons\win32\installer_wizard_image.bmp

[LangOptions]
; make the installer use standard (WinXP, ...) fonts
DialogFontName=Tahoma
DialogFontSize=8

[Files]
Source: win32\poedit.exe; DestDir: {app}\bin; DestName: poedit.exe; Components: core
Source: deps\gettext\COPYING; DestDir: {app}\doc; Components: docs; DestName: GNU_Gettext_COPYING.txt
Source: deps\gettext\xgettext.exe; DestDir: {app}\bin; Components: core
Source: deps\gettext\msgmerge.exe; DestDir: {app}\bin; Components: core
Source: deps\gettext\msgunfmt.exe; DestDir: {app}\bin; Components: core
Source: deps\gettext\msgfmt.exe; DestDir: {app}\bin; Components: core
Source: deps\gettext\msgcat.exe; DestDir: {app}\bin; Components: core
Source: deps\gettext\libintl-*.dll; DestDir: {app}\bin; Components: core
Source: deps\gettext\libgettextlib-*.dll; DestDir: {app}\bin; Components: core
Source: deps\gettext\libgettextsrc-*.dll; DestDir: {app}\bin; Components: core
Source: deps\gettext\libcharset-*.dll; DestDir: {app}\bin; Components: core
Source: deps\gettext\libiconv-*.dll; DestDir: {app}\bin; Components: core
Source: deps\gettext\mingwm10.dll; DestDir: {app}\bin; Components: core
Source: deps\gettext\pthreadGC2.dll; DestDir: {app}\bin; Components: core
Source: README; DestDir: {app}\doc; DestName: readme.txt; Components: docs
Source: docs\chm\poedit.chm; DestDir: {app}\share\poedit\help\en; Components: docs
Source: docs\chm\gettext.chm; DestDir: {app}\share\poedit\help\en\gettext; Components: docs
Source: docs\chm\poedit-hr.chm; DestDir: {app}\share\poedit\help\hr; Components: i18n
Source: COPYING; DestDir: {app}\doc; DestName: copying.txt; Components: docs
Source: NEWS; DestDir: {app}\doc; DestName: news.txt; Components: docs
Source: icons\ui\*.png; DestDir: {app}\share\poedit\icons; Components: core
Source: icons\win32\xp\*.ico; DestDir: {app}\share\poedit\icons; Components: core; OnlyBelowVersion: 0,6.0.6000
Source: icons\win32\vista\*.ico; DestDir: {app}\share\poedit\icons; Components: core; MinVersion: 0,6.0.6000
Source: {#WINDIR}\WinSxS\Manifests\{#ASSEMBLY}.manifest; DestDir: {app}\bin; Components: core; DestName: Microsoft.VC80.CRT.manifest
Source: {#WINDIR}\WinSxS\{#ASSEMBLY}\*.dll; DestDir: {app}\bin; Components: core
#emit LocaleFiles

[InstallDelete]
; delete files from previous versions that are no longer needed (and in case of
; poedit.exe.manifest, actually harmful):
Name: {app}\bin\poedit.exe.manifest; Type: files; Components: core
Name: {app}\bin\gettextlib.dll; Type: files; Components: core
Name: {app}\bin\gettextsrc.dll; Type: files; Components: core
Name: {app}\bin\iconv.dll; Type: files; Components: core
Name: {app}\bin\intl.dll; Type: files; Components: core
Name: {app}\bin\mingwm10.dll; Type: files; Components: core

[Registry]
Root: HKCR; SubKey: .po; ValueType: string; ValueData: GettextFile; Flags: uninsdeletekey noerror
Root: HKCR; SubKey: GettextFile; ValueType: string; ValueData: Gettext message catalog; Flags: uninsdeletekey noerror
Root: HKCR; SubKey: GettextFile\Shell\Open\Command; ValueType: string; ValueData: """{app}\bin\poedit.exe"" ""%1"""; Flags: uninsdeletevalue noerror
Root: HKCR; Subkey: GettextFile\DefaultIcon; ValueType: string; ValueData: {app}\share\poedit\icons\poedit-translation-generic.ico; Flags: uninsdeletekey noerror
Root: HKCU; Subkey: Software\Vaclav Slavik; Flags: uninsdeletekeyifempty dontcreatekey
Root: HKCU; Subkey: Software\Vaclav Slavik\poedit; Flags: uninsdeletekey dontcreatekey

[Icons]
Name: {group}\Poedit; Filename: {app}\bin\poedit.exe; WorkingDir: {app}; IconIndex: 0
Name: {group}\Readme; Filename: {app}\doc\readme.txt; IconIndex: 0

[Run]
Filename: {app}\doc\readme.txt; Description: View readme.txt; Flags: shellexec postinstall unchecked; Components: docs
Filename: {app}\bin\poedit.exe; WorkingDir: {app}; Description: Run Poedit now; Flags: postinstall unchecked nowait

[_ISTool]
UseAbsolutePaths=false

[Dirs]
Name: {app}\bin; Components: core
Name: {app}\doc; Components: docs
Name: {app}\share; Components: core
Name: {app}\share\locale; Components: i18n
Name: {app}\share\poedit; Components: core
Name: {app}\share\poedit\icons; Components: core
Name: {app}\share\poedit\help; Components: docs
Name: {app}\share\poedit\help\en; Components: docs
Name: {app}\share\poedit\help\en\gettext; Components: docs
Name: {app}\share\poedit\help\hr; Components: docs

[Components]
Name: core; Description: Core files; Flags: fixed; Types: custom compact full
Name: docs; Description: Documentation; Types: custom compact full
Name: i18n; Description: Localization files for the UI; Types: full

[Messages]
BeveledLabel=http://www.poedit.net

