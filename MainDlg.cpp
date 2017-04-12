// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"

#define STRSAFE_NO_DEPRECATE
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

#pragma comment(lib,"winscard.lib")

CMainDlg::CMainDlg() : m_hEvent(TRUE, FALSE)
{
	m_hSC = NULL; 
	m_hThread = NULL;
	m_nActiveRow = -1;

	m_cReaderStates = 64;
	memset(m_rgReaderStates, 0, sizeof(SCARD_READERSTATE) * 64);

	m_rgReaderStates[63].szReader = TEXT("\\\\?PnP?\\Notification");
}

DWORD WINAPI CMainDlg::_TheadProc(LPVOID lpParameter)
{
	DWORD dwRet = ERROR_SUCCESS;
	CMainDlg* pThis = static_cast<CMainDlg*>(lpParameter);

	if(lpParameter)
	{
		dwRet = pThis->TheadProc();
	}

	return dwRet;
}

DWORD CMainDlg::TheadProc()
{
	LRESULT lRet;
	DWORD nReaderIndex, nReaderCount;

	while(WAIT_TIMEOUT == ::WaitForSingleObject(m_hEvent, 0))
	{
		lRet = ::SCardGetStatusChange(m_hSC, 8000, m_rgReaderStates, m_cReaderStates);

		if(ERROR_SUCCESS == lRet)
		{
			nReaderCount = m_cReaderStates;

			for(nReaderIndex = 0; nReaderIndex < nReaderCount; nReaderIndex++)
			{
				if(m_rgReaderStates[nReaderIndex].dwEventState & SCARD_STATE_CHANGED)
				{
					::SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_BUTTON_REFRESH, BN_CLICKED), 0);
					break;
				}
			}
		}
	}

	return ERROR_SUCCESS;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	LONG lResunt;

	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);

	DoDataExchange();
	DlgResize_Init();

	m_ctlList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	m_ctlList.InsertColumn(0, TEXT("Считыватель"), LVCFMT_LEFT, 175);
	m_ctlList.InsertColumn(1, TEXT("Статус"), LVCFMT_LEFT, 75);
	m_ctlList.InsertColumn(2, TEXT("ATR"), LVCFMT_LEFT, 300);
	m_ctlList.InsertColumn(3, TEXT("Hint"),  LVCFMT_LEFT, 200);

	lResunt = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &m_hSC);

	if(SCARD_S_SUCCESS == lResunt)
	{
		DoRefresh();

		SetTimer(1, 8000);

		::EnableWindow(GetDlgItem(IDC_BUTTON_COPY_ALL), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON_REFRESH), TRUE);
		::EnableWindow(GetDlgItem(IDC_MAIN_LIST), TRUE);
	}

	return TRUE;
}

LRESULT CMainDlg::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	DoRefresh();

	return 0;
}

LRESULT CMainDlg::OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	DoRefresh();

	return 0;
}

LRESULT CMainDlg::OnCopyAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	WTL::CString strData, strItem, strAtr, strReader;
	int nIndex, nCount;

	nCount = m_ctlList.GetItemCount();

	for(nIndex = 0; nIndex < nCount; nIndex++)
	{
		m_ctlList.GetItemText(nIndex, 0, strReader);
		m_ctlList.GetItemText(nIndex, 2, strAtr);

		strItem.Format(TEXT("%s\r\n%s\r\n\r\n"), strReader, strAtr);

		strData += strItem;
	}

	CopyToClipboard(strData.LockBuffer());

	strData.UnlockBuffer();

	return 0;
}

void CMainDlg::DoRefresh()
{
	SCARDHANDLE hCard;
	SCARD_READERSTATE sReaderState[1];

	LONG lResunt;
	TCHAR szReaders[8192], szReaderNames[1024], szATRString[80], szATRChar[8];
	LPTSTR szReader;
	DWORD cchReaders, nIndex, nCount;
	DWORD dwProtocol, nATRIndex;
	int nIndexOld;
	WTL::CString strData;
	
	nIndexOld = m_ctlList.GetSelectedIndex();
	m_ctlList.GetItemText(nIndexOld, 0, strData);
	
	m_ctlList.DeleteAllItems();

	if(m_hSC)
	{
		cchReaders = 8192;

		lResunt = SCardListReaders(m_hSC, NULL, szReaders, &cchReaders);
		if(SCARD_S_SUCCESS == lResunt)
		{
			nCount = 0;
			nIndex = 0;
			while(szReaders[nIndex] != 0) // "\\\\?PnP?\\Notification" 
			{
				szReader = szReaders + nIndex;
				nIndex += _tcslen(szReader) + 1;

				m_ctlList.InsertItem(nCount, szReader);

				if(strData.Compare(szReader) == 0)
					nIndexOld = nCount;

				hCard = 0;
				dwProtocol = 0;

				memset(sReaderState, 0, sizeof(SCARD_READERSTATE));
				sReaderState[0].szReader = szReader;
				sReaderState[0].dwCurrentState = SCARD_STATE_UNAWARE;

				lResunt = SCardGetStatusChange(m_hSC, 0, sReaderState, 1);

				if(lResunt == SCARD_S_SUCCESS) // SCARD_E_TIMEOUT
				{
					StringCchPrintf(szReaderNames, 1024, TEXT("%04X %04X"), HIWORD(sReaderState[0].dwEventState), LOWORD(sReaderState[0].dwEventState));
					m_ctlList.AddItem(nCount, 1, szReaderNames);

					// if((sReaderState[0].dwEventState & SCARD_STATE_PRESENT))
					{
						szATRString[0] = 0;
						for(nATRIndex = 0; nATRIndex < sReaderState[0].cbAtr; nATRIndex++)
						{
							StringCchPrintf(szATRChar, 8, TEXT("%02X"), sReaderState[0].rgbAtr[nATRIndex]);
							StringCchCat(szATRString, 80, szATRChar);
						}

						m_ctlList.AddItem(nCount, 2, szATRString);
					}

					StringCchCopy(szReaderNames, 1024, TEXT(""));

// 					if((sReaderState[0].dwEventState & SCARD_STATE_IGNORE))
// 						StringCchCat(szReaderNames, 1024, TEXT("IGNORE "));
// 
// 					if((sReaderState[0].dwEventState & SCARD_STATE_CHANGED))
// 						StringCchCat(szReaderNames, 1024, TEXT("CHANGED "));
// 
// 					if((sReaderState[0].dwEventState & SCARD_STATE_UNKNOWN))
// 						StringCchCat(szReaderNames, 1024, TEXT("UNKNOWN "));

					if((sReaderState[0].dwEventState & SCARD_STATE_UNAVAILABLE))
						StringCchCat(szReaderNames, 1024, TEXT("UNAVAILABLE "));

					if((sReaderState[0].dwEventState & SCARD_STATE_EMPTY))
						StringCchCat(szReaderNames, 1024, TEXT("EMPTY "));

					if((sReaderState[0].dwEventState & SCARD_STATE_PRESENT))
						StringCchCat(szReaderNames, 1024, TEXT("PRESENT "));

					if((sReaderState[0].dwEventState & SCARD_STATE_ATRMATCH))
						StringCchCat(szReaderNames, 1024, TEXT("ATRMATCH "));

					if((sReaderState[0].dwEventState & SCARD_STATE_EXCLUSIVE))
						StringCchCat(szReaderNames, 1024, TEXT("EXCLUSIVE "));

					if((sReaderState[0].dwEventState & SCARD_STATE_INUSE))
						StringCchCat(szReaderNames, 1024, TEXT("INUSE "));

					if((sReaderState[0].dwEventState & SCARD_STATE_MUTE))
						StringCchCat(szReaderNames, 1024, TEXT("MUTE "));

					if((sReaderState[0].dwEventState & SCARD_STATE_UNPOWERED))
						StringCchCat(szReaderNames, 1024, TEXT("UNPOWERED "));

					StrTrim(szReaderNames, TEXT(" "));
					m_ctlList.AddItem(nCount, 3, szReaderNames);
				}

				nCount++;
			}
		}

	}

	m_ctlList.SelectItem(nIndexOld);
}

void CMainDlg::CloseDialog(int nVal)
{
	m_hEvent.Set();

	if(WAIT_TIMEOUT == WaitForSingleObject(m_hThread, 10000))
		::TerminateThread(m_hThread, ERROR_TIMEOUT);

	SCardReleaseContext(m_hSC);

	DestroyWindow();
	::PostQuitMessage(nVal);
}

LRESULT CMainDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// TODO: добавьте свой код обработчика сообщений или вызов стандартного
	CloseDialog(IDCANCEL);
	return 0;
}


LRESULT CMainDlg::OnNMRclickMainList(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	// TODO: Add your control notification handler code here
	CMenu menu1, menu2;
	LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)pNMHDR;

	m_nActiveRow = lpnmitem->iItem;

	if(m_nActiveRow >= 0)
	{
		m_ctlList.ClientToScreen(&lpnmitem->ptAction);
	
		menu1.LoadMenu(IDR_CONTEXT_MENU);
		
		menu2 = menu1.GetSubMenu(0);
		menu2.TrackPopupMenu(0, lpnmitem->ptAction.x, lpnmitem->ptAction.y, m_hWnd);
	}

	return 0;
}

LRESULT CMainDlg::OnCopyReader(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	WTL::CString strData;

	m_ctlList.GetItemText(m_nActiveRow, 0, strData);

	CopyToClipboard(strData.LockBuffer());

	strData.UnlockBuffer();

	return 0;
}

LRESULT CMainDlg::OnCopyATR(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	WTL::CString strData;

	m_ctlList.GetItemText(m_nActiveRow, 2, strData);

	CopyToClipboard(strData.LockBuffer());

	strData.UnlockBuffer();

	return 0;
}

void CMainDlg::CopyToClipboard(LPCTSTR szText)
{
	HGLOBAL hMem;
	LPTSTR szData;
	size_t nSize;

	if(OpenClipboard())
	{
		EmptyClipboard();
		
		nSize = (_tcslen(szText) + 1) * sizeof(TCHAR);
		hMem = GlobalAlloc(GMEM_MOVEABLE, nSize); 

		if(hMem)
		{
			szData = LPTSTR(GlobalLock(hMem));
			StringCchCopy(szData, nSize, szText);
			GlobalUnlock(hMem);

			SetClipboardData(CF_UNICODETEXT, hMem);
		}

		CloseClipboard();
	}

}
