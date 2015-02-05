
// socket_testDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "socket_test.h"
#include "socket_testDlg.h"
#include "afxdialogex.h"
#include <boost/lexical_cast.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Csocket_testDlg 对话框




Csocket_testDlg::Csocket_testDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Csocket_testDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Csocket_testDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT4, _outputEdit);
	DDX_Control(pDX, IDC_EDIT5, _msgEdit);
	DDX_Control(pDX, IDC_IPADDRESS1, _clientIpEdit);
	DDX_Control(pDX, IDC_EDIT2, _clientPortEdit);
	DDX_Control(pDX, IDC_EDIT3, _clientTimeoutEdit);
	DDX_Control(pDX, IDC_EDIT6, _serverPortEdit);
	DDX_Control(pDX, IDC_EDIT7, _maxSessionEdit);
	DDX_Control(pDX, IDC_EDIT8, _extSessNumEdit);
}

BEGIN_MESSAGE_MAP(Csocket_testDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_QUERYDRAGICON()
	REGIEST_MODAL_RUN(Csocket_testDlg)
	ON_BN_CLICKED(IDC_BUTTON1, &Csocket_testDlg::OnBnClickedClientConnect)
	ON_BN_CLICKED(IDC_BUTTON2, &Csocket_testDlg::OnBnClickedClientDisconnect)
	ON_BN_CLICKED(IDC_BUTTON3, &Csocket_testDlg::OnBnClickedSendClientMsg)
	ON_BN_CLICKED(IDC_BUTTON4, &Csocket_testDlg::OnBnClickedListen)
	ON_BN_CLICKED(IDC_BUTTON5, &Csocket_testDlg::OnBnClickedStopLst)
	ON_BN_CLICKED(IDC_BUTTON6, &Csocket_testDlg::OnBnClickedClear)
END_MESSAGE_MAP()


// Csocket_testDlg 消息处理程序

BOOL Csocket_testDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	_editFont.CreatePointFont(90, "宋体");
	_clientIpEdit.SetFont(&_editFont);
	_clientPortEdit.SetFont(&_editFont);
	_clientTimeoutEdit.SetFont(&_editFont);
	_serverPortEdit.SetFont(&_editFont);
	_msgEdit.SetFont(&_editFont);
	_outputEdit.SetFont(&_editFont);
	_maxSessionEdit.SetFont(&_editFont);
	_extSessNumEdit.SetFont(&_editFont);

	_clientIpEdit.SetAddress(127, 0, 0, 1);
	_clientPortEdit.SetWindowText("1000");
	_clientTimeoutEdit.SetWindowText("500");
	_serverPortEdit.SetWindowText("1000");
	_maxSessionEdit.SetWindowText("3");

	boost_coro::enable_stack_pool();
	timeout_trig::enable_high_resolution();
	boost_coro::disable_auto_make_timer();
	_ios.run();
	_strand = boost_strand::create(_ios);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void Csocket_testDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void Csocket_testDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR Csocket_testDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Csocket_testDlg::showClientMsg(shared_data msg)
{
	int nLength = _outputEdit.GetWindowTextLength();
	_outputEdit.SetSel(nLength, nLength);
	_outputEdit.ReplaceSel(msg->c_str());
	_outputEdit.ReplaceSel("\n");
}

void Csocket_testDlg::showSessionNum(int n)
{
	_extSessNumEdit.SetWindowText(boost::lexical_cast<string>(n).c_str());
}

void Csocket_testDlg::OnDestroy()
{
	OnBnClickedClientDisconnect();
	OnBnClickedStopLst();
	_ios.stop();
	CLEAR_MSG();
	CDialogEx::OnDestroy();
}

void Csocket_testDlg::connectCoro(boost_coro* coro, boost::shared_ptr<client_param> param)
{
	async_trig_handle<boost::system::error_code> ath;
	post(boost::bind(&Csocket_testDlg::showClientMsg, this, msg_data::create("连接中...")));
	param->_clientSocket->async_connect(param->_ip.c_str(), param->_port, coro->begin_trig(ath));
	coro->open_timer();
	boost::system::error_code err;
	if (coro->wait_trig(ath, err, param->_tm) && !err && param->_clientSocket->no_delay())
	{
		post(boost::bind(&Csocket_testDlg::showClientMsg, this, msg_data::create("连接成功")));
		coro_msg_handle<shared_data> cmh;
		child_coro_handle readCoro = coro->create_child_coro([this, &cmh](boost_coro* coro)
		{
			while (true)
			{
				//接收服务器的消息，然后发送给对话框
				shared_data msg = coro->pump_msg(cmh);
				if (!msg)
				{
					break;
				}
				post(boost::bind(&Csocket_testDlg::showClientMsg, this, msg));
			}
			coro->close_msg_notify(cmh);
		});
		auto textio = text_stream_io::create(coro->this_strand(), param->_clientSocket, readCoro.get_coro()->make_msg_notify(cmh));
		child_coro_handle writerCoro = coro->create_child_coro([&textio, &param](boost_coro* coro)
		{
			coro_msg_handle<shared_data> cmh;
			param->_msgPump(coro, cmh);
			while (true)
			{
				//接收对话框给的消息，然后发送给服务器
				textio->write(coro->pump_msg(cmh));
			}
			coro->close_msg_notify(cmh);
		});
		coro->child_coro_run(readCoro);
		coro->child_coro_run(writerCoro);
		coro->child_coro_wait_quit(readCoro);
		coro->child_coro_quit(writerCoro);
		textio->close();
		post(boost::bind(&Csocket_testDlg::showClientMsg, this, msg_data::create("断开连接")));
	} 
	else
	{
		post(boost::bind(&Csocket_testDlg::showClientMsg, this, msg_data::create("连接失败")));
		param->_clientSocket->close();
	}
	post(boost::bind(&Csocket_testDlg::OnBnClickedClientDisconnect, this));
}

void Csocket_testDlg::OnBnClickedClientConnect()
{
	OnBnClickedClientDisconnect();
	BYTE bip[4];
	_clientIpEdit.GetAddress(bip[0], bip[1], bip[2], bip[3]);
	char sip[32];
	sprintf_s(sip, "%d.%d.%d.%d", bip[0], bip[1], bip[2], bip[3]);
	CString sport;
	_clientPortEdit.GetWindowText(sport);
	CString stm;
	_clientTimeoutEdit.GetWindowText(stm);
	try
	{
		_extClient = boost::shared_ptr<client_param>(new client_param);
		_extClient->_ip = sip;
		_extClient->_port = boost::lexical_cast<int>(sport.GetBuffer());
		_extClient->_tm = boost::lexical_cast<int>(stm.GetBuffer());
		_extClient->_clientSocket = socket_io::create(_ios);
		_extClient->_msgPump = msg_pipe<shared_data>::make(_clientPostPipe);
		_clientCoro = boost_coro::create(_strand, 
			boost::bind(&Csocket_testDlg::connectCoro, this, _1, _extClient));
		_clientCoro->notify_start_run();
		GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON2)->EnableWindow(TRUE);
	}
	catch (...)
	{
		_extClient.reset();
		_clientCoro.reset();
	}
}


void Csocket_testDlg::OnBnClickedClientDisconnect()
{
	if (_clientCoro)
	{
		_extClient->_clientSocket->close();
		_clientCoro->outside_wait_quit();
		_extClient.reset();
		_clientCoro.reset();
		GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
	}
}

void Csocket_testDlg::newSession(boost::shared_ptr<socket_io> socket, msg_pipe<>::regist_reader closePump, boost::function<void ()> cb)
{
	dlg_session dlg;
	dlg._strand = _strand;
	dlg._socket = socket;
	dlg._lstClose = closePump;
	dlg.DoModal();
	cb();
}

void Csocket_testDlg::sessionMng(boost_coro* coro, 
	const msg_pipe<list<boost::shared_ptr<session_pck> >::iterator>::regist_reader& sessClose)
{
	coro_msg_handle<list<boost::shared_ptr<session_pck> >::iterator> cmh;
	sessClose(coro, cmh);
	while (true)
	{
		auto it = coro->pump_msg(cmh);
		//得到一个会话关闭消息，先等待对话框线程关闭，再从列表中删除
		(*it)->_thread.join();
		(*it)->_socket->close();
		_sessList.erase(it);
		post(boost::bind(&Csocket_testDlg::showSessionNum, this, _sessList.size()));
	}
	coro->close_msg_notify(cmh);
}

void Csocket_testDlg::lstClose(boost_coro* coro, boost::shared_ptr<acceptor_socket> accept, const msg_pipe<>::regist_reader& lst)
{
	coro_msg_handle<> cmh;
	lst(coro, cmh);
	coro->pump_msg(cmh);
	//得到服务器关闭消息，关闭连接侦听器
	accept->close();
	coro->close_msg_notify(cmh);
}

void Csocket_testDlg::serverCoro(boost_coro* coro, boost::shared_ptr<server_param> param)
{
	coro_msg_handle<boost::shared_ptr<socket_io> > cmh;
	auto accept = acceptor_socket::create(coro->this_strand(), param->_port, coro->make_msg_notify(cmh), false);//创建连接侦听器
	if (!accept)
	{
		coro->close_msg_notify(cmh);
		post(boost::bind(&Csocket_testDlg::OnBnClickedStopLst, this));
		return;
	}
	msg_pipe<list<boost::shared_ptr<session_pck> >::iterator>::writer_type sessDisconnNotify;
	//创建侦听服务器关闭
	child_coro_handle lstCloseProxyCoro = coro->create_child_coro(boost::bind(&Csocket_testDlg::lstClose, this, _1, accept, boost::ref(param->_closePump)));//侦听服务器关闭
	//创建会话关闭响应器
	child_coro_handle sessMngCoro = coro->create_child_coro(boost::bind(&Csocket_testDlg::sessionMng, this, _1,
		msg_pipe<list<boost::shared_ptr<session_pck> >::iterator>::make(sessDisconnNotify)));
	coro->child_coro_run(lstCloseProxyCoro);
	coro->child_coro_run(sessMngCoro);
	while (true)
	{
		auto newSocket = coro->pump_msg(cmh);
		if (!newSocket)
		{
			break;
		}
		if (_sessList.size() < (size_t)param->_maxSessionNum && newSocket->no_delay())
		{
			boost::shared_ptr<session_pck> newSess(new session_pck);
			_sessList.push_front(newSess);
			post(boost::bind(&Csocket_testDlg::showSessionNum, this, _sessList.size()));
			newSess->_socket = newSocket;
			//创建一个单独线程运行对话框
			newSess->_thread.swap(boost::thread(boost::bind(&Csocket_testDlg::newSession, this, newSocket,
				msg_pipe<>::make(newSess->_closeNtf),//外部通知对话框关闭
				(boost::function<void ()>)boost::bind(sessDisconnNotify, _sessList.begin()))));
		}
		else
		{
			newSocket->close();
		}
	}
	coro->child_coro_quit(lstCloseProxyCoro);
	coro->child_coro_quit(sessMngCoro);
	coro->close_msg_notify(cmh);
	//通知所有存在的对话框关闭
	for (auto it = _sessList.begin(); it != _sessList.end(); it++)
	{
		(*it)->_closeNtf();
	}
}

void Csocket_testDlg::OnBnClickedSendClientMsg()
{
	if (_clientCoro)
	{
		CString cs;
		_msgEdit.GetWindowText(cs);
		if (cs.GetLength())
		{
			_msgEdit.SetWindowText("");
			_clientPostPipe(msg_data::create(cs.GetBuffer()));
		}
	}
	_msgEdit.SetFocus();
}


void Csocket_testDlg::OnBnClickedClear()
{
	_outputEdit.SetWindowText("");
}


void Csocket_testDlg::OnBnClickedListen()
{
	OnBnClickedStopLst();
	CString sport;
	CString snum;
	_serverPortEdit.GetWindowText(sport);
	_maxSessionEdit.GetWindowText(snum);
	try
	{
		boost::shared_ptr<server_param> param(new server_param);
		param->_port = boost::lexical_cast<int>(sport.GetBuffer());
		param->_maxSessionNum = boost::lexical_cast<int>(snum.GetBuffer());
		param->_closePump = msg_pipe<>::make(_serverNtfClose);
		_serverCoro = boost_coro::create(_strand, boost::bind(&Csocket_testDlg::serverCoro, this, _1, param));
		_serverCoro->notify_start_run();
		_extSessNumEdit.SetWindowText("");
		GetDlgItem(IDC_BUTTON4)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON5)->EnableWindow(TRUE);
	}
	catch (...)
	{
		_serverCoro.reset();
	}
}


void Csocket_testDlg::OnBnClickedStopLst()
{
	if (_serverCoro)
	{
		_serverNtfClose();
		_serverCoro->outside_wait_quit();
		_serverCoro.reset();
		//等待对话框关闭完毕
		while (!_sessList.empty())
		{
			_sessList.front()->_thread.join();
			_sessList.pop_front();
		}
		_extSessNumEdit.SetWindowText("");
		GetDlgItem(IDC_BUTTON4)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON5)->EnableWindow(FALSE);
	}
}
