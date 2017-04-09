/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2017 Vaclav Slavik
 *  Copyright (C) 2005 Olivier Sannier
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

#ifndef Poedit_edlistctrl_h
#define Poedit_edlistctrl_h

#include <wx/dataview.h>
#include <wx/frame.h>

#include <vector>

class WXDLLIMPEXP_FWD_CORE wxListCtrl;
class WXDLLIMPEXP_FWD_CORE wxListEvent;

#include "catalog.h"
#include "cat_sorting.h"
#include "language.h"

// list control with both columns equally wide:
class PoeditListCtrl : public wxDataViewCtrl
{
    public:
        PoeditListCtrl(wxWindow *parent, wxWindowID id = -1, bool dispIDs = false);

        virtual ~PoeditListCtrl();

        /// Re-sort the control according to user-specified criteria.
        void Sort();

        void SizeColumns();

        void SetDisplayLines(bool dl);

        void CatalogChanged(const CatalogPtr& catalog);

        int ListItemToListIndex(const wxDataViewItem& item) const
        {
            return item.IsOk() ? m_model->GetRow(item) : -1;
        }

        wxDataViewItem ListIndexToListItem(int index) const
        {
            return index != -1 ? m_model->GetItem(index) : wxDataViewItem();
        }

        /// Returns the list item index for the given catalog index
        int CatalogIndexToList(int index) const
        {
            return m_model->RowFromCatalogIndex(index);
        }

        wxDataViewItem CatalogIndexToListItem(int index) const
        {
            return m_model->GetItem(m_model->RowFromCatalogIndex(index));
        }

        /// Returns item's index in the catalog
        int ListIndexToCatalog(int index) const
        {
            return index != -1 ? m_model->CatalogIndex(index) : -1;
        }

        int ListItemToCatalogIndex(const wxDataViewItem& item) const
        {
            return item.IsOk() ? m_model->CatalogIndex(m_model->GetRow(item)) : -1;
        }

        CatalogItemPtr ListItemToCatalogItem(const wxDataViewItem& item) const
        {
            return item.IsOk() ? m_model->Item(m_model->GetRow(item)) : nullptr;
        }

        /// Returns item from the catalog based on list index
        CatalogItemPtr ListIndexToCatalogItem(int index) const
        {
            return index != -1 ? m_model->Item(index) : nullptr;
        }

        CatalogItemPtr GetCurrentCatalogItem()
        {
            return ListItemToCatalogItem(GetCurrentItem());
        }

        std::vector<CatalogItemPtr> GetSelectedCatalogItems() const
        {
            auto catalog = m_model->m_catalog;
            std::vector<CatalogItemPtr> s;
            for (auto i: GetSelectedCatalogItemIndexes())
                s.push_back((*catalog)[i]);
            return s;
        }

        std::vector<int> GetSelectedCatalogItemIndexes() const
        {
            wxDataViewItemArray sel;
            int count = GetSelections(sel);

            std::vector<int> s;
            s.reserve(count);
            for (auto i: sel)
                s.push_back(m_model->CatalogIndex(m_model->GetRow(i)));

            return s;
        }

        void SetSelectedCatalogItemIndexes(const std::vector<int>& selection)
        {
            wxDataViewItemArray sel;
            for (auto i: selection)
                sel.push_back(CatalogIndexToListItem(i));
            SetSelections(sel);
        }

        // Perform given function for all selected items. The function takes
        // reference to the item as its argument. Also refresh the items touched,
        // on the assumption that the operation modifies them.
        template<typename T>
        void ForSelectedCatalogItemsDo(T func)
        {
            wxDataViewItemArray sel;
            GetSelections(sel);
            for (auto item: sel)
                func(*ListItemToCatalogItem(item));
            m_model->ItemsChanged(sel);
        }

        void SelectOnly(const wxDataViewItem& item)
        {
            wxDataViewItemArray sel;
            sel.push_back(item);
            SetSelections(sel);
            EnsureVisible(item);

        #ifndef __WXOSX__
          #if wxCHECK_VERSION(3,1,1)
            wxDataViewEvent event(wxEVT_DATAVIEW_SELECTION_CHANGED, this, item);
          #else
            wxDataViewEvent event(wxEVT_DATAVIEW_SELECTION_CHANGED, GetId());
            event.SetEventObject(this);
            event.SetModel(GetModel());
            event.SetItem(item);
          #endif
            ProcessWindowEvent(event);
        #endif
        }

        void SelectAndFocus(const wxDataViewItem& item)
        {
        #ifndef __WXOSX__ // implicit in selection on macOS
            SetCurrentItem(item);
        #endif
            SelectOnly(item);
        }

        void SelectOnly(int n)
        {
            // TODO: Remove this API

            SelectOnly(m_model->GetItem(n));
        }

        void SelectAndFocus(int n)
        {
            // TODO: Remove this API

        #ifndef __WXOSX__ // implicit in selection on macOS
            SetCurrentItem(m_model->GetItem(n));
        #endif
            SelectOnly(n);
        }

        /// Returns true if exactly one item is selected.
        bool HasSingleSelection() const { return GetSelectedItemsCount() == 1; }

        /// Returns true if more than one item are selected.
        bool HasMultipleSelection() const { return GetSelectedItemsCount() > 1; }

        void RefreshSelectedItems()
        {
            // TODO: Remove this API

            wxDataViewItemArray sel;
            GetSelections(sel);
            m_model->ItemsChanged(sel);
        }

        void RefreshAllItems();

        void RefreshItem(const wxDataViewItem& item)
        {
            m_model->ItemChanged(item);
        }

        int GetCurrentItemListIndex()
        {
            return m_model->GetRow(GetCurrentItem());
        }

        int GetItemCount() const
        {
            return m_model->GetCount();
        }

        void SetCustomFont(wxFont font);

        // Order used for sorting
        SortOrder& sortOrder() { return m_model->sortOrder; }

    protected:
        void DoFreeze() override;
        void DoThaw() override;

    private:
        /// Model for the translation data
        class Model : public wxDataViewVirtualListModel
        {
        public:
            enum Column
            {
                Col_ID,
                Col_Icon,
                Col_Source,
                Col_Translation,
                Col_Max // invalid
            };

            Model(TextDirection appTextDir, wxVisualAttributes visual);
            virtual ~Model() {}

            void SetCatalog(CatalogPtr catalog);
            void UpdateSort();

            unsigned int GetColumnCount() const override { return Col_Max; }
            wxString GetColumnType( unsigned int col ) const override;

            void GetValueByRow(wxVariant& variant, unsigned row, unsigned col) const override;
            bool SetValueByRow(const wxVariant&, unsigned, unsigned) override;
            bool GetAttrByRow(unsigned row, unsigned col, wxDataViewItemAttr& attr) const override;

            CatalogItemPtr Item(int row) const
            {
                int index = CatalogIndex(row);
                return index != -1 ? (*m_catalog)[index] : nullptr;
            }

            /// Returns item's index in the catalog
            int CatalogIndex(int row) const
            {
                if ( row < 0 || row >= (int)m_mapListToCatalog.size() )
                    return -1;
                else
                    return m_mapListToCatalog[row];
            }

            int RowFromCatalogIndex(int index) const
            {
                if ( index < 0 || index >= (int)m_mapCatalogToList.size() )
                    return -1;
                else
                    return m_mapCatalogToList[index];
            }

            void CreateSortMap();

            void Freeze() { m_frozen = true; }
            void Thaw() { m_frozen = false; }

        public:
            CatalogPtr m_catalog;
            SortOrder sortOrder;

        private:
            bool m_frozen;
            std::vector<int> m_mapListToCatalog;
            std::vector<int> m_mapCatalogToList;

            TextDirection m_sourceTextDir, m_transTextDir, m_appTextDir;

            wxColour m_clrID, m_clrInvalid, m_clrFuzzy;
            wxString m_clrContextFg, m_clrContextBg;
            wxBitmap m_iconComment, m_iconBookmark, m_iconError, m_iconWarning;

        #if defined(__WXGTK__) && !wxCHECK_VERSION(3,0,3)
            #define HAS_BROKEN_NULL_BITMAPS
            wxBitmap m_nullBitmap;
        #endif
        };


        void UpdateHeaderAttrs();
        void CreateColumns();
        void OnSize(wxSizeEvent& event);

        bool m_displayIDs;
        TextDirection m_appTextDir;

        wxDataViewColumn *m_colID, *m_colIcon, *m_colSource, *m_colTrans;

        CatalogPtr m_catalog;
        wxObjectDataPtr<Model> m_model;
};

#endif // Poedit_edlistctrl_h
