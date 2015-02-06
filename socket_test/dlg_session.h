#pragma once
#include "afxwin.h"
#include "bind_modal_run.h"
#include "boost_coroutine.h"
#include "msg_pipe.h"
#include "socket_io.h"
#include "text_stream_io.h"


// dlg_session 对话框

class dlg_session : public CDialogEx, bind_modal_run
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

	afx_msg void OnDestroy();
	afx_msg void OnBnClickedSendMsg();
	afx_msg void OnBnClickedClear();
	void showClientMsg(shared_data msg);
	DECLARE_MESSAGE_MAP()
	BIND_MODAL_RUN()
private:
	void sessionCoro(boost_coro* coro);
	void lstClose(boost_coro* coro);
public:
	shared_strand _strand;
	boost::shared_ptr<socket_io> _socket;
	msg_pipe<>::regist_reader _lstClose;
private:
	bool _exit;
	msg_pipe<shared_data>::writer_type _sendPipe;
	msg_pipe<shared_data>::regist_reader _sendPump;
	coro_handle _sessionCoro;
	coro_handle _lstCloseCoro;
	CFont _editFont;
public:
	CEdit _msgEdit;
	CEdit _outputEdit;
};
