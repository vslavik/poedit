
#define Path
#define DestName
#define FindHandle
#define FindResult

#sub ProcessCat
  #define FileName FindGetFileName(FindHandle)
  #define BaseName Copy(FileName, 1, Len(FileName)-3)
  Source: {#Path}\{#FileName}; DestDir: {app}\share\locale\{#BaseName}\LC_MESSAGES; Components: i18n; DestName: {#DestName}
#endsub

#sub DoLocaleFiles
  #for {FindHandle = FindResult = FindFirst("..\"+Path+"\*.mo", 0); FindResult; FindResult = FindNext(FindHandle)} ProcessCat
  #if !FindHandle
    #error "No catalogs found"
  #endif
  #expr FindClose(FindHandle)
#endsub

#sub LocaleFiles
  #expr Path = "locales"
  #expr DestName = "poedit.mo"
  #emit DoLocaleFiles
  
  #expr Path = "deps\wx\locale"
  #expr DestName = "wxstd.mo"
  #emit DoLocaleFiles

  #expr Path = "deps\wx\locale\msw"
  #expr DestName = "wxmsw.mo"
  #emit DoLocaleFiles
#endsub

