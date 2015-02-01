// dlg_session.cpp : 实现文件
//

#include "stdafx.h"
#include "socket_test.h"
#include "dlg_session.h"
#include "afxdialogex.h"


// dlg_session 对话框

IMPLEMENT_DYNAMIC(dlg_session, CDialogEx)

dlg_session::dlg_session(CWnd* pParent /*=NULL*/)
	: CDialogEx(dlg_session::IDD, pParent)
{
	_exit = false;
}

dlg_session::~dlg_session()
{
}

void dlg_session::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT5, _msgEdit);
	DDX_Control(pDX, IDC_EDIT4, _outputEdit);
}


BEGIN_MESSAGE_MAP(dlg_session, CDialogEx)
	ON_WM_DESTROY()
	REGIEST_MODAL_RUN(dlg_session)
	ON_BN_CLICKED(IDC_BUTTON3, &dlg_session::OnBnClickedSendMsg)
	ON_BN_CLICKED(IDC_BUTTON1, &dlg_session::OnBnClickedClear)
END_MESSAGE_MAP()

BOOL dlg_session::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	_editFont.CreatePointFont(90, "宋体");
	_msgEdit.SetFont(&_editFont);
	_outputEdit.SetFont(&_editFont);

	_sendPump = msg_pipe<shared_data>::make(_sendPipe);
	_sessionCoro = boost_coro::create(_strand, boost::bind(&dlg_session::sessionCoro, this, _1));
	_sessionCoro->notify_start_run();
	_lstCloseCoro = boost_coro::create(_strand, boost::bind(&dlg_session::lstClose, this, _1));
	_lstCloseCoro->notify_start_run();
	return TRUE;
}

// dlg_session 消息处理程序

void dlg_session::OnDestroy()
{
	if (_lstCloseCoro)
	{
		_lstCloseCoro->notify_force_quit();
		_lstCloseCoro->outside_wait_quit();
		_socket->close();
		_sessionCoro->outside_wait_quit();
		_lstCloseCoro.reset();
		_socket.reset();
		_sessionCoro.reset();
		CLEAR_MSG();
	}
	CDialogEx::OnDestroy();
}

void dlg_session::showClientMsg(shared_data msg)
{
	if (!_exit)
	{
		int nLength = _outputEdit.GetWindowTextLength();
		_outputEdit.SetSel(nLength, nLength);
		_outputEdit.ReplaceSel(msg->c_str());
		_outputEdit.ReplaceSel("\n");
	}
	else
	{
		CLEAR_MSG();
		PostMessage(WM_CLOSE);
	}
}

void dlg_session::lstClose(boost_coro* coro)
{
	coro_msg_handle<> cmh;
	_lstClose(coro, cmh);
	//侦听请求对话框关闭消息，然后通知对话框关闭
	coro->pump_msg(cmh);
	_exit = true;
	PostMessage(WM_CLOSE);
	coro->close_msg_notify(cmh);
}

void dlg_session::writeCoro(boost_coro* coro, boost::shared_ptr<text_stream_io> textio)
{
	coro_msg_handle<shared_data> cmh;
	_sendPump(coro, cmh);
	while (true)
	{
		auto msg = coro->pump_msg(cmh);
		if (!msg)
		{
			break;
		}
		textio->write(msg);
	}
	coro->close_msg_notify(cmh);
}

void dlg_session::sessionCoro(boost_coro* coro)
{
	post(boost::bind(&dlg_session::showClientMsg, this, msg_data::create(_socket->ip())));
	coro_msg_handle<shared_data> cmh;
	boost::shared_ptr<text_stream_io> textio = text_stream_io::create(_strand, _socket, coro->make_msg_notify(cmh));
	child_coro_handle wd = coro->create_child_coro(boost::bind(&dlg_session::writeCoro, this, _1, textio));
	coro->child_coro_run(wd);
	while (true)
	{
		auto msg = coro->pump_msg(cmh);
		if (!msg)
		{
			break;
		}
		post(boost::bind(&dlg_session::showClientMsg, this, msg));
	}
	coro->child_coro_quit(wd);
	coro->close_msg_notify(cmh);
	post(boost::bind(&dlg_session::showClientMsg, this, msg_data::create("连接断开")));
}

void dlg_session::OnBnClickedSendMsg()
{
	CString cs;
	_msgEdit.GetWindowText(cs);
	if (cs.GetLength())
	{
		_msgEdit.SetWindowText("");
		_sendPipe(msg_data::create(cs.GetBuffer()));
	}
	_msgEdit.SetFocus();
}


void dlg_session::OnBnClickedClear()
{
	_outputEdit.SetWindowText("");
}
