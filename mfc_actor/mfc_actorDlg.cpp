
// mfc_actorDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "mfc_actor.h"
#include "mfc_actorDlg.h"
#include "afxdialogex.h"
#include "dlg_add.h"
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


// Cmfc_actorDlg 对话框




Cmfc_actorDlg::Cmfc_actorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Cmfc_actorDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

Cmfc_actorDlg::~Cmfc_actorDlg()
{
	_ios.stop();
}

void Cmfc_actorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, _dataEdit1);
	DDX_Control(pDX, IDC_EDIT2, _dataEdit2);
	DDX_Control(pDX, IDC_EDIT3, _dataEdit3);
	DDX_Control(pDX, IDC_EDIT4, _dataEdit4);
	DDX_Control(pDX, IDC_EDIT5, _dataEdit5);
	DDX_Control(pDX, IDC_EDIT6, _dataEdit6);
	DDX_Control(pDX, IDC_EDIT7, _dataEdit7);
	DDX_Control(pDX, IDC_EDIT8, _dataEdit8);
	DDX_Control(pDX, IDC_EDIT9, _dataEdit9);
	DDX_Control(pDX, IDC_EDIT10, _dataEdit10);
	DDX_Control(pDX, IDC_EDIT11, _dataEdit11);
	DDX_Control(pDX, IDC_EDIT12, _dataEdit12);
}

BEGIN_MESSAGE_MAP(Cmfc_actorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	REGIEST_MFC_RUN(Cmfc_actorDlg)
	ON_BN_CLICKED(IDC_BUTTON1, &Cmfc_actorDlg::OnBnClickedAdd)
END_MESSAGE_MAP()


// Cmfc_actorDlg 消息处理程序

BOOL Cmfc_actorDlg::OnInitDialog()
{
	SET_THREAD_ID();
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
	_ios.run();
	boost_actor::enable_stack_pool();
	_strand = boost_strand::create(_ios);
	_mfcActor1 = create_mfc_actor(_ios, boost::bind(&Cmfc_actorDlg::mfc_actor1, this, _1));
	_mfcActor2 = create_mfc_actor(_ios, boost::bind(&Cmfc_actorDlg::mfc_actor2, this, _1));
	_boostActor1 = boost_actor::create(_strand, boost::bind(&Cmfc_actorDlg::boost_actor1, this, _1));
	_boostActor2 = boost_actor::create(_strand, boost::bind(&Cmfc_actorDlg::boost_actor2, this, _1));
	_mfcActor1->notify_start_run();
	_mfcActor2->notify_start_run();
	_boostActor1->notify_start_run();
	_boostActor2->notify_start_run();
	actor_msg_handle<>::ptr lstTask = actor_msg_handle<>::make_ptr();
	_calcActor = create_mfc_actor(_ios, boost::bind(&Cmfc_actorDlg::calc_add, this, _1, lstTask));
	_calcNotify = _calcActor->make_msg_notify(*lstTask);
	_calcActor->notify_start_run();
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void Cmfc_actorDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void Cmfc_actorDlg::OnPaint()
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
HCURSOR Cmfc_actorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Cmfc_actorDlg::mfc_actor1(boost_actor* actor)
{
	while (true)
	{//这里是mfc线程
		_dataEdit1.SetWindowText("+++");
		_dataEdit2.SetWindowText("");
		_dataEdit3.SetWindowText("");
		actor->sleep(1000);
		_dataEdit1.SetWindowText("");
		_dataEdit2.SetWindowText("+++");
		_dataEdit3.SetWindowText("");
		actor->sleep(1000);
		_dataEdit1.SetWindowText("");
		_dataEdit2.SetWindowText("");
		_dataEdit3.SetWindowText("+++");
		actor->sleep(1000);
	}
}

void Cmfc_actorDlg::mfc_actor2(boost_actor* actor)
{
	while (true)
	{//这里是mfc线程
		_dataEdit4.SetWindowText("---");
		_dataEdit5.SetWindowText("");
		_dataEdit6.SetWindowText("");
		actor->sleep(300);
		_dataEdit4.SetWindowText("");
		_dataEdit5.SetWindowText("---");
		_dataEdit6.SetWindowText("");
		actor->sleep(300);
		_dataEdit4.SetWindowText("");
		_dataEdit5.SetWindowText("");
		_dataEdit6.SetWindowText("---");
		actor->sleep(300);
	}
}

void Cmfc_actorDlg::boost_actor1(boost_actor* actor)
{
	while (true)
	{//这里是ios线程
		send(actor, [this]()
		{//这里是mfc线程
			_dataEdit7.SetWindowText("***");
			_dataEdit8.SetWindowText("");
			_dataEdit9.SetWindowText("");
		});
		actor->sleep(200);
		send(actor, [this]()
		{
			_dataEdit7.SetWindowText("");
			_dataEdit8.SetWindowText("***");
			_dataEdit9.SetWindowText("");
		});
		actor->sleep(200);
		send(actor, [this]()
		{
			_dataEdit7.SetWindowText("");
			_dataEdit8.SetWindowText("");
			_dataEdit9.SetWindowText("***");
		});
		actor->sleep(200);
	}
}

void Cmfc_actorDlg::boost_actor2(boost_actor* actor)
{
	while (true)
	{//这里是ios线程
		send(actor, [this]()
		{//这里是mfc线程
			_dataEdit10.SetWindowText("///");
			_dataEdit11.SetWindowText("");
			_dataEdit12.SetWindowText("");
		});
		actor->sleep(100);
		send(actor, [this]()
		{
			_dataEdit10.SetWindowText("");
			_dataEdit11.SetWindowText("///");
			_dataEdit12.SetWindowText("");
		});
		actor->sleep(100);
		send(actor, [this]()
		{
			_dataEdit10.SetWindowText("");
			_dataEdit11.SetWindowText("");
			_dataEdit12.SetWindowText("///");
		});
		actor->sleep(100);
	}
}

void Cmfc_actorDlg::calc_add(boost_actor* actor, actor_msg_handle<>::ptr lstTask)
{
	list<child_actor_handle::ptr> taskList;
	actor_msg_handle<list<child_actor_handle::ptr>::iterator> lstTaskOver;
	child_actor_handle removeTask = actor->create_child_actor([&](boost_actor* actor)
	{//这里是mfc线程
		while (true)
		{
			(*actor->pump_msg(lstTaskOver))->peel();
		}
	});
	auto overNtf = removeTask.get_actor()->make_msg_notify(lstTaskOver);
	actor->child_actor_run(removeTask);
	while (true)
	{//这里是mfc线程
		actor->pump_msg(*lstTask);
		child_actor_handle::ptr newt = child_actor_handle::make_ptr();
		*newt = actor->create_child_actor([this](boost_actor* actor)
		{//这里是mfc线程
			dlg_add dlgAdd;
			async_trig_handle<> waitDlg;
			dlgAdd._closeNotify = actor->begin_trig(waitDlg);
			dlgAdd.Create(IDD_DIALOG1, this);
			dlgAdd.ShowWindow(SW_SHOW);
			auto qh = actor->regist_quit_handler(boost::bind(&dlg_add::DestroyWindow, &dlgAdd));
			actor->wait_trig(waitDlg);
			actor->cancel_quit_handler(qh);
			dlgAdd.DestroyWindow();
			try 
			{
				int ia = boost::lexical_cast<int>(dlgAdd._as.GetBuffer());
				int ib = boost::lexical_cast<int>(dlgAdd._bs.GetBuffer());
				int ir = actor->send<int>(_strand, [&]()->int
				{//这里是ios线程
					return ia+ib;
				});
				string rs = boost::lexical_cast<string>(ir);
				this->MessageBox(rs.c_str());
			}
			CATCH_ACTOR_QUIT()
			catch(...) {}
		});
		taskList.push_front(newt);
		newt->get_actor()->append_quit_callback(boost::bind(overNtf, taskList.begin()));
		actor->child_actor_run(*newt);
	}
	actor->child_actor_force_quit(removeTask);
	actor->close_msg_notify(*lstTask);
}

void Cmfc_actorDlg::OnBnClickedAdd()
{
	_calcNotify();
}

void Cmfc_actorDlg::OnCancel()
{
	_mfcActor1->notify_force_quit();
	_mfcActor2->notify_force_quit();
	_calcActor->notify_force_quit();
	_boostActor1->notify_force_quit();
	_boostActor2->notify_force_quit();
	_mfcActor1->append_quit_callback(close_mfc_handler());
	_mfcActor2->append_quit_callback(close_mfc_handler());
	_calcActor->append_quit_callback(close_mfc_handler());
	_boostActor1->append_quit_callback(close_mfc_handler());
	_boostActor2->append_quit_callback(close_mfc_handler());
}
