; This script was first created by ISTool
; http://www.lerstad.com/istool/

#define VERSION          "1.3.6"

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
UninstallStyle=modern
LicenseFile=COPYING
OutputDir=.
InfoAfterFile=
Compression=lzma/ultra

WindowShowCaption=true
WindowStartMaximized=false
FlatComponentsList=true
WindowResizable=false



SolidCompression=true
ShowLanguageDialog=yes
AllowUNCPath=false
InternalCompressLevel=ultra
[Files]
Source: extras\win32-runtime\isxdl.dll; Flags: dontcopy
Source: BUILD-mingw\src\poedit.exe; DestDir: {app}\bin; DestName: poedit.exe; Components: core
Source: install\poedit.exe.manifest; DestDir: {app}\bin; MinVersion: 0,5.01.2600; Components: core
Source: extras\win32-gettext\gnu_gettext.COPYING; DestDir: {app}\doc; Components: docs
Source: README; DestDir: {app}\doc; DestName: readme.txt; Components: docs
Source: docs\chm\poedit.chm; DestDir: {app}\share\poedit\help\en; Components: docs
Source: docs\chm\gettext.chm; DestDir: {app}\share\poedit\help\en\gettext; Components: docs
Source: docs\chm\poedit-hr.chm; DestDir: {app}\share\poedit\help\hr; Components: i18n
Source: COPYING; DestDir: {app}\doc; DestName: copying.txt; Components: docs
Source: NEWS; DestDir: {app}\doc; DestName: news.txt; Components: docs
Source: src\icons\*.png; DestDir: {app}\share\poedit\icons; Components: core
Source: extras\win32-gettext\iconv.dll; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\intl.dll; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\gettextsrc.dll; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\gettextlib.dll; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\xgettext.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\msgmerge.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\msgunfmt.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\msgfmt.exe; DestDir: {app}\bin; Components: core
Source: BUILD-mingw\src\mingwm10.dll; DestDir: {app}\bin; DestName: mingwm10.dll; Components: core
DestDir: {app}\bin; Source: {tmp}\unicows.exe; Flags: deleteafterinstall external skipifsourcedoesntexist; Tasks: unicows
#include "poedit-locale-files.iss"

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
Filename: {app}\bin\unicows.exe; Tasks: unicows; Flags: skipifdoesntexist; Parameters: "/T:""{app}\bin"""; StatusMsg: Installing MSLU...

[_ISTool]
EnableISX=true
UseAbsolutePaths=false

Use7zip=false
[Dirs]
Name: {app}\bin; Components: core
Name: {app}\doc; Components: docs
Name: {app}\share; Components: core
Name: {app}\share\locale; Components: core
Name: {app}\share\poedit; Components: core
Name: {app}\share\poedit\icons; Components: core
Name: {app}\share\poedit\help; Components: docs
Name: {app}\share\poedit\help\en; Components: docs
Name: {app}\share\poedit\help\en\gettext; Components: docs
Name: {app}\share\poedit\help\hr; Components: docs
#include "poedit-locale-dirs.iss"

[_ISToolPreCompile]

[Components]
Name: core; Description: Core files; Flags: fixed; Types: custom compact full
Name: docs; Description: Documentation; Types: custom compact full
Name: i18n; Description: Localization files for the UI; Types: full

[Messages]
BeveledLabel=poEdit
[Tasks]
Name: unicows; Description: Download and install MSLU; Flags: unchecked; Components: core; GroupDescription: Due to Microsoft's draconian licensing terms for The Microsoft Layer for Unicode on Windows 95/98/ME, poEdit cannot ship with working Unicode support, additional DLL files must be obtained from Microsoft. The installer can download them from Microsoft's site for you and run the installer.; MinVersion: 4.0.950,0; OnlyBelowVersion: 0,0
[UninstallDelete]
Name: {app}\bin\License.Txt; Type: files; Tasks: unicows
Name: {app}\bin\redist.txt; Type: files; Tasks: unicows
Name: {app}\bin\unicows.*; Type: files; Tasks: unicows
[Code]
{ ------------------------------------------------------------------ }
{  Download of unicows.dll from Microsoft site:                      }
{ ------------------------------------------------------------------ }

const
  UNICOWS_URL =
      'http://download.microsoft.com/download/b/7/5/b75eace3-00e2-4aa0-9a6f-0b6882c71642/unicows.exe';

procedure isxdl_AddFile(URL, Filename: PChar);
  external 'isxdl_AddFile@files:isxdl.dll stdcall';

function isxdl_DownloadFiles(hWnd: Integer): Integer;
  external 'isxdl_DownloadFiles@files:isxdl.dll stdcall';

function isxdl_SetOption(Option, Value: PChar): Integer;
  external 'isxdl_SetOption@files:isxdl.dll stdcall';


function DownloadFiles: Boolean;
var
  hWnd: Integer;
begin
  isxdl_SetOption('label', 'Downloading MSLU');
  isxdl_SetOption('description', 'Please wait while Setup is downloading MSLU files to your computer.');

  isxdl_AddFile(UNICOWS_URL, ExpandConstant('{tmp}\unicows.exe'));

  if isxdl_DownloadFiles(hWnd) = 0 then
    MsgBox('Error while downloading. Setup will now continue installing normally.', mbError, mb_Ok);

  Result := True;
end;


{ ------------------------------------------------------------------ }
{ Helper functions:                                                  }
{ ------------------------------------------------------------------ }

function InstallLocally : boolean;
begin
  result := not IsAdminLoggedOn;
end;

function InstallGlobally : boolean;
begin
  result := IsAdminLoggedOn;
end;

{ ------------------------------------------------------------------ }
{ UI scripting:                                                      }
{ ------------------------------------------------------------------ }

function ScriptDlgPages(CurPage: Integer; BackClicked: Boolean): Boolean;
begin
  if not BackClicked and (CurPage = wpReady) and
     (ShouldProcessEntry('core', 'unicows') = srYes) then begin
    Result := DownloadFiles;
  end
  else begin
    Result := True;
  end;
end;

function NextButtonClick(CurPage: Integer): Boolean;
begin
  Result := ScriptDlgPages(CurPage, False);
end;

function BackButtonClick(CurPage: Integer): Boolean;
begin
  Result := ScriptDlgPages(CurPage, True);
end;


