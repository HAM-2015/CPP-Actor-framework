#pragma once
#include "afxwin.h"
#include "bind_mfc_run.h"
#include "socket_io.h"
#include "text_stream_io.h"


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
	BIND_MFC_RUN(CDialogEx)
	DECLARE_MESSAGE_MAP()
public:
	void run(my_actor* parent);
	void waitQuit(my_actor* parent);
private:
	void sessionActor(my_actor* self);
public:
	shared_strand _strand;
	socket_handle _socket;
	actor_trig_notifer<> _closeCallback;
	post_actor_msg<> _closeNtf;
	child_actor_handle _sessionActor;
private:
	bool _exit;
	post_actor_msg<shared_data> _sendPipe;
	CFont _editFont;
public:
	CEdit _msgEdit;
	CEdit _outputEdit;
};
