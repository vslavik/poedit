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
# PROP Output_Dir "objs/release"
# PROP Intermediate_Dir "objs/release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /Ob2 /I "$(wxwin)/include" /I "$(wxwin)/lib/mswu" /I "extras/wx-contrib/src/xrc/expat" /I "extras/wx-contrib/include" /I "extras/win32-db3" /D "NDEBUG" /D "USE_TRANSMEM" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D DB_HEADER=\"db.h\" /YX"wx/wxprec.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /i "$(wxwin)/include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 wxmswu.lib zlib.lib unicows.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib rpcrt4.lib wsock32.lib winmm.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libci.lib" /nodefaultlib:"msvcrtd.lib" /libpath:"$(wxwin)\lib"

!ELSEIF  "$(CFG)" == "poedit - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "objs/debug"
# PROP Intermediate_Dir "objs/debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(wxwin)/include" /I "$(wxwin)/lib/mswud" /I "extras/wx-contrib/src/xrc/expat" /I "extras/wx-contrib/include" /I "extras/win32-db3" /D "_DEBUG" /D DEBUG=1 /D "__WXDEBUG__" /D "USE_TRANSMEM" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /D DB_HEADER=\"db.h\" /YX"wx/wxprec.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /i "$(wxwin)/include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wxmswud.lib zlibd.lib unicows.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib rpcrt4.lib wsock32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"$(wxwin)\lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "poedit - Win32 Release"
# Name "poedit - Win32 Debug"
# Begin Group "poEdit files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\catalog.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\catalog.h
# End Source File
# Begin Source File

SOURCE=.\src\commentdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\commentdlg.h
# End Source File
# Begin Source File

SOURCE=.\src\digger.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\digger.h
# End Source File
# Begin Source File

SOURCE=.\src\edapp.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\edapp.h
# End Source File
# Begin Source File

SOURCE=.\src\edframe.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\edframe.h
# End Source File
# Begin Source File

SOURCE=.\src\export_html.cpp
# End Source File
# Begin Source File

SOURCE=.\src\fileviewer.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\fileviewer.h
# End Source File
# Begin Source File

SOURCE=.\src\findframe.cpp
# End Source File
# Begin Source File

SOURCE=.\src\findframe.h
# End Source File
# Begin Source File

SOURCE=.\src\gexecute.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\gexecute.h
# End Source File
# Begin Source File

SOURCE=.\src\isocodes.cpp
# End Source File
# Begin Source File

SOURCE=.\src\isocodes.h
# End Source File
# Begin Source File

SOURCE=.\src\chooselang.cpp
# End Source File
# Begin Source File

SOURCE=.\src\chooselang.h
# End Source File
# Begin Source File

SOURCE=.\src\transmemupd_wizard.cpp
# End Source File
# Begin Source File

SOURCE=.\src\transmemupd_wizard.h
# End Source File
# Begin Source File

SOURCE=.\src\manager.cpp
# End Source File
# Begin Source File

SOURCE=.\src\manager.h
# End Source File
# Begin Source File

SOURCE=.\src\parser.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\parser.h
# End Source File
# Begin Source File

SOURCE=.\src\poedit.rc
# End Source File
# Begin Source File

SOURCE=.\src\prefsdlg.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\prefsdlg.h
# End Source File
# Begin Source File

SOURCE=.\src\progressinfo.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\progressinfo.h
# End Source File
# Begin Source File

SOURCE=.\src\settingsdlg.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\settingsdlg.h
# End Source File
# Begin Source File

SOURCE=.\src\summarydlg.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\summarydlg.h
# End Source File
# Begin Source File

SOURCE=.\src\transmem.cpp
# End Source File
# Begin Source File

SOURCE=.\src\transmem.h
# End Source File
# Begin Source File

SOURCE=.\src\transmemupd.cpp
# End Source File
# Begin Source File

SOURCE=.\src\transmemupd.h
# End Source File
# End Group
# Begin Group "Gizmos"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\extras\wx-contrib\src\gizmos\editlbox.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\gizmos\editlbox.h"
# End Source File
# End Group
# Begin Group "Expat"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\expat\xmlparse.c"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\expat\xmlrole.c"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\expat\xmltok.c"
# End Source File
# End Group
# Begin Group "XML resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_all.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_bmp.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_bmp.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_bmpbt.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_bmpbt.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_bttn.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_bttn.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_cald.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_cald.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_chckb.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_chckb.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_chckl.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_chckl.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_choic.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_choic.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_combo.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_combo.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_dlg.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_dlg.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_frame.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_frame.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_gauge.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_gauge.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_gdctl.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_html.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_html.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_listb.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_listb.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_listc.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_listc.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_menu.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_menu.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_notbk.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_notbk.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_panel.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_panel.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_radbt.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_radbt.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_radbx.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_radbx.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_scrol.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_scrol.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_scwin.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_sizer.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_sizer.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_slidr.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_slidr.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_spin.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_spin.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_stbmp.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_stbmp.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_stbox.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_stbox.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_stlin.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_stlin.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_sttxt.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_sttxt.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_text.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_text.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_toolb.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_toolb.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_tree.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_tree.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_unkwn.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xh_wizrd.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xh_unkwn.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xml.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xml.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\expat\xmldef.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xmlres.cpp"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xrc\xmlres.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xrc\xmlrsall.cpp"
# End Source File
# End Group
# End Target
# End Project
