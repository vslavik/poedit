; This script was first created by ISTool
; http://www.lerstad.com/istool/

#define VERSION          "1.2.2"

[Setup]
OutputBaseFilename=poedit-{#VERSION}-setup
AppName=poEdit
AppVerName=poEdit {#VERSION}

ChangesAssociations=true
AlwaysShowComponentsList=true
SourceDir=..
DefaultDirName={pf}\poEdit

DefaultGroupName=poEdit
AllowNoIcons=true
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
Source: BUILD-mingw\src\poedit; DestDir: {app}\bin; DestName: poedit.exe; Components: core
Source: install\poedit.exe.manifest; DestDir: {app}\bin; MinVersion: 0,5.01.2600; Components: core
Source: extras\win32-gettext\gnu_gettext.COPYING; DestDir: {app}\doc; Components: docs
Source: install\readme.txt; DestDir: {app}\doc; DestName: readme.txt; Components: docs
Source: docs\chm\poedit.chm; DestDir: {app}\share\poedit; Components: docs
Source: docs\chm\gettext.chm; DestDir: {app}\share\poedit; Components: docs
Source: docs\chm\poedit-hr.chm; DestDir: {app}\share\poedit; Components: i18n
Source: install\license.txt; DestDir: {app}\doc; DestName: license.txt; Components: docs
Source: install\news.txt; DestDir: {app}\doc; DestName: news.txt; Components: docs
Source: BUILD-mingw\src\resources\resources.zip; DestDir: {app}\share\poedit; DestName: resources.zip; Components: core
Source: extras\win32-gettext\xgettext.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\msgmerge.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\msgunfmt.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\msgfmt.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-db3\libdb31.dll; DestDir: {app}\bin; Components: core; CopyMode: alwaysoverwrite
Source: extras\win32-runtime\unicows.dll; DestDir: {app}\bin; MinVersion: 4.0.950,0; Components: core; CopyMode: alwaysoverwrite
Source: locales\wxwin\cs.mo; DestDir: {app}\share\locale\cs_CZ\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\cs.mo; DestDir: {app}\share\locale\cs_CZ\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\zh_TW.mo; DestDir: {app}\share\locale\zh_TW\LC_MESSAGES; DestName: poedit.mo; Components: i18n
Source: locales\wxwin\zh.mo; DestDir: {app}\share\locale\zh\LC_MESSAGES; DestName: wxstd.mo; Components: i18n
Source: locales\wxwin\nl.mo; DestDir: {app}\share\locale\nl\LC_MESSAGES; DestName: wxstd.mo; Components: i18n
Source: locales\nl.mo; DestDir: {app}\share\locale\nl\LC_MESSAGES; DestName: poedit.mo; Components: i18n
Source: locales\et.mo; DestDir: {app}\share\locale\et\LC_MESSAGES; DestName: poedit.mo; Components: i18n
Source: locales\wxwin\de.mo; DestDir: {app}\share\locale\de\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\de.mo; DestDir: {app}\share\locale\de\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\fr.mo; DestDir: {app}\share\locale\fr\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\hr.mo; DestDir: {app}\share\locale\hr\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\pl.mo; DestDir: {app}\share\locale\pl\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\nn.mo; DestDir: {app}\share\locale\nn\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\tr.mo; DestDir: {app}\share\locale\tr\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\tr.mo; DestDir: {app}\share\locale\tr\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\wxwin\it.mo; DestDir: {app}\share\locale\it\LC_MESSAGES; DestName: wxstd.mo; Components: i18n
Source: locales\it.mo; DestDir: {app}\share\locale\it\LC_MESSAGES; DestName: poedit.mo; Components: i18n
Source: locales\ta.mo; DestDir: {app}\share\locale\ta\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\bg.mo; DestDir: {app}\share\locale\bg\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\sk.mo; DestDir: {app}\share\locale\sk\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\sv_SE.mo; DestDir: {app}\share\locale\sv_SE\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\sv.mo; DestDir: {app}\share\locale\sv_SE\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\ca.mo; DestDir: {app}\share\locale\ca\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\ka.mo; DestDir: {app}\share\locale\ka\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\ro.mo; DestDir: {app}\share\locale\ro\LC_MESSAGES; Components: i18n; DestName: poedit.mo

[Registry]
Root: HKCR; SubKey: .po; ValueType: string; ValueData: GettextFile; Flags: uninsdeletekey noerror
Root: HKCR; SubKey: GettextFile; ValueType: string; ValueData: Gettext message catalog; Flags: uninsdeletekey noerror
Root: HKCR; SubKey: GettextFile\Shell\Open\Command; ValueType: string; ValueData: """{app}\bin\poedit.exe"" ""%1"""; Flags: uninsdeletevalue noerror
Root: HKCR; Subkey: GettextFile\DefaultIcon; ValueType: string; ValueData: {app}\bin\poedit.exe,0; Flags: uninsdeletekey noerror
Root: HKCU; Subkey: Software\Vaclav Slavik; Flags: uninsdeletekeyifempty dontcreatekey
Root: HKCU; Subkey: Software\Vaclav Slavik\poedit; Flags: uninsdeletekey dontcreatekey
Root: HKLM; Subkey: Software\Vaclav Slavik; Flags: uninsdeletekeyifempty; Check: InstallGlobally
Root: HKLM; Subkey: Software\Vaclav Slavik\poedit; Flags: uninsdeletekey; Check: InstallGlobally
Root: HKLM; Subkey: Software\Vaclav Slavik\poedit\{#VERSION}; ValueType: string; ValueName: application_path; ValueData: {app}; Flags: uninsdeletevalue; Check: InstallGlobally
Root: HKCU; Subkey: Software\Vaclav Slavik; Flags: uninsdeletekeyifempty; Check: InstallLocally
Root: HKCU; Subkey: Software\Vaclav Slavik\poedit\{#VERSION}; ValueType: string; ValueName: application_path; ValueData: {app}; Flags: uninsdeletevalue; Check: InstallLocally

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
Name: {app}\share\locale\zh_TW; Components: i18n
Name: {app}\share\locale\zh_TW\LC_MESSAGES; Components: i18n
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
Name: {app}\share\locale\po; Components: i18n
Name: {app}\share\locale\po\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\nn; Components: i18n
Name: {app}\share\locale\nn\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\tr; Components: i18n
Name: {app}\share\locale\tr\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\it; Components: i18n
Name: {app}\share\locale\it\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\ta; Components: i18n
Name: {app}\share\locale\ta\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\bg; Components: i18n
Name: {app}\share\locale\bg\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\sk; Components: i18n
Name: {app}\share\locale\sk\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\sv_SE; Components: i18n
Name: {app}\share\locale\sv_SE\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\ca; Components: i18n
Name: {app}\share\locale\ca\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\ka; Components: i18n
Name: {app}\share\locale\ka\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\ro; Components: i18n
Name: {app}\share\locale\ro\LC_MESSAGES; Components: i18n

[_ISToolPreCompile]

[Components]
Name: core; Description: Core files; Flags: fixed; Types: custom compact full
Name: docs; Description: Documentation; Types: custom compact full
Name: i18n; Description: Localization files for the UI; Types: full

[Messages]
BeveledLabel=poEdit
[Code]
function InstallLocally : boolean;
begin
  result := not IsAdminLoggedOn;
end;

function InstallGlobally : boolean;
begin
  result := IsAdminLoggedOn;
end;

