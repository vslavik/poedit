
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
