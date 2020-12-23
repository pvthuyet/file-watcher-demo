
// FileWatcherDemoDlg.h : header file
//

#pragma once
#include "directory_watcher_mgr.h"

// CFileWatcherDemoDlg dialog
class CFileWatcherDemoDlg : public CDialogEx
{
private:
	died::directory_watcher_mgr mWatcher;

// Construction
public:
	CFileWatcherDemoDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FILEWATCHERDEMO_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
};
