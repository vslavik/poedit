; This script was first created by ISTool
; http://www.lerstad.com/istool/

[Setup]
CompressLevel=9
OutputBaseFilename=poedit113_setup
AppName=poEdit
AppVerName=poEdit 1.1.3

ChangesAssociations=true
AlwaysShowComponentsList=false
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


[Files]
Source: objs\release\poedit.exe; DestDir: {app}\bin
Source: extras\win32-gettext\gnu_gettext.COPYING; DestDir: {app}\doc
Source: install\readme.txt; DestDir: {app}\doc; DestName: readme.txt
Source: docs\poedit.chm; DestDir: {app}\share\poedit
Source: install\license.txt; DestDir: {app}\doc; DestName: license.txt
Source: install\news.txt; DestDir: {app}\doc; DestName: news.txt
Source: src\resources\tmp.zip; DestDir: {app}\share\poedit; DestName: resources.zip
Source: extras\win32-gettext\xgettext.exe; DestDir: {app}\bin
Source: extras\win32-gettext\msgmerge.exe; DestDir: {app}\bin
Source: extras\win32-gettext\msgunfmt.exe; DestDir: {app}\bin
Source: extras\win32-gettext\msgfmt.exe; DestDir: {app}\bin
Source: extras\win32-db3\libdb31.dll; DestDir: {app}\bin

[Registry]
Root: HKCR; SubKey: .po; ValueType: string; ValueData: GettextFile; Flags: uninsdeletekey
Root: HKCR; SubKey: GettextFile; ValueType: string; ValueData: Gettext message catalog; Flags: uninsdeletekey
Root: HKCR; SubKey: GettextFile\Shell\Open\Command; ValueType: string; ValueData: "{app}\bin\poedit.exe ""%1"""; Flags: uninsdeletevalue
Root: HKCR; Subkey: GettextFile\DefaultIcon; ValueType: string; ValueData: {app}\bin\poedit.exe,0; Flags: uninsdeletekey
Root: HKCU; Subkey: Software\Vaclav Slavik; Flags: uninsdeletekeyifempty
Root: HKCU; Subkey: Software\Vaclav Slavik\poedit; Flags: uninsdeletekey
Root: HKLM; Subkey: Software\Vaclav Slavik\poedit; ValueType: string; ValueName: application_path; ValueData: {app}; Flags: uninsdeletevalue
Root: HKLM; Subkey: Software\Vaclav Slavik; Flags: uninsdeletekeyifempty

[Icons]
Name: {group}\poEdit; Filename: {app}\bin\poedit.exe; WorkingDir: {app}; IconIndex: 0
Name: {group}\Readme; Filename: {app}\doc\readme.txt; IconIndex: 0
Name: {group}\poEdit Help; Filename: {app}\share\poedit\poedit.chm; IconIndex: 0

[Run]
Filename: {app}\doc\readme.txt; Description: View readme.txt; Flags: shellexec postinstall unchecked
Filename: {app}\bin\poedit.exe; WorkingDir: {app}; Description: Run poEdit now; Flags: postinstall unchecked

[_ISTool]
EnableISX=true
UseAbsolutePaths=false

[Dirs]
Name: {app}\share
Name: {app}\share\poedit

Name: {app}\bin
Name: {app}\doc

[_ISToolPreCompile]
Name: bash; Parameters: "-c ""cd ../src/resources && zip -9 tmp.zip '*.xrc' '*.gif'"""; Flags: abortonerror runminimized

[_ISToolPostCompile]
Name: bash; Parameters: -c 'cd ../src/resources && rm tmp.zip'; Flags: runminimized
