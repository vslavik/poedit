/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2000-2015 Vaclav Slavik
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
 */

#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/bmpbuttn.h>
#include <wx/combobox.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/dnd.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#include <wx/textdlg.h>
#include <wx/editlbox.h>
#include <wx/xrc/xmlres.h>
#include <wx/config.h>
#include <wx/intl.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/menu.h>
#include <wx/notebook.h>

#include <algorithm>
#include <functional>
#include <memory>

#include "propertiesdlg.h"
#include "hidpi.h"
#include "language.h"
#include "pluralforms/pl_evaluate.h"
#include "utility.h"

namespace
{

inline wxString NormalizedPath(const wxString& fn)
{
    auto f = MakeFileName(fn);
    f.Normalize();
    return f.GetFullPath();
}

inline wxString RelativePath(const wxString& fn, const wxString& to)
{
    if (fn == to)
        return ".";
    auto f = MakeFileName(fn);
    f.MakeRelativeTo(to);
    return f.GetFullPath();
}

} // anonymous namespace

struct PropertiesDialog::PathsData
{
    PathsData() : Changed(false) {}

    std::function<void()> RefreshView;

    // Did the data change in any way?
    bool Changed;

    // all paths here are absolute, normalized paths

    // directory where the PO(T) file is
    wxString filedir;

    // catalog settings
    wxString basepath;
    wxArrayString paths;
    wxArrayString excluded;

    void GetFromCatalog(CatalogPtr cat)
    {
        Changed = false;
        auto& hdr = cat->Header();
        filedir = wxFileName(cat->GetFileName()).GetPathWithSep();
        basepath = cat->GetSourcesBasePath();
        if (basepath.empty())
            basepath = filedir;
        paths.clear();
        excluded.clear();
        for (auto& p: hdr.SearchPaths)
        {
            if (p.empty())
                continue;
            paths.push_back(NormalizedPath(basepath + p));
        }
        for (auto& p: hdr.SearchPathsExcluded)
        {
            if (p.empty())
                continue;
            if (wxIsWild(p))
                excluded.push_back(p);
            else
                excluded.push_back(NormalizedPath(basepath + p));
        }
    }

    void SetToCatalog(CatalogPtr cat) const
    {
        auto& hdr = cat->Header();
        hdr.BasePath = RelativePath(basepath, filedir);
        hdr.SearchPaths.clear();
        hdr.SearchPathsExcluded.clear();
        for (auto& p: paths)
        {
            auto rel = RelativePath(p, basepath);
            if (rel.Last() == wxFILE_SEP_PATH)
                rel.RemoveLast();
            hdr.SearchPaths.push_back(rel);
        }
        for (auto& p: excluded)
        {
            if (wxIsWild(p))
            {
                hdr.SearchPathsExcluded.push_back(p);
            }
            else
            {
                auto rel = RelativePath(p, basepath);
                if (rel.Last() == wxFILE_SEP_PATH)
                    rel.RemoveLast();
                hdr.SearchPathsExcluded.push_back(rel);
            }
        }
    }

    void UpdateBasePath()
    {
        if (!paths.empty())
            basepath = CommonDirectory(paths).GetFullPath();
        else
            basepath = filedir;
    }
};


class PropertiesDialog::PathsList : public wxPanel
{
public:
    PathsList(wxWindow *parent, const wxString& label, std::shared_ptr<PathsData> data)
        : wxPanel(parent, wxID_ANY),
          m_data(data)
    {
#if defined(__WXOSX__)
        SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        // FIXME: gross hack to make inside-notebook color to match
        SetBackgroundColour(parent->GetBackgroundColour().ChangeLightness(93));
#elif defined(__WXMSW__)
        SetBackgroundColour(*wxWHITE);
#endif

        auto sizer = new wxBoxSizer(wxVERTICAL);
        SetSizer(sizer);

        auto lbl = new wxStaticText(this, wxID_ANY, label);
        sizer->Add(lbl, wxSizerFlags().Expand());
        m_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_EXTENDED);
        sizer->Add(m_list, wxSizerFlags(1).Expand());

#if defined(__WXOSX__)
        auto add = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap("NSAddTemplate"), wxDefaultPosition, wxSize(18, 18), wxBORDER_SUNKEN);
        auto remove = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap("NSRemoveTemplate"), wxDefaultPosition, wxSize(18,18), wxBORDER_SUNKEN);
#elif defined(__WXMSW__)
        auto add = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap("list-add"), wxDefaultPosition, wxSize(19,19));
        auto remove = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap("list-remove"), wxDefaultPosition, wxSize(19,19));
#elif defined(__WXGTK__)
        auto add = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap("list-add"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        auto remove = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap("list-remove"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
#endif
        auto buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        buttonSizer->Add(add);
        buttonSizer->Add(remove);
        sizer->Add(buttonSizer);

        SetDropTarget(new DropTarget(this));

        add->Bind(wxEVT_BUTTON, &PathsList::OnAddMenu, this);
        remove->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e){
            wxArrayInt dummy;
            e.Enable(m_list->GetSelections(dummy) > 0);
        });
        remove->Bind(wxEVT_BUTTON, [=](wxCommandEvent&){
            wxArrayInt s;
            m_list->GetSelections(s);
            Remove(s);
        });

#if 0
        // TODO: Add background overlay with instructions:
        _("<big>Drag and Drop Folders Here</big>\n\nor use the + button")
        _("<big>Drag and drop folders here</big>\n\nor use the + button")
#endif
    }

    void UpdateFromData()
    {
        m_list->Clear();
        for (auto& p: Array())
        {
            if (wxIsWild(p))
                m_list->Append(p);
            else
                m_list->Append(RelativePath(p, m_data->basepath));
        }
    }

    void Add(const wxArrayString& files)
    {
        auto& a = Array();
        for (auto& f: files)
        {
            if (wxIsWild(f))
                a.push_back(f);
            else
                a.push_back(NormalizedPath(f));
        }
        m_data->Changed = true;
        m_data->UpdateBasePath();
        m_data->RefreshView();
    }

    void Add(const wxString& f)
    {
        Add(wxArrayString(1, &f));
    }

    void Remove(wxArrayInt selection)
    {
        auto& a = Array();
        std::sort(selection.begin(), selection.end());
        for (auto i = selection.rbegin(); i != selection.rend(); ++i)
            a.RemoveAt(*i);
        m_data->Changed = true;
        m_data->UpdateBasePath();
        m_data->RefreshView();
    }

protected:
    virtual wxArrayString& Array() = 0;
    virtual bool AllowWildcards() const = 0;

    class DropTarget : public wxFileDropTarget
    {
    public:
        DropTarget(PathsList *parent) : m_parent(parent) {}

        bool OnDropFiles(wxCoord, wxCoord, const wxArrayString& files) override
        {
            m_parent->Add(files);
            return true;
        }

        wxDragResult OnDragOver(wxCoord, wxCoord, wxDragResult) override { return wxDragCopy; }

        PathsList *m_parent;
    };

    void OnAddMenu(wxCommandEvent& e)
    {
        static const auto idFolder = wxNewId();
        static const auto idFile = wxNewId();
        static const auto idWild = wxNewId();

        wxMenu *menu = new wxMenu();
        menu->Append(idFolder, MSW_OR_OTHER(_("Add folders..."), _("Add Folders...")));
        menu->Append(idFile, MSW_OR_OTHER(_("Add files..."), _("Add Files...")));
        if (AllowWildcards())
            menu->Append(idWild, MSW_OR_OTHER(_("Add wildcard..."), _("Add Wildcard...")));

        menu->Bind(wxEVT_MENU, [=](wxCommandEvent&){
            wxDirDialog dlg(this,
                            OSX_OR_OTHER("", _("Select directory")),
                            m_data->filedir,
                            wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
            if (dlg.ShowModal() == wxID_OK)
                Add(dlg.GetPath());
        },
        idFolder);

        menu->Bind(wxEVT_MENU, [=](wxCommandEvent&){
            wxFileDialog dlg(this,
                             "",
                             m_data->filedir,
                             "",
                             wxFileSelectorDefaultWildcardStr,
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
            if (dlg.ShowModal() != wxID_OK)
                return;
            wxArrayString files;
            dlg.GetPaths(files);
            Add(files);
        },
        idFile);

        menu->Bind(wxEVT_MENU, [=](wxCommandEvent&){
            wxTextEntryDialog dlg(this, "", "");
            if (dlg.ShowModal() != wxID_OK)
                return;
            Add(dlg.GetValue());
        },
        idWild);

        auto win = dynamic_cast<wxButton*>(e.GetEventObject());
#ifdef __WXOSX__
        win->PopupMenu(menu, 9, 29);
#else
        win->PopupMenu(menu, 0, win->GetSize().y);
#endif
    }

    std::shared_ptr<PathsData> m_data;
    wxListBox *m_list;
};

class PropertiesDialog::SourcePathsList : public PropertiesDialog::PathsList
{
public:
    SourcePathsList(wxWindow *parent, std::shared_ptr<PathsData> data)
        : PathsList(parent, _("Paths"), data) {}

protected:
    wxArrayString& Array() override { return m_data->paths; }
    bool AllowWildcards() const override { return false; }
};

class PropertiesDialog::ExcludedPathsList : public PropertiesDialog::PathsList
{
public:
    ExcludedPathsList(wxWindow *parent, std::shared_ptr<PathsData> data)
        : PathsList(parent, _("Excluded paths"), data) {}

protected:
    wxArrayString& Array() override { return m_data->excluded; }
    bool AllowWildcards() const override { return true; }
};


PropertiesDialog::PropertiesDialog(wxWindow *parent, CatalogPtr cat, bool fileExistsOnDisk, int initialPage)
    : m_validatedPlural(-1), m_validatedLang(-1)
{
    m_hasLang = cat->HasCapability(Catalog::Cap::LanguageSetting);

    wxXmlResource::Get()->LoadDialog(this, parent, "properties");

    m_team = XRCCTRL(*this, "team_name", wxTextCtrl);
    m_teamEmail = XRCCTRL(*this, "team_email", wxTextCtrl);
    m_project = XRCCTRL(*this, "prj_name", wxTextCtrl);
    m_language = XRCCTRL(*this, "language", LanguageCtrl);
    m_charset = XRCCTRL(*this, "charset", wxComboBox);
    m_sourceCodeCharset = XRCCTRL(*this, "source_code_charset", wxComboBox);

    m_pluralFormsDefault = XRCCTRL(*this, "plural_forms_default", wxRadioButton);
    m_pluralFormsCustom = XRCCTRL(*this, "plural_forms_custom", wxRadioButton);
    m_pluralFormsExpr = XRCCTRL(*this, "plural_forms_expr", wxTextCtrl);
#if defined(__WXMSW__) && !wxCHECK_VERSION(3,1,0)
    m_pluralFormsExpr->SetFont(m_pluralFormsExpr->GetFont().Smaller());
#else
    m_pluralFormsExpr->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
    m_basePath = XRCCTRL(*this, "basepath", wxTextCtrl);

    if (!m_hasLang)
    {
        for (auto w: { (wxWindow*)m_language,
                       (wxWindow*)m_pluralFormsDefault,
                       (wxWindow*)m_pluralFormsCustom,
                       (wxWindow*)m_pluralFormsExpr,
                       XRCCTRL(*this, "language_label", wxWindow),
                       XRCCTRL(*this, "plural_forms_label", wxWindow) })
        {
            w->GetContainingSizer()->Hide(w);
        }
    }

    // my custom controls:
    m_keywords = new wxEditableListBox(this, -1, _("Additional keywords"));

    m_pathsData.reset(new PathsData);
    m_paths = new SourcePathsList(this, m_pathsData);
    m_excludedPaths = new ExcludedPathsList(this, m_pathsData);
    m_pathsData->RefreshView = [=]{
        m_basePath->SetValue(RelativePath(m_pathsData->basepath, m_pathsData->filedir));
        m_paths->UpdateFromData();
        m_excludedPaths->UpdateFromData();
    };

    m_paths->SetMinSize(wxSize(-1, PX(90)));
    m_excludedPaths->SetMinSize(wxSize(-1, PX(90)));

#ifdef __WXOSX__
    for (auto c: m_keywords->GetChildren())
    {
        c->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        for (auto c2: c->GetChildren())
            c2->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    }
#endif // __WXOSX__

    wxXmlResource::Get()->AttachUnknownControl("keywords", m_keywords);
    wxXmlResource::Get()->AttachUnknownControl("paths", m_paths);
    wxXmlResource::Get()->AttachUnknownControl("excluded_paths", m_excludedPaths);

    // Controls setup:
    m_project->SetHint(_("Name of the project the translation is for"));
    m_pluralFormsExpr->SetHint(_("e.g. nplurals=2; plural=(n > 1);"));

    Layout();
    GetSizer()->SetSizeHints(this);

    if (!fileExistsOnDisk)
        DisableSourcesControls();

    auto nb = XRCCTRL(*this, "properties_notebook", wxNotebook);
    nb->SetSelection(initialPage);

    m_language->Bind(wxEVT_TEXT, &PropertiesDialog::OnLanguageChanged, this);
    m_language->Bind(wxEVT_COMBOBOX, &PropertiesDialog::OnLanguageChanged, this);

    m_pluralFormsDefault->Bind(wxEVT_RADIOBUTTON, &PropertiesDialog::OnPluralFormsDefault, this);
    m_pluralFormsCustom->Bind(wxEVT_RADIOBUTTON, &PropertiesDialog::OnPluralFormsCustom, this);
    m_pluralFormsExpr->Bind(
        wxEVT_UPDATE_UI,
        [=](wxUpdateUIEvent& e){ e.Enable(m_pluralFormsCustom->GetValue()); });
    m_pluralFormsExpr->Bind(
        wxEVT_TEXT,
        [=](wxCommandEvent& e){ m_validatedPlural = -1; e.Skip(); });
    Bind(wxEVT_UPDATE_UI,
        [=](wxUpdateUIEvent& e){ e.Enable(Validate()); },
        wxID_OK);
    CallAfter([=]{
        m_project->SetFocus();
    });
}


namespace
{

#define UTF_8_CHARSET  _("UTF-8 (recommended)")

void SetCharsetToCombobox(wxComboBox *ctrl, const wxString& value)
{
    static const wxString all_charsets[] =
        {
        UTF_8_CHARSET,
        // and legacy ones
        "iso-8859-1",
        "iso-8859-2",
        "iso-8859-3",
        "iso-8859-4",
        "iso-8859-5",
        "iso-8859-6",
        "iso-8859-7",
        "iso-8859-8",
        "iso-8859-9",
        "iso-8859-10",
        "iso-8859-11",
        "iso-8859-12",
        "iso-8859-13",
        "iso-8859-14",
        "iso-8859-15",
        "koi8-r",
        "windows-1250",
        "windows-1251",
        "windows-1252",
        "windows-1253",
        "windows-1254",
        "windows-1255",
        "windows-1256",
        "windows-1257"
        };

    ctrl->Clear();
    for ( int i = 0; i < (int)WXSIZEOF(all_charsets); i++ )
        ctrl->Append(all_charsets[i]);

    const wxString low = value.Lower();
    if ( low == "utf-8" || low == "utf8" )
        ctrl->SetValue(UTF_8_CHARSET);
    else
        ctrl->SetValue(value);
}

wxString GetCharsetFromCombobox(wxComboBox *ctrl)
{
    wxString c = ctrl->GetValue();
    if ( c == UTF_8_CHARSET )
        c = "UTF-8";
    return c;
}

void GetKeywordsFromControl(wxEditableListBox *box, wxArrayString& output)
{
    wxArrayString arr;
    box->GetStrings(arr);

    output.clear();
    for (auto x: arr)
    {
        if (x.empty())
            continue;
        wxString rest;
        if (x.EndsWith(" ()", &rest) || x.EndsWith("()", &rest))
            x = rest;
        output.push_back(x);
    }
}

} // anonymous namespace


void PropertiesDialog::TransferTo(const CatalogPtr& cat)
{
    SetCharsetToCombobox(m_charset, cat->Header().Charset);
    SetCharsetToCombobox(m_sourceCodeCharset, cat->Header().SourceCodeCharset);

    #define SET_VAL(what,what2) m_##what2->SetValue(cat->Header().what)
    SET_VAL(Team, team);
    SET_VAL(TeamEmail, teamEmail);
    SET_VAL(Project, project);
    #undef SET_VAL

    if (m_hasLang)
    {
        m_language->SetLang(cat->Header().Lang);
        OnLanguageValueChanged(m_language->GetValue());

        wxString pf_def = cat->Header().Lang.DefaultPluralFormsExpr();
        wxString pf_cat = cat->Header().GetHeader("Plural-Forms");
        if (pf_cat == "nplurals=INTEGER; plural=EXPRESSION;")
            pf_cat = pf_def;

        m_pluralFormsExpr->SetValue(pf_cat);
        if (!pf_cat.empty() && pf_cat == pf_def)
            m_pluralFormsDefault->SetValue(true);
        else
            m_pluralFormsCustom->SetValue(true);
    }

    m_keywords->SetStrings(cat->Header().Keywords);

    m_pathsData->GetFromCatalog(cat);
    m_pathsData->RefreshView();
}


void PropertiesDialog::TransferFrom(const CatalogPtr& cat)
{
    cat->Header().Charset = GetCharsetFromCombobox(m_charset);
    cat->Header().SourceCodeCharset = GetCharsetFromCombobox(m_sourceCodeCharset);

    #define GET_VAL(what,what2) cat->Header().what = m_##what2->GetValue()
    GET_VAL(Team, team);
    GET_VAL(TeamEmail, teamEmail);
    GET_VAL(Project, project);
    #undef GET_VAL

    if (m_hasLang)
    {
        Language lang = m_language->GetLang();
        if (lang.IsValid())
            cat->Header().Lang = lang;

        wxString pluralForms;
        if (m_pluralFormsDefault->GetValue() && cat->Header().Lang.IsValid())
        {
            pluralForms = cat->Header().Lang.DefaultPluralFormsExpr();
        }

        if (pluralForms.empty())
        {
            pluralForms = m_pluralFormsExpr->GetValue().Strip(wxString::both);
            if ( !pluralForms.empty() && !pluralForms.EndsWith(";") )
                pluralForms += ";";
        }
        cat->Header().SetHeaderNotEmpty("Plural-Forms", pluralForms);
    }

    GetKeywordsFromControl(m_keywords, cat->Header().Keywords);

    if (m_pathsData->Changed)
        m_pathsData->SetToCatalog(cat);
}


void PropertiesDialog::DisableSourcesControls()
{
    m_basePath->Disable();
    for (auto p: {m_paths, m_excludedPaths})
    {
        p->Disable();
        for (auto c: p->GetChildren())
            c->Disable();
    }

    auto label = XRCCTRL(*this, "sources_path_label", wxStaticText);
    label->SetLabel(_("Please save the file first. This section cannot be edited until then."));
    label->SetForegroundColour(*wxRED);
}


void PropertiesDialog::OnLanguageChanged(wxCommandEvent& event)
{
    m_validatedLang = -1;
    OnLanguageValueChanged(event.GetString());
    event.Skip();
}

void PropertiesDialog::OnLanguageValueChanged(const wxString& langstr)
{
    Language lang = Language::TryParse(langstr.ToStdWstring());
    wxString pluralForm = lang.DefaultPluralFormsExpr();
    if (pluralForm.empty())
    {
        m_pluralFormsDefault->Disable();
        m_pluralFormsCustom->SetValue(true);
    }
    else
    {
        m_pluralFormsDefault->Enable();
        if (m_pluralFormsExpr->GetValue().empty() ||
            m_pluralFormsExpr->GetValue() == pluralForm)
        {
            m_pluralFormsDefault->SetValue(true);
        }
    }
}

void PropertiesDialog::OnPluralFormsDefault(wxCommandEvent& event)
{
    m_rememberedPluralForm = m_pluralFormsExpr->GetValue();

    Language lang = m_language->GetLang();
    if (lang.IsValid())
    {
        wxString defaultForm = lang.DefaultPluralFormsExpr();
        if (!defaultForm.empty())
            m_pluralFormsExpr->SetValue(defaultForm);
    }

    event.Skip();
}

void PropertiesDialog::OnPluralFormsCustom(wxCommandEvent& event)
{
    if (!m_rememberedPluralForm.empty())
        m_pluralFormsExpr->SetValue(m_rememberedPluralForm);

    event.Skip();
}


bool PropertiesDialog::Validate()
{
    if (!m_hasLang)
        return true;

    if (m_validatedPlural == -1)
    {
        m_validatedPlural = 1;
        if (m_pluralFormsCustom->GetValue())
        {
            wxString form = m_pluralFormsExpr->GetValue();
            if (!form.empty())
            {
                std::unique_ptr<PluralFormsCalculator> calc(PluralFormsCalculator::make(form.ToAscii()));
                if (!calc)
                    m_validatedPlural = 0;
            }
        }
    }

    if (m_validatedLang == -1)
    {
        m_validatedLang = m_language->IsValid() ? 1 : 0;
    }

    return m_validatedLang == 1 &&
           m_validatedPlural == 1;
}
