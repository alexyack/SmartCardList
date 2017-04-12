// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SMARTCARDLIST_MAINDLG_H__
#define __SMARTCARDLIST_MAINDLG_H__

#include <atlddx.h>
#include <atlctrls.h>
#include <atlddx.h>
#include <atlsync.h>

class CMainDlg : public CDialogImpl<CMainDlg>, 
		public CDialogResize<CMainDlg>, 
		public CWinDataExchange<CMainDlg>
{
public:
	enum { IDD = IDD_MAINDLG };

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		COMMAND_ID_HANDLER(IDC_BUTTON_REFRESH, OnRefresh)
		COMMAND_ID_HANDLER(IDC_BUTTON_COPY_ALL, OnCopyAll)
		COMMAND_ID_HANDLER(ID_COPYREADERNAME, OnCopyReader)
		COMMAND_ID_HANDLER(ID_COPYATR, OnCopyATR)
		NOTIFY_HANDLER(IDC_MAIN_LIST, NM_RCLICK, OnNMRclickMainList)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		CHAIN_MSG_MAP(CDialogResize<CMainDlg>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CMainDlg)
		DLGRESIZE_CONTROL(IDC_MAIN_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_BUTTON_COPY_ALL, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BUTTON_REFRESH, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	BEGIN_DDX_MAP(CMainDlg)
		DDX_CONTROL_HANDLE(IDC_MAIN_LIST, m_ctlList)
	END_DDX_MAP()

	CListViewCtrl m_ctlList;
	int m_nActiveRow;

	SCARDCONTEXT m_hSC;
	DWORD m_cReaderStates;
	SCARD_READERSTATE m_rgReaderStates[64];

	HANDLE m_hThread; 
	ATL::CEvent m_hEvent;

	CMainDlg();

	static DWORD WINAPI _TheadProc(LPVOID lpParameter);
	DWORD TheadProc();

	void CopyToClipboard(LPCTSTR szText);

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCopyAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCopyReader(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCopyATR(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void CloseDialog(int nVal);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void DoRefresh();

	LRESULT OnNMRclickMainList(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
};

#endif // __SMARTCARDLIST_MAINDLG_H__
