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
	REGIEST_MFC_RUN(dlg_session)
	ON_BN_CLICKED(IDC_BUTTON3, &dlg_session::OnBnClickedSendMsg)
	ON_BN_CLICKED(IDC_BUTTON1, &dlg_session::OnBnClickedClear)
END_MESSAGE_MAP()

BOOL dlg_session::OnInitDialog()
{
	set_thread_id();
	CDialogEx::OnInitDialog();

	_editFont.CreatePointFont(90, "宋体");
	_msgEdit.SetFont(&_editFont);
	_outputEdit.SetFont(&_editFont);

	_sendPump = msg_pipe<shared_data>::make(_sendPipe);
	actor_handle sessionActor = my_actor::create(_strand, boost::bind(&dlg_session::sessionActor, this, _1));
	sessionActor->notify_run();
	return TRUE;
}

// dlg_session 消息处理程序

void dlg_session::OnCancel()
{
	_closeNtf();
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
}

void dlg_session::sessionActor(my_actor* self)
{
	post(boost::bind(&dlg_session::showClientMsg, this, msg_data::create(_socket->ip())));
	child_actor_handle lstClose = self->create_child_actor([&, this](my_actor* self)
	{
		actor_msg_handle<> amh;
		_lstClose(self, amh);
		//侦听请求对话框关闭消息，然后通知对话框关闭
		self->pump_msg(amh);
		this->_exit = true;
		_socket->close();
		self->close_msg_notify(amh);
	});
	self->child_actor_run(lstClose);
	actor_msg_handle<shared_data> amh;
	boost::shared_ptr<text_stream_io> textio = text_stream_io::create(_strand, _socket, self->make_msg_notify(amh));
	child_actor_handle wd = self->create_child_actor([this, &textio](my_actor* self)
	{
		actor_msg_handle<shared_data> amh;
		_sendPump(self, amh);
		while (true)
		{
			auto msg = self->pump_msg(amh);
			if (!msg)
			{
				break;
			}
			textio->write(msg);
		}
		self->close_msg_notify(amh);
	});
	self->child_actor_run(wd);
	while (true)
	{
		auto msg = self->pump_msg(amh);
		if (!msg)
		{
			break;
		}
		post(boost::bind(&dlg_session::showClientMsg, this, msg));
	}
	self->child_actor_force_quit(wd);
	self->close_msg_notify(amh);
	post(boost::bind(&dlg_session::showClientMsg, this, msg_data::create("连接断开")));
	send(self, [this]()
	{
		this->GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
	});
	self->child_actor_wait_quit(lstClose);
	send(self, wrap([this]()
	{
		this->mfc_close();
		_closeCallback();
	}));
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
