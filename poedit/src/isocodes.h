
//
// This file contains languages & countries names and 2-letter codes as
// defined by ISO 639 and ISO 3166 standards.
//

struct LanguageStruct
{
    const wxChar *iso, *lang;
};

extern LanguageStruct isoLanguages[];
extern LanguageStruct isoCountries[];

extern const wxChar *LookupLanguageCode(const wxChar *language);
extern const wxChar *LookupCountryCode(const wxChar *country);

extern bool IsKnownLanguageCode(const wxChar *code);
extern bool IsKnownCountryCode(const wxChar *code);
