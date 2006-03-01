; This script was first created by ISTool
; http://www.lerstad.com/istool/

[Setup]
OutputBaseFilename=poedit_DLLs
AppName=poEdit support DLLs


ChangesAssociations=false
AlwaysShowComponentsList=false
SourceDir=..
DefaultDirName={pf}\poEdit


UninstallIconName=Uninstall poEdit
AllowNoIcons=true
AlwaysCreateUninstallIcon=true
DisableAppendDir=false
UninstallStyle=modern
WizardStyle=modern

OutputDir=.

Compression=bzip

WindowShowCaption=false
WindowStartMaximized=false
FlatComponentsList=true
CreateAppDir=false
UsePreviousGroup=false
Uninstallable=false
UpdateUninstallLogAppName=false
AppVerName=poEdit support DLLs
DisableProgramGroupPage=true
DisableFinishedPage=false
DisableReadyMemo=false
DisableReadyPage=true
AdminPrivilegesRequired=true


[Files]
Source: extras\win32-runtime\msvcrt.dll; DestDir: {sys}; CopyMode: alwaysskipifsameorolder; Flags: restartreplace uninsneveruninstall
Source: extras\win32-runtime\50comupd.exe; DestDir: {tmp}; Flags: deleteafterinstall
Source: extras\win32-runtime\hhupd.exe; DestDir: {tmp}; Flags: deleteafterinstall

[Registry]

[Icons]

[Run]
Filename: {tmp}\50comupd.exe; Parameters: /Q; WorkingDir: {tmp}; StatusMsg: Installing Common Controls DLL...
Filename: {tmp}\hhupd.exe; Parameters: /Q; WorkingDir: {tmp}; StatusMsg: Installing MS HTML Help component...

[_ISTool]
EnableISX=true
UseAbsolutePaths=false

[Dirs]

[_ISToolPreCompile]

