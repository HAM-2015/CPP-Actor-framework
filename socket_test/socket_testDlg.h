
// socket_testDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "bind_mfc_run.h"
#include "actor_framework.h"
#include "msg_pipe.h"
#include "shared_data.h"
#include "socket_io.h"
#include "text_stream_io.h"
#include "acceptor_socket.h"
#include "dlg_session.h"
#include "mfc_strand.h"


// Csocket_testDlg 对话框
class Csocket_testDlg : public CDialogEx, bind_mfc_run
{
// 构造
public:
	Csocket_testDlg(CWnd* pParent = NULL);	// 标准构造函数
	~Csocket_testDlg();

// 对话框数据
	enum { IDD = IDD_SOCKET_TEST_DIALOG };

	enum ui_cmd
	{
		ui_connect,
		ui_disconnect,
		ui_listen,
		ui_stopListen,
		ui_close,
		ui_postMsg
	};

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	struct client_param 
	{
		boost::shared_ptr<socket_io> _clientSocket;
		string _ip;
		int _port;
		int _tm;
		msg_pipe<shared_data>::regist_reader _msgPump;
	};

	struct server_param 
	{
		int _port;
		int _maxSessionNum;
		msg_pipe<>::regist_reader _closePump;
	};

	struct session_pck 
	{
		boost::shared_ptr<socket_io> _socket;
		msg_pipe<>::writer_type _closeNtf;
		actor_handle _sessionDlg;
	};
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedClientConnect();
	afx_msg void OnBnClickedClientDisconnect();
	afx_msg void OnBnClickedSendClientMsg();
	afx_msg void OnBnClickedListen();
	afx_msg void OnBnClickedStopLst();
	afx_msg void OnBnClickedClear();
	void showClientMsg(shared_data msg);
	void showSessionNum(int n);
	DECLARE_MESSAGE_MAP()
	BIND_MFC_RUN()
	BIND_ACTOR_SEND()
	BIND_MFC_ACTOR(Csocket_testDlg, CDialogEx)
private:
	void connectActor(boost_actor* actor, boost::shared_ptr<client_param> param);
	void newSession(boost_actor* actor, boost::shared_ptr<socket_io> socket, msg_pipe<>::regist_reader closePump);
	void serverActor(boost_actor* actor, boost::shared_ptr<server_param> param);
	void mainActor(boost_actor* actor, actor_msg_handle<ui_cmd>::ptr lstCMD);
private:
	ios_proxy _ios;
	shared_strand _strand;
	CFont _editFont;
	boost::function<void (ui_cmd)> _uiCMD;
public:
	CEdit _outputEdit;
	CEdit _msgEdit;
	CIPAddressCtrl _clientIpEdit;
	CEdit _clientPortEdit;
	CEdit _clientTimeoutEdit;
	CEdit _serverPortEdit;
	CEdit _maxSessionEdit;
	CEdit _extSessNumEdit;
};
