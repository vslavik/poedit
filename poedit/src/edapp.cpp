
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      edapp.cpp
    
      Application class
    
      (c) Vaclav Slavik, 1999

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/wxprec.h>

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/fs_zip.h>
#include <wx/image.h>
#include <wx/xrc/xmlres.h>
#include <wx/xrc/xh_all.h>

#include "edapp.h"
#include "edframe.h"
#include "prefsdlg.h"


IMPLEMENT_APP(poEditApp);

wxString poEditApp::GetAppPath() const
{
#if defined(__UNIX__)
    return POEDIT_PREFIX;
#elif defined(__WXMSW__)
    wxString path = wxConfigBase::Get()->Read("application_path", "");
    if (!path)
    {
        wxLogError(_("poEdit installation is broken, cannot find application's home directory."));
        path = ".";
    }
    return path;
#else
#error "Unsupported platform!"
#endif
}

wxString poEditApp::GetAppVersion() const
{
    return "1.1.2";
}


#ifdef __UNIX__
#define CFG_FILE (home + ".poedit/config")
#else
#define CFG_FILE ""
#endif

bool poEditApp::OnInit()
{
#ifdef __UNIX__
    wxString home = wxGetHomeDir() + "/";

    // create poEdit cfg dir, move ~/.poedit to ~/.poedit/config
    // (upgrade from older versions of poEdit which used ~/.poedit file)
    if (!wxDirExists(home + ".poedit"))
    {
        if (wxFileExists(home + ".poedit"))
            wxRenameFile(home + ".poedit", home + ".poedit2");
        wxMkdir(home + ".poedit");
        if (wxFileExists(home + ".poedit2"))
            wxRenameFile(home + ".poedit2", home + ".poedit/config");
    }
#endif

    SetVendorName("Vaclav Slavik");
    SetAppName("poedit");
    wxConfigBase::Set(
        new wxConfig("", "", CFG_FILE, "", 
                     wxCONFIG_USE_GLOBAL_FILE | wxCONFIG_USE_LOCAL_FILE));
    
#ifdef __UNIX__
    // we have to do this because otherwise xgettext might
    // speak in native language, not English, and we cannot parse
    // it correctly (not yet)
    // FIXME - better parsing of xgettext's stderr output
    putenv("LC_ALL=en");
    putenv("LC_MESSAGES=en");
    putenv("LANG=en");
#endif

    wxImage::AddHandler(new wxGIFHandler);
    wxFileSystem::AddHandler(new wxZipFSHandler);

    wxTheXmlResource->InitAllHandlers();
    wxTheXmlResource->Load(GetAppPath() + "/share/poedit/resources.zip");
    
    poEditFrame *frame = new poEditFrame("poEdit", argc > 1 ? argv[1] : "");

    frame->Show(true);
    SetTopWindow(frame);
    
    SetDefaultCfg(wxConfig::Get());

    if (wxConfig::Get()->Read("translator_name", "nothing") == "nothing")
    {
        wxMessageBox(_("This is first time you run poEdit.\n"
                       "Please fill in your name and e-mail address.\n"
                       "(This information is used only in catalogs headers)"), "Setup",
                       wxOK | wxICON_INFORMATION);
                       
        PreferencesDialog dlg(frame);
        dlg.TransferTo(wxConfig::Get());
        if (dlg.ShowModal() == wxID_OK)
            dlg.TransferFrom(wxConfig::Get());
    }

    return true;
}

void poEditApp::SetDefaultCfg(wxConfigBase *cfg)
{
    if (cfg->Read("version", "") == GetAppVersion()) return;

    if (cfg->Read("Parsers/List", "").IsEmpty())
    {
        cfg->Write("Parsers/List", "C/C++");

        cfg->Write("Parsers/C_C++/Extensions", 
                   "*.c;*.cpp;*.h;*.hpp;*.cc;*.C;*.cxx;*.hxx");
        cfg->Write("Parsers/C_C++/Command", 
                   "xgettext --force-po -C -o %o %K %F");
        cfg->Write("Parsers/C_C++/KeywordItem", 
                   "-k%k");
        cfg->Write("Parsers/C_C++/FileItem", 
                   "%f");
    }

    if (cfg->Read("TM/database_path", "").IsEmpty())
    {
        wxString dbpath;
#if defined(__UNIX__)
        dbpath = wxGetHomeDir() + "/.poedit/tm";
#elif defined(__WXMSW__)
        // VS: this distinguishes between NT and Win9X systems -- the former
        //     has users' home directories while on the latter wxGetHomeDir
        //     will return path of the executable
        if (wxGetHomeDir().IsSameAs(GetAppPath() + "\\bin", false))      
            dbpath = GetAppPath() + "\\share\\poedit\\tm";
        else
            dbpath = wxGetHomeDir() + "\\poedit_tm";
#endif
        cfg->Write("TM/database_path", dbpath);
    }

    if (cfg->Read("TM/search_paths", "").IsEmpty())
    {
        wxString paths;
#if defined(__UNIX__)
        paths = wxGetHomeDir() + ":/usr/share/locale:/usr/local/share/locale";
#elif defined(__WXMSW__)
        paths = "C:";
#endif
        cfg->Write("TM/search_paths", paths);
    }

    cfg->Write("version", GetAppVersion());
}
