/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2018 Vaclav Slavik
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

#ifndef Poedit_catalog_po_h
#define Poedit_catalog_po_h

#include "catalog.h"

class POCatalogItem;
class POCatalog;
typedef std::shared_ptr<POCatalogItem> POCatalogItemPtr;
typedef std::shared_ptr<POCatalog> POCatalogPtr;


class POCatalogItem : public CatalogItem
{
public:
    POCatalogItem() {}
    POCatalogItem(const CatalogItem&) = delete;

protected:

    friend class POLoadParser;
    friend class POCatalog;
};


/** This class holds information about one particular deleted item.
    This includes deleted lines, references, translation's status
    (fuzzy, non translated, translated) and optional comment(s).

    This class is mostly internal, used by Catalog to store data.
 */
class POCatalogDeletedData
{
public:
    /// Ctor.
    POCatalogDeletedData()
            : m_lineNum(0) {}
    POCatalogDeletedData(const wxArrayString& deletedLines)
            : m_deletedLines(deletedLines),
              m_lineNum(0) {}

    POCatalogDeletedData(const POCatalogDeletedData& dt)
            : m_deletedLines(dt.m_deletedLines),
              m_references(dt.m_references),
              m_extractedComments(dt.m_extractedComments),
              m_flags(dt.m_flags),
              m_comment(dt.m_comment),
              m_lineNum(dt.m_lineNum) {}

    /// Returns the deleted lines.
    const wxArrayString& GetDeletedLines() const { return m_deletedLines; }

    /// Returns references (#:) lines for the entry
    const wxArrayString& GetRawReferences() const { return m_references; }

    /// Returns comment added by the translator to this entry
    const wxString& GetComment() const { return m_comment; }

    /// Returns array of all auto comments.
    const wxArrayString& GetExtractedComments() const { return m_extractedComments; }

    /// Convenience function: does this entry has a comment?
    bool HasComment() const { return !m_comment.empty(); }

    /// Adds new reference to the entry (used by SourceDigger).
    void AddReference(const wxString& ref)
    {
        if (m_references.Index(ref) == wxNOT_FOUND)
            m_references.Add(ref);
    }

    /// Sets the string.
    void SetDeletedLines(const wxArrayString& a)
    {
        m_deletedLines = a;
    }

    /// Sets the comment.
    void SetComment(const wxString& c)
    {
        m_comment = c;
    }

    /** Sets gettext flags directly in string format. It may be
        either empty string or "#, fuzzy", "#, c-format",
        "#, fuzzy, c-format" or others (not understood by Poedit).
     */
    void SetFlags(const wxString& flags) {m_flags = flags;};

    /// Gets gettext flags. \see SetFlags
    wxString GetFlags() const {return m_flags;};

    /// Sets the number of the line this entry occurs on.
    void SetLineNumber(int line) { m_lineNum = line; }
    /// Get line number of this entry.
    int GetLineNumber() const { return m_lineNum; }

    /// Adds new extracted comments (#. )
    void AddExtractedComments(const wxString& com)
    {
        m_extractedComments.Add(com);
    }

private:
    wxArrayString m_deletedLines;

    wxArrayString m_references, m_extractedComments;
    wxString m_flags;
    wxString m_comment;
    int m_lineNum;
};

typedef std::vector<POCatalogDeletedData> POCatalogDeletedDataArray;


class POCatalog : public Catalog
{
public:
    // Common wrapping values
    static const int NO_WRAPPING = -1;
    static const int DEFAULT_WRAPPING = -2;

    /// Default ctor. Creates empty catalog, you have to call Load.
    explicit POCatalog(Type type = Type::PO);

    /// Ctor that loads the catalog from \a po_file with Load.
    /// \a flags is CreationFlags combination.
    explicit POCatalog(const wxString& po_file, int flags = 0);

    ~POCatalog() {}

    bool HasCapability(Cap cap) const override;

    void Clear() override;

    bool Save(const wxString& po_file, bool save_mo,
              ValidationResults& validation_results,
              CompilationStatus& mo_compilation_status) override;

    std::string SaveToBuffer() override;

    ValidationResults Validate() override;

    /// Compiles the catalog into binary MO file.
    bool CompileToMO(const wxString& mo_file,
                     ValidationResults& validation_results,
                     CompilationStatus& mo_compilation_status);

    /// Detect a particular common breakage of catalogs.
    bool HasDuplicateItems() const;

    /// Fixes a common invalid kind of entries, when msgids aren't unique.
    bool FixDuplicateItems();

    bool HasDeletedItems() const override
        { return !m_deletedItems.empty(); }

    void RemoveDeletedItems() override
        { m_deletedItems.clear(); }

    /// Updates the catalog from POT file.
    bool UpdateFromPOT(const wxString& pot_file, bool replace_header = false);
    bool UpdateFromPOT(POCatalogPtr pot, bool replace_header = false);
    static POCatalogPtr CreateFromPOT(const wxString& pot_file);

protected:
    /** Loads catalog from .po file.
        If file named po_file ".poedit" (e.g. "cs.po.poedit") exists,
        this function loads additional information from it. .po.poedit
        file contains parts of catalog header data that are not part
        of standard .po format, namely SearchPaths, Keywords, BasePath
        and Language.

        @param flags CreationFlags combination.
     */
    bool Load(const wxString& po_file, int flags = 0);

    void Clear() override;

    /// Adds entry to the catalog (the catalog will take ownership of
    /// the object).
    void AddItem(const POCatalogItemPtr& data)
        { m_items.push_back(data); }

    /// Adds entry to the catalog (the catalog will take ownership of
    /// the object).
    void AddDeletedItem(const POCatalogDeletedData& data)
        { m_deletedItems.push_back(data); }

protected:
    /// Fix commonly encountered fixable problems with loaded files
    void FixupCommonIssues();

    ValidationResults DoValidate(const wxString& po_file);
    bool DoSaveOnly(const wxString& po_file, wxTextFileType crlf);
    bool DoSaveOnly(wxTextBuffer& f, wxTextFileType crlf);

    /** Merges the catalog with reference catalog
        (in the sense of msgmerge -- this catalog is old one with
        translations, \a refcat is reference catalog created by Update().)

        \return true if the merge was successful, false otherwise.
                Note that if it returns false, the catalog was
                \em not modified!
     */
    bool Merge(const POCatalogPtr& refcat);

protected:
    POCatalogDeletedDataArray m_deletedItems;

    wxTextFileType m_fileCRLF;
    int m_fileWrappingWidth;

    friend class POLoadParser;
};


/// Internal class - used for parsing of po files.
class POCatalogParser
{
public:
    POCatalogParser(wxTextFile *f)
        : m_textFile(f),
          m_detectedLineWidth(0),
          m_detectedWrappedLines(false),
          m_lastLineHardWrapped(true), m_previousLineHardWrapped(true),
          m_ignoreHeader(false),
          m_ignoreTranslations(false)
    {}

    virtual ~POCatalogParser() {}

    /// Tell the parser to ignore header entries when processing
    void IgnoreHeader(bool ignore) { m_ignoreHeader = ignore; }

    /// Tell the parser to treat input as POT and ignore translations
    void IgnoreTranslations(bool ignore) { m_ignoreTranslations = ignore; }

    /** Parses the entire file, calls OnEntry each time
        new msgid/msgstr pair is found.

        @return false if parsing failed, true otherwise
     */
    bool Parse();

    int GetWrappingWidth() const;

protected:
    // Read one line from file, remove all \r and \n characters, ignore empty lines:
    wxString ReadTextLine();

    void PossibleWrappedLine()
    {
        if (!m_previousLineHardWrapped)
            m_detectedWrappedLines = true;
    }

    /** Called when new entry was parsed. Parsing continues
        if returned value is true and is cancelled if it
        is false.
     */
    virtual bool OnEntry(const wxString& msgid,
                         const wxString& msgid_plural,
                         bool has_plural,
                         bool has_context,
                         const wxString& context,
                         const wxArrayString& mtranslations,
                         const wxString& flags,
                         const wxArrayString& references,
                         const wxString& comment,
                         const wxArrayString& extractedComments,
                         const wxArrayString& msgid_old,
                         unsigned lineNumber) = 0;

    /** Called when new deleted entry was parsed. Parsing continues
        if returned value is true and is cancelled if it
        is false. Defaults to an empty implementation.
     */
    virtual bool OnDeletedEntry(const wxArrayString& /*deletedLines*/,
                                const wxString& /*flags*/,
                                const wxArrayString& /*references*/,
                                const wxString& /*comment*/,
                                const wxArrayString& /*extractedComments*/,
                                unsigned /*lineNumber*/)
    {
        return true;
    }

    virtual void OnIgnoredEntry() {}

    /// Textfile being parsed.
    wxTextFile *m_textFile;
    int m_detectedLineWidth;
    bool m_detectedWrappedLines;
    bool m_lastLineHardWrapped, m_previousLineHardWrapped;

    /// Whether the header should be parsed or not
    bool m_ignoreHeader;

    /// Whether the translations should be ignored (as if it was a POT)
    bool m_ignoreTranslations;
};

#endif // Poedit_catalog_po_h
