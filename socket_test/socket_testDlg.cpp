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

	_ios.run();
	_strand = boost_strand::create(_ios);
	actor_handle mainActor = my_actor::create(_strand, boost::bind(&Csocket_testDlg::mainActor, this, _1));
	_uiCMD = mainActor->connect_msg_notifer<ui_cmd>();
	mainActor->notify_run();
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void Csocket_testDlg::showClientMsg(const char* msg)
{
	int nLength = _outputEdit.GetWindowTextLength();
	_outputEdit.SetSel(nLength, nLength);
	_outputEdit.ReplaceSel(msg);
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
	_msgEdit.SetWindowText("");
}

void Csocket_testDlg::connectActor(my_actor* self, std::shared_ptr<client_param> param)
{
	actor_trig_handle<boost::system::error_code> ath;
	param->_clientSocket->async_connect(param->_ip.c_str(), param->_port, self->make_trig_notifer_to_self(ath));
	actor_handle connecting = create_mfc_actor(_ios, [this](my_actor* self)
	{//让“连接”按钮在连接中闪烁
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
	boost::system::error_code err;
	if (self->timed_wait_trig(param->_tm, ath, err) && !err && param->_clientSocket->no_delay())
	{
		self->actor_force_quit(connecting);
		send(self, [this]
		{
			this->GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);
			this->GetDlgItem(IDC_BUTTON1)->SetWindowText("连接");
			this->showClientMsg("连接成功");
		});
		actor_msg_handle<shared_data> amh;
		child_actor_handle readActor = self->create_child_actor([this, &amh](my_actor* self)
		{
			while (true)
			{
				//接收服务器的消息，然后发送给对话框
				shared_data msg = self->wait_msg(amh);
				if (!msg)
				{
					break;
				}
				auto this_ = this;
				this->send(self, [this_, &msg]{this_->showClientMsg(msg->c_str()); });
			}
			self->close_msg_notifer(amh);
		});
		self->child_actor_run(readActor);
		auto textio = text_stream_io::create(self->self_strand(), param->_clientSocket, readActor->make_msg_notifer_to_self(amh));
		child_actor_handle writerActor = self->msg_agent_to_actor<shared_data>(true, 
			[&textio, &param](my_actor* self, msg_pump_handle<shared_data> amh)
		{
			while (true)
			{
				//接收对话框给的消息，然后发送给服务器
				textio->write(self->pump_msg(amh));
			}
		});
		send(self, [this]
		{
			this->GetDlgItem(IDC_BUTTON3)->EnableWindow(TRUE);
		});
		self->child_actor_wait_quit(readActor);
		self->child_actor_force_quit(writerActor);
		textio->close();
		send(self, [this]{this->showClientMsg("断开连接"); });
	} 
	else
	{
		self->actor_force_quit(connecting);
		send(self, [this]
		{
			this->GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);
			this->GetDlgItem(IDC_BUTTON1)->SetWindowText("连接");
			this->showClientMsg("连接失败");
		});
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
	dlg._closeNtf = sess->_closeNtf;
	dlg._closeCallback = self->make_trig_notifer_to_self(lstClose);
	dlg.Create(IDD_SESSION, this);
	dlg.run(self);
	dlg.ShowWindow(SW_SHOW);
	self->wait_trig(lstClose);
	dlg.waitQuit(self);
	dlg.DestroyWindow();
}

void Csocket_testDlg::serverActor(my_actor* self, std::shared_ptr<server_param> param)
{
	actor_msg_handle<socket_handle> amh;
	accept_handle accept = acceptor_socket::create(self->self_strand(), param->_port, self->make_msg_notifer_to_self(amh), false);//创建连接侦听器
	if (!accept)
	{
		self->close_msg_notifer(amh);
		_uiCMD(ui_stopListen);
		return;
	}
	bool norClosed = false;
	//创建侦听服务器关闭
	child_actor_handle lstCloseProxyActor = self->msg_agent_to_actor(true, [&](my_actor* self, msg_pump_handle<> amh)//侦听服务器关闭
	{
		self->pump_msg(amh);
		//得到服务器关闭消息，关闭连接侦听器
		norClosed = true;
		accept->close();
	});
	//创建会话关闭响应器
	list<std::shared_ptr<session_pck> > sessList;
	actor_msg_handle<list<std::shared_ptr<session_pck> >::iterator> sessDissonnLst;
	child_actor_handle sessMngActor = self->create_child_actor([&, this](my_actor* self)
	{
		while (true)
		{
			sessList.erase(self->wait_msg(sessDissonnLst));
			auto this_ = this;
			size_t size = sessList.size();
			this->send(self, [this_, size]
			{
				this_->showSessionNum(size);
			});
		}
		self->close_msg_notifer(sessDissonnLst);
	});
	auto sessDissonnNtf = sessMngActor->make_msg_notifer_to_self(sessDissonnLst);
	self->child_actor_run(sessMngActor);
	while (true)
	{
		socket_handle newSocket = self->wait_msg(amh);
		if (!newSocket)
		{
			if (!norClosed)
			{
				self->child_actor_force_quit(lstCloseProxyActor);
				send(self, [this]
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
			sessList.push_front(newSess);
			size_t size = sessList.size();
			this->send(self, [this, size]
			{
				this->showSessionNum(size);
			});
			newSess->_sessionDlg = create_mfc_actor(boost::bind(&Csocket_testDlg::newSession, this, _1, newSess));
			newSess->_closeNtf = self->connect_msg_notifer_to(newSess->_sessionDlg);
			auto nit = sessList.begin();
			newSess->_sessionDlg->append_quit_callback([&sessDissonnNtf, nit]()
			{
				sessDissonnNtf(nit);
			});
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
	post_actor_msg<> serverNtfClose;
	post_actor_msg<shared_data> clientPostPipe;
	actor_handle clientActorHandle;
	actor_handle serverActorHandle;
	auto disconnectHandler = [&, this]
	{
		if (clientActorHandle)
		{
			extClient->_clientSocket->close();
			self->actor_wait_quit(clientActorHandle);
			clientActorHandle.reset();
			extClient.reset();
			clientPostPipe.clear();
			auto _this = this;
			this->send(self, [_this]
			{
				_this->GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);
				_this->GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
				_this->GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
			});
		}
	};
	auto stopListenHandler = [&, this]
	{
		if (serverActorHandle)
		{
			serverNtfClose();
			self->actor_wait_quit(serverActorHandle);
			serverActorHandle.reset();
			auto _this = this;
			this->send(self, [_this]
			{
				_this->_extSessNumEdit.SetWindowText("");
				_this->GetDlgItem(IDC_BUTTON4)->EnableWindow(TRUE);
				_this->GetDlgItem(IDC_BUTTON5)->EnableWindow(FALSE);
			});
		}
	};

	msg_pump_handle<ui_cmd> lstCMD = self->connect_msg_pump<ui_cmd>();
	while (true)
	{
		send(self, [this]{this->EnableWindow(TRUE); });
		ui_cmd cmd = self->pump_msg(lstCMD);
		send(self, [this]{this->EnableWindow(FALSE); });
		switch (cmd)
		{
		case ui_connect:
			{
				if (!clientActorHandle)
				{
					char sip[32];
					CString sport;
					CString stm;
					send(self, [&, this]
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
						clientActorHandle = my_actor::create(_strand, 
							boost::bind(&Csocket_testDlg::connectActor, this, _1, extClient));
						clientActorHandle->notify_run();
						clientPostPipe = self->connect_msg_notifer_to<shared_data>(clientActorHandle);
						send(self, [this]
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
				if (!serverActorHandle)
				{
					CString sport;
					CString snum;
					send(self, [&, this]
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
						serverActorHandle = my_actor::create(_strand, boost::bind(&Csocket_testDlg::serverActor, this, _1, param));
						serverActorHandle->notify_run();
						serverNtfClose = self->connect_msg_notifer_to(serverActorHandle);
						send(self, [this]
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
					CString cs;
					send(self, [&]
					{
						_msgEdit.GetWindowText(cs);
						_msgEdit.SetWindowText("");
					});
					if (cs.GetLength())
					{
						clientPostPipe(msg_data::create(cs.GetBuffer()));
					}
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
				send(self, [this]{this->mfc_close();});
				return;
			}
			break;
		default:
			break;
		}
	}
	END_CHECK_FORCE_QUIT;
}