
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


#include <stdio.h>
#include <wx/utils.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <wx/intl.h>
#include <wx/datetime.h>
#include <wx/encconv.h>
#include <wx/fontmap.h>
#include <wx/config.h>
#include <wx/textfile.h>

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
    if (m_TextFile->GetLineCount() == 0) return;

    wxString line, dummy;
    wxString mflags, mstr, mtrans, mcomment;
    wxArrayString mrefs;
    
    line = m_TextFile->GetFirstLine();
    if (line.IsEmpty()) line = ReadTextLine(m_TextFile);

    while (!line.IsEmpty())
    {
        // flags:
        if (ReadParam(line, "#, ", dummy))
        {
            mflags = "#, " + dummy;
            line = ReadTextLine(m_TextFile);
        }
        
        // references:
        if (ReadParam(line, "#: ", dummy))
        {
            wxStringTokenizer tkn(dummy, "\t\r\n ");
            while (tkn.HasMoreTokens())
                mrefs.Add(tkn.GetNextToken());
            line = ReadTextLine(m_TextFile);
        }
        
        // msgid:
        else if (ReadParam(line, "msgid \"", dummy))
        {
            mstr = dummy.RemoveLast();
            while (!(line = ReadTextLine(m_TextFile)).IsEmpty())
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
            while (!(line = ReadTextLine(m_TextFile)).IsEmpty())
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
                line = ReadTextLine(m_TextFile);
            }
        }
        
        else
            line = ReadTextLine(m_TextFile);
    }
}




class LoadParser : public CatalogParser
{
    public:
        LoadParser(Catalog *c, wxTextFile *f) : CatalogParser(f), m_Catalog(c) {}

    protected:
        Catalog *m_Catalog;
    
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
            ReadParam(ln2, "Project-Id-Version: ", m_Catalog->m_Header.Project);
            ReadParam(ln2, "POT-Creation-Date: ", m_Catalog->m_Header.CreationDate);
            ReadParam(ln2, "PO-Revision-Date: ", m_Catalog->m_Header.RevisionDate);
            if (ReadParam(ln2, "Last-Translator: ", dummy2))
            {
                wxStringTokenizer tkn2(dummy2, "<>");
                if (tkn2.CountTokens() != 2) wxLogWarning(_("Corrupted translator record, please correct in Catalog/Settings"));
                m_Catalog->m_Header.Translator = tkn2.GetNextToken().Strip(wxString::trailing);
                m_Catalog->m_Header.TranslatorEmail = tkn2.GetNextToken();
            }
            if (ReadParam(ln2, "Language-Team: ", dummy2))
            {
                wxStringTokenizer tkn2(dummy2, "<>");
                if (tkn2.CountTokens() != 2) wxLogWarning(_("Corrupted team record, please correct in Catalog/Settings"));
                m_Catalog->m_Header.Team = tkn2.GetNextToken().Strip(wxString::trailing);
                m_Catalog->m_Header.TeamEmail = tkn2.GetNextToken();
            }
            ReadParam(ln2, "Content-Type: text/plain; charset=", m_Catalog->m_Header.Charset);
        }
        m_Catalog->m_Header.Comment = comment;
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

        m_Catalog->m_Data->Put(msgid, d);
        m_Catalog->m_DataArray.Add(d);
        m_Catalog->m_Count++;
    }
    return true;
}





// catalog class:

Catalog::Catalog()
{
    m_Data = new wxHashTable(wxKEY_STRING);
    m_Count = 0;
    m_IsOk = true;
    m_Header.BasePath = "";
    m_FileEncoding = m_MemEncoding = wxFONTENCODING_SYSTEM;
}



Catalog::~Catalog() 
{
    Clear();
    delete m_Data;
}



Catalog::Catalog(const wxString& po_file)
{
    m_Data = NULL;
    m_Count = 0;
    m_IsOk = false;
    Load(po_file);
}



bool Catalog::Load(const wxString& po_file)
{
    wxTextFile f;

    Clear();
    delete m_Data; m_Data = NULL;    
    m_Count = 0;
    m_IsOk = false;
    m_FileName = po_file;
    m_FileEncoding = m_MemEncoding = wxFONTENCODING_SYSTEM;
    
    m_Header.BasePath = "";
   
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
            m_Data = new wxHashTable(wxKEY_STRING, 2 * sz /*optimal hash table size*/);
        }
        else
            m_Data = new wxHashTable(wxKEY_STRING);
        ReadParam(ReadTextLine(&f), "#. Language: ", m_Header.Language);
        ReadParam(ReadTextLine(&f), "#. Basepath: ", m_Header.BasePath);

        if (ReadParam(ReadTextLine(&f), "#. Paths: ", dummy))
        {
            long sz;
            dummy.ToLong(&sz);
            for (; sz > 0; sz--)
            if (ReadParam(ReadTextLine(&f), "#.     ", dummy))
                m_Header.SearchPaths.Add(dummy);
        }

        if (ReadParam(ReadTextLine(&f), "#. Keywords: ", dummy))
        {
            long sz;
            dummy.ToLong(&sz);
            for (; sz > 0; sz--)
            if (ReadParam(ReadTextLine(&f), "#.     ", dummy))
                m_Header.Keywords.Add(dummy);
        }

        f.Close();
    }
    else
        m_Data = new wxHashTable(wxKEY_STRING);

    if (!f.Open(po_file)) return false;

    LoadParser parser(this, &f);
    parser.Parse();

    m_IsOk = true;
    
    f.Close();
    
    // try to reencode it to platform's encoding
    wxFontMapper fmap;
    wxFontEncoding enc_in = fmap.CharsetToEncoding(m_Header.Charset, false);
    if (enc_in == wxFONTENCODING_SYSTEM) return true;
    wxFontEncodingArray equi = 
        wxEncodingConverter::GetPlatformEquivalents(enc_in);
    if (equi.GetCount() == 0) return true;
    wxFontEncoding enc_out = equi[0];

    m_MemEncoding = enc_out;
    m_FileEncoding = enc_in;
    if (enc_out == enc_in) return true;

    wxEncodingConverter encconv;
    if (!encconv.Init(enc_in, enc_out)) return true;
    
    for (size_t i = 0; i < m_DataArray.GetCount(); i++)
    {
        m_DataArray[i].SetTranslation(
                 encconv.Convert(m_DataArray[i].GetTranslation()));
    }
    
    return true;
}



void Catalog::Clear()
{
    delete m_Data; 
    m_DataArray.Empty();
    m_IsOk = true;
    m_Count = 0;
    m_Data = new wxHashTable(wxKEY_STRING);
}



static void GetCRLFBehaviour(wxTextFileType& type, bool& preserve)
{
    wxString format = wxConfigBase::Get()->Read("crlf_format", "unix");

    if (format == "win") type = wxTextFileType_Dos;
    else if (format == "mac") type = wxTextFileType_Mac;
    else if (format == "native") type = wxTextFile::typeDefault;
    else /* "unix" */ type = wxTextFileType_Unix;

    preserve = wxConfigBase::Get()->Read("keep_crlf", true);
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
    wxEncodingConverter encconv;
    wxTextFileType crlfOrig, crlf;
    bool crlfPreserve;
    wxTextFile f;
    
    GetCRLFBehaviour(crlfOrig, crlfPreserve);

    /* Find encoding: */

    wxFontMapper fmap;
    if (m_FileEncoding != wxFONTENCODING_SYSTEM)
        m_FileEncoding = fmap.CharsetToEncoding(m_Header.Charset, false);
    if (m_FileEncoding != m_MemEncoding)
    {
        if (!encconv.Init(m_MemEncoding, m_FileEncoding))
        {
            wxLogError(_("Cannot save in encoding '%s', please change it."),
                       m_Header.Charset.c_str());
            return false;
        }
    }
    
    /* Update information about last modification time: */
  
    wxDateTime timenow = wxDateTime::Now();
    int offs = wxDateTime::TimeZone(wxDateTime::Local).GetOffset();
    m_Header.RevisionDate.Printf("%s%s%02i%02i",
                                 timenow.Format("%Y-%m-%d %H:%M").c_str(),
                                 (offs > 0) ? "+" : "-",
                                 offs / 3600, (abs(offs) / 60) % 60);

    /* Detect CRLF format: */

    if (crlfPreserve && wxFileExists(po_file) && f.Open(po_file))
    {
        crlf = f.GuessType();
        if (crlf == wxTextFileType_None)
            crlf = crlfOrig;
        f.Close();
    }
    else
        crlf = crlfOrig;
        

    /* If neccessary, save extended info into .po.poedit file: */

    if (!m_Header.Language.IsEmpty() || 
        !m_Header.BasePath.IsEmpty() || 
        m_Header.SearchPaths.GetCount() > 0 ||
        m_Header.Keywords.GetCount() > 0)
    {
        if (!f.Open(po_file + ".poedit"))
            if (!f.Create(po_file + ".poedit"))
                return false;
        for (j = f.GetLineCount() - 1; j >= 0; j--)
            f.RemoveLine(j);

        f.AddLine("#. This catalog was generated by poedit");
        dummy.Printf("#. Number of items: %i", GetCount());
        f.AddLine(dummy);
        f.AddLine("#. Language: " + m_Header.Language);
        f.AddLine("#. Basepath: " + m_Header.BasePath);

        dummy.Printf("#. Paths: %i", m_Header.SearchPaths.GetCount());
        f.AddLine(dummy);
        for (i = 0; i < m_Header.SearchPaths.GetCount(); i++)
            f.AddLine("#.     " + m_Header.SearchPaths[i]);

        dummy.Printf("#. Keywords: %i", m_Header.Keywords.GetCount());
        f.AddLine(dummy);
        for (i = 0; i < m_Header.Keywords.GetCount(); i++)
            f.AddLine("#.     " + m_Header.Keywords[i]);
        f.Write(crlf);
        f.Close();
    }


    /* Save .po file: */

    if (!f.Open(po_file))
        if (!f.Create(po_file))
            return false;
    for (j = f.GetLineCount() - 1; j >= 0; j--)
        f.RemoveLine(j);

    SaveMultiLines(f, m_Header.Comment);
    f.AddLine("msgid \"\"");
    f.AddLine("msgstr \"\"");
    f.AddLine("\"Project-Id-Version: " + m_Header.Project + "\\n\"");
    f.AddLine("\"POT-Creation-Date: " + m_Header.CreationDate + "\\n\"");
    f.AddLine("\"PO-Revision-Date: " + m_Header.RevisionDate + "\\n\"");
    f.AddLine("\"Last-Translator: " + m_Header.Translator + 
              " <" + m_Header.TranslatorEmail + ">\\n\"");
    f.AddLine("\"Language-Team: " + m_Header.Team +
              " <" + m_Header.TeamEmail + ">\\n\"");
    f.AddLine("\"MIME-Version: 1.0\\n\"");
    f.AddLine("\"Content-Type: text/plain; charset=" + 
              m_Header.Charset + "\\n\"");
    f.AddLine("\"Content-Transfer-Encoding: 8bit\\n\"");
    f.AddLine("");

    for (i = 0; i < m_DataArray.GetCount(); i++)
    {
        data = &(m_DataArray[i]);
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
        if (m_FileEncoding != m_MemEncoding)
            SaveMultiLines(f, "msgstr \"" + encconv.Convert(dummy) + "\"");
        else
            SaveMultiLines(f, "msgstr \"" + dummy + "\"");
        f.AddLine("");
    }
    
    f.Write(crlf);
    f.Close();

    /* If user wants it, compile .mo file right now: */
    
    if (save_mo && wxConfig::Get()->Read("compile_mo", true))
        ExecuteGettext("msgfmt -o " + 
                       po_file.BeforeLast('.') + ".mo " + po_file);

    m_FileName = po_file;
    
    return true;
}



bool Catalog::Update()
{
    if (!m_IsOk) return false;

    ProgressInfo pinfo;
    pinfo.SetTitle(_("Updating catalog..."));

    wxString cwd = wxGetCwd();
    if (m_FileName != "") 
    {
        wxString path;
        
        if (wxIsAbsolutePath(m_Header.BasePath))
            path = m_Header.BasePath;
        else
            path = wxPathOnly(m_FileName) + "/" + m_Header.BasePath;
        
        if (wxIsAbsolutePath(path))
            wxSetWorkingDirectory(path);
        else
            wxSetWorkingDirectory(cwd + "/" + path);
    }
    
    SourceDigger dig(&pinfo);
    Catalog *newcat = dig.Dig(m_Header.SearchPaths, m_Header.Keywords);

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
    wxString oldname = m_FileName;
    wxString tmp1 = wxGetTempFileName("poedit"),
             tmp2 = wxGetTempFileName("poedit"),
             tmp3 = wxGetTempFileName("poedit");

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
    
    m_FileName = oldname;
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



CatalogData* Catalog::FindItem(const wxString& key)
{
    return (CatalogData*) m_Data->Get(key);
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
            m_Data->Put(dt->GetString(), mydt);
            m_DataArray.Add(mydt);
            m_Count++;
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
    m_IsFuzzy = false;
    m_MoreFlags.Empty();
    
    if (flags.IsEmpty()) return;
    wxStringTokenizer tkn(flags.Mid(1), " ,", wxTOKEN_STRTOK);
    wxString s;
    while (tkn.HasMoreTokens())
    {
        s = tkn.GetNextToken();
        if (s == "fuzzy") m_IsFuzzy = true;
        else m_MoreFlags << ", " << s;
    }
}



wxString CatalogData::GetFlags()
{
    wxString f;
    if (m_IsFuzzy) f << ", fuzzy";
    f << m_MoreFlags;
    if (!f.IsEmpty()) 
        return "#" + f;
    else 
        return wxEmptyString;
}
