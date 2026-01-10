/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2025-2026 Vaclav Slavik
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

#include "catalog_xcloc.h"

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/translation.h>


bool XCLOCCatalog::CanLoadFile(const wxString& extension)
{
    return extension == "xcloc";
}


bool XCLOCCatalog::HasCapability(Cap cap) const
{
    // We don't support changing the language yet, as it is more complicated: the .xcloc bundle has
    // other files in there and multiple places where it would need to be modified. Poedit takes the
    // position that it is an editor for only the l10n part of it.
    if (cap == Cap::LanguageSetting)
        return false;

    return XLIFF1Catalog::HasCapability(cap);
}


void XCLOCCatalog::SetLanguage(Language)
{
    wxASSERT_MSG(false, "Setting XCLOC language not supported.");
}


std::shared_ptr<XLIFFCatalog> XCLOCCatalog::Open(const wxString& filename)
{
    struct Creator : public InstanceCreatorImpl
    {
        Creator(const wxString& filename_) : filename(filename_) {}

        std::shared_ptr<XLIFFCatalog> CreateFromDoc(pugi::xml_document&& doc, const std::string& xliff_version) override
        {
            // Apple .xcloc uses XLIFF 1.2 only:
            if (xliff_version != "1.2")
                return nullptr;

            auto c = std::make_shared<XCLOCCatalog>(std::move(doc), /*subversion=*/2);
            c->m_originalFilename = filename;
            c->m_embeddedXLIFFFilename = embedded_xliff;
            return c;
        }

        wxString filename;
        wxString embedded_xliff;
    };

    Creator c(filename);

    // XCLOC bundles contain additional metadata and other localizable resources (images, text files),
    // but we only care about l18n content. Find the embedded XLIFF file and open that:
    wxDir dir(filename + "/Localized Contents");
    if (!dir.IsOpened())
        BOOST_THROW_EXCEPTION(Exception(_("Unexpectedly missing content in the XCLOC file.")));

    if (!dir.GetFirst(&c.embedded_xliff, "*.xliff", wxDIR_FILES))
        BOOST_THROW_EXCEPTION(Exception(_("Unexpectedly missing content in the XCLOC file.")));

    return OpenImpl(dir.GetNameWithSep() + c.embedded_xliff, c);
}


bool XCLOCCatalog::Save(const wxString& filename, bool save_mo,
                        ValidationResults& validation_results,
                        CompilationStatus& mo_compilation_status)
{
    if (filename != m_originalFilename)
        BOOST_THROW_EXCEPTION(Exception(_("Saving in a different location is not supported for XCLOC files.")));

    // update mtime for the directory to indicate that the file was modified; without this, it wouldn't
    // be apparent as we're actually writing a file inside a subdirectory in the bundle:
    wxFileName(filename).Touch();

    wxString xliff_fn = filename + "/Localized Contents/" + m_embeddedXLIFFFilename;
    return XLIFF1Catalog::Save(xliff_fn, save_mo, validation_results, mo_compilation_status);
}
