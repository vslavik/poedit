# Microsoft Developer Studio Project File - Name="poedit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=poedit - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "poedit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "poedit.mak" CFG="poedit - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "poedit - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "poedit - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "poedit - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /Ob2 /I "../wxWindows-2.2/Bwin/include" /I "src/wxxml_snapshot/include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /i "../wxWindows/include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 wx.lib xpm.lib png.lib zlib.lib jpeg.lib tiff.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib rpcrt4.lib wsock32.lib winmm.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libci.lib" /nodefaultlib:"msvcrtd.lib" /libpath:"../wxWindows-2.2/Bwin/lib"

!ELSEIF  "$(CFG)" == "poedit - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../wxWindows-2.2/Bwin/include" /I "src/wxxml_snapshot/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D DEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /i "../wxWindows/include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wxd.lib xpmd.lib zlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib rpcrt4.lib wsock32.lib winmm.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd.lib" /nodefaultlib:"libcid.lib" /nodefaultlib:"msvcrt.lib" /pdbtype:sept /libpath:"../wxWindows-2.2/Bwin/lib"

!ENDIF 

# Begin Target

# Name "poedit - Win32 Release"
# Name "poedit - Win32 Debug"
# Begin Group "wxxml_snapshot"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_bttn.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_chckb.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_chckl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_choic.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_combo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_dlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_gauge.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_html.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_listb.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_menu.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_notbk.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_panel.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_radbt.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_radbx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_sizer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_slidr.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_spin.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_stbmp.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_sttxt.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_text.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xh_toolb.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xml.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xmlbin.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xmlbinz.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xmlres.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wxxml_snapshot\xmlrsall.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\catalog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\catalog.h
# End Source File
# Begin Source File

SOURCE=.\src\compiled_resource.cpp
# End Source File
# Begin Source File

SOURCE=.\src\digger.cpp
# End Source File
# Begin Source File

SOURCE=.\src\digger.h
# End Source File
# Begin Source File

SOURCE=.\src\edapp.cpp
# End Source File
# Begin Source File

SOURCE=.\src\edframe.cpp
# End Source File
# Begin Source File

SOURCE=.\src\edframe.h
# End Source File
# Begin Source File

SOURCE=.\src\fileviewer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\fileviewer.h
# End Source File
# Begin Source File

SOURCE=.\src\gexecute.cpp
# End Source File
# Begin Source File

SOURCE=.\src\gexecute.h
# End Source File
# Begin Source File

SOURCE=.\src\iso639.h
# End Source File
# Begin Source File

SOURCE=.\src\parser.cpp
# End Source File
# Begin Source File

SOURCE=.\src\parser.h
# End Source File
# Begin Source File

SOURCE=.\src\poedit.rc
# End Source File
# Begin Source File

SOURCE=.\src\prefsdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\prefsdlg.h
# End Source File
# Begin Source File

SOURCE=.\src\progressinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\progressinfo.h
# End Source File
# Begin Source File

SOURCE=.\src\settingsdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\settingsdlg.h
# End Source File
# Begin Source File

SOURCE=.\src\summarydlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\summarydlg.h
# End Source File
# End Target
# End Project
