#pragma once
#include "afxwin.h"
#include "bind_mfc_run.h"
#include "actor_framework.h"
#include "msg_pipe.h"
#include "socket_io.h"
#include "text_stream_io.h"
#include "mfc_strand.h"


// dlg_session 对话框

class dlg_session : public CDialogEx, bind_mfc_run
{
	DECLARE_DYNAMIC(dlg_session)

public:
	dlg_session(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~dlg_session();

// 对话框数据
	enum { IDD = IDD_SESSION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	afx_msg void OnBnClickedSendMsg();
	afx_msg void OnBnClickedClear();
	void showClientMsg(shared_data msg);
	DECLARE_MESSAGE_MAP()
	BIND_MFC_RUN()
	BIND_ACTOR_SEND()
	BIND_MFC_ACTOR(dlg_session, CDialogEx)
private:
	void sessionActor(boost_actor* actor);
public:
	shared_strand _strand;
	boost::shared_ptr<socket_io> _socket;
	boost::function<void ()> _closeCallback;
	msg_pipe<>::writer_type _closeNtf;
	msg_pipe<>::regist_reader _lstClose;
private:
	bool _exit;
	msg_pipe<shared_data>::writer_type _sendPipe;
	msg_pipe<shared_data>::regist_reader _sendPump;
	CFont _editFont;
public:
	CEdit _msgEdit;
	CEdit _outputEdit;
};
