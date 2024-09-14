/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2024 Vaclav Slavik
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

#import "ThumbnailProvider.h"

@implementation ThumbnailProvider

- (void)provideThumbnailForFileRequest:(QLFileThumbnailRequest *)request
                     completionHandler:(void (^)(QLThumbnailReply * _Nullable, NSError * _Nullable))handler
{
    // The entire purpose of this thumbnail provider is to fail to create
    // thumbnails, i.e. do absolutely nothing.
    //
    // The reason for this bizarre piece of code to exist is that macOS has
    // hardcoded builtin thumbnailer for plain-text files and the only way
    // to disable for derived file types such as PO files is to provide a more
    // specialized thumbnail provider. But we don't want thumbnails to begin
    // with, so here we are, faking our way to a reasonable behavior that
    // should be macOS' default...
    //
    // ¯\_(ツ)_/¯

    handler([QLThumbnailReply replyWithContextSize:request.maximumSize currentContextDrawingBlock:^BOOL {
        // return YES if the thumbnail was successfully drawn inside this block
        return NO;
    }], nil);
}

@end
