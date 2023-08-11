/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2023 Vaclav Slavik
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

#include "cloud_accounts_ui.h"

#ifdef HAVE_HTTP_CLIENT

#include "customcontrols.h"
#include "http_client.h"
#include "languagectrl.h"
#include "str_helpers.h"
#include "utility.h"

#include "crowdin_client.h"
#include "crowdin_gui.h"
#include "localazy_client.h"
#include "localazy_gui.h"

#include <unicode/coll.h>
#include <boost/algorithm/string.hpp>

#include <wx/choice.h>
#include <wx/simplebook.h>
#include <wx/sizer.h>
#include <wx/statline.h>

#ifdef __WXMSW__
    #include <wx/generic/private/markuptext.h>
#endif

#if !wxCHECK_VERSION(3,1,0)
    #define CenterVertical() Center()
#endif

namespace
{

inline std::vector<CloudAccountClient*> GetSignedInAccounts()
{
    std::vector<CloudAccountClient*> all;

    if (CrowdinClient::Get().IsSignedIn())
        all.push_back(&CrowdinClient::Get());
    if (LocalazyClient::Get().IsSignedIn())
        all.push_back(&LocalazyClient::Get());

    return all;
}

} // anonymous namespace


ServiceSelectionPanel::ServiceSelectionPanel(wxWindow *parent) : wxPanel(parent, wxID_ANY)
{
    auto topsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topsizer);
    m_sizer = new wxBoxSizer(wxVERTICAL);
    topsizer->AddStretchSpacer();
    topsizer->Add(m_sizer, wxSizerFlags().Expand().Border(wxALL, PX(16)));
    topsizer->AddStretchSpacer();
}


void ServiceSelectionPanel::AddService(AccountDetailPanel *account)
{
    if (!GetChildren().empty())
        m_sizer->Add(new wxStaticLine(this, wxID_ANY), wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM, PX(24)));

    auto logo = new StaticBitmap(this, account->GetServiceLogo());
    logo->SetCursor(wxCURSOR_HAND);
    logo->Bind(wxEVT_LEFT_UP, [=](wxMouseEvent&){ wxLaunchDefaultBrowser(account->GetServiceLearnMoreURL()); });
    m_sizer->Add(logo, wxSizerFlags().PXDoubleBorder(wxBOTTOM));
    auto explain = new ExplanationLabel(this, account->GetServiceDescription());
    m_sizer->Add(explain, wxSizerFlags().Expand());

    auto signIn = new wxButton(this, wxID_ANY, MSW_OR_OTHER(_("Add account"), _("Add Account")));
    signIn->Bind(wxEVT_BUTTON, [=](wxCommandEvent&){ account->SignIn(); });

    auto learnMore = new LearnMoreLink(this,
                                       account->GetServiceLearnMoreURL(),
                                       // TRANSLATORS: %s is online service name, e.g. "Crowdin" or "Localazy"
                                       wxString::Format(_("Learn more about %s"), account->GetServiceName()));

    auto buttons = new wxBoxSizer(wxHORIZONTAL);
    m_sizer->Add(buttons, wxSizerFlags().Expand().Border(wxTOP, PX(16)));
    buttons->Add(learnMore, wxSizerFlags().Center().Border(wxLEFT, PX(LearnMoreLink::EXTRA_INDENT)));
    buttons->AddStretchSpacer();
    buttons->Add(signIn, wxSizerFlags());
}


AccountsPanel::AccountsPanel(wxWindow *parent) : wxPanel(parent, wxID_ANY)
{
    wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topsizer);

    topsizer->Add(new ExplanationLabel(this, _("Connect Poedit with supported online localization platforms to seamlessly sync translations managed on them.")),
                  wxSizerFlags().Expand().PXDoubleBorder(wxBOTTOM));

    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    topsizer->Add(sizer, wxSizerFlags(1).Expand());

    m_list = new IconAndSubtitleListCtrl(this, _("Account"), MSW_OR_OTHER(wxBORDER_SIMPLE, wxBORDER_SUNKEN));
    sizer->Add(m_list, wxSizerFlags().Expand().Border(wxRIGHT, PX(10)));

    m_panelsBook = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | MSW_OR_OTHER(wxBORDER_SIMPLE, wxBORDER_SUNKEN));
    ColorScheme::SetupWindowColors(m_panelsBook, [=]
    {
        if (ColorScheme::GetWindowMode(m_panelsBook) == ColorScheme::Light)
            m_panelsBook->SetBackgroundColour(*wxWHITE);
        else
            m_panelsBook->SetBackgroundColour(wxNullColour);
    });

    sizer->Add(m_panelsBook, wxSizerFlags(1).Expand());

    m_introPanel = new ServiceSelectionPanel(m_panelsBook);
    m_panelsBook->AddPage(m_introPanel, "");

    AddAccount("Crowdin", "AccountCrowdin", new CrowdinLoginPanel(m_panelsBook));
    AddAccount("Localazy", "AccountLocalazy", new LocalazyLoginPanel(m_panelsBook));

    m_list->SetMinSize(wxSize(PX(180), -1));
    m_panelsBook->SetMinSize(wxSize(PX(320), -1));

    m_list->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &AccountsPanel::OnSelectAccount, this);
}


void AccountsPanel::InitializeAfterShown()
{
    for (auto& p: m_panels)
        p->EnsureInitialized();
    m_list->SetFocus();
}


void AccountsPanel::AddAccount(const wxString& name, const wxString& iconId, AccountDetailPanel *panel)
{
    auto pos = (unsigned)m_panels.size();
    m_panels.push_back(panel);
    m_panelsBook->AddPage(panel, "");

    m_introPanel->AddService(panel);

    m_list->AppendFormattedItem(wxArtProvider::GetBitmap(iconId), name, " ... ");

    panel->NotifyContentChanged = [=]{
        wxString desc = panel->IsSignedIn() ? panel->GetLoginName() : _("(not signed in)");
        m_list->UpdateFormattedItem(pos, name, desc);
        // select 1st available signed-in service if we can and hide the intro panel:
        if (m_list->GetSelectedRow() == wxNOT_FOUND && panel->IsSignedIn())
            m_list->SelectRow(pos);
    };
    panel->NotifyShouldBeRaised = [=]{
        m_list->SelectRow(pos);
    };
}


void AccountsPanel::OnSelectAccount(wxDataViewEvent& event)
{
    auto index = m_list->ItemToRow(event.GetItem());
    if (index == wxNOT_FOUND || index >= (int)m_panels.size())
        return;

    m_panelsBook->SetSelection(1 + index);
}


namespace
{

template<typename T, typename Key>
void SortAlphabetically(std::vector<T>& items, Key func)
{
    UErrorCode err = U_ZERO_ERROR;
    std::unique_ptr<icu::Collator> coll(icu::Collator::createInstance(err));
    if (coll)
        coll->setStrength(icu::Collator::SECONDARY); // case insensitive

    std::sort
    (
        items.begin(), items.end(),
        [&coll,&func](const T& a, const T& b)
        {
            auto ka = func(a);
            auto kb = func(b);
            if (coll)
            {
                UErrorCode e = U_ZERO_ERROR;
                return coll->compare(str::to_icu(ka), str::to_icu(kb), e) == UCOL_LESS;
            }
            else
            {
                return ka < kb;
            }
        }
    );
}


class CrowdinLoginDialog : public wxDialog
{
public:
    CrowdinLoginDialog(wxWindow *parent) : wxDialog(parent, wxID_ANY, _("Sign in to Crowdin"))
    {
        auto topsizer = new wxBoxSizer(wxHORIZONTAL);
        auto panel = new Panel(this);
        panel->SetClientSize(panel->GetBestSize());
        topsizer->Add(panel, wxSizerFlags(1).Expand());
        SetSizerAndFit(topsizer);
        CenterOnParent();
    }

private:
    class Panel : public CrowdinLoginPanel
    {
    public:
        Panel(CrowdinLoginDialog *parent) : CrowdinLoginPanel(parent, DialogButtons), m_owner(parent)
        {
            EnsureInitialized();
        }

    protected:
        void OnUserSignedIn() override
        {
            m_owner->Raise();
            m_owner->EndModal(wxID_OK);
        }

        CrowdinLoginDialog *m_owner;
    };
};


class CloudFileList : public wxDataViewListCtrl
{
public:
    using FileInfo = CloudAccountClient::ProjectFile;

    CloudFileList(wxWindow *parent)
        : wxDataViewListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             wxDV_NO_HEADER | MSW_OR_OTHER(wxBORDER_SIMPLE, wxBORDER_SUNKEN))
    {
    #if wxCHECK_VERSION(3,1,1)
        SetRowHeight(PX(36));
    #endif
        SetMinSize(wxSize(PX(500), PX(200)));
    #ifdef __WXOSX__
        if (@available(macOS 11.0, *))
        {
            NSScrollView *scrollView = (NSScrollView*)GetHandle();
            NSTableView *tableView = (NSTableView*)[scrollView documentView];
            tableView.style = NSTableViewStyleFullWidth;
        }
    #endif

        auto renderer = new MultilineTextRenderer();
        auto column = new wxDataViewColumn(_("File"), renderer, 0, -1, wxALIGN_NOT, wxDATAVIEW_COL_RESIZABLE);
        AppendColumn(column, "string");

        ColorScheme::SetupWindowColors(this, [=]{ RefreshFileList(); });
    }

    void ClearFiles()
    {
        m_files.clear();
        DeleteAllItems();
    }

    void SetFiles(std::vector<FileInfo>& files)
    {
        m_files = files;
        RefreshFileList();
    }

private:
    void RefreshFileList()
    {
    #ifdef __WXGTK__
        auto secondaryFormatting = "alpha='50%'";
    #else
        auto secondaryFormatting = wxString::Format("foreground='%s'", ColorScheme::Get(Color::SecondaryLabel).GetAsString(wxC2S_HTML_SYNTAX));
    #endif

        DeleteAllItems();

        for (auto& f : m_files)
        {
        #if wxCHECK_VERSION(3,1,1)
            wxString text = wxString::Format
            (
                "%s\n<small><span %s>%s</span></small>",
                EscapeMarkup(f.title),
                secondaryFormatting,
                EscapeMarkup(f.description)
            );
        #else
            wxString text = str::to_wx(f.title);
        #endif

            wxVector<wxVariant> data;
            data.push_back({text});
            AppendItem(data);
        }
    }

    class MultilineTextRenderer : public wxDataViewTextRenderer
    {
    public:
        MultilineTextRenderer() : wxDataViewTextRenderer()
        {
    #if wxCHECK_VERSION(3,1,1)
            EnableMarkup();
    #endif
        }

    #ifdef __WXMSW__
        bool Render(wxRect rect, wxDC *dc, int state)
        {
            int flags = 0;
            if ( state & wxDATAVIEW_CELL_SELECTED )
                flags |= wxCONTROL_SELECTED;

            for (auto& line: wxSplit(m_text, '\n'))
            {
                wxItemMarkupText markup(line);
                markup.Render(GetView(), *dc, rect, flags, GetEllipsizeMode());
                rect.y += rect.height / 2;
            }

            return true;
        }

        wxSize GetSize() const
        {
            if (m_text.empty())
                return wxSize(wxDVC_DEFAULT_RENDERER_SIZE,wxDVC_DEFAULT_RENDERER_SIZE);

            auto size = wxDataViewTextRenderer::GetSize();
            size.y *= 2; // approximation enough for our needs
            return size;
        }
    #endif // __WXMSW__
    };

private:
    std::vector<FileInfo> m_files;
};


class CloudOpenDialog : public wxDialog
{
public:
    CloudOpenDialog(wxWindow *parent) : wxDialog(parent, wxID_ANY, _("Open online translation"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    {
        m_accounts = GetSignedInAccounts();

        auto topsizer = new wxBoxSizer(wxVERTICAL);
        topsizer->SetMinSize(PX(400), -1);

        auto loginSizer = new wxBoxSizer(wxHORIZONTAL);
        topsizer->AddSpacer(PX(6));
        topsizer->Add(loginSizer, wxSizerFlags().Right().PXDoubleBorder(wxLEFT|wxRIGHT));
        m_loginText = new SecondaryLabel(this, "");
        m_loginImage = new AvatarIcon(this, wxSize(PX(24), PX(24)));
        loginSizer->Add(m_loginText, wxSizerFlags().ReserveSpaceEvenIfHidden().Center().Border(wxRIGHT, PX(5)));
        loginSizer->Add(m_loginImage, wxSizerFlags().ReserveSpaceEvenIfHidden().Center());
        m_loginText->Hide();
        m_loginImage->Hide();

        auto pickers = new wxFlexGridSizer(2, wxSize(PX(5),PX(6)));
        pickers->AddGrowableCol(1);
        topsizer->Add(pickers, wxSizerFlags().Expand().PXDoubleBorderAll());

        pickers->Add(new wxStaticText(this, wxID_ANY, _("Project:")), wxSizerFlags().CenterVertical().Right());
        m_project = new wxChoice(this, wxID_ANY);
        pickers->Add(m_project, wxSizerFlags().Expand().CenterVertical());

        pickers->Add(new wxStaticText(this, wxID_ANY, _("Language:")), wxSizerFlags().CenterVertical().Right());
        m_language = new wxChoice(this, wxID_ANY);
        pickers->Add(m_language, wxSizerFlags().Expand().CenterVertical());

        m_files = new CloudFileList(this);
        topsizer->Add(m_files, wxSizerFlags(1).Expand().PXDoubleBorderAll());

        m_activity = new ActivityIndicator(this);
        topsizer->Add(m_activity, wxSizerFlags().Expand().PXDoubleBorder(wxLEFT|wxRIGHT));
        topsizer->AddSpacer(MSW_OR_OTHER(PX(4), PX(2)));

        auto buttons = CreateButtonSizer(wxOK | wxCANCEL);
        auto ok = static_cast<wxButton*>(FindWindow(wxID_OK));
        ok->SetDefault();
    #ifdef __WXOSX__
        topsizer->Add(buttons, wxSizerFlags().Expand());
    #else
        topsizer->Add(buttons, wxSizerFlags().Expand().PXBorderAll());
        topsizer->AddSpacer(PX(5));
    #endif

        SetSizerAndFit(topsizer);

        m_project->Bind(wxEVT_CHOICE, [=](wxCommandEvent&){ OnProjectSelected(); });
        ok->Bind(wxEVT_UPDATE_UI, &CloudOpenDialog::OnUpdateOK, this);
        ok->Bind(wxEVT_BUTTON, &CloudOpenDialog::OnOK, this);

        ok->Disable();
        EnableAllChoices(false);
    }

    void LoadFromCloud()
    {
        FetchProjects();
        if (m_accounts.size() == 1)
            FetchLoginInfo(m_accounts[0]);
    }

    wxString OutLocalFilename;

private:
    void FetchLoginInfo(CloudAccountClient *account)
    {
        if (account == m_loginAccountShown)
            return;
        m_loginAccountShown = account;

        account->GetUserInfo()
        .then_on_window(this, [=](CrowdinClient::UserInfo u)
        {
            if (account != m_loginAccountShown)
                return;  // user changed selection since invocation, there's another pending async call

            wxString text = _("Signed in as:") + " " + u.name;
            if (m_accounts.size() > 1)
                text += wxString::Format(" (%s)", account->GetServiceName());

            m_loginText->SetLabel(text);
            m_loginImage->SetUserName(u.name);
            if (u.avatarUrl.empty())
            {
                m_loginImage->Show();
            }
            else
            {
                http_client::download_from_anywhere(u.avatarUrl)
                .then_on_window(this, [=](downloaded_file f)
                {
                    m_loginImage->LoadIcon(f.filename());
                    m_loginImage->Show();
                });
            }
            Layout();
            m_loginText->Show();
        })
        .catch_all(m_activity->HandleError);
    }

    void EnableAllChoices(bool enable = true)
    {
        m_project->Enable(enable);
        m_language->Enable(enable);
        m_files->Enable(enable);
    }

    void FetchProjects()
    {
        m_activity->Start();

        m_projects.clear();
        m_projectsPendingLoad = m_accounts.size();
        for (auto acc : m_accounts)
        {
            acc->GetUserProjects()
                .then_on_window(this, &CloudOpenDialog::OnFetchedProjects)
                .catch_all(m_activity->HandleError);
        }
    }

    void OnFetchedProjects(std::vector<CloudAccountClient::ProjectInfo> prjs)
    {
        m_projects.insert(m_projects.end(), prjs.begin(), prjs.end());

        if (--m_projectsPendingLoad > 0)
            return; // wait for other loads to finish

        SortAlphabetically(m_projects, [](const auto& p){ return p.name; });

        m_project->Append("");
        for (auto& p: m_projects)
            m_project->Append(p.name);
        m_project->Enable(!m_projects.empty());

        if (m_projects.empty())
        {
            m_activity->StopWithError(_("No translation projects listed in your account."));
            return;
        }
        else
        {
            m_activity->Stop();
        }

        if (m_projects.size() == 1)
        {
            m_project->SetSelection(1);
            OnProjectSelected();
        }
        else
        {
            auto last = Config::CloudLastProject();
            auto lasti = last.empty()? m_projects.end() : std::find_if(m_projects.begin(), m_projects.end(), [=](const auto& p){ return p.slug == last; });
            if (lasti != m_projects.end())
            {
                m_project->SetSelection(1 + int(lasti - m_projects.begin()));
                OnProjectSelected();
            }
        }
    }

    void OnProjectSelected()
    {
        auto sel = m_project->GetSelection();
        if (sel > 0)
        {
            m_currentProject = m_projects[sel-1];
            auto account = AccountFor(m_currentProject);

            Config::CloudLastProject(m_currentProject.slug);
            m_activity->Start();
            EnableAllChoices(false);
            m_files->ClearFiles();
            account->GetProjectDetails(m_currentProject)
                .then_on_window(this, &CloudOpenDialog::OnFetchedProjectInfo)
                .catch_all([=](dispatch::exception_ptr e){
                    m_activity->HandleError(e);
                    EnableAllChoices(true);
                });
            FetchLoginInfo(account);
        }
    }

    void OnFetchedProjectInfo(CloudAccountClient::ProjectDetails prj)
    {
        auto previouslySelectedLanguage = m_language->GetStringSelection(); // may be empty

        m_info = prj;
        SortAlphabetically(m_info.languages, [](const auto& l){ return l.DisplayName(); });

        m_language->Clear();
        m_language->Append("");
        for (auto& i: m_info.languages)
            m_language->Append(i.DisplayName());

        m_files->SetFiles(m_info.files);

        EnableAllChoices();
        m_activity->Stop();

        if (m_info.languages.size() == 1)
        {
            m_language->SetSelection(1);
        }
        else
        {
            if (previouslySelectedLanguage.empty() || !m_language->SetStringSelection(previouslySelectedLanguage))
            {
                auto preferred = LanguageDialog::GetLastChosen();
                if (preferred.IsValid())
                {
                    for (size_t i = 0; i < m_info.languages.size(); i++)
                    {
                        if (m_info.languages[i] == preferred)
                        {
                            m_language->SetSelection(1 + int(i));
                            break;
                        }
                    }
                }
            }
        }

        if (m_info.files.size() == 1)
            m_files->SelectRow(0);

    }

    void OnUpdateOK(wxUpdateUIEvent& e)
    {
        e.Enable(!m_activity->IsRunning() &&
                 m_project->GetSelection() > 0 &&
                 m_language->GetSelection() > 0 &&
                 m_files->GetSelectedRow() != wxNOT_FOUND);
    }

    void OnOK(wxCommandEvent&)
    {
        auto cloudFile = m_info.files[m_files->GetSelectedRow()];
        auto cloudLang = m_info.languages[m_language->GetSelection() - 1];
        LanguageDialog::SetLastChosen(cloudLang);
        OutLocalFilename = CreateLocalFilename(m_currentProject, cloudFile, cloudLang);

        m_activity->Start(_(L"Downloading latest translations…"));

        auto outfile = std::make_shared<TempOutputFileFor>(OutLocalFilename);
        AccountFor(m_currentProject)->DownloadFile(str::to_wstring(outfile->FileName()), m_currentProject, cloudFile, cloudLang)
            .then_on_window(this, [=]{
                outfile->Commit();
                AcceptAndClose();
            })
            .catch_all(m_activity->HandleError);
    }

    wxString CreateLocalFilename(const CloudAccountClient::ProjectInfo& project, const CloudAccountClient::ProjectFile& file, const Language& lang)
    {
        auto account = AccountFor(project);
        auto filename = account->CreateLocalFilename(project, file, lang);

        wxFileName localFileName(wxString::Format("%s/%s/%s", CloudSyncDestination::GetCacheDir(), account->GetServiceName(), filename));

        if (!wxFileName::DirExists(localFileName.GetPath()))
            wxFileName::Mkdir(localFileName.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

        return localFileName.GetFullPath();
    }

    // Find account based on x.service tag:
    template<typename T>
    CloudAccountClient *AccountFor(const T& x) const
    {
        for (auto acc : m_accounts)
        {
            if (acc->GetServiceName() == x.service)
                return acc;
        }
        wxFAIL_MSG("logic error - no matching account");
        return nullptr;
    }

private:
    SecondaryLabel *m_loginText;
    AvatarIcon *m_loginImage;
    CloudAccountClient *m_loginAccountShown = nullptr;

    wxButton *m_ok;
    wxChoice *m_project, *m_language;
    CloudFileList *m_files;
    ActivityIndicator *m_activity;

    std::vector<CloudAccountClient*> m_accounts;
    std::vector<CloudAccountClient::ProjectInfo> m_projects;
    size_t m_projectsPendingLoad = 0;
    CloudAccountClient::ProjectDetails m_info;
    CloudAccountClient::ProjectInfo m_currentProject;
};

} // anonymous namespace


void CloudOpenFile(wxWindow *parent, std::function<void(int, wxString)> onDone)
{
    wxWindowPtr<CloudOpenDialog> dlg(new CloudOpenDialog(parent));

    if (GetSignedInAccounts().empty())
    {
        // FIXME: use some kind of wizard UI with going to next page instead?
        // We need to show this window-modal after the ShowModal() call below is
        // executed. Use CallAfter() to delay:
        dlg->CallAfter([=]
        {
            wxWindowPtr<CrowdinLoginDialog> login(new CrowdinLoginDialog(dlg.get()));
            login->ShowWindowModalThenDo([dlg,login](int retval)
            {
                if (retval == wxID_OK)
                    dlg->LoadFromCloud();
                else
                    dlg->EndModal(wxID_CANCEL);
            });
        });
    }
    else
    {
        dlg->LoadFromCloud();
    }

    auto retval = dlg->ShowModal(); // FIXME: Use global modal-less dialog
    onDone(retval, dlg->OutLocalFilename);
}


#endif // #ifdef HAVE_HTTP_CLIENT
