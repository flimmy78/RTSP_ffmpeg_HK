#pragma once
#include "VcaCommonOperations.h"

// CDlgUploadHd �Ի���
#define MAX_BUFFER_LEN   (32*1024)

class CDlgUploadHd : public CDialogEx, public CVcaCommonOperations
{
	DECLARE_DYNAMIC(CDlgUploadHd)

public:
	CDlgUploadHd(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CDlgUploadHd();

// �Ի�������
	enum { IDD = IDD_DLG_UPLOAD_HD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
    static DWORD WINAPI UpLoadSendThread(LPVOID pParam);
	DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    CString m_csFilePath;
    LONG    m_lUploadHandle;
    BOOL    m_bUpLoading;
    HANDLE	m_hUpLoadThread;
    afx_msg void OnBnClickedBtnFilePath();
    afx_msg void OnBnClickedBtnFileUpload();
    CRect   m_rcPlayWnd;
    afx_msg void OnBnClickedBtnSnap();
};
