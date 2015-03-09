
// mfc_actorDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "bind_mfc_run.h"
#include "mfc_strand.h"
#include "actor_framework.h"


// Cmfc_actorDlg 对话框
class Cmfc_actorDlg : public CDialogEx, bind_mfc_run
{
	// 构造
public:
	Cmfc_actorDlg(CWnd* pParent = NULL);	// 标准构造函数
	~Cmfc_actorDlg();

	// 对话框数据
	enum { IDD = IDD_MFC_ACTOR_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


	// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedAdd();
	DECLARE_MESSAGE_MAP()
	BIND_MFC_RUN()
	BIND_ACTOR_SEND()
	BIND_MFC_ACTOR(Cmfc_actorDlg, CDialogEx)
private:
	void mfc_actor1(boost_actor* actor);
	void mfc_actor2(boost_actor* actor);
	void boost_actor1(boost_actor* actor);
	void boost_actor2(boost_actor* actor);
	void calc_add(boost_actor* actor, actor_msg_handle<>::ptr lstTask);
private:
	ios_proxy _ios;
	shared_strand _strand;
	actor_handle _mfcActor1;
	actor_handle _mfcActor2;
	actor_handle _boostActor1;
	actor_handle _boostActor2;
	actor_handle _calcActor;
	boost::function<void ()> _calcNotify;
public:
	CEdit _dataEdit1;
	CEdit _dataEdit2;
	CEdit _dataEdit3;
	CEdit _dataEdit4;
	CEdit _dataEdit5;
	CEdit _dataEdit6;
	CEdit _dataEdit7;
	CEdit _dataEdit8;
	CEdit _dataEdit9;
	CEdit _dataEdit10;
	CEdit _dataEdit11;
	CEdit _dataEdit12;
};