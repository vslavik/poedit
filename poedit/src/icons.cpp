/*
 *  This file is part of poEdit (http: *www.poedit.org)
 *
 *  Copyright (C) 2004-2005 Vaclav Slavik
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *  $Id$
 *
 *  Art provider classes
 *
 */

#include <wx/log.h>

#include "icons.h"

#ifdef __UNIX__
#include "icons/appicon/poedit.xpm"
#endif

#include "icons/poedit-comment.xpm"
#include "icons/poedit-fileopen.xpm"
#include "icons/poedit-filesave.xpm"
#include "icons/poedit-fullscreen.xpm"
#include "icons/poedit-fuzzy.xpm"
#include "icons/poedit-help.xpm"
#include "icons/poedit-prj-delete.xpm"
#include "icons/poedit-prj-edit.xpm"
#include "icons/poedit-prj-new.xpm"
#include "icons/poedit-quotes.xpm"
#include "icons/poedit-update.xpm"

#include "icons/poedit-status-automatic.xpm"
#include "icons/poedit-status-cat-mid.xpm"
#include "icons/poedit-status-cat-no.xpm"
#include "icons/poedit-status-cat-ok.xpm"
#include "icons/poedit-status-comment-modif.xpm"
#include "icons/poedit-status-comment.xpm"
#include "icons/poedit-status-modified.xpm"
#include "icons/poedit-status-nothing.xpm"


wxBitmap wxPoeditStdArtProvider::CreateBitmap(const wxArtID& id,
                                              const wxArtClient& client,
                                              const wxSize& size)
{
#ifdef __UNIX__
    if (id == _T("poedit-appicon"))
        return wxBitmap(appicon_xpm);
#endif

    #define ICON(name, data) if (id == _T(name)) return wxBitmap(data);

    ICON("poedit-comment",               poedit_comment_xpm)
    ICON("poedit-fileopen",              poedit_fileopen_xpm)
    ICON("poedit-filesave",              poedit_filesave_xpm)
    ICON("poedit-fullscreen",            poedit_fullscreen_xpm)
    ICON("poedit-fuzzy",                 poedit_fuzzy_xpm)
    ICON("poedit-help",                  poedit_help_xpm)
    ICON("poedit-quotes",                poedit_quotes_xpm)
    ICON("poedit-update",                poedit_update_xpm)
    ICON("poedit-prj-delete",            poedit_prj_delete_xpm)
    ICON("poedit-prj-edit",              poedit_prj_edit_xpm)
    ICON("poedit-prj-new",               poedit_prj_new_xpm)
    
    ICON("poedit-status-automatic",      poedit_status_automatic_xpm)
    ICON("poedit-status-comment",        poedit_status_comment_xpm)
    ICON("poedit-status-comment-modif",  poedit_status_comment_modif_xpm)
    ICON("poedit-status-modified",       poedit_status_modified_xpm)
    ICON("poedit-status-nothing",        poedit_status_nothing_xpm)
    ICON("poedit-status-cat-mid",        poedit_status_cat_mid_xpm)
    ICON("poedit-status-cat-no",         poedit_status_cat_no_xpm)
    ICON("poedit-status-cat-ok",         poedit_status_cat_ok_xpm)
    
    #undef ICON
 
    return wxNullBitmap;
}


#ifdef HAS_THEMES_SUPPORT
wxBitmap wxPoeditThemeArtProvider::CreateBitmap(const wxArtID& id,
                                                const wxArtClient& client,
                                                const wxSize& size)
{
    wxLogTrace(_T("poedit"), _T("icon '%s' cli '%s'"),
            id.c_str(), client.c_str());
    #define ICON(poedit, theme) \
        if (id == _T(poedit)) \
            return wxArtProvider::GetBitmap(theme, client, size);

    ICON("poedit-appicon",         _T("poedit"))
    ICON("poedit-update",          _T("stock_update-data"))
    ICON("poedit-fullscreen",      _T("stock_fullscreen"))
    ICON("poedit-fileopen",        wxART_FILE_OPEN)
    ICON("poedit-filesave",        _T("gtk-save"))
    ICON("poedit-help",            wxART_HELP)
    ICON("poedit-comment",         _T("stock_notes"))
    ICON("poedit-fuzzy",           _T("stock_unknown"))
    ICON("poedit-quotes",          _T("stock_nonprinting-chars"))
    ICON("poedit-prj-new",         _T("gtk-new"))
    ICON("poedit-prj-edit",        _T("stock_edit"))
    ICON("poedit-prj-delete",      _T("gtk-delete"))

    #undef ICON
    return wxNullBitmap;
}
#endif // HAS_THEMES_SUPPORT
