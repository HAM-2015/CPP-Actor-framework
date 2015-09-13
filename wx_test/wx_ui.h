#ifndef __WX_UI_H
#define __WX_UI_H

#include "bind_wx_run.h"
#include "wx_ui_base.h"

class wx_ui : public bind_wx_run<MyDialog1>
{
	enum ui_cmd
	{
		ui_run,
		ui_close
	};
public:
	wx_ui(io_engine& ios);
	~wx_ui();
private:
	void btn_click_run(wxCommandEvent& event);
	void OnClose();
private:
	io_engine& _ios;
	wxFont _font;
	post_actor_msg<ui_cmd> _uiCMD;

};

template <typename dlg>
class cancel_dlg: public bind_wx_run<dlg>
{
public:
	cancel_dlg(io_engine& ios, wxWindow* parent, const std::function<void ()>& cancelNtf)
		:bind_wx_run(parent), _ios(ios), _cancelNtf(cancelNtf)
	{

	}

	~cancel_dlg()
	{

	}
public:
	void run(my_actor* host)
	{
		_mainActor = my_actor::create(host->self_strand(), [this](my_actor* self)
		{
			actor_handle modalRun = my_actor::create(self->self_strand(), [this](my_actor* self)
			{
				RUN_IN_WX(SHOW_MODAL(ShowModal()));
			});
			modalRun->notify_run();
			self->pump_msg(self->connect_msg_pump());
			begin_RUN_IN_WX();
			if (isCancel())
			{
				_cancelNtf();
			}
			wx_close();
			end_RUN_IN_WX();
			self->actor_wait_quit(modalRun);
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
private:
	io_engine& _ios;
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