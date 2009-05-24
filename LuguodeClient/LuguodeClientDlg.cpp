
// LuguodeClientDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "LuguodeClient.h"
#include "LuguodeClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <sstream>
#include <limits>
using namespace std;
using namespace boost;
// CLuguodeClientDlg 对话框




CLuguodeClientDlg::CLuguodeClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLuguodeClientDlg::IDD, pParent), isTalking(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLuguodeClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_HOST, m_Host);
	DDX_Control(pDX, IDC_EDIT_PORT, m_Port);
	DDX_Control(pDX, IDC_EDIT_WORDS, m_Words);
	DDX_Control(pDX, IDC_RICHEDIT2_MESSAGE, m_Message);
	DDX_Control(pDX, IDC_BUTTON_SEND_OR_CONNECT, m_SendOrConnect);
}

BEGIN_MESSAGE_MAP(CLuguodeClientDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_SEND_OR_CONNECT, &CLuguodeClientDlg::OnBnClickedButtonSendOrConnect)
	ON_MESSAGE(MY_WM_NEW_MESSAGE, OnNewMessage)
END_MESSAGE_MAP()


// CLuguodeClientDlg 消息处理程序

BOOL CLuguodeClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	m_SendOrConnect.SetWindowText( L"连接" );
	m_Host.SetWindowText( L"localhost" );
	m_Port.SetWindowText( L"9125" );

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CLuguodeClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CLuguodeClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

DWORD CALLBACK MyStreamInCallback( DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb )
{
   wistringstream* pStrm = (wistringstream*) dwCookie;

   *pcb = sizeof(wchar_t) * pStrm->readsome( (wchar_t*)pbBuff, cb/(sizeof(wchar_t)) );

   return 0;
}

void CLuguodeClientDlg::AppendToRichEdit( const wstring& msg, int extraFlags )
{
	int size = m_Message.GetTextLength();
	m_Message.SetSel( size, size );
	
	EDITSTREAM es;

	wistringstream strm( msg );
	es.dwCookie = (DWORD_PTR) &strm;
	es.pfnCallback = &MyStreamInCallback;

	m_Message.StreamIn( SF_TEXT|SFF_SELECTION|extraFlags, es );
}

void CLuguodeClientDlg::notify( const wstring& msg )
{
	mutex::scoped_lock lck(mtx);
	unshownMessages.push( msg + L"\r\n" );
	PostMessage( MY_WM_NEW_MESSAGE ) ;
}

void CLuguodeClientDlg::talk( const wstring& msg )
{
	mutex::scoped_lock lck(mtx);
	unshownMessages.push( L"路人甲：" + msg + L"\r\n" );
	PostMessage( MY_WM_NEW_MESSAGE ) ;
}

void CLuguodeClientDlg::stranger_come_in()
{
	isTalking = true;
	AppendToRichEdit( L"已经连接上了一位路人甲。先对 他/她 说个你好吧。\r\n" );
}

void CLuguodeClientDlg::NetworkThread()
{
	CString host, portStr;
	m_Host.GetWindowText( host );
	m_Port.GetWindowText( portStr );
	int port;
	wistringstream( (wstring)portStr ) >> port;
	LuguodeInterface::connect( (LPCSTR)CStringA(host), port, this );
}

void CLuguodeClientDlg::OnBnClickedButtonSendOrConnect()
{
	if ( conn )
	{
		CString words;
		m_Words.GetWindowText( words );
		m_Words.SetWindowText( L"" );
		conn->talk( (LPCTSTR)words, bind( &CLuguodeClientDlg::close_on_error, this, _1 ) );
		AppendToRichEdit( (LPCTSTR) (L"你：" + words + L"\r\n") );
		m_Message.SendMessage(WM_VSCROLL,SB_BOTTOM,0);
	}
	else
	{
		networkThread.reset( new thread( bind( &CLuguodeClientDlg::NetworkThread, this ) ) );
	}
}

void CLuguodeClientDlg::on_connected( const boost::system::error_code& err, const shared_ptr<LuguodeInterface>& conn_ )
{
	if (err)
	{
		AppendToRichEdit( L"连接失败：" );
		AppendToRichEdit( (LPCTSTR)CStringW(err.message().c_str()) );
		AppendToRichEdit( L"\r\n" );
	}
	else
	{
		conn = conn_;
		conn->on_connection_closed = bind( &CLuguodeClientDlg::on_connection_closed, this );
		AppendToRichEdit( L"已连接上服务器。\r\n" );
		m_SendOrConnect.SetWindowText( L"发送消息" );
	}
}

void CLuguodeClientDlg::on_connection_closed()
{
	isTalking = false;
	conn.reset();
	AppendToRichEdit( L"连接中断。\r\n" );
	m_SendOrConnect.SetWindowText( L"连接" );
}

void CLuguodeClientDlg::close_on_error( const rpc::remote_call_error& err )
{
	if ( err )
	{
		if ( err.code != rpc::remote_call_error::connection_error )
		{
			conn->close();
		}
		MessageBoxA( GetSafeHwnd(), err.what().c_str(), "出错", MB_OK|MB_ICONERROR );
	}
}

LRESULT CLuguodeClientDlg::OnNewMessage( WPARAM, LPARAM )
{
	mutex::scoped_lock lck(mtx);
	AppendToRichEdit( unshownMessages.front() );
	unshownMessages.pop();
	m_Message.SendMessage(WM_VSCROLL,SB_BOTTOM,0);
	return 0;
}