
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      transmemupd.h
    
      Translation memory database updater
    
      (c) Vaclav Slavik, 2001-2003

*/


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface
#endif

#ifndef _TRANSMEMUPD_H_
#define _TRANSMEMUPD_H_

#ifdef USE_TRANSMEM

class WXDLLEXPORT wxString;
class WXDLLEXPORT wxArrayString;

class TranslationMemory;
class ProgressInfo;

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
#ifdef __UNIX__
        bool UpdateFromRPM(const wxString& filename);
#endif        
        bool UpdateFromCatalog(const wxString& filename);
        
    private:
        ProgressInfo *m_progress;
        TranslationMemory *m_mem;
};

#endif // USE_TRANSMEM

#endif // _TRANSMEMUPD_H_
