
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      catalog.cpp
    
      Translations catalog
    
      (c) Vaclav Slavik, 1999

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/wxprec.h>

#include <stdio.h>
#include <wx/utils.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <wx/intl.h>
#include <wx/datetime.h>
#include <wx/config.h>
#include <wx/textfile.h>
#include <wx/strconv.h>

#include "catalog.h"
#include "digger.h"
#include "gexecute.h"
#include "progressinfo.h"
#include "summarydlg.h"



#include "wx/arrimpl.cpp"
WX_DEFINE_OBJARRAY(CatalogDataArray)


// textfile processing utilities:

// Read one line from file, remove all \r and \n characters, ignore empty lines
static wxString ReadTextLine(wxTextFile* f)
{
    wxString s;
    
    while (s.IsEmpty())
    {
        if (f->Eof()) return wxEmptyString;
        s = f->GetNextLine();
    }
    return s;
}


// If input begins with pattern, fill output with end of input (without pattern; strips trailing spaces) 
// and return true.
// Return false otherwise and don't touch output
static bool ReadParam(const wxString& input, const wxString& pattern, wxString& output)
{
    if (input.Length() < pattern.Length()) return false;
    
    for (unsigned i = 0; i < pattern.Length(); i++)
        if (input[i] != pattern[i]) return false;
    output = input.Mid(pattern.Length()).Strip(wxString::trailing);
    return true;
}
        
        
// parsers:

void CatalogParser::Parse()
{
    if (m_textFile->GetLineCount() == 0) return;

    wxString line, dummy;
    wxString mflags, mstr, mtrans, mcomment;
    wxArrayString mrefs;
    
    line = m_textFile->GetFirstLine();
    if (line.IsEmpty()) line = ReadTextLine(m_textFile);

    while (!line.IsEmpty())
    {
        // flags:
        if (ReadParam(line, "#, ", dummy))
        {
            mflags = "#, " + dummy;
            line = ReadTextLine(m_textFile);
        }
        
        // references:
        if (ReadParam(line, "#: ", dummy))
        {
            wxStringTokenizer tkn(dummy, "\t\r\n ");
            while (tkn.HasMoreTokens())
                mrefs.Add(tkn.GetNextToken());
            line = ReadTextLine(m_textFile);
        }
        
        // msgid:
        else if (ReadParam(line, "msgid \"", dummy))
        {
            mstr = dummy.RemoveLast();
            while (!(line = ReadTextLine(m_textFile)).IsEmpty())
            {
                if (line[0] == '"' && line.Last() == '"')
                    mstr += line.Mid(1, line.Length() - 2);
                else break;
            }
        }
        
        // msgstr:
        else if (ReadParam(line, "msgstr \"", dummy))
        {
            mtrans = dummy.RemoveLast();
            while (!(line = ReadTextLine(m_textFile)).IsEmpty())
            {
                if (line[0] == '"' && line.Last() == '"')
                    mtrans += line.Mid(1, line.Length() - 2);
                else break;
            }

            if (!OnEntry(mstr, mtrans, mflags, mrefs, mcomment)) return;

            mcomment = mstr = mtrans = mflags = wxEmptyString;
            mrefs.Clear();
        }
        
        // comment:
        else if (line[0] == '#')
        {
            while (!line.IsEmpty() && line[0] == '#' &&
                   (line.Length() < 2 || (line[1] != ',' && line[1] != ':')))
            {
                mcomment << line << '\n';
                line = ReadTextLine(m_textFile);
            }
        }
        
        else
            line = ReadTextLine(m_textFile);
    }
}


class LoadParser : public CatalogParser
{
    public:
        LoadParser(Catalog *c, wxTextFile *f) : CatalogParser(f), m_catalog(c) {}

    protected:
        Catalog *m_catalog;
    
        virtual bool OnEntry(const wxString& msgid,
                             const wxString& msgstr,
                             const wxString& flags,
                             const wxArrayString& references,
                             const wxString& comment);
                             
};


bool LoadParser::OnEntry(const wxString& msgid,
                         const wxString& msgstr,
                         const wxString& flags,
                         const wxArrayString& references,
                         const wxString& comment)
{
    if (msgid.IsEmpty())
    {
        // gettext header:
        wxString mtrans = msgstr;
        mtrans.Replace("\\n", "\n");
        wxStringTokenizer tkn(mtrans, "\n");
        wxString dummy2, ln2;

        while (tkn.HasMoreTokens())
        {
            ln2 = tkn.GetNextToken();
            ReadParam(ln2, "Project-Id-Version: ", m_catalog->m_header.Project);
            ReadParam(ln2, "POT-Creation-Date: ", m_catalog->m_header.CreationDate);
            ReadParam(ln2, "PO-Revision-Date: ", m_catalog->m_header.RevisionDate);
            if (ReadParam(ln2, "Last-Translator: ", dummy2))
            {
                wxStringTokenizer tkn2(dummy2, "<>");
                if (tkn2.CountTokens() != 2) wxLogWarning(_("Corrupted translator record, please correct in Catalog/Settings"));
                m_catalog->m_header.Translator = tkn2.GetNextToken().Strip(wxString::trailing);
                m_catalog->m_header.TranslatorEmail = tkn2.GetNextToken();
            }
            if (ReadParam(ln2, "Language-Team: ", dummy2))
            {
                wxStringTokenizer tkn2(dummy2, "<>");
                if (tkn2.CountTokens() != 2) wxLogWarning(_("Corrupted team record, please correct in Catalog/Settings"));
                m_catalog->m_header.Team = tkn2.GetNextToken().Strip(wxString::trailing);
                m_catalog->m_header.TeamEmail = tkn2.GetNextToken();
            }
            ReadParam(ln2, "Content-Type: text/plain; charset=", 
                      m_catalog->m_header.Charset);
        }
        m_catalog->m_header.Comment = comment;
    }
    else
    {
        CatalogData *d = new CatalogData("", "");
        if (!flags.IsEmpty()) d->SetFlags(flags);
        d->SetString(msgid);
        d->SetTranslation(msgstr);
        d->SetComment(comment);
        for (size_t i = 0; i < references.GetCount(); i++)
            d->AddReference(references[i]);

        m_catalog->m_data->Put(msgid, d);
        m_catalog->m_dataArray.Add(d);
        m_catalog->m_count++;
    }
    return true;
}





// catalog class:

Catalog::Catalog()
{
    m_data = new wxHashTable(wxKEY_STRING);
    m_count = 0;
    m_isOk = true;
    m_header.BasePath = "";
}


Catalog::~Catalog() 
{
    Clear();
    delete m_data;
}


Catalog::Catalog(const wxString& po_file)
{
    m_data = NULL;
    m_count = 0;
    m_isOk = false;
    Load(po_file);
}


void Catalog::CreateNewHeader()
{
    HeaderData &dt = Header();
    
    wxDateTime timenow = wxDateTime::Now();
    int offs = wxDateTime::TimeZone(wxDateTime::Local).GetOffset();
    dt.CreationDate.Printf("%s%s%02i%02i",
                                 timenow.Format("%Y-%m-%d %H:%M").c_str(),
                                 (offs > 0) ? "+" : "-",
                                 offs / 3600, (abs(offs) / 60) % 60);
    dt.RevisionDate = dt.CreationDate;

    dt.Language = "";
    dt.Project = "";
    dt.Team = "";
    dt.TeamEmail = "";
    dt.Charset = "utf-8";
    dt.Translator = wxConfig::Get()->Read("translator_name", "");
    dt.TranslatorEmail = wxConfig::Get()->Read("translator_email", "");
    dt.Keywords.Add("_");
    dt.Keywords.Add("gettext");
    dt.Keywords.Add("gettext_noop");
    dt.BasePath = ".";
}


bool Catalog::Load(const wxString& po_file)
{
    wxTextFile f;

    Clear();
    delete m_data; m_data = NULL;    
    m_count = 0;
    m_isOk = false;
    m_fileName = po_file;
    m_header.BasePath = "";

    /* Load extended information from .po.poedit file, if present: */
    
    if (wxFileExists(po_file + ".poedit") && f.Open(po_file + ".poedit"))
    {
        wxString dummy;
        // poedit header (optional, we should be able to read any catalog):
        f.GetFirstLine();
        if (ReadParam(ReadTextLine(&f), "#. Number of items: ", dummy))
        {
            long sz;
            dummy.ToLong(&sz);
            if (sz == 0) sz = 500;
            m_data = new wxHashTable(wxKEY_STRING, 2 * sz /*optimal hash table size*/);
        }
        else
            m_data = new wxHashTable(wxKEY_STRING);
        ReadParam(ReadTextLine(&f), "#. Language: ", m_header.Language);
        ReadParam(ReadTextLine(&f), "#. Basepath: ", m_header.BasePath);

        if (ReadParam(ReadTextLine(&f), "#. Paths: ", dummy))
        {
            long sz;
            dummy.ToLong(&sz);
            for (; sz > 0; sz--)
            if (ReadParam(ReadTextLine(&f), "#.     ", dummy))
                m_header.SearchPaths.Add(dummy);
        }

        if (ReadParam(ReadTextLine(&f), "#. Keywords: ", dummy))
        {
            long sz;
            dummy.ToLong(&sz);
            for (; sz > 0; sz--)
            if (ReadParam(ReadTextLine(&f), "#.     ", dummy))
                m_header.Keywords.Add(dummy);
        }

        f.Close();
    }
    else
        m_data = new wxHashTable(wxKEY_STRING);


    /* Load the .po file: */

    if (!f.Open(po_file)) return false;

    LoadParser parser(this, &f);
    parser.Parse();

    m_isOk = true;
    
    f.Close();
    
    /* Convert loaded data from file's encoding to UTF-8 which is our
       internal representation : */
    if (wxStricmp(Header().Charset, "utf-8") != 0)
    {
        wxString charset = Header().Charset;
        if (!charset || charset == "CHARSET")
            charset = "iso-8859-1"; // fallback
        wxCSConv encConv(charset);
        for (size_t i = 0; i < m_dataArray.GetCount(); i++)
            m_dataArray[i].SetTranslation(wxString(
                m_dataArray[i].GetTranslation().wc_str(encConv),
                wxConvUTF8));
    }
    
    return true;
}


void Catalog::Clear()
{
    delete m_data; 
    m_dataArray.Empty();
    m_isOk = true;
    m_count = 0;
    m_data = new wxHashTable(wxKEY_STRING);
}


static void GetCRLFBehaviour(wxTextFileType& type, bool& preserve)
{
    wxString format = wxConfigBase::Get()->Read("crlf_format", "unix");

    if (format == "win") type = wxTextFileType_Dos;
    else if (format == "mac") type = wxTextFileType_Mac;
    else if (format == "native") type = wxTextFile::typeDefault;
    else /* "unix" */ type = wxTextFileType_Unix;

    preserve = (bool)(wxConfigBase::Get()->Read("keep_crlf", true));
}

static void SaveMultiLines(wxTextFile &f, const wxString& text)
{
    wxStringTokenizer tkn(text, '\n');
    while (tkn.HasMoreTokens())
        f.AddLine(tkn.GetNextToken());
}


bool Catalog::Save(const wxString& po_file, bool save_mo)
{
    CatalogData *data;
    wxString dummy;
    size_t i;
    int j;
    wxTextFileType crlfOrig, crlf;
    bool crlfPreserve;
    wxTextFile f;
    
    GetCRLFBehaviour(crlfOrig, crlfPreserve);

    /* Update information about last modification time: */
  
    wxDateTime timenow = wxDateTime::Now();
    int offs = wxDateTime::TimeZone(wxDateTime::Local).GetOffset();
    m_header.RevisionDate.Printf("%s%s%02i%02i",
                                 timenow.Format("%Y-%m-%d %H:%M").c_str(),
                                 (offs > 0) ? "+" : "-",
                                 offs / 3600, (abs(offs) / 60) % 60);

    /* Detect CRLF format: */

    if (crlfPreserve && wxFileExists(po_file) && f.Open(po_file))
    {
        {
        wxLogNull null;
        crlf = f.GuessType();
        }
        if (crlf == wxTextFileType_None || crlf == wxTextFile::typeDefault)
            crlf = crlfOrig;
        f.Close();
    }
    else
        crlf = crlfOrig;
        

    /* If neccessary, save extended info into .po.poedit file: */

    if (!m_header.Language.IsEmpty() || 
        !m_header.BasePath.IsEmpty() || 
        m_header.SearchPaths.GetCount() > 0 ||
        m_header.Keywords.GetCount() > 0)
    {
        if (!wxFileExists(po_file + ".poedit") || !f.Open(po_file + ".poedit"))
            if (!f.Create(po_file + ".poedit"))
                return false;
        for (j = f.GetLineCount() - 1; j >= 0; j--)
            f.RemoveLine(j);

        f.AddLine("#. This catalog was generated by poedit");
        dummy.Printf("#. Number of items: %i", GetCount());
        f.AddLine(dummy);
        f.AddLine("#. Language: " + m_header.Language);
        f.AddLine("#. Basepath: " + m_header.BasePath);

        dummy.Printf("#. Paths: %i", m_header.SearchPaths.GetCount());
        f.AddLine(dummy);
        for (i = 0; i < m_header.SearchPaths.GetCount(); i++)
            f.AddLine("#.     " + m_header.SearchPaths[i]);

        dummy.Printf("#. Keywords: %i", m_header.Keywords.GetCount());
        f.AddLine(dummy);
        for (i = 0; i < m_header.Keywords.GetCount(); i++)
            f.AddLine("#.     " + m_header.Keywords[i]);
        f.Write(crlf);
        f.Close();
    }


    /* Save .po file: */

    wxString charset(m_header.Charset);
    if (!charset)
        charset = "utf-8";

    if (!wxFileExists(po_file) || !f.Open(po_file))
        if (!f.Create(po_file))
            return false;
    for (j = f.GetLineCount() - 1; j >= 0; j--)
        f.RemoveLine(j);

    SaveMultiLines(f, m_header.Comment);
    f.AddLine("msgid \"\"");
    f.AddLine("msgstr \"\"");
    f.AddLine("\"Project-Id-Version: " + m_header.Project + "\\n\"");
    f.AddLine("\"POT-Creation-Date: " + m_header.CreationDate + "\\n\"");
    f.AddLine("\"PO-Revision-Date: " + m_header.RevisionDate + "\\n\"");
    f.AddLine("\"Last-Translator: " + m_header.Translator + 
              " <" + m_header.TranslatorEmail + ">\\n\"");
    f.AddLine("\"Language-Team: " + m_header.Team +
              " <" + m_header.TeamEmail + ">\\n\"");
    f.AddLine("\"MIME-Version: 1.0\\n\"");
    f.AddLine("\"Content-Type: text/plain; charset=" + charset + "\\n\"");
    f.AddLine("\"Content-Transfer-Encoding: 8bit\\n\"");
    f.AddLine("");
    
    wxCSConv *encConv;
    if (wxStricmp(Header().Charset, "utf-8") != 0)
        encConv = new wxCSConv(Header().Charset);
    else
        encConv = NULL;

    for (i = 0; i < m_dataArray.GetCount(); i++)
    {
        data = &(m_dataArray[i]);
        SaveMultiLines(f, data->GetComment());
        for (unsigned i = 0; i < data->GetReferences().GetCount(); i++)
            f.AddLine("#: " + data->GetReferences()[i]);
        dummy = data->GetFlags();
        if (!dummy.IsEmpty())
            f.AddLine(dummy);
        dummy = data->GetString();
        if (dummy.Find("\\n") != wxNOT_FOUND)
            dummy = "\"\n\"" + dummy;
        dummy.Replace("\\n", "\\n\"\n\"");
        SaveMultiLines(f, "msgid \"" + dummy + "\"");
        dummy = data->GetTranslation();
        if (dummy.Find("\\n") != wxNOT_FOUND)
            dummy = "\"\n\"" + dummy;
        dummy.Replace("\\n", "\\n\"\n\"");
        if (encConv)
            dummy = wxString(dummy.wc_str(wxConvUTF8), *encConv);

        SaveMultiLines(f, "msgstr \"" + dummy + "\"");
        f.AddLine("");
    }
    
    delete encConv;    
    f.Write(crlf);
    f.Close();

    /* If user wants it, compile .mo file right now: */
    
    if (save_mo && wxConfig::Get()->Read("compile_mo", true))
        ExecuteGettext("msgfmt -o " + 
                       po_file.BeforeLast('.') + ".mo " + po_file);

    m_fileName = po_file;
    
    return true;
}


bool Catalog::Update()
{
    if (!m_isOk) return false;

    ProgressInfo pinfo;
    pinfo.SetTitle(_("Updating catalog..."));

    wxString cwd = wxGetCwd();
    if (m_fileName != "") 
    {
        wxString path;
        
        if (wxIsAbsolutePath(m_header.BasePath))
            path = m_header.BasePath;
        else
            path = wxPathOnly(m_fileName) + "/" + m_header.BasePath;
        
        if (wxIsAbsolutePath(path))
            wxSetWorkingDirectory(path);
        else
            wxSetWorkingDirectory(cwd + "/" + path);
    }
    
    SourceDigger dig(&pinfo);
    Catalog *newcat = dig.Dig(m_header.SearchPaths, m_header.Keywords);

    if (newcat != NULL) 
    {
        pinfo.UpdateMessage(_("Merging differences..."));

        if (wxConfig::Get()->Read("show_summary", true))
        {
            wxArrayString snew, sobsolete;
            GetMergeSummary(newcat, snew, sobsolete);
            MergeSummaryDialog sdlg;
            sdlg.TransferTo(snew, sobsolete);
            if (sdlg.ShowModal() == wxID_OK)
                Merge(newcat);
            else
            {
                delete newcat; newcat = NULL;
            }
        }
        else
            Merge(newcat);
    }
    
    wxSetWorkingDirectory(cwd);
    
    if (newcat == NULL) return false;
  
    delete newcat;
    return true;
}


void Catalog::Merge(Catalog *refcat)
{
    wxString oldname = m_fileName;
    wxString tmp1 = wxGetTempFileName("poedit");
    wxString tmp2 = wxGetTempFileName("poedit");
    wxString tmp3 = wxGetTempFileName("poedit");

    refcat->Save(tmp1, false);
    Save(tmp2, false);
    
    ExecuteGettext("msgmerge --force-po -o " + tmp3 + " " + tmp2 + " " + tmp1);

    Catalog *c = new Catalog(tmp3);
    Clear();
    Append(*c);
    delete c;

    wxRemoveFile(tmp1);
    wxRemoveFile(tmp2);
    wxRemoveFile(tmp3);
    wxRemoveFile(tmp1 + ".poedit");
    wxRemoveFile(tmp2 + ".poedit");
    
    m_fileName = oldname;
}


void Catalog::GetMergeSummary(Catalog *refcat, 
                          wxArrayString& snew, wxArrayString& sobsolete)
{
    snew.Empty();
    sobsolete.Empty();
    unsigned i;
    
    for (i = 0; i < GetCount(); i++)
        if (refcat->FindItem((*this)[i].GetString()) == NULL)
        sobsolete.Add((*this)[i].GetString());

    for (i = 0; i < refcat->GetCount(); i++)
        if (FindItem((*refcat)[i].GetString()) == NULL)
        snew.Add((*refcat)[i].GetString());
}


CatalogData* Catalog::FindItem(const wxString& key) const
{
    return (CatalogData*) m_data->Get(key);
}


bool Catalog::Translate(const wxString& key, const wxString& translation)
{
    CatalogData *d = FindItem(key);
    
    if (d == NULL) 
        return false;
    else
    {
        d->SetTranslation(translation);
        return true;
    }
}


void Catalog::Append(Catalog& cat)
{
    CatalogData *dt, *mydt;
    
    for (unsigned i = 0; i < cat.GetCount(); i++)
    {
        dt = &(cat[i]);
        mydt = FindItem(dt->GetString());
        if (mydt == NULL)
        {
            mydt = new CatalogData(*dt);
            m_data->Put(dt->GetString(), mydt);
            m_dataArray.Add(mydt);
            m_count++;
        }
        else
        {
            for (unsigned j = 0; j < dt->GetReferences().GetCount(); j++)
                mydt->AddReference(dt->GetReferences()[j]);
            if (!dt->GetTranslation().IsEmpty())
                mydt->SetTranslation(dt->GetTranslation());
            if (dt->IsFuzzy()) mydt->SetFuzzy(true);
        }
    }
}


void Catalog::GetStatistics(int *all, int *fuzzy, int *untranslated)
{
    if (all) *all = 0;
    if (fuzzy) *fuzzy = 0;
    if (untranslated) *untranslated = 0;
    for (size_t i = 0; i < GetCount(); i++)
    {
        if (all) (*all)++;
        if ((*this)[i].IsFuzzy()) (*fuzzy)++;
        if (!(*this)[i].IsTranslated()) (*untranslated)++;
    }
}


void CatalogData::SetFlags(const wxString& flags)
{
    m_isFuzzy = false;
    m_moreFlags.Empty();
    
    if (flags.IsEmpty()) return;
    wxStringTokenizer tkn(flags.Mid(1), " ,", wxTOKEN_STRTOK);
    wxString s;
    while (tkn.HasMoreTokens())
    {
        s = tkn.GetNextToken();
        if (s == "fuzzy") m_isFuzzy = true;
        else m_moreFlags << ", " << s;
    }
}


wxString CatalogData::GetFlags()
{
    wxString f;
    if (m_isFuzzy) f << ", fuzzy";
    f << m_moreFlags;
    if (!f.IsEmpty()) 
        return "#" + f;
    else 
        return wxEmptyString;
}
