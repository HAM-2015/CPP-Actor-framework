#ifndef __WX_UI_H
#define __WX_UI_H

#include "bind_wx_run.h"
#include "wx_ui_base.h"

class wx_ui : public MyDialog1, bind_wx_run
{
	enum ui_cmd
	{
		ui_run,
		ui_run_over,
		ui_close
	};
public:
	wx_ui(ios_proxy& ios);
	~wx_ui();
	BIND_WX_RUN(wx_ui, MyDialog1)
private:
	void btn_click_run(wxCommandEvent& event);
	void OnClose();
private:
	ios_proxy& _ios;
	wxFont _font;
	actor_handle _mainActor;
	post_actor_msg<ui_cmd> _uiCMD;

};

template <typename dlg>
class cancel_dlg: public dlg, bind_wx_run
{
public:
	cancel_dlg(ios_proxy& ios, wxWindow* parent, const std::function<void ()>& cancelNtf)
		:dlg(parent), _ios(ios), _cancelNtf(cancelNtf)
	{
		start_wx_actor();
	}

	~cancel_dlg()
	{

	}
public:
	void run(my_actor* host)
	{
		_mainActor = my_actor::create(host->self_strand(), [this](my_actor* self)
		{
			RUN_IN_WX(Show());
			self->pump_msg(self->connect_msg_pump());
			BEGIN_RUN_IN_WX();
			if (isCancel())
			{
				_cancelNtf();
			}
			wx_close();
			END_RUN_IN_WX();
		});
		_ntfClose = host->connect_msg_notifer_to(_mainActor);
		_mainActor->notify_run();
	}

	void close_dlg(my_actor* host)
	{
		_ntfClose();
		host->actor_wait_quit(_mainActor);
	}
private:
	void OnClose()
	{
		_ntfClose();
	}

	BIND_WX_RUN(cancel_dlg, dlg)
private:
	ios_proxy& _ios;
	actor_handle _mainActor;
	const std::function<void()> _cancelNtf;
	post_actor_msg<> _ntfClose;
};

class cancel_ui : public MyDialog2
{
public:
	cancel_ui(wxWindow* parent);
	void btn_cancel(wxCommandEvent& event);
	bool isCancel();
private:
	bool _cancel;
};

#endif