
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      chooselang.h
    
      Functions for choosing UI language
    
      (c) Vaclav Slavik, 2003

*/


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation
#endif

#ifndef _CHOOSELANG_H_
#define _CHOOSELANG_H_

#include <wx/string.h>
#include <wx/intl.h>

#ifndef __UNIX__
/// Let the user select language
wxLanguage ChooseLanguage();

/// Let the user change UI language
void ChangeUILanguage();
#endif

/** Return currently choosen language. Calls  ChooseLanguage if neccessary. */
wxLanguage GetUILanguage();
        

#endif
