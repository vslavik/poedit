; This script was first created by ISTool
; http://www.lerstad.com/istool/

[Setup]
OutputBaseFilename=poedit1110_setup
AppName=poEdit
AppVerName=poEdit 1.1.9

ChangesAssociations=true
AlwaysShowComponentsList=true
SourceDir=..
DefaultDirName={pf}\poEdit

DefaultGroupName=poEdit
UninstallIconName=Uninstall poEdit
AllowNoIcons=true
AlwaysCreateUninstallIcon=true
DisableAppendDir=true
UninstallStyle=modern
WizardStyle=modern
LicenseFile=install\license.rtf
OutputDir=.
InfoAfterFile=install\news.rtf
Compression=bzip

WindowShowCaption=true
WindowStartMaximized=false
FlatComponentsList=true
WindowResizable=false



[Files]
Source: Bmingw\src\poedit; DestDir: {app}\bin; DestName: poedit.exe; Components: core
Source: extras\win32-gettext\gnu_gettext.COPYING; DestDir: {app}\doc; Components: docs
Source: install\readme.txt; DestDir: {app}\doc; DestName: readme.txt; Components: docs
Source: docs\poedit.chm; DestDir: {app}\share\poedit; Components: docs
Source: docs\gettext.chm; DestDir: {app}\share\poedit; Components: docs
Source: docs\poedit-hr.chm; DestDir: {app}\share\poedit; Components: i18n
Source: install\license.txt; DestDir: {app}\doc; DestName: license.txt; Components: docs
Source: install\news.txt; DestDir: {app}\doc; DestName: news.txt; Components: docs
Source: Bmingw\src\resources\resources.zip; DestDir: {app}\share\poedit; DestName: resources.zip; Components: core
Source: extras\win32-gettext\xgettext.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\msgmerge.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\msgunfmt.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\msgfmt.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-db3\libdb31.dll; DestDir: {app}\bin; Components: core
Source: install\poedit.exe.manifest; DestDir: {app}\bin; MinVersion: 0,5.0.2195; Components: core
Source: extras\win32-runtime\unicows.dll; DestDir: {app}\bin; MinVersion: 4.0.950,0; Components: core
Source: locales\cs-wxstd.mo; DestDir: {app}\share\locale\cs_CZ\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\cs.mo; DestDir: {app}\share\locale\cs_CZ\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\zh_TW.Big5.mo; DestDir: {app}\share\locale\zh_TW.Big5\LC_MESSAGES; DestName: poedit.mo; Components: i18n
Source: locales\zh-wxstd.mo; DestDir: {app}\share\locale\zh\LC_MESSAGES; DestName: wxstd.mo; Components: i18n
Source: locales\nl-wxstd.mo; DestDir: {app}\share\locale\nl\LC_MESSAGES; DestName: wxstd.mo; Components: i18n
Source: locales\nl.mo; DestDir: {app}\share\locale\nl\LC_MESSAGES; DestName: poedit.mo; Components: i18n
Source: locales\et.mo; DestDir: {app}\share\locale\et\LC_MESSAGES; DestName: poedit.mo; Components: i18n
Source: locales\de-wxstd.mo; DestDir: {app}\share\locale\de\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\de.mo; DestDir: {app}\share\locale\de\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\fr.mo; DestDir: {app}\share\locale\fr\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\hr.mo; DestDir: {app}\share\locale\hr\LC_MESSAGES; Components: i18n; DestName: poedit.mo

[Registry]
Root: HKCR; SubKey: .po; ValueType: string; ValueData: GettextFile; Flags: uninsdeletekey
Root: HKCR; SubKey: GettextFile; ValueType: string; ValueData: Gettext message catalog; Flags: uninsdeletekey
Root: HKCR; SubKey: GettextFile\Shell\Open\Command; ValueType: string; ValueData: """{app}\bin\poedit.exe"" ""%1"""; Flags: uninsdeletevalue
Root: HKCR; Subkey: GettextFile\DefaultIcon; ValueType: string; ValueData: {app}\bin\poedit.exe,0; Flags: uninsdeletekey
Root: HKCU; Subkey: Software\Vaclav Slavik; Flags: uninsdeletekeyifempty
Root: HKCU; Subkey: Software\Vaclav Slavik\poedit; Flags: uninsdeletekey
Root: HKLM; Subkey: Software\Vaclav Slavik\poedit; ValueType: string; ValueName: application_path; ValueData: {app}; Flags: uninsdeletevalue
Root: HKLM; Subkey: Software\Vaclav Slavik; Flags: uninsdeletekeyifempty

[Icons]
Name: {group}\poEdit; Filename: {app}\bin\poedit.exe; WorkingDir: {app}; IconIndex: 0
Name: {group}\Readme; Filename: {app}\doc\readme.txt; IconIndex: 0

[Run]
Filename: {app}\doc\readme.txt; Description: View readme.txt; Flags: shellexec postinstall unchecked; Components: docs
Filename: {app}\bin\poedit.exe; WorkingDir: {app}; Description: Run poEdit now; Flags: postinstall unchecked nowait

[_ISTool]
EnableISX=true
UseAbsolutePaths=false

[Dirs]
Name: {app}\doc; Components: docs
Name: {app}\share; Components: core
Name: {app}\share\poedit; Components: core
Name: {app}\share\locale; Components: core
Name: {app}\bin; Components: core
Name: {app}\share\locale\cs_CZ; Components: i18n
Name: {app}\share\locale\cs_CZ\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\zh; Components: i18n
Name: {app}\share\locale\zh\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\zh_TW.Big5; Components: i18n
Name: {app}\share\locale\zh_TW.Big5\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\nl; Components: i18n
Name: {app}\share\locale\nl\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\et; Components: i18n
Name: {app}\share\locale\et\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\de; Components: i18n
Name: {app}\share\locale\de\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\fr; Components: i18n
Name: {app}\share\locale\fr\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\hr; Components: i18n
Name: {app}\share\locale\hr\LC_MESSAGES; Components: i18n

[_ISToolPreCompile]

[Components]
Name: core; Description: Core files; Flags: fixed; Types: custom compact full
Name: docs; Description: Documentation; Types: custom compact full
Name: i18n; Description: Localization files for the UI; Types: full

[Messages]
BeveledLabel=poEdit
