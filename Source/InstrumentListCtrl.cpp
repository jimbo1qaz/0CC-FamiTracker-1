/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.  To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include "InstrumentListCtrl.h"		// // //
#include "../resource.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include "InstrumentManager.h"		// // //
#include "MainFrm.h"
#include "FamiTrackerEnv.h"		// // //
#include "Instrument.h"		// // //
#include "NumConv.h"	// // //
#include "str_conv/str_conv.hpp"		// // //

///
/// CInstrumentListCtrl
///

// This class takes care of handling the instrument list, since mapping
// between instruments list and instruments are not necessarily 1:1

IMPLEMENT_DYNAMIC(CInstrumentListCtrl, CListCtrl)

BEGIN_MESSAGE_MAP(CInstrumentListCtrl, CListCtrl)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_BEGINLABELEDIT, &CInstrumentListCtrl::OnLvnBeginlabeledit)
	ON_NOTIFY_REFLECT(NM_CLICK, &CInstrumentListCtrl::OnNMClick)
	ON_NOTIFY_REFLECT(LVN_KEYDOWN, &CInstrumentListCtrl::OnLvnKeydown)
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, &CInstrumentListCtrl::OnLvnEndlabeledit)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CInstrumentListCtrl::OnLvnItemchanged)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CInstrumentListCtrl::OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, &CInstrumentListCtrl::OnLvnBegindrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

CInstrumentListCtrl::CInstrumentListCtrl(CMainFrame *pMainFrame) :
	m_pMainFrame(pMainFrame),
	m_nDragIndex(-1),
	m_nDropIndex(-1),
	m_bDragging(false)
{
}

void CInstrumentListCtrl::CreateImageList() {
	m_pImageList = std::make_unique<CImageList>();
	m_pImageList->Create(16, 16, ILC_COLOR32, 1, 1);
	m_pImageList->Add(FTEnv.GetMainApp()->LoadIconW(IDI_INST_2A03));
	m_pImageList->Add(FTEnv.GetMainApp()->LoadIconW(IDI_INST_VRC6));
	m_pImageList->Add(FTEnv.GetMainApp()->LoadIconW(IDI_INST_VRC7));
	m_pImageList->Add(FTEnv.GetMainApp()->LoadIconW(IDI_INST_FDS));
	m_pImageList->Add(FTEnv.GetMainApp()->LoadIconW(IDI_INST_N163));
	m_pImageList->Add(FTEnv.GetMainApp()->LoadIconW(IDI_INST_S5B));		// // //

	SetImageList(m_pImageList.get(), LVSIL_NORMAL);
	SetImageList(m_pImageList.get(), LVSIL_SMALL);
}

int CInstrumentListCtrl::GetInstrumentIndex(int Selection) const
{
	// Get the instrument number from an item in the list (Selection = list index)
	if (Selection >= 0 && Selection < GetItemCount())		// // //
		return GetItemData(Selection);
	return INVALID_INSTRUMENT;
}

int CInstrumentListCtrl::FindInstrument(int Index) const
{
	// Find the instrument item from the list (Index = instrument number)
	CStringW instname = FormattedW(L"%02X", Index);
	LVFINDINFOW info;
	info.flags = LVFI_PARTIAL | LVFI_STRING;
	info.psz = (LPCWSTR)instname;

	return FindItem(&info);
}

void CInstrumentListCtrl::SelectInstrument(int Index)
{
	// Highlight a specified instrument (Index = instrument number)
	int ListIndex = FindInstrument(Index);
	SetSelectionMark(ListIndex);
	SetItemState(ListIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	EnsureVisible(ListIndex, FALSE);
}

void CInstrumentListCtrl::SelectNextItem()
{
	// Select next instrument
	int SelIndex = GetSelectionMark();
	int Count = GetItemCount();
	if (SelIndex < (Count - 1)) {
		int Slot = GetInstrumentIndex(SelIndex + 1);
		m_pMainFrame->SelectInstrument(Slot);
	}
}

void CInstrumentListCtrl::SelectPreviousItem()
{
	// Select previous instrument
	int SelIndex = GetSelectionMark();
	if (SelIndex > 0) {
		int Slot = GetInstrumentIndex(SelIndex - 1);
		m_pMainFrame->SelectInstrument(Slot);
	}
}

void CInstrumentListCtrl::InsertInstrument(int Index)
{
	// Inserts an instrument in the list (Index = instrument number)
	auto *pManager = CFamiTrackerDoc::GetDoc()->GetModule()->GetInstrumentManager();		// // //

	if (auto pInst = pManager->GetInstrument(Index)) {		// // //
		int Type = pInst->GetType();

		// Name is of type index - name
		auto sv = conv::to_wide(pInst->GetName());
		InsertItem(Index, FormattedW(L"%02X - %.*s", Index, sv.size(), sv.data()), Type - 1);
		SetItemData(FindInstrument(Index), Index);		// // //
		SelectInstrument(Index);		// // //
	}
}

void CInstrumentListCtrl::RemoveInstrument(int Index)
{
	// Remove an instrument from the list (Index = instrument number)
	int Selection = FindInstrument(Index);
	if (Selection != -1)
		DeleteItem(Selection);
}

void CInstrumentListCtrl::SetInstrumentName(int Index, LPCWSTR pName)		// // //
{
	// Update instrument name in the list
	SetItemText(GetSelectionMark(), 0, FormattedW(L"%02X - %s", Index, pName));
}

void CInstrumentListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	int Instrument = GetInstrumentIndex(GetSelectionMark());

	// Select the instrument
	if (Instrument != -1)
		m_pMainFrame->SelectInstrument(Instrument);

	// Display the popup menu
	CMenu *pPopupMenu, PopupMenuBar;
	PopupMenuBar.LoadMenuW(IDR_INSTRUMENT_POPUP);
	pPopupMenu = PopupMenuBar.GetSubMenu(0);
	// Route the menu messages to mainframe
	pPopupMenu->TrackPopupMenu(TPM_LEFTBUTTON, point.x, point.y, m_pMainFrame);

	// Return focus to pattern editor
	m_pMainFrame->GetActiveView()->SetFocus();
}

void CInstrumentListCtrl::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
//	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	// Selection changed
//	if (pNMLV->uNewState & LVIS_SELECTED)
//		m_pMainFrame->SelectInstrument(GetInstrumentIndex(pNMLV->iItem));

	*pResult = 0;
}

void CInstrumentListCtrl::OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
//	LPNMLVDISPINFOW pDispInfo = reinterpret_cast<LPNMLVDISPINFOW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CInstrumentListCtrl::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
//	LPNMLVDISPINFOW pDispInfo = reinterpret_cast<LPNMLVDISPINFOW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CInstrumentListCtrl::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// Select instrument
	m_pMainFrame->SelectInstrument(GetInstrumentIndex(pNMItemActivate->iItem));

	// Move focus to pattern editor
	m_pMainFrame->GetActiveView()->SetFocus();

	*pResult = 0;
}

void CInstrumentListCtrl::OnLvnKeydown(NMHDR *pNMHDR, LRESULT *pResult)
{
	// Empty

	*pResult = 0;
}

void CInstrumentListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	// Double-click = instrument editor
	m_pMainFrame->OpenInstrumentEditor();

	*pResult = 0;
}

void CInstrumentListCtrl::OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	// Begin drag operation
	m_bDragging = true;
	m_nDragIndex = pNMLV->iItem;
	m_nDropIndex = -1;

	// Create a drag image
	POINT pt;
	int nOffset = 10;
	pt.x = nOffset;
	pt.y = nOffset;

	m_pDragImage.reset(CreateDragImage(m_nDragIndex, &pt));		// // // Delete this later
	ASSERT(m_pDragImage);

	m_pDragImage->BeginDrag(0, CPoint(nOffset, nOffset));
	CImageList::DragEnter(this, pNMLV->ptAction);

	// Capture all mouse messages
	SetCapture();

	*pResult = 0;
}

void CInstrumentListCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	// Handle drag operation
	if (m_bDragging) {
		// Move the drag image
		CImageList::DragMove(point);
		CImageList::DragShowNolock(false);

		// Turn off hilight for previous drop target
		SetItemState(m_nDropIndex, 0, LVIS_DROPHILITED);
		// Redraw previous item
		RedrawItems(m_nDropIndex, m_nDropIndex);

		// Get drop index
		m_nDropIndex = HitTest(point);

		// Highlight drop index
		if (m_nDropIndex != -1) {
			SetItemState(m_nDropIndex, LVIS_DROPHILITED, LVIS_DROPHILITED);
			RedrawItems(m_nDropIndex, m_nDropIndex);
			UpdateWindow();
		}

		CImageList::DragShowNolock(true);
	}

	CListCtrl::OnMouseMove(nFlags, point);
}

void CInstrumentListCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// End a drag operation
	if (m_bDragging) {
		ReleaseCapture();
		m_bDragging = false;

		CImageList::DragLeave(this);
		CImageList::EndDrag();
		m_pDragImage.reset();		// // //

		// Remove highlight
		SetItemState(-1, 0, LVIS_DROPHILITED);
		RedrawItems(-1, -1);
		UpdateWindow();

		if (m_nDropIndex != -1 && m_nDragIndex != m_nDropIndex) {
			// Perform operation
			int First = GetInstrumentIndex(m_nDragIndex);
			int Second = GetInstrumentIndex(m_nDropIndex);
			m_pMainFrame->SwapInstruments(First, Second);
		}
	}

	CListCtrl::OnLButtonUp(nFlags, point);
}
