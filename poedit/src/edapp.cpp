
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

#include <wx/wx.h>
#include <wx/fs_zip.h>
#include <wx/image.h>
#include <wx/xml/xmlres.h>
#include <wx/xml/xh_all.h>

#include "edframe.h"
#include "prefsdlg.h"

class poEditApp : public wxApp
{
    public:
        bool OnInit();
};

IMPLEMENT_APP(poEditApp)



static void InitParsersCfg(wxConfigBase *cfg)
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





extern void InitXmlResource();

bool poEditApp::OnInit()
{
    SetVendorName("Vaclav Slavik");
    SetAppName("poedit");
    
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
#ifndef __WXMSW__
    wxFileSystem::AddHandler(new wxZipFSHandler);
#endif

    //wxTheXmlResource->InitAllHandlers();
    wxTheXmlResource->AddHandler(new wxMenuXmlHandler);
    wxTheXmlResource->AddHandler(new wxMenuBarXmlHandler);
    wxTheXmlResource->AddHandler(new wxDialogXmlHandler);
    wxTheXmlResource->AddHandler(new wxPanelXmlHandler);
    wxTheXmlResource->AddHandler(new wxButtonXmlHandler);
    wxTheXmlResource->AddHandler(new wxGaugeXmlHandler);
    wxTheXmlResource->AddHandler(new wxCheckBoxXmlHandler);
    wxTheXmlResource->AddHandler(new wxStaticTextXmlHandler);
    wxTheXmlResource->AddHandler(new wxStaticBitmapXmlHandler);
    wxTheXmlResource->AddHandler(new wxComboBoxXmlHandler);
    wxTheXmlResource->AddHandler(new wxSizerXmlHandler);
    wxTheXmlResource->AddHandler(new wxNotebookXmlHandler);
    wxTheXmlResource->AddHandler(new wxTextCtrlXmlHandler);
    wxTheXmlResource->AddHandler(new wxListBoxXmlHandler);
    wxTheXmlResource->AddHandler(new wxToolBarXmlHandler);
    
    InitXmlResource();
    
    poEditFrame *frame = new poEditFrame("poEdit", argc > 1 ? argv[1] : "");

    frame->Show(true);
    SetTopWindow(frame);
    
    if (wxConfig::Get()->Read("translator_name", "nothing") == "nothing")
    {
        wxMessageBox(_("This is first time you run poEdit.\n"
                       "Please fill in your name and e-mail address.\n"
                       "(This information is used only in catalogs headers)"), "Setup",
                       wxOK | wxICON_INFORMATION);
                       
        InitParsersCfg(wxConfig::Get());
                       
        PreferencesDialog dlg(frame);
        dlg.TransferTo(wxConfig::Get());
        if (dlg.ShowModal() == wxID_OK)
            dlg.TransferFrom(wxConfig::Get());
    }

    return true;
}
