// socket_testDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "socket_test.h"
#include "socket_testDlg.h"
#include "afxdialogex.h"
#include "scattered.h"
#include <boost/lexical_cast.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Csocket_testDlg 对话框




Csocket_testDlg::Csocket_testDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Csocket_testDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

Csocket_testDlg::~Csocket_testDlg()
{
	_ios.stop();
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
	REGIEST_MFC_RUN(Csocket_testDlg)
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
	set_thread_id();
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
	GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON5)->EnableWindow(FALSE);

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
	GetDlgItem(IDC_BUTTON1)->SetFont(&_editFont);
	GetDlgItem(IDC_BUTTON2)->SetFont(&_editFont);
	GetDlgItem(IDC_BUTTON3)->SetFont(&_editFont);
	GetDlgItem(IDC_BUTTON4)->SetFont(&_editFont);
	GetDlgItem(IDC_BUTTON5)->SetFont(&_editFont);
	GetDlgItem(IDC_BUTTON6)->SetFont(&_editFont);

	_clientIpEdit.SetAddress(127, 0, 0, 1);
	_clientPortEdit.SetWindowText("1000");
	_clientTimeoutEdit.SetWindowText("500");
	_serverPortEdit.SetWindowText("1000");
	_maxSessionEdit.SetWindowText("3");

	my_actor::enable_stack_pool();
	my_actor::disable_auto_make_timer();
	_ios.run();
	_strand = boost_strand::create(_ios);
	actor_handle mainActor = my_actor::create(_strand, boost::bind(&Csocket_testDlg::mainActor, this, _1));
	_uiCMD = mainActor->connect_msg_notifer<ui_cmd>();
	mainActor->notify_run();
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

void Csocket_testDlg::OnCancel()
{
	_uiCMD(ui_close);
}

void Csocket_testDlg::OnBnClickedClientConnect()
{
	_uiCMD(ui_connect);
}

void Csocket_testDlg::OnBnClickedClientDisconnect()
{
	_uiCMD(ui_disconnect);
}

void Csocket_testDlg::OnBnClickedListen()
{
	_uiCMD(ui_listen);
}

void Csocket_testDlg::OnBnClickedStopLst()
{
	_uiCMD(ui_stopListen);
}

void Csocket_testDlg::OnBnClickedSendClientMsg()
{
	_uiCMD(ui_postMsg);
	_msgEdit.SetFocus();
}

void Csocket_testDlg::OnBnClickedClear()
{
	_outputEdit.SetWindowText("");
}

void Csocket_testDlg::connectActor(my_actor* self, std::shared_ptr<client_param> param)
{
	actor_trig_handle<boost::system::error_code> ath;
	post(boost::bind(&Csocket_testDlg::showClientMsg, this, msg_data::create("连接中...")));
	param->_clientSocket->async_connect(param->_ip.c_str(), param->_port, self->make_trig_notifer(ath));
	actor_handle connecting = create_mfc_actor(_ios, [this](my_actor* self)
	{//让“连接”按钮在连接中闪烁
		self->open_timer();
		this->GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);
		while (true)
		{
			this->GetDlgItem(IDC_BUTTON1)->SetWindowText("连接中   ");
			self->sleep(250);
			this->GetDlgItem(IDC_BUTTON1)->SetWindowText("连接中.  ");
			self->sleep(250);
			this->GetDlgItem(IDC_BUTTON1)->SetWindowText("连接中.. ");
			self->sleep(250);
			this->GetDlgItem(IDC_BUTTON1)->SetWindowText("连接中...");
			self->sleep(250);
		}
	});
	connecting->notify_run();
	self->open_timer();
	boost::system::error_code err;
	if (self->timed_wait_trig(param->_tm, ath, err) && !err && param->_clientSocket->no_delay())
	{
		self->actor_force_quit(connecting);
		send(self, [this]()
		{
			this->GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);
			this->GetDlgItem(IDC_BUTTON1)->SetWindowText("连接");
		});
		post(boost::bind(&Csocket_testDlg::showClientMsg, this, msg_data::create("连接成功")));
		actor_msg_handle<shared_data> amh;
		child_actor_handle readActor = self->create_child_actor([this, &amh](my_actor* self)
		{
			while (true)
			{
				//接收服务器的消息，然后发送给对话框
				shared_data msg = self->pump_msg(amh);
				if (!msg)
				{
					break;
				}
				post(boost::bind(&Csocket_testDlg::showClientMsg, this, msg));
			}
			self->close_msg_notifer(amh);
		});
		auto textio = text_stream_io::create(self->this_strand(), param->_clientSocket, readActor.get_actor()->make_msg_notifer(amh));
		child_actor_handle writerActor = self->create_child_actor([&textio, &param](my_actor* self)
		{
			msg_pump<shared_data>::handle amh = self->connect_msg_pump<shared_data>();
			while (true)
			{
				//接收对话框给的消息，然后发送给服务器
				textio->write(self->pump_msg<shared_data>(amh));
			}
		});
		self->msg_agent_to<shared_data>(writerActor);
		self->child_actor_run(readActor);
		self->child_actor_run(writerActor);
		send(self, [this]()
		{
			this->GetDlgItem(IDC_BUTTON3)->EnableWindow(TRUE);
		});
		self->child_actor_wait_quit(readActor);
		self->child_actor_force_quit(writerActor);
		textio->close();
		post(boost::bind(&Csocket_testDlg::showClientMsg, this, msg_data::create("断开连接")));
	} 
	else
	{
		self->actor_force_quit(connecting);
		send(self, [this]()
		{
			this->GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);
			this->GetDlgItem(IDC_BUTTON1)->SetWindowText("连接");
		});
		post(boost::bind(&Csocket_testDlg::showClientMsg, this, msg_data::create("连接失败")));
		param->_clientSocket->close();
	}
	_uiCMD(ui_disconnect);
}

void Csocket_testDlg::newSession(my_actor* self, std::shared_ptr<session_pck> sess)
{//这个Actor运行在对话框线程中
	actor_trig_handle<> lstClose;
	dlg_session dlg;
	dlg._strand = _strand;
	dlg._socket = sess->_socket;
	dlg._lstClose = sess->_lstClose;
	dlg._closeNtf = sess->_closeNtf;
	dlg._closeCallback = self->make_trig_notifer(lstClose);
	dlg.Create(IDD_SESSION, this);
	dlg.ShowWindow(SW_SHOW);
	self->wait_trig(lstClose);
	dlg.DestroyWindow();
}

void Csocket_testDlg::serverActor(my_actor* self, std::shared_ptr<server_param> param)
{
	actor_msg_handle<socket_handle> amh;
	accept_handle accept = acceptor_socket::create(self->this_strand(), param->_port, self->make_msg_notifer(amh), false);//创建连接侦听器
	if (!accept)
	{
		self->close_msg_notifer(amh);
		_uiCMD(ui_stopListen);
		return;
	}
	bool norClosed = false;
	//创建侦听服务器关闭
	child_actor_handle lstCloseProxyActor = self->create_child_actor([&](my_actor* self)//侦听服务器关闭
	{
		msg_pump<>::handle amh = self->connect_msg_pump();
		self->pump_msg(amh);
		//得到服务器关闭消息，关闭连接侦听器
		norClosed = true;
		accept->close();
	});
	self->msg_agent_to(lstCloseProxyActor);
	//创建会话关闭响应器
	list<std::shared_ptr<session_pck> > sessList;
	actor_msg_handle<list<std::shared_ptr<session_pck> >::iterator> sessDissonnLst;
	child_actor_handle sessMngActor = self->create_child_actor([&](my_actor* self)
	{
		while (true)
		{
			sessList.erase(self->pump_msg(sessDissonnLst));
			post(boost::bind(&Csocket_testDlg::showSessionNum, this, sessList.size()));
		}
		self->close_msg_notifer(sessDissonnLst);
	});
	auto sessDissonnNtf = sessMngActor.get_actor()->make_msg_notifer(sessDissonnLst);
	self->child_actor_run(lstCloseProxyActor);
	self->child_actor_run(sessMngActor);
	while (true)
	{
		socket_handle newSocket = self->pump_msg(amh);
		if (!newSocket)
		{
			if (!norClosed)
			{
				self->child_actor_force_quit(lstCloseProxyActor);
				send(self, [this]()
				{
					this->MessageBox("服务器意外关闭");
				});
			}
			break;
		}
		if (sessList.size() < (size_t)param->_maxSessionNum && newSocket->no_delay())
		{
			std::shared_ptr<session_pck> newSess(new session_pck);
			newSess->_socket = newSocket;
			newSess->_lstClose = msg_pipe<>::make(newSess->_closeNtf);
			sessList.push_front(newSess);
			post(boost::bind(&Csocket_testDlg::showSessionNum, this, sessList.size()));
			newSess->_sessionDlg = create_mfc_actor(boost::bind(&Csocket_testDlg::newSession, this, _1, newSess));
			newSess->_sessionDlg->append_quit_callback(boost::bind((std::function<void(list<std::shared_ptr<session_pck> >::iterator)>)sessDissonnNtf, sessList.begin()));
			newSess->_sessionDlg->notify_run();
		}
		else
		{
			newSocket->close();
		}
	}
	self->child_actor_wait_quit(lstCloseProxyActor);
	self->child_actor_force_quit(sessMngActor);
	self->close_msg_notifer(amh);
	//通知所有存在的对话框关闭
	for (auto it = sessList.begin(); it != sessList.end(); it++)
	{
		(*it)->_closeNtf();
		self->actor_wait_quit((*it)->_sessionDlg);
	}
}

void Csocket_testDlg::mainActor(my_actor* self)
{
	BEGIN_CHECK_FORCE_QUIT;
	std::shared_ptr<client_param> extClient;
	std::function<void ()> serverNtfClose;
	std::function<void (shared_data)> clientPostPipe;
	child_actor_handle clientActorHandle;
	child_actor_handle serverActorHandle;
	auto disconnectHandler = [&, this]()
	{
		if (!clientActorHandle.empty())
		{
			extClient->_clientSocket->close();
			self->child_actor_wait_quit(clientActorHandle);
			extClient.reset();
			clear_function(clientPostPipe);
			auto _this = this;
			this->send(self, [&, _this]()
			{
				_this->GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);
				_this->GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
				_this->GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
			});
		}
	};
	auto stopListenHandler = [&, this]()
	{
		if (!serverActorHandle.empty())
		{
			serverNtfClose();
			self->child_actor_wait_quit(serverActorHandle);
			auto _this = this;
			this->send(self, [_this]()
			{
				_this->_extSessNumEdit.SetWindowText("");
				_this->GetDlgItem(IDC_BUTTON4)->EnableWindow(TRUE);
				_this->GetDlgItem(IDC_BUTTON5)->EnableWindow(FALSE);
			});
		}
	};

	msg_pump<ui_cmd>::handle lstCMD = self->connect_msg_pump<ui_cmd>();
	while (true)
	{
		send(self, [this](){this->EnableWindow(TRUE);});
		ui_cmd cmd = self->pump_msg<ui_cmd>(lstCMD);
		send(self, [this](){this->EnableWindow(FALSE);});
		switch (cmd)
		{
		case ui_connect:
			{
				if (clientActorHandle.empty())
				{
					char sip[32];
					CString sport;
					CString stm;
					send(self, [&, this]()
					{
						BYTE bip[4];
						_clientIpEdit.GetAddress(bip[0], bip[1], bip[2], bip[3]);
						sprintf_s(sip, "%d.%d.%d.%d", bip[0], bip[1], bip[2], bip[3]);
						_clientPortEdit.GetWindowText(sport);
						_clientTimeoutEdit.GetWindowText(stm);
					});
					try
					{
						extClient = std::shared_ptr<client_param>(new client_param);
						extClient->_ip = sip;
						extClient->_port = boost::lexical_cast<int>(sport.GetBuffer());
						extClient->_tm = boost::lexical_cast<int>(stm.GetBuffer());
						extClient->_clientSocket = socket_io::create(_ios);
						clientActorHandle = self->create_child_actor(_strand, 
							boost::bind(&Csocket_testDlg::connectActor, this, _1, extClient));
						self->child_actor_run(clientActorHandle);
						clientPostPipe = self->connect_msg_notifer_to<shared_data>(clientActorHandle);
						send(self, [this]()
						{
							this->GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);
							this->GetDlgItem(IDC_BUTTON2)->EnableWindow(TRUE);
						});
					}
					catch (...)
					{
						extClient.reset();
					}
				}
			}
			break;
		case ui_listen:
			{
				if (serverActorHandle.empty())
				{
					CString sport;
					CString snum;
					send(self, [&, this]()
					{
						_serverPortEdit.GetWindowText(sport);
						_maxSessionEdit.GetWindowText(snum);
						_extSessNumEdit.SetWindowText("");
					});
					try
					{
						std::shared_ptr<server_param> param(new server_param);
						param->_port = boost::lexical_cast<int>(sport.GetBuffer());
						param->_maxSessionNum = boost::lexical_cast<int>(snum.GetBuffer());
						serverActorHandle = self->create_child_actor(_strand, boost::bind(&Csocket_testDlg::serverActor, this, _1, param));
						self->child_actor_run(serverActorHandle);
						serverNtfClose = self->connect_msg_notifer_to(serverActorHandle);
						send(self, [this]()
						{
							this->GetDlgItem(IDC_BUTTON4)->EnableWindow(FALSE);
							this->GetDlgItem(IDC_BUTTON5)->EnableWindow(TRUE);
						});
					}
					catch (...)
					{
						
					}
				}
			}
			break;
		case ui_postMsg:
			{
				if (clientPostPipe)
				{
					send(self, [&]()
					{
						CString cs;
						_msgEdit.GetWindowText(cs);
						if (cs.GetLength())
						{
							_msgEdit.SetWindowText("");
							clientPostPipe(msg_data::create(cs.GetBuffer()));
						}
					});
				}
			}
			break;
		case ui_disconnect:
			{
				disconnectHandler();
			}
			break;
		case ui_stopListen:
			{
				stopListenHandler();
			}
			break;
		case ui_close:
			{
				disconnectHandler();
				stopListenHandler();
				send(self, [this](){this->mfc_close();});
				return;
			}
			break;
		default:
			break;
		}
	}
	END_CHECK_FORCE_QUIT;
}
