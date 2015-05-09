#include "wx_ui.h"
#include "stack_object.h"
#include <wx/msgdlg.h>

void wx_ui::btn_click_run(wxCommandEvent& event)
{
	Disable();
	_uiCMD(ui_run);
	event.Skip();
}

void wx_ui::OnClose()
{
	Disable();
	_uiCMD(ui_close);
}

wx_ui::wx_ui(ios_proxy& ios)
:MyDialog1(NULL), _ios(ios)
{
	start_wx_actor();

	_font = wxFont(9, wxDEFAULT, wxNORMAL, wxNORMAL, false, "宋体");
	m_textCtrl1->SetFont(_font);
	_ios.run();
	_mainActor = my_actor::create(boost_strand::create(_ios), [this](my_actor* self)
	{
		actor_handle runActor;
		post_actor_msg<> runStop;
		auto cmd_pump = self->connect_msg_pump<ui_cmd>();
		while (true)
		{
			RUN_IN_WX(Enable());
			switch (self->pump_msg(*cmd_pump))
			{
			case ui_run:
				if (!runActor)
				{
					runActor = my_actor::create(self->self_strand(), [&, this](my_actor* self)
					{//启动一个Actor管理新"任务"
						bool taskCompleted = false;
						child_actor_handle actionActor = self->create_child_actor(wx_strand::create(_ios, this), [&, this](my_actor* self)
						{
							child_actor_handle lstCancel = self->msg_agent_to_actor(true, [](my_actor* self, msg_pump<>::handle mh)
							{//侦听任务是否要取消
								self->pump_msg(mh);
								self->parent_actor()->notify_quit();
							});

							for (int i = 0; i < 5; i++)
							{//运行任务
								m_textCtrl1->SetLabelText("running ");
								self->sleep(250);
								m_textCtrl1->SetLabelText("running *");
								self->sleep(250);
								m_textCtrl1->SetLabelText("running **");
								self->sleep(250);
								m_textCtrl1->SetLabelText("running ***");
								self->sleep(250);
							}
							taskCompleted = true;
							self->child_actor_force_quit(lstCancel);
						});
						self->msg_agent_to(actionActor);
						self->child_actor_run(actionActor);
						if (!self->timed_child_actor_wait_quit(1000, actionActor))
						{
							stack_obj<cancel_dlg<cancel_ui>> cu;
							RUN_IN_WX(cu.make(_ios, this, runStop));
							cu->run(self);
							self->child_actor_wait_quit(actionActor);
							cu->close_dlg(self);
							BEGIN_RUN_IN_WX();
							m_textCtrl1->SetLabelText(taskCompleted? "完成": "取消");
							cu.destroy();
							END_RUN_IN_WX();
						}
						else
						{
							RUN_IN_WX(m_textCtrl1->SetLabelText("完成"));
						}
						_uiCMD(ui_run_over);
					});
					runStop = self->connect_msg_notifer_to(runActor);
					runActor->notify_run();
				}
				else
				{

				}
				break;
			case ui_run_over:
				if (runActor)
				{
					self->actor_wait_quit(runActor);
					runActor.reset();
					runStop.clear();
				}
				break;
			case ui_close:
				if (runActor)
				{
					runStop();
					self->actor_wait_quit(runActor);
				}

				bool sure = false;
				BEGIN_ACTOR_RUN_IN_WX(_ios);
				m_textCtrl1->SetLabelText("delay over 3");
				self->sleep(500);
				m_textCtrl1->SetLabelText("delay over 2");
				self->sleep(500);
				m_textCtrl1->SetLabelText("delay over 1");
				self->sleep(500);
				m_textCtrl1->SetLabelText("delay over 0");
				self->sleep(500);
				sure = wxMessageBox("关闭?", "", wxYES_NO) == wxYES;
				END_ACTOR_RUN_IN_WX();
				if (!sure)
				{
					BEGIN_RUN_IN_WX();
					wx_ui wxTest(_ios);
					wxTest.ShowModal();
					END_RUN_IN_WX();
				}
				RUN_IN_WX(wx_close());
				return;
			}
		}
	});
	_uiCMD = _mainActor->connect_msg_notifer<ui_cmd>();
	_mainActor->notify_run();
}

wx_ui::~wx_ui()
{

}

//////////////////////////////////////////////////////////////////////////
cancel_ui::cancel_ui(wxWindow* parent)
:MyDialog2(parent)
{
	_cancel = false;
}

void cancel_ui::btn_cancel(wxCommandEvent& event)
{
	_cancel = true;
	Close();
	event.Skip();
}

bool cancel_ui::isCancel()
{
	return _cancel;
}
