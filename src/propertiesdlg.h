/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2017 Vaclav Slavik
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

#ifndef _PROPERTIESDLG_H_
#define _PROPERTIESDLG_H_

#include <wx/dialog.h>
#include <wx/notebook.h>

#include <memory>

#include "catalog.h"
#include "languagectrl.h"

class WXDLLIMPEXP_FWD_ADV wxEditableListBox;
class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
class WXDLLIMPEXP_FWD_CORE wxRadioButton;
class WXDLLIMPEXP_FWD_CORE wxCheckBox;
class WXDLLIMPEXP_FWD_CORE wxComboBox;

/// Dialog setting various catalog parameters.
class PropertiesDialog : public wxDialog
{
    public:
        PropertiesDialog(wxWindow *parent, CatalogPtr cat, bool fileExistsOnDisk, int initialPage = 0);
        ~PropertiesDialog();

        /// Reads data from the catalog and fill dialog's controls.
        void TransferTo(const CatalogPtr& cat);

        /// Saves data from the dialog to the catalog.
        void TransferFrom(const CatalogPtr& cat);

        virtual bool Validate();
            
    private:
        void DisableSourcesControls();

        void OnLanguageChanged(wxCommandEvent& event);
        void OnLanguageValueChanged(const wxString& langstr);
        void OnPluralFormsDefault(wxCommandEvent& event);
        void OnPluralFormsCustom(wxCommandEvent& event);
        void OnGettextSettings(wxCommandEvent& event);

        struct PathsData;
        class BasePathCtrl;
        class PathsList;
        class SourcePathsList;
        class ExcludedPathsList;

        struct GettextSettings;
        class GettextSettingsDialog;

        wxTextCtrl *m_team, *m_project;
        LanguageCtrl *m_language;
        wxComboBox *m_charset, *m_sourceCodeCharset;
        wxRadioButton *m_pluralFormsDefault, *m_pluralFormsCustom;
        wxTextCtrl *m_pluralFormsExpr;
        BasePathCtrl *m_basePath;
        std::shared_ptr<PathsData> m_pathsData;
        PathsList *m_paths, *m_excludedPaths;
        wxEditableListBox *m_keywords;
        wxCheckBox *m_defaultKeywords;
        wxString m_rememberedPluralForm;
        std::shared_ptr<GettextSettings> m_gettextSettings;
        bool m_hasLang;
        int m_validatedPlural, m_validatedLang;
};



#endif // _PROPERTIESDLG_H_
