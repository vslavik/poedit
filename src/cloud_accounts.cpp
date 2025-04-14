/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2023-2025 Vaclav Slavik
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

#include "cloud_accounts.h"

#ifdef HAVE_HTTP_CLIENT

#include "crowdin_client.h"
#include "localazy_client.h"


CloudAccountClient& CloudAccountClient::Get(const std::string& service_name)
{
    if (service_name == CrowdinClient::SERVICE_NAME)
        return CrowdinClient::Get();
    if (service_name == LocalazyClient::SERVICE_NAME)
        return LocalazyClient::Get();

    BOOST_THROW_EXCEPTION(std::logic_error("invalid cloud service name"));
}


void CloudAccountClient::CleanUp()
{
    CrowdinClient::CleanUp();
    LocalazyClient::CleanUp();
}


std::shared_ptr<CloudAccountClient::FileSyncMetadata> CloudAccountClient::ExtractSyncMetadataIfAny(Catalog& catalog)
{
    std::shared_ptr<CloudAccountClient::FileSyncMetadata> meta;
    meta = CrowdinClient::Get().ExtractSyncMetadata(catalog);
    if (meta)
        return meta;
    meta = LocalazyClient::Get().ExtractSyncMetadata(catalog);
    if (meta)
        return meta;
    return nullptr;
}


#endif // #ifdef HAVE_HTTP_CLIENT
