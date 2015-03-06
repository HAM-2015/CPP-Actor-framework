
// mfc_coroDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "bind_mfc_run.h"
#include "mfc_strand.h"
#include "boost_coroutine.h"


// Cmfc_coroDlg 对话框
class Cmfc_coroDlg : public CDialogEx, bind_mfc_run
{
	// 构造
public:
	Cmfc_coroDlg(CWnd* pParent = NULL);	// 标准构造函数
	~Cmfc_coroDlg();

	// 对话框数据
	enum { IDD = IDD_MFC_CORO_DIALOG };

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
	DECLARE_MESSAGE_MAP()
	BIND_MFC_RUN()
	BIND_CORO_SEND()
	BIND_MFC_CORO(Cmfc_coroDlg, CDialogEx)
private:
	void mfc_coro1(boost_coro* coro);
	void mfc_coro2(boost_coro* coro);
	void boost_coro1(boost_coro* coro);
	void boost_coro2(boost_coro* coro);
private:
	ios_proxy _ios;
	coro_handle _mfcCoro1;
	coro_handle _mfcCoro2;
	coro_handle _boostCoro1;
	coro_handle _boostCoro2;
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