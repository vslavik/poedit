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
# ADD CPP /nologo /MD /W3 /GX /O1 /Ob2 /I "../wxwindows/include" /I "extras/wx-contrib/include" /I "extras/wx-contrib/src/xml/expat" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /i "../wxWindows/include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 wx.lib zlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib rpcrt4.lib wsock32.lib winmm.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libci.lib" /nodefaultlib:"msvcrtd.lib" /libpath:"../wxwindows/lib"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "extras/wx-contrib/src/xml/expat" /I "../wxwindows/include" /I "extras/wx-contrib/include" /D "_DEBUG" /D DEBUG=1 /D "__WXDEBUG__" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /i "../wxWindows/include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wxd.lib zlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib rpcrt4.lib wsock32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd.lib" /nodefaultlib:"libcid.lib" /nodefaultlib:"msvcrt.lib" /pdbtype:sept /libpath:"../wxwindows/lib"

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

SOURCE=.\src\fileviewer.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\fileviewer.h
# End Source File
# Begin Source File

SOURCE=.\src\gexecute.cpp
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=.\src\gexecute.h
# End Source File
# Begin Source File

SOURCE=.\src\iso639.h
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
# End Group
# Begin Group "XML resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_all.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_bmp.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_bmp.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_bmpbt.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_bmpbt.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_bttn.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_bttn.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_cald.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_cald.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_chckb.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_chckb.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_chckl.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_chckl.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_choic.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_choic.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_combo.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_combo.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_dlg.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_dlg.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_frame.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_frame.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_gauge.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_gauge.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_html.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_html.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_listb.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_listb.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_listc.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_listc.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_menu.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_menu.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_notbk.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_notbk.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_panel.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_panel.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_radbt.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_radbt.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_radbx.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_radbx.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_scrol.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_scrol.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_sizer.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_sizer.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_slidr.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_slidr.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_spin.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_spin.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_stbmp.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_stbmp.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_stbox.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_stbox.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_stlin.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_stlin.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_sttxt.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_sttxt.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_text.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_text.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_toolb.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_toolb.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_tree.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_tree.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xh_unkwn.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xh_unkwn.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xml.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xml.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xmlbin.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xmlbinz.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xmlexpat.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xmlio.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\expat\xmlparse.c"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xmlres.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\include\wx\xml\xmlres.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\expat\xmlrole.c"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xmlrsall.cpp"
# ADD CPP /YX"wx/wxprec.h"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\expat\xmltok.c"
# End Source File
# Begin Source File

SOURCE=".\extras\wx-contrib\src\xml\xmlwrite.cpp"
# ADD CPP /YX"wx/wxprec.h"
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
# End Target
# End Project
