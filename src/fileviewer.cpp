/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2024 Vaclav Slavik
 *  Copyright (C) 2015 PrismJS (CSS parts)
 *  Copyright (c) 2013-2017 Cole Bemis (Feather Icons)
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

#include "fileviewer.h"

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/config.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/listctrl.h>
#include <wx/fontenum.h>
#include <wx/ffile.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <wx/webview.h>

#ifdef __WXMSW__
#include <wx/msw/webview_ie.h>
#endif

#include <unordered_map>

#include "customcontrols.h"
#include "hidpi.h"
#include "utility.h"
#include "unicode_helpers.h"

namespace
{

#ifdef __WXOSX__
const int FRAME_STYLE = wxDEFAULT_FRAME_STYLE | wxFRAME_TOOL_WINDOW;
#else
const int FRAME_STYLE = wxDEFAULT_FRAME_STYLE;
#endif

wxString FileToHTMLMarkup(const wxTextFile& file, const wxString& ext, size_t lineno);

extern const char *HTML_POEDIT_CSS;
extern const char *SVG_ICON;

} // anonymous namespace


#ifdef __WXMSW__
struct FileViewer::TempFile
{
    TempFile() { file.Assign(dir.CreateFileName(".src.html")); }
    wxFileName file;
    TempDirectory dir;
};
#endif


FileViewer *FileViewer::ms_instance = nullptr;

FileViewer *FileViewer::GetAndActivate()
{
    if (!ms_instance)
        ms_instance = new FileViewer(nullptr);
    ms_instance->Show();
    if (ms_instance->IsIconized())
        ms_instance->Iconize(false);
    ms_instance->Raise();
    return ms_instance;
}

FileViewer::FileViewer(wxWindow*)
        // TRANSLATORS: Meaning occurrences of the string in source code
        : wxFrame(nullptr, wxID_ANY, _("Code Occurrences"), wxDefaultPosition, wxDefaultSize, FRAME_STYLE)
{
    SetName("fileviewer");

    ColorScheme::SetupWindowColors(this, [=]
    {
        // match CSS background color:
        if (ColorScheme::GetWindowMode(this) == ColorScheme::Light)
            SetBackgroundColour(*wxWHITE);
        else
            SetBackgroundColour("#1d1f21");
    });

    wxPanel *panel = new wxPanel(this, -1);
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(sizer);
#ifdef __WXOSX__
    panel->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
#ifdef __WXMSW__
    SetIcons(wxIconBundle(wxStandardPaths::Get().GetResourcesDir() + "\\Resources\\Poedit.ico"));
#endif

    m_topBarSizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_topBarSizer, wxSizerFlags().ReserveSpaceEvenIfHidden().Expand().Border(wxALL, PX(10)));

    m_file = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(PX(300), -1));
    m_topBarSizer->Add(m_file, wxSizerFlags().Center().ReserveSpaceEvenIfHidden());

    m_description = new SecondaryLabel(panel, "");
    m_topBarSizer->Add(m_description, wxSizerFlags(1).Center().PXBorder(wxLEFT|wxRIGHT));

    m_openInEditor = new wxButton(panel, wxID_ANY, MSW_OR_OTHER(_("Open in editor"), _("Open in Editor")));
#ifdef __WXOSX__
    static_cast<NSButton*>(m_openInEditor->GetHandle()).bezelStyle = NSBezelStyleRoundRect;
#endif
    m_topBarSizer->Add(m_openInEditor, wxSizerFlags().Center().ReserveSpaceEvenIfHidden().Border(wxLEFT, PX(10)));

    sizer->Add(new wxStaticLine(panel, wxID_ANY), wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, PX(5)));

#ifdef __WXMSW__
    if (wxWebView::IsBackendAvailable(wxWebViewBackendEdge))
    {
        m_usesMSIE = false;
        m_content = wxWebView::New(panel, wxID_ANY);
    }
    else
    {
        m_usesMSIE = true;
        wxWebViewIE::MSWSetEmulationLevel(wxWEBVIEWIE_EMU_IE11);
        m_content = wxWebView::New(panel, wxID_ANY);
    }
#else
    m_content = wxWebView::New(panel, wxID_ANY);
#endif
    sizer->Add(m_content, 1, wxEXPAND);

    RestoreWindowState(this, wxSize(PX(600), PX(400)));

    wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(panel, wxSizerFlags(1).Expand());
    SetSizer(topsizer);

    // avoid flicker with these initial settings:
    sizer->Hide(m_content);
    Layout();

    m_file->Bind(wxEVT_CHOICE, &FileViewer::OnChoice, this);
    m_openInEditor->Bind(wxEVT_BUTTON, &FileViewer::OnEditFile, this);

#ifdef __WXOSX__
    wxAcceleratorEntry entries[] = {
        { wxACCEL_CMD, 'W', wxID_CLOSE }
    };
    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);

    Bind(wxEVT_MENU, [=](wxCommandEvent&){ Destroy(); }, wxID_CLOSE);
#endif
}


FileViewer::~FileViewer()
{
    ms_instance = nullptr;
    SaveWindowState(this);
}


wxFileName FileViewer::GetFilename(wxString ref) const
{
    if ( ref.length() >= 3 &&
         ref[1] == _T(':') &&
         (ref[2] == _T('\\') || ref[2] == _T('/')) )
    {
        // This is an absolute Windows path (c:\foo... or c:/foo...); fix
        // the latter case.
        ref.Replace("/", "\\");
    }

    wxPathFormat pathfmt = ref.Contains(_T('\\')) ? wxPATH_WIN : wxPATH_UNIX;
    wxFileName filename(ref.BeforeLast(_T(':')), pathfmt);

    if ( filename.IsRelative() )
    {
        wxFileName relative(filename);
        wxString basePath(m_basePath);

        // Sometimes, the path in source reference is not relative to the PO
        // file's location, but is relative to e.g. the root directory. See
        // https://code.djangoproject.com/ticket/13936 for exhaustive
        // discussion with plenty of examples.
        //
        // Deal with this by trying parent directories of m_basePath too. So if
        // a file named project/locales/cs/foo.po has a reference to src/main.c,
        // try not only project/locales/cs/src/main.c, but also
        // project/locales/src/main.c and project/src/main.c etc.
        while ( !basePath.empty() )
        {
            filename = relative;
            filename.MakeAbsolute(basePath);
            if ( filename.FileExists() )
            {
                return filename; // good, found the file
            }
            else
            {
                // remove the last path component
                size_t last = basePath.find_last_of("\\/");
                if ( last == wxString::npos )
                    break;
                else
                    basePath.erase(last);
            }
        }
    }

    return wxFileName(); // invalid
}


void FileViewer::ShowReferences(CatalogPtr catalog, CatalogItemPtr item, int defaultReference)
{
    m_basePath = catalog->GetSourcesBasePath();
    if (m_basePath.empty())
        m_basePath = wxPathOnly(catalog->GetFileName());

    if (item)
        m_references = item->GetReferences();

    m_file->Clear();
    m_topBarSizer->Show(!m_references.empty());

    if (m_references.empty())
    {
        m_description->SetLabel("");
        ShowError(SVG_ICON, _("No usage information"), _(L"No information about this string’s occurrences in the source code is provided in the file."));
    }
    else
    {
        m_description->SetLabel
        (
            wxString::Format(wxPLURAL("%d code occurrence", "%d code occurrences", (int)m_references.size()),
                             (int)m_references.size())
        );
        for (auto& r: m_references)
            m_file->Append(bidi::platform_mark_direction(r));
        m_file->SetSelection(defaultReference);
        SelectReference(m_references[defaultReference]);
    }
}

void FileViewer::SelectReference(const wxString& ref)
{
    const wxFileName filename = GetFilename(ref);
    if (!filename.IsOk())
    {
        ShowError(SVG_ICON, _("Source code not found"),
                  _(L"Poedit cannot show source code where the string is used, because the file is either not available in the referenced location or it is a symbolic reference that doesn’t point to a real file."),
                  wxJoin(m_references, '\n')
                  );
        m_openInEditor->Disable();
        return;
    }

    const wxString fullpath = filename.GetFullPath();

    wxTextFile file;
    wxString data;

    if ( !filename.IsFileReadable() || !file.Open(fullpath) )
    {
        ShowError(SVG_ICON, _("File cannot be opened"),
                  wxString::Format(_(L"Poedit was unable to open the “%s” file."), fullpath));
        m_openInEditor->Disable();
        return;
    }

    m_openInEditor->Enable();

    // support GNOME's xml2po's extension to references in the form of
    // filename:line(xml_node):
    wxString linenumStr = ref.AfterLast(_T(':')).BeforeFirst(_T('('));

    long linenum;
    if (!linenumStr.ToLong(&linenum))
        linenum = 0;

    auto markup = FileToHTMLMarkup(file, filename.GetExt(), (size_t)linenum);
    ShowHTMLContent(markup);
}


void FileViewer::ShowHTMLContent(const wxString& markup)
{
    m_content->GetContainingSizer()->Show(m_content);
    Layout();

#ifdef __WXMSW__
    // Streaming document content to MSIE it via SetPage() behaves
    // a bit differently from loading a file or HTTP document and in particular,
    // we're hit by two issues:
    //
    // 1. X-UA-Compatible is ignored; this could be fixed by an explicit call
    //    to wxWebViewIE::MSWSetEmulationLevel(wxWEBVIEWIE_EMU_IE11) somewhere
    // 2. It then reports "unknown script code" instead of file URIs for
    //    externally loaded JS files, which breaks PrismJS's autoloader
    //
    // So we instead put the content into a temporary file and load that. This
    // sidesteps both issues at a negligible performance cost.
    if (m_usesMSIE)
    {
        if (!m_tmpFile)
            m_tmpFile = std::make_shared<TempFile>();

        wxFFile f_html(m_tmpFile->file.GetFullPath(), "wb");
        f_html.Write(markup, wxConvUTF8);
        f_html.Close();

        m_content->LoadURL(wxFileName::FileNameToURL(m_tmpFile->file));
    }
    else
#endif
    {
        m_content->SetPage(markup, "file:///");
    }
}


void FileViewer::ShowError(const char *icon, const wxString& msg, const wxString& description, const wxString& references)
{
    wxString html = wxString::Format
    (
        R"(<!DOCTYPE html>
        <html>
            <head>
                <meta http-equiv="X-UA-Compatible" content="IE=edge">
                <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
                <style>%s</style>
            </head>
            <body>
                <div class="message-wrapper">
                    <div class="message">
                        %s
                        <h2>%s</h2>
        )",
        HTML_POEDIT_CSS,
        icon,
        EscapeMarkup(msg)
    );

    if (!references.empty())
        html += "<pre>" + EscapeMarkup(references) + "</pre>";

    if (!description.empty())
        html += wxString::Format("<div class=\"explanation\">%s</div>", EscapeMarkup(description));

    html += R"(
                    </div>
                </div>
            </body>
        </html>
        )";

    ShowHTMLContent(html);
}


void FileViewer::OnChoice(wxCommandEvent &event)
{
    SelectReference(m_references[event.GetSelection()]);
}

void FileViewer::OnEditFile(wxCommandEvent&)
{
    wxFileName filename = GetFilename(bidi::strip_control_chars(m_file->GetStringSelection()));
    if (filename.IsOk())
        wxLaunchDefaultApplication(filename.GetFullPath());
}


namespace
{

inline std::string FilenameToLanguage(const std::string& ext)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        #include "fileviewer.extensions.h"
    };

    auto i = mapping.find(ext);
    if (i != mapping.end())
        return i->second;

    return ext;
}

inline void OutputBlock(wxString& html, const wxTextFile& file, size_t lfrom, size_t lto)
{
    for (size_t i = lfrom; i < lto; i++)
    {
        html += EscapeMarkup(file[i]);
        html += '\n';
    }
}

wxString FileToHTMLMarkup(const wxTextFile& file, const wxString& ext, size_t lineno)
{
    wxString html = wxString::Format(
        R"(<!DOCTYPE html>
        <html>
            <head>
                <meta http-equiv="X-UA-Compatible" content="IE=edge">
                <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
                <style>%s</style>
            </head>
            <body>
        )",
        HTML_POEDIT_CSS);

    html += wxString::Format("<pre class=\"line-numbers\">"
                                 "<code>"
                                     "<code class=\"language-%s\">",
                             FilenameToLanguage(ext.Lower().utf8_string()));

    const size_t count = file.GetLineCount();
    if (lineno && lineno <= count)
    {
        OutputBlock(html, file, 0, lineno-1);
        html += "<mark>";
        OutputBlock(html, file, lineno-1, lineno);
        html += "</mark>";
        OutputBlock(html, file, lineno, count);
    }
    else
    {
        OutputBlock(html, file, 0, count);
    }

    // add line numbers:
    html += "</code>"
            "<span aria-hidden=\"true\" class=\"line-numbers-rows\">";
    for (size_t i = 0; i < count; i++)
    {
        if (i == lineno-1)
            html += "<span id=\"mark\"><span id=\"msie_anchor\"></span></span>";
        else
            html += "<span></span>";
    }

    html += "</span></code></pre>";

    if (lineno)
    {
        // Alternative implementation that doesn't need msie_anchor, but doesn't work on MSIE, is to do:
        //     document.getElementById('mark').scrollIntoView({behavior: 'instant', block: 'center'});
        // instead of
        //     window.location.hash = "#msie_anchor";
        html += R"(<script>
                   window.location.hash = "#msie_anchor";
                   </script>)";
    }

    // PrismJS is added after everything else so that basic rendering works even
    // when offline.
    html += R"(
            <script crossorigin src="https://cdnjs.cloudflare.com/ajax/libs/prism/1.21.0/components/prism-core.min.js"></script>
            <script crossorigin src="https://cdnjs.cloudflare.com/ajax/libs/prism/1.21.0/plugins/keep-markup/prism-keep-markup.min.js"></script>
            <script crossorigin src="https://cdnjs.cloudflare.com/ajax/libs/prism/1.21.0/plugins/autoloader/prism-autoloader.min.js"></script>
            </body>
        </html>
        )";

    return html;
}



const char *SVG_ICON = R"(
<svg width="48" height="48" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
<path fill-rule="evenodd" clip-rule="evenodd" d="M12.5 9.75C12.5 9.05964 13.0596 8.5 13.75 8.5H26.5931C27.4551 8.5 28.2817 8.84241 28.8912 9.4519L34.5481 15.1088C35.1576 15.7183 35.5 16.5449 35.5 17.4069V25.8507C36.0797 26.3696 36.5856 26.9692 37 27.6318V17.4069C37 16.1471 36.4996 14.9389 35.6088 14.0481L29.9519 8.39124C29.0611 7.50045 27.8529 7 26.5931 7H13.75C12.2312 7 11 8.23122 11 9.75V36.25C11 37.7688 12.2312 39 13.75 39H25.6318C24.9692 38.5856 24.3696 38.0797 23.8507 37.5H13.75C13.0596 37.5 12.5 36.9404 12.5 36.25V9.75ZM36.9973 36.3724C36.9351 37.7938 35.7938 38.9351 34.3724 38.9973C35.4336 38.3328 36.3328 37.4336 36.9973 36.3724Z" fill="currentColor"/>
<path fill-rule="evenodd" clip-rule="evenodd" d="M27.25 9.75C27.25 8.64543 26.3546 7.75 25.25 7.75H23.25H13.75C12.6454 7.75 11.75 8.64543 11.75 9.75V36.25C11.75 37.3546 12.6454 38.25 13.75 38.25H24.6146C22.8604 36.7371 21.75 34.4982 21.75 32C21.75 27.4437 25.4437 23.75 30 23.75C32.4982 23.75 34.7371 24.8604 36.25 26.6146V20.75V18.75C36.25 17.6454 35.3546 16.75 34.25 16.75H29.25C28.1454 16.75 27.25 15.8546 27.25 14.75V9.75Z" fill="currentColor" fill-opacity="0.15"/>
<path d="M24.6146 38.25V39H26.6326L25.1044 37.682L24.6146 38.25ZM36.25 26.6146L35.682 27.1044L37 28.6326V26.6146H36.25ZM25.25 8.5C25.9404 8.5 26.5 9.05964 26.5 9.75H28C28 8.23122 26.7688 7 25.25 7V8.5ZM23.25 8.5H25.25V7H23.25V8.5ZM13.75 8.5H23.25V7H13.75V8.5ZM12.5 9.75C12.5 9.05964 13.0596 8.5 13.75 8.5V7C12.2312 7 11 8.23122 11 9.75H12.5ZM12.5 36.25V9.75H11V36.25H12.5ZM13.75 37.5C13.0596 37.5 12.5 36.9404 12.5 36.25H11C11 37.7688 12.2312 39 13.75 39V37.5ZM24.6146 37.5H13.75V39H24.6146V37.5ZM25.1044 37.682C23.5084 36.3056 22.5 34.271 22.5 32H21C21 34.7255 22.2124 37.1687 24.1248 38.818L25.1044 37.682ZM22.5 32C22.5 27.8579 25.8579 24.5 30 24.5V23C25.0294 23 21 27.0294 21 32H22.5ZM30 24.5C32.271 24.5 34.3056 25.5084 35.682 27.1044L36.818 26.1248C35.1687 24.2124 32.7255 23 30 23V24.5ZM35.5 20.75V26.6146H37V20.75H35.5ZM35.5 18.75V20.75H37V18.75H35.5ZM34.25 17.5C34.9404 17.5 35.5 18.0596 35.5 18.75H37C37 17.2312 35.7688 16 34.25 16V17.5ZM29.25 17.5H34.25V16H29.25V17.5ZM26.5 14.75C26.5 16.2688 27.7312 17.5 29.25 17.5V16C28.5596 16 28 15.4404 28 14.75H26.5ZM26.5 9.75V14.75H28V9.75H26.5Z" fill="currentColor"/>
<circle cx="30" cy="32" r="8.25" stroke="currentColor" stroke-width="1.5"/>
<path d="M40.4697 43.5303C40.7626 43.8232 41.2374 43.8232 41.5303 43.5303C41.8232 43.2374 41.8232 42.7626 41.5303 42.4697L40.4697 43.5303ZM35.4697 38.5303L40.4697 43.5303L41.5303 42.4697L36.5303 37.4697L35.4697 38.5303Z" fill="currentColor"/>
</svg>
)";


/*

The MIT License (MIT)

Copyright (c) 2015 PrismJS
Copyright (c) 2020 Vaclav Slavik <vaclav@slavik.io>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


Based on the following PrismJS themes:
  https://github.com/PrismJS/prism-themes/blob/master/themes/prism-ghcolors.css
  * GHColors theme by Avi Aryan (http://aviaryan.in)
  * Inspired by Github syntax coloring
  https://github.com/PrismJS/prism-themes/blob/master/themes/prism-atom-dark.css
  * atom-dark theme for `prism.js`
  * Based on Atom's `atom-dark` theme: https://github.com/atom/atom-dark-syntax
  * @author Joe Gibson (@gibsjose)

 */

const char *HTML_POEDIT_CSS = R"(

:root {
    color-scheme: light dark;
}

body {
    font-family: system-ui, -apple-system, BlinkMacSystemFont, Segoe UI, Roboto, Helvetica Neue, Arial, Noto Sans, sans-serif;
    font-size: 10pt;
    color: #393A34;
    background-color: white;
}

code, pre {
	font-family: "ui-monospace", "SFMono-Regular", Consolas, "Liberation Mono", Menlo, monospace;
	font-size: 9pt;
    line-height: 1.2em;
    tab-size: 4;
    padding: 0;
    margin: 0;
}

.token.comment,
.token.prolog,
.token.doctype,
.token.cdata {
    color: #999988;
    font-style: italic;
}

.token.namespace {
    opacity: .7;
}

.token.string,
.token.attr-value {
    color: #e3116c;
}

.token.punctuation,
.token.operator {
    color: #393A34; /* no highlight */
}

.token.entity,
.token.url,
.token.symbol,
.token.number,
.token.boolean,
.token.variable,
.token.constant,
.token.property,
.token.regex,
.token.inserted {
    color: #36acaa;
}

.token.atrule,
.token.keyword,
.token.attr-name,
.language-autohotkey .token.selector {
    color: #00a4db;
}

.token.function,
.token.deleted,
.language-autohotkey .token.tag {
    color: #9a050f;
}

.token.tag,
.token.selector,
.language-autohotkey .token.keyword {
    color: #00009f;
}

.token.important,
.token.bold {
    font-weight: bold;
}

.token.italic {
    font-style: italic;
}


@media (prefers-color-scheme: dark) {
    body {
        color: #c5c8c6;
        background-color: #1d1f21;
    }

    .token.comment,
    .token.prolog,
    .token.doctype,
    .token.cdata {
        color: #7C7C7C;
    }

    .token.punctuation {
        color: #c5c8c6;
    }

    .token.property,
    .token.keyword,
    .token.tag {
        color: #96CBFE;
    }

    .token.boolean,
    .token.constant {
        color: #99CC99;
    }

    .token.symbol,
    .token.deleted {
        color: #f92672;
    }

    .token.number {
        color: #FF73FD;
    }

    .token.selector,
    .token.attr-name,
    .token.string,
    .token.char,
    .token.builtin,
    .token.inserted {
        color: #A8FF60;
    }

    .token.variable {
        color: #C6C5FE;
    }

    .token.operator {
        color: #EDEDED;
    }

    .token.entity {
        color: #FFFFB6;
    }

    .token.url {
        color: #96CBFE;
    }

    .language-css .token.string,
    .style .token.string {
        color: #87C38A;
    }

    .token.atrule,
    .token.attr-value {
        color: #F9EE98;
    }

    .token.function {
        color: #DAD085;
    }

    .token.regex {
        color: #E9C062;
    }

    .token.important {
        color: #fd971f;
    }
}


/* Line numbers: */

pre.line-numbers {
    position: relative;
    padding-left: 3.8em;
    counter-reset: linenumber;
}

pre.line-numbers > code {
    position: relative;
    white-space: inherit;
}

.line-numbers-rows {
    position: absolute;
    pointer-events: none;
    top: 0;
    font-size: 100%;
    left: -3.8em;
    width: 3em;
    letter-spacing: -1px;
    border-right: 1px solid #999;

    -webkit-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;

}

.line-numbers-rows > span {
    display: block;
    counter-increment: linenumber;
}

.line-numbers-rows > span:before {
    content: counter(linenumber);
    color: #999;
    display: block;
    padding-right: 0.8em;
    text-align: right;
}

/* Highlighting of selected line: */

.line-numbers-rows > #mark:before {
    background-color: rgb(253, 235, 176);
}
mark {
    background-color: rgb(253, 235, 176);
}

@media (prefers-color-scheme: dark) {
    .line-numbers-rows > #mark:before {
        background-color: rgb(198, 171, 113);
        color: #393A34;
    }
    mark {
        background-color: rgb(198, 171, 113);
        color: #393A34;
    }
}

#msie_anchor {
    display: block;
    visibility: hidden;
    height: 50vh; /* 50% viewport height */
    margin-top: -50vh;
}

/* Error messages: */

.message-wrapper {
    height: 90vh;
    display: flex;
    align-items: center;
    justify-content: center;
}

.message {
    text-align: center;
    opacity: 0.8;
    padding-top: 1em;
}

.message h2 {
    margin-top: 0.2em;
    font-size: 125%;
    font-weight: 600;
}

.explanation {
    width: 80%;
    font-size: 90%;
    margin: 1em auto;
    opacity: 0.6;
}

.message pre {
    text-align: left;
    display: table;
    margin-left: auto;
    margin-right: auto;
}

)";

} // anonymous namespace
