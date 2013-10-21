/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2001-2013 Vaclav Slavik
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

#ifndef _TRANSMEMUPD_H_
#define _TRANSMEMUPD_H_

class WXDLLIMPEXP_FWD_BASE wxString;
class WXDLLIMPEXP_FWD_BASE wxArrayString;

class TranslationMemory;
class ProgressInfo;

#if defined(__UNIX__) && !defined(__WXMAC__)
    #define HAVE_RPM 1
#else
    #define HAVE_RPM 0
#endif

/** TranslationMemoryUpdater is a worker object that fills given
    translation memory object with all existing translations found in
    the system (in specified paths, to be exact).

    \see TranslationMemory
 */
class TranslationMemoryUpdater
{
    public:
        /// Ctor.
        TranslationMemoryUpdater(TranslationMemory *mem, ProgressInfo *pi);
        
        /** Finds all files that can be used to update given TM in \a paths.
            The files are searched based on TM's language and PO, MO and RPM
            files are currently returned. Found files are stored in \a files.
            \return  false if an error occured, true otherwise
         */
        static bool FindFilesInPaths(const wxArrayString& paths,
                                     wxArrayString& files,
                                     const wxString& lang);
        
        /** Updates TM \a m with data from all catalogs listed in \a files.
            \return  false if an error occured, true otherwise
            \remarks This method currently scans all PO, MO and RPM files.
                     It uses msgunfmt to convert MOs to POs and rpm2cpio
                     and cpio to extract catalogs from RPM packages.
         */
        bool Update(const wxArrayString& files);

    protected:
        bool UpdateFromPO(const wxString& filename);
        bool UpdateFromMO(const wxString& filename);
#if HAVE_RPM
        bool UpdateFromRPM(const wxString& filename);
#endif
        bool UpdateFromCatalog(const wxString& filename);

    private:
        ProgressInfo *m_progress;
        TranslationMemory *m_mem;
};

#endif // _TRANSMEMUPD_H_
