
// LuguodeClientDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

#include <boost/thread.hpp>
#include "../rpc/rpc.hpp"
#define DRI_IS_CLIENT
#include "../LuguodeServer/LuguodeInterface.h"
#include <queue>


#define MY_WM_NEW_MESSAGE (WM_USER + 1125)
// CLuguodeClientDlg 对话框
class CLuguodeClientDlg : public CDialog
{
// 构造
public:
	CLuguodeClientDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_LUGUODECLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
	//路过的 的函数
	void notify( const std::wstring& msg );
	void talk( const std::wstring& msg );
	void stranger_come_in();
	void on_connected( const boost::system::error_code& err, const boost::shared_ptr<LuguodeInterface>& conn );
	void on_connection_closed();
	void close_on_error( const rpc::remote_call_error& err );

	boost::mutex mtx;
	bool isTalking;
	boost::shared_ptr<LuguodeInterface> conn;
	boost::shared_ptr<boost::thread> networkThread;
	std::queue<std::wstring> unshownMessages;

	void AppendToRichEdit( const std::wstring& msg, int extraFlags = SF_UNICODE );
	void NetworkThread();

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnNewMessage( WPARAM, LPARAM );
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_Host;
	CEdit m_Port;
	CEdit m_Words;
	CRichEditCtrl m_Message;
	CButton m_SendOrConnect;
	afx_msg void OnBnClickedButtonSendOrConnect();
};
