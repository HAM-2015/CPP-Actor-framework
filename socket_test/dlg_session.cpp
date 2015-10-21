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
	return TRUE;
}

void dlg_session::run(my_actor* parent)
{
	_sessionActor = parent->create_child_actor(_strand, boost::bind(&dlg_session::sessionActor, this, _1));
	parent->child_actor_run(_sessionActor);
	_sendPipe = parent->connect_msg_notifer_to<shared_data>(_sessionActor);
	parent->msg_agent_to(_sessionActor);
}

void dlg_session::waitQuit(my_actor* parent)
{
	parent->child_actor_wait_quit(_sessionActor);
}

// dlg_session 消息处理程序

void dlg_session::OnCancel()
{
	_closeNtf();
}

void dlg_session::showClientMsg(const char* msg)
{
	if (!_exit)
	{
		int nLength = _outputEdit.GetWindowTextLength();
		_outputEdit.SetSel(nLength, nLength);
		_outputEdit.ReplaceSel(msg);
		_outputEdit.ReplaceSel("\n");
	}
}

void dlg_session::sessionActor(my_actor* self)
{
	send(self, [this]{this->showClientMsg(_socket->ip().c_str()); });
	child_actor_handle lstClose = self->create_child_actor([&, this](my_actor* self)
	{
		auto amh = self->connect_msg_pump();
		//侦听请求对话框关闭消息，然后通知对话框关闭
		self->pump_msg(amh);
		this->_exit = true;
		_socket->close();
	});
	self->msg_agent_to(lstClose);
	self->child_actor_run(lstClose);
	actor_msg_handle<shared_data> amh;
	std::shared_ptr<text_stream_io> textio = text_stream_io::create(_strand, _socket, self->make_msg_notifer_to_self(amh));
	child_actor_handle wd = self->create_child_actor([this, &textio](my_actor* self)
	{
		auto amh = self->connect_msg_pump<shared_data>();
		while (true)
		{
			auto msg = self->pump_msg(amh);
			if (!msg)
			{
				break;
			}
			textio->write(msg);
		}
	});
	self->child_actor_run(wd);
	self->msg_agent_to<shared_data>(wd);
	while (true)
	{
		auto msg = self->wait_msg(amh);
		if (!msg)
		{
			break;
		}
		send(self, [this, &msg]{this->showClientMsg(msg->c_str()); });
	}
	self->child_actor_force_quit(wd);
	self->close_msg_notifer(amh);
	send(self, [this]
	{
		this->GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
		this->showClientMsg("连接断开");
	});
	self->child_actor_wait_quit(lstClose);
	send(self, [this]
	{
		this->mfc_close();
	});
	_closeCallback();
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
	_msgEdit.SetWindowText("");
}
