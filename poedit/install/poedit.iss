; This script was first created by ISTool
; http://www.lerstad.com/istool/

#define VERSION          "1.3.1"

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
Compression=lzma

WindowShowCaption=true
WindowStartMaximized=false
FlatComponentsList=true
WindowResizable=false



SolidCompression=true
ShowLanguageDialog=yes
AllowUNCPath=false
[Files]
Source: extras\win32-runtime\isxdl.dll; Flags: dontcopy
Source: BUILD-mingw\src\poedit.exe; DestDir: {app}\bin; DestName: poedit.exe; Components: core
Source: install\poedit.exe.manifest; DestDir: {app}\bin; MinVersion: 0,5.01.2600; Components: core
Source: extras\win32-gettext\gnu_gettext.COPYING; DestDir: {app}\doc; Components: docs
Source: README; DestDir: {app}\doc; DestName: readme.txt; Components: docs
Source: docs\chm\poedit.chm; DestDir: {app}\share\poedit; Components: docs
Source: docs\chm\gettext.chm; DestDir: {app}\share\poedit; Components: docs
Source: docs\chm\poedit-hr.chm; DestDir: {app}\share\poedit; Components: i18n
Source: COPYING; DestDir: {app}\doc; DestName: copying.txt; Components: docs
Source: NEWS; DestDir: {app}\doc; DestName: news.txt; Components: docs
Source: BUILD-mingw\src\resources\resources.zip; DestDir: {app}\share\poedit; DestName: resources.zip; Components: core
Source: extras\win32-gettext\iconv.dll; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\xgettext.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\msgmerge.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\msgunfmt.exe; DestDir: {app}\bin; Components: core
Source: extras\win32-gettext\msgfmt.exe; DestDir: {app}\bin; Components: core
Source: locales\wxwin\cs.mo; DestDir: {app}\share\locale\cs_CZ\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\cs.mo; DestDir: {app}\share\locale\cs_CZ\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\zh_TW.mo; DestDir: {app}\share\locale\zh_TW\LC_MESSAGES; DestName: poedit.mo; Components: i18n
Source: locales\wxwin\zh_CN.mo; DestDir: {app}\share\locale\zh_CN\LC_MESSAGES; DestName: wxstd.mo; Components: i18n
Source: locales\zh_CN.mo; DestDir: {app}\share\locale\zh_CN\LC_MESSAGES; DestName: poedit.mo; Components: i18n
Source: locales\wxwin\nl.mo; DestDir: {app}\share\locale\nl\LC_MESSAGES; DestName: wxstd.mo; Components: i18n
Source: locales\nl.mo; DestDir: {app}\share\locale\nl\LC_MESSAGES; DestName: poedit.mo; Components: i18n
Source: locales\et.mo; DestDir: {app}\share\locale\et\LC_MESSAGES; DestName: poedit.mo; Components: i18n
Source: locales\wxwin\de.mo; DestDir: {app}\share\locale\de\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\de.mo; DestDir: {app}\share\locale\de\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\fr.mo; DestDir: {app}\share\locale\fr\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\fr.mo; DestDir: {app}\share\locale\fr\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\hr.mo; DestDir: {app}\share\locale\hr\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\pl.mo; DestDir: {app}\share\locale\pl\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\nn.mo; DestDir: {app}\share\locale\nn\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\nb.mo; DestDir: {app}\share\locale\nb\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\nb.mo; DestDir: {app}\share\locale\nb\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\tr.mo; DestDir: {app}\share\locale\tr\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\tr.mo; DestDir: {app}\share\locale\tr\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\wxwin\it.mo; DestDir: {app}\share\locale\it\LC_MESSAGES; DestName: wxstd.mo; Components: i18n
Source: locales\it.mo; DestDir: {app}\share\locale\it\LC_MESSAGES; DestName: poedit.mo; Components: i18n
Source: locales\ta.mo; DestDir: {app}\share\locale\ta\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\bg.mo; DestDir: {app}\share\locale\bg\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\sk.mo; DestDir: {app}\share\locale\sk\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\sv_SE.mo; DestDir: {app}\share\locale\sv_SE\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\sv.mo; DestDir: {app}\share\locale\sv_SE\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\ca.mo; DestDir: {app}\share\locale\ca\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\ca.mo; DestDir: {app}\share\locale\ca\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\ka.mo; DestDir: {app}\share\locale\ka\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\ro.mo; DestDir: {app}\share\locale\ro\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\el.mo; DestDir: {app}\share\locale\el\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\ja.mo; DestDir: {app}\share\locale\el\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\ru.mo; DestDir: {app}\share\locale\ru\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\ru.mo; DestDir: {app}\share\locale\ru\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\is.mo; DestDir: {app}\share\locale\is\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\pt_BR.mo; DestDir: {app}\share\locale\pt_BR\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\es.mo; DestDir: {app}\share\locale\es\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\es.mo; DestDir: {app}\share\locale\es\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\da.mo; DestDir: {app}\share\locale\da\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\da.mo; DestDir: {app}\share\locale\da\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\hu.mo; DestDir: {app}\share\locale\hu\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\hu.mo; DestDir: {app}\share\locale\hu\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\lt.mo; DestDir: {app}\share\locale\lt\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\fa_IR.mo; DestDir: {app}\share\locale\fa_IR\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\af_ZA.mo; DestDir: {app}\share\locale\af_ZA\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\af_ZA.mo; DestDir: {app}\share\locale\af_ZA\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\sl.mo; DestDir: {app}\share\locale\sl\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\sl.mo; DestDir: {app}\share\locale\sl\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\pt_PT.mo; DestDir: {app}\share\locale\pt_PT\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\mn.mo; DestDir: {app}\share\locale\mn\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\pa.mo; DestDir: {app}\share\locale\pa\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\sq_AL.mo; DestDir: {app}\share\locale\sq_AL\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\am_ET.mo; DestDir: {app}\share\locale\am_ET\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\hi.mo; DestDir: {app}\share\locale\hi\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\eo.mo; DestDir: {app}\share\locale\eo\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\be.mo; DestDir: {app}\share\locale\be\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\br.mo; DestDir: {app}\share\locale\br\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wa.mo; DestDir: {app}\share\locale\wa\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\bn.mo; DestDir: {app}\share\locale\bn\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\eu.mo; DestDir: {app}\share\locale\eu\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\eu.mo; DestDir: {app}\share\locale\eu\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\ko.mo; DestDir: {app}\share\locale\ko\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\ko.mo; DestDir: {app}\share\locale\ko\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: locales\he.mo; DestDir: {app}\share\locale\he\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\ky.mo; DestDir: {app}\share\locale\ky\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\uk.mo; DestDir: {app}\share\locale\uk\LC_MESSAGES; Components: i18n; DestName: poedit.mo
Source: locales\wxwin\uk.mo; DestDir: {app}\share\locale\uk\LC_MESSAGES; Components: i18n; DestName: wxstd.mo
Source: BUILD-mingw\src\mingwm10.dll; DestDir: {app}\bin; DestName: mingwm10.dll; Components: core
DestDir: {app}\bin; Source: {tmp}\unicows.exe; Flags: deleteafterinstall external skipifsourcedoesntexist; Tasks: unicows

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
Name: {app}\doc; Components: docs
Name: {app}\share; Components: core
Name: {app}\share\poedit; Components: core
Name: {app}\share\locale; Components: core
Name: {app}\bin; Components: core
Name: {app}\share\locale\cs_CZ; Components: i18n
Name: {app}\share\locale\cs_CZ\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\zh_CN; Components: i18n
Name: {app}\share\locale\zh_CN\LC_MESSAGES; Components: i18n
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
Name: {app}\share\locale\nb; Components: i18n
Name: {app}\share\locale\nb\LC_MESSAGES; Components: i18n
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
Name: {app}\share\locale\el; Components: i18n
Name: {app}\share\locale\el\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\ja; Components: i18n
Name: {app}\share\locale\ja\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\ru; Components: i18n
Name: {app}\share\locale\ru\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\is; Components: i18n
Name: {app}\share\locale\is\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\pt_BR; Components: i18n
Name: {app}\share\locale\pt_BR\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\es; Components: i18n
Name: {app}\share\locale\es\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\hu; Components: i18n
Name: {app}\share\locale\hu\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\lt; Components: i18n
Name: {app}\share\locale\lt\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\fa_IR; Components: i18n
Name: {app}\share\locale\fa_IR\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\af_ZA; Components: i18n
Name: {app}\share\locale\af_ZA\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\sl; Components: i18n
Name: {app}\share\locale\sl\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\pa; Components: i18n
Name: {app}\share\locale\pa\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\sq_AL; Components: i18n
Name: {app}\share\locale\sq_AL\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\am_ET; Components: i18n
Name: {app}\share\locale\am_ET\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\hi; Components: i18n
Name: {app}\share\locale\hi\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\eo; Components: i18n
Name: {app}\share\locale\eo\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\be; Components: i18n
Name: {app}\share\locale\be\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\br; Components: i18n
Name: {app}\share\locale\br\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\wa; Components: i18n
Name: {app}\share\locale\wa\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\bn; Components: i18n
Name: {app}\share\locale\bn\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\eu; Components: i18n
Name: {app}\share\locale\eu\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\da; Components: i18n
Name: {app}\share\locale\da\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\pl; Components: i18n
Name: {app}\share\locale\pl\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\pt_PT; Components: i18n
Name: {app}\share\locale\pt_PT\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\mn; Components: i18n
Name: {app}\share\locale\mn\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\ko; Components: i18n
Name: {app}\share\locale\ko\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\he; Components: i18n
Name: {app}\share\locale\he\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\ky; Components: i18n
Name: {app}\share\locale\ky\LC_MESSAGES; Components: i18n
Name: {app}\share\locale\uk; Components: i18n
Name: {app}\share\locale\uk\LC_MESSAGES; Components: i18n

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


