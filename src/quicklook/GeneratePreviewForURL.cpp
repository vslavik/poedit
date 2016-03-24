/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2015-2016 Vaclav Slavik
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

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>
#include <sys/syslimits.h>

#include <sstream>

#include <wx/init.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/osx/core/cfstring.h>

#include <unicode/uclean.h>
#include <unicode/putil.h>

#include "catalog.h"

#if wxUSE_GUI
    #error "compiled with GUI features of wx - not needed"
#endif

namespace
{

CFDataRef CreateHTMLDataForURL(CFURLRef url, CFStringRef contentTypeUTI)
{
    #pragma unused(contentTypeUTI)

    wxCFStringRef path(CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle));
    if (!path)
        return NULL;

    Catalog cat(path.AsString());
    if (!cat.IsOk())
        return NULL;

    std::ostringstream s;
    cat.ExportToHTML(s);
    std::string data = s.str();
    return CFDataCreate(NULL, (const UInt8*)data.data(), data.length());
}

} // anonymous namespace

extern "C"
{

void Initialize_plugin()
{
    wxInitialize();

    CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("net.poedit.PoeditQuicklook"));
    if (bundle)
    {
        CFURLRef url = CFBundleCopyResourcesDirectoryURL(bundle);
        char path[PATH_MAX] = {0};
        if (CFURLGetFileSystemRepresentation(url, true, (UInt8*)path, PATH_MAX))
        {
            u_setDataDirectory(path);
            wxFileTranslationsLoader::AddCatalogLookupPathPrefix(path);
            
            wxTranslations *trans = new wxTranslations();
            wxTranslations::Set(trans);
            trans->AddCatalog("poedit");

            wxString bestTrans = trans->GetBestTranslation("poedit");
            Language uiLang = Language::TryParse(bestTrans.ToStdWstring());
            UErrorCode err = U_ZERO_ERROR;
            icu::Locale::setDefault(uiLang.ToIcu(), err);
        }
        CFRelease(url);
    }
}

void Uninitialize_plugin()
{
    u_cleanup();

    wxUninitialize();
}


/* -----------------------------------------------------------------------------
   Generate a preview for file

   This function's job is to create preview for designated file
   ----------------------------------------------------------------------------- */

OSStatus GeneratePreviewForURL(void *thisInterface, QLPreviewRequestRef preview, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options)
{
    #pragma unused(thisInterface, options)

    CFDataRef data = CreateHTMLDataForURL(url, contentTypeUTI);
    if (data)
    {
        QLPreviewRequestSetDataRepresentation(preview, data, kUTTypeHTML, NULL);
        CFRelease(data);
    }

    return noErr;
}

void CancelPreviewGeneration(void *thisInterface, QLPreviewRequestRef preview)
{
    #pragma unused(thisInterface, preview)
    // not supported
}


/* -----------------------------------------------------------------------------
    Generate a thumbnail for file

   This function's job is to create thumbnail for designated file as fast as possible
   ----------------------------------------------------------------------------- */

OSStatus GenerateThumbnailForURL(void *thisInterface, QLThumbnailRequestRef thumbnail, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options, CGSize maxSize)
{
    #pragma unused(thisInterface, thumbnail, url, contentTypeUTI, options, maxSize)
    
    // We could generate a thumbnail easily from HTML representation using
    // QLThumbnailRequestSetThumbnailWithDataRepresentation. But it would look
    // the same for all PO files at small (or even not so small) sizes, so it
    // would be pretty pointless as thumbnails go. Instead, don't generate any.
    return noErr;
}

void CancelThumbnailGeneration(void *thisInterface, QLThumbnailRequestRef thumbnail)
{
    #pragma unused(thisInterface, thumbnail)
    // not supported
}

} // extern "C"
