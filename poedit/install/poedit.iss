; This script was first created by ISTool
; http://www.lerstad.com/istool/

[Setup]
CompressLevel=9
OutputBaseFilename=poedit_setup
AppName=poEdit
AppVerName=poEdit 1.0.1

ChangesAssociations=true
AlwaysShowComponentsList=false
SourceDir=..
DefaultDirName={pf}\poEdit
OutputDir=.
DefaultGroupName=poEdit
UninstallIconName=Uninstall poEdit
AllowNoIcons=true
AlwaysCreateUninstallIcon=true
DisableAppendDir=true
UninstallStyle=modern
WizardStyle=modern
LicenseFile=install\license.txt
[Files]
Source: gettext-win32\msgfmt.exe; DestDir: {app}
Source: gettext-win32\xgettext.exe; DestDir: {app}
Source: Release\poedit.exe; DestDir: {app}
Source: gettext-win32\msgmerge.exe; DestDir: {app}
Source: gettext-win32\gnu_gettext.COPYING; DestDir: {app}
Source: README; DestDir: {app}; DestName: readme.txt; Flags: isreadme
Source: docs\poedit.chm; DestDir: {app}
Source: LICENSE; DestDir: {app}
Source: ChangeLog; DestDir: {app}; DestName: ChangeLog.txt
[Registry]
Root: HKCR; SubKey: .po; ValueType: string; ValueData: GettextFile; Flags: uninsdeletekey
Root: HKCR; SubKey: GettextFile; ValueType: string; ValueData: Gettext message catalog; Flags: uninsdeletekey
Root: HKCR; SubKey: GettextFile\Shell\Open\Command; ValueType: string; ValueData: "{app}\poedit.exe ""%1"""; Flags: uninsdeletevalue
Root: HKCR; Subkey: GettextFile\DefaultIcon; ValueType: string; ValueData: {app}\poedit.exe,0; Flags: uninsdeletekey
Root: HKCU; Subkey: Software\Vaclav Slavik; Flags: uninsdeletekeyifempty
Root: HKCU; Subkey: Software\Vaclav Slavik\poedit; Flags: uninsdeletekey
[Icons]
Name: {group}\poEdit; Filename: {app}\poedit.exe; WorkingDir: {app}; IconIndex: 0
Name: {group}\Readme; Filename: {app}\readme.txt; IconIndex: 0
Name: {group}\poEdit Help; Filename: {app}\poedit.chm; IconIndex: 0
[Run]
Filename: {app}\ChangeLog.txt; WorkingDir: {app}; Description: View changes since previous version; Flags: postinstall unchecked shellexec
Filename: {app}\poedit.exe; WorkingDir: {app}; Description: Run poEdit now; Flags: postinstall unchecked
