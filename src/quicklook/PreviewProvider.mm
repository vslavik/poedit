/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2024-2025 Vaclav Slavik
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

#import "PreviewProvider.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <wx/defs.h>
#pragma clang diagnostic pop

#include <wx/init.h>
#include <wx/intl.h>

#include <unicode/uloc.h>
#include <unicode/uclean.h>

#include <sstream>

#include "catalog.h"
#include "errors.h"
#include "str_helpers.h"

#if wxUSE_GUI
    #error "compiled with GUI features of wx - not needed"
#endif

namespace
{

void initialize_extension()
{
    UErrorCode err = U_ZERO_ERROR;
    u_init(&err);

    wxInitialize();

    auto resources = str::to_wx(NSBundle.mainBundle.resourcePath);
    wxFileTranslationsLoader::AddCatalogLookupPathPrefix(resources);

    wxTranslations *trans = new wxTranslations();
    wxTranslations::Set(trans);
    trans->AddCatalog("poedit-quicklook");

    wxString bestTrans = trans->GetBestTranslation("poedit-quicklook");
    Language uiLang = Language::TryParse(bestTrans.ToStdWstring());
    uloc_setDefault(uiLang.IcuLocaleName().c_str(), &err);
}

void cleanup_extension()
{
    wxUninitialize();
    u_cleanup();
}

NSData *create_html_preview(const wxString& filename)
{
    auto cat = Catalog::Create(filename);

    std::ostringstream s;
    cat->ExportToHTML(s);
    auto data = s.view();
    return [NSData dataWithBytes:data.data() length:data.length()];
}

} // anonymous namespace


@implementation PreviewProvider

- (id)init
{
    self = [super init];
    if (self)
    {
        initialize_extension();
    }
    return self;
}

- (void)dealloc
{
    cleanup_extension();
}

- (void)providePreviewForFileRequest:(QLFilePreviewRequest *)request completionHandler:(void (^)(QLPreviewReply * _Nullable reply, NSError * _Nullable error))handler
{
    QLPreviewReply* reply = [[QLPreviewReply alloc] initWithDataOfContentType:UTTypeHTML
                                                                  contentSize:NSZeroSize
                                                            dataCreationBlock:^NSData * _Nullable(QLPreviewReply * _Nonnull replyToUpdate, NSError *__autoreleasing  _Nullable * _Nullable error)
    {
        try
        {
            replyToUpdate.stringEncoding = NSUTF8StringEncoding;
            return create_html_preview(str::to_wx(request.fileURL.path));
        }
        catch (...)
        {
            *error = [NSError errorWithDomain:@"net.poedit.Poedit" code:0 userInfo:@{
                NSLocalizedDescriptionKey: str::to_NS(DescribeCurrentException())
            }];
            return nil;
        }
    }];

    handler(reply, nil);
}

@end
