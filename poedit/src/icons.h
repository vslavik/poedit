
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      icons.h
    
      Art provider classes
    
      (c) Vaclav Slavik, 2004

*/

#ifndef _ICONS_H_
#define _ICONS_H_

#include <wx/artprov.h>

#if defined(__WXGTK20__) && defined(wxCHECK_VERSION_FULL) && \
                            wxCHECK_VERSION_FULL(2,5,2,8)
    #define HAS_THEMES_SUPPORT
#endif

class wxPoeditStdArtProvider : public wxArtProvider
{
protected:
    virtual wxBitmap CreateBitmap(const wxArtID& id,
                                  const wxArtClient& client,
                                  const wxSize& size);
};


#ifdef HAS_THEMES_SUPPORT
class wxPoeditThemeArtProvider : public wxArtProvider
{
protected:
    virtual wxBitmap CreateBitmap(const wxArtID& id,
                                  const wxArtClient& client,
                                  const wxSize& size);
};
#endif

#endif // _ICONS_H_
