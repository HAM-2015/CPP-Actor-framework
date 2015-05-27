// actor_test.cpp : 定义控制台应用程序的入口点。
//

#include "ios_proxy.h"
#include "actor_framework.h"
#include "async_buffer.h"
#include "scattered.h"
#include <list>
#include <Windows.h>

using namespace std;

void check_key_down(my_actor* self, int id)
{//检测按键按下
	while (GetAsyncKeyState(id) >= 0)
	{
		self->sleep(1);
	}
}

void check_key_up(my_actor* self, int id)
{//检测按键弹起
	while (GetAsyncKeyState(id) < 0)
	{
		self->sleep(1);
	}
}

void check_key_test(my_actor* self, int id)
{
	while (true)
	{//按键按下后，检测弹起，1000ms内没有弹起就是超时错误
		check_key_down(self, id);//检测按下
		child_actor_handle checkUp = self->create_child_actor([&](my_actor* self){check_key_up(self, id); }, 24 kB);//创建一个检测弹起的子Actor
		self->child_actor_run(checkUp);//开始运行子Actor
		self->delay_trig(1000, [&]{checkUp.get_actor()->notify_quit(); });//启用弹起超时处理，超时后强制关闭checkUp
		if (self->child_actor_wait_quit(checkUp))//正常退出的返回true，被强制关闭的返回false
		{
			self->cancel_delay_trig();
			printf("ok %d\n", id);
		} 
		else
		{//弹起超时
			printf("timeout %d\n", id);
			check_key_up(self, id);//检测弹起
		}
	}
}

void actor_print(my_actor* self, int id)
{//按下后先打印一个字符，500ms后每隔50ms打印一次
	printf("_");
	self->sleep(500);
	while (true)
	{
		printf("-");
		self->sleep(50);
	}
}

void actor_test_print(my_actor* self, int id)
{
	while (true)
	{
		check_key_down(self, id);//检测按下
		child_actor_handle ch = self->create_child_actor(boost::bind(&actor_print, _1, id));
		self->child_actor_run(ch);
		check_key_up(self, id);//检测弹起
		self->child_actor_force_quit(ch);
	}
}

void actor_suspend(my_actor* self, const list<actor_handle>& chs)
{
	while (true)
	{
		check_key_down(self, VK_RBUTTON);
		self->actors_suspend(chs);
		check_key_up(self, VK_RBUTTON);
	}
}

void actor_resume(my_actor* self, const list<actor_handle>& chs)
{
	while (true)
	{
		check_key_down(self, VK_LBUTTON);
		self->actors_resume(chs);
		check_key_up(self, VK_LBUTTON);
	}
}

void check_both_up(my_actor* self, int id, bool& own, const bool& another)
{
	goto __start;
	while (!another)
	{
		self->sleep(1);
		own = false;
__start:
		check_key_up(self, id);
		own = true;
	}
}

void check_two_down(my_actor* self, int dt, int id1, int id2)
{
	while (true)
	{
		{//检测两个按键是否都处于弹起状态
			bool st1 = false;
			bool st2 = false;
			child_actor_handle up1 = self->create_child_actor([&](my_actor* self){check_both_up(self, id1, st1, st2); });
			child_actor_handle up2 = self->create_child_actor([&](my_actor* self){check_both_up(self, id2, st2, st1); });
			actor_trig_handle<bool> ath;
			auto nh = self->make_trig_notifer(ath);
			up1.get_actor()->append_quit_callback(nh);
			up2.get_actor()->append_quit_callback(nh);
			self->child_actor_run(up1);
			self->child_actor_run(up2);
			LAMBDA_REF3(ref3, self, up1, up2);
			if (!self->timed_wait_trig(3000, ath, [&ref3](bool)
			{
				ref3.self->child_actor_force_quit(ref3.up1);
				ref3.self->child_actor_force_quit(ref3.up2);
			}))
			{
				printf("退出 check_two_down\n");
				self->child_actor_force_quit(up1);
				self->child_actor_force_quit(up2);
				self->close_trig_notifer(ath);
				break;
			}
		}
		{
			child_actor_handle check1 = self->create_child_actor([&](my_actor* self){check_key_down(self, id1); });
			child_actor_handle check2 = self->create_child_actor([&](my_actor* self){check_key_down(self, id2); });
			actor_trig_handle<actor_handle, bool> ath;
			auto nh = self->make_trig_notifer(ath);
			check1.get_actor()->append_quit_callback([&](bool ok){nh(check2.get_actor(), ok); });
			check2.get_actor()->append_quit_callback([&](bool ok){nh(check1.get_actor(), ok); });
			self->child_actor_run(check1);
			self->child_actor_run(check2);
			actor_handle ah;
			bool ok = false;
			self->wait_trig(ath, ah, ok);
			self->delay_trig(dt, [&]{ah->notify_quit(); });
			if (self->actor_wait_quit(ah))
			{
				printf("*success*\n");
			} 
			else
			{
				printf("*failure*\n");
			}
			self->cancel_delay_trig();
			self->child_actor_force_quit(check1);
			self->child_actor_force_quit(check2);
		}
	}
}

void wait_key(my_actor* self, int id, std::function<void (int)> cb)
{
	check_key_up(self, id);
	while (true)
	{
		check_key_down(self, id);
		cb(id);
		check_key_up(self, id);
	}
}

void shift_key(my_actor* self, actor_handle pauseactor)
{
	list<child_actor_handle::ptr> childs;
	actor_msg_handle<int> amh;
	auto h = self->make_msg_notifer(amh);
	for (int i = 'A'; i <= 'Z'; i++)
	{
		auto tp = child_actor_handle::make_ptr();
		*tp = self->create_child_actor(boost::bind(&wait_key, _1, i, h));
		self->child_actor_run(*tp);
		childs.push_back(tp);
	}
	while (true)
	{
		int id = self->wait_msg(amh);
		if ('P' == id)
		{
			bool isPause = self->actor_switch(pauseactor);
			printf("%s性能测试\n", isPause? "暂停": "恢复");
		}
		else
		{
			printf("shift+%c\n", id);
		}
	}
	self->close_msg_notifer(amh);
	self->child_actors_force_quit(childs);
}

void test_shift(my_actor* self, actor_handle pauseactor)
{
	child_actor_handle ch;
	while (true)
	{
		check_key_down(self, VK_SHIFT);
		ch = self->create_child_actor(boost::bind(&shift_key, _1, pauseactor));
		self->child_actor_run(ch);
		check_key_up(self, VK_SHIFT);
		self->child_actor_force_quit(ch);
	}
}

void perfor_test(my_actor* self, ios_proxy& ios)
{
	self->check_stack();
	vector<shared_strand> strands;
	strands.resize(ios.threadNumber());
	for (size_t i = 0; i < strands.size(); i++)
	{
		strands[i] = boost_strand::create(ios);
	}

	for (int n = 1; n < 200; n++)
	{
		int num = n*n;
		long long tk = get_tick_us();
		list<child_actor_handle::ptr> childList;
		vector<int> count;
		count.resize(num);
#ifdef _DEBUG
		size_t stackSize = DEFAULT_STACKSIZE;
#else
		size_t stackSize = 12 kB;
#endif
		for (int i = 0; i < num; i++)
		{
			count[i] = 0;
			auto newactor = child_actor_handle::make_ptr();
			*newactor = self->create_child_actor(strands[i%strands.size()], [&count, i](my_actor* self)
			{
				while (true)
				{
					count[i]++;
					self->sleep(0);
				}
			}, stackSize);
			childList.push_front(newactor);
			self->child_actor_run(*childList.front());
		}
		self->sleep(1000);
		self->child_actors_force_quit(childList);
		int ct = 0;
		for (int i = 0; i < num; i++)
		{
			ct += count[i];
		}
		double f = (double)ct * 1000000 / (get_tick_us()-tk);
		printf("Actor数=%d, 切换频率=%d\n", num, (int)f);
	}
	printf("性能测试结束\n");
}

void create_null_actor(my_actor* self, int* count)
{
	while (true)
	{
		size_t yieldTick = self->yield_count();
		list<child_actor_handle::ptr> childList;
		for (int i = 0; i < 100; i++)
		{
			(*count)++;
			auto newactor = child_actor_handle::make_ptr();
			*newactor = self->create_child_actor([](my_actor*){});
			childList.push_back(newactor);
		}
		self->child_actors_force_quit(childList);
		if (self->yield_count() == yieldTick)
		{
			self->yield();
		}
	}
}

void create_actor_test(my_actor* self)
{
	while (true)
	{
		int count = 0;
		child_actor_handle createactor = self->create_child_actor(boost::bind(&create_null_actor, _1, &count));
		self->child_actor_run(createactor);
		self->sleep(1000);
		self->child_actor_force_quit(createactor);
		printf("1秒创建Actor数=%d\n", count);
	}
}

void actor_test(my_actor* self)
{
	ios_proxy perforIos;//用于测试多线程下的Actor切换性能
	perforIos.run(ios_proxy::hardwareConcurrency());//调度器线程设置为CPU线程数
	perforIos.runPriority(ios_proxy::idle);//调度器优先级设置为最低
	child_actor_handle actorLeft = self->create_child_actor(boost::bind(&check_key_test, _1, VK_LEFT));
	child_actor_handle actorRight = self->create_child_actor(boost::bind(&check_key_test, _1, VK_RIGHT));
	child_actor_handle actorUp = self->create_child_actor(boost::bind(&check_key_test, _1, VK_UP));
	child_actor_handle actorDown = self->create_child_actor(boost::bind(&check_key_test, _1, VK_DOWN));
	child_actor_handle actorPrint = self->create_child_actor(boost::bind(&actor_test_print, _1, VK_SPACE));//检测空格键
	child_actor_handle actorTwo = self->create_child_actor(boost::bind(&check_two_down, _1, 10, 'D', 'F'));//检测D,F键是否同时按下(设置间隔误差10ms)
	child_actor_handle actorPerfor = self->create_child_actor(boost::bind(&perfor_test, _1, boost::ref(perforIos)));//Actor切换性能测试
	child_actor_handle actorCreate = self->create_child_actor(boost::bind(&create_actor_test, _1));//Actor创建性能测试
	child_actor_handle actorShift = self->create_child_actor(boost::bind(&test_shift, _1, actorPerfor.get_actor()));//shift+字母检测
	child_actor_handle actorProducer1;
	child_actor_handle actorProducer2;
	child_actor_handle actorConsumer;
	child_actor_handle actorMutex1;
	child_actor_handle actorMutex2;
	child_actor_handle actorMutex3;
	child_actor_handle actorConVarWait;
	child_actor_handle actorConVarNtf;
	child_actor_handle buffPush;
	child_actor_handle buffPop;
	//创建生产者/消费者模型测试
	{//模拟消息转发，转发切换
		actorConsumer = self->create_child_actor(self->self_strand()->clone(), [](my_actor* self)
		{//消费者
			auto agentRun = [](my_actor* self)
			{
				auto conCmh = self->connect_msg_pump<int, int>();
				int p0;
				int id;
				self->pump_msg(conCmh, p0, id);
				printf("数据:%d 发送者:%d 接收者:%d\n", p0, id, (int)self->self_id());
				while (true)
				{
					BEGIN_TRY_
					{
						self->pump_msg(conCmh, p0, id, true);
						printf("数据:%d 发送者:%d 接收者:%d\n", p0, id, (int)self->self_id());
					}
					CATCH_PUMP_DISCONNECTED
					{
						printf("接收者:%d 被断开\n", (int)self->self_id());//消息泵被父关闭，抛出异常
						self->pump_msg(conCmh, p0, id);
						printf("数据:%d 发送者:%d 接收者:%d\n", p0, id, (int)self->self_id());
					}
					END_TRY_;
				}
			};
			auto st = self->self_strand()->clone();
			child_actor_handle agent1 = self->create_child_actor(st, agentRun, 64 kB);
			child_actor_handle agent2 = self->create_child_actor(st, agentRun, 64 kB);
			self->child_actor_run(agent1);
			self->child_actor_run(agent2);
			while (true)
			{
				self->msg_agent_to<int, int>(agent1);//消息由agent1代理
				self->sleep(5000);
				self->msg_agent_to<int, int>(agent2);//断开agent1代理，切换到agent2代理
				self->sleep(2000);
				//self->msg_agent_off<int, int>();//断开agent2代理
				//self->sleep(200);
				auto conCmh = self->connect_msg_pump<int, int>();//停止消息代理，由自己处理
				for (int i = 0; i < 3; i++)
				{
					self->pump_msg<int, int>(conCmh, [self](int p0, int id)
					{
						printf("数据:%d 发送者:%d 接收者:%d\n", p0, id, (int)self->self_id());
					});
				}
			}
		});
		self->child_actor_run(actorConsumer);
		auto writer = self->connect_msg_notifer_to<int, int>(actorConsumer);//连接消息到actorConsumer
		auto test_producer = [writer](my_actor* self)
		{//生产者
			for (int i = 0; true; i++)
			{
				writer(i, (int)self->self_id());
				self->sleep(2000);
			}
		};
		actorProducer1 = self->create_child_actor(test_producer);
		actorProducer2 = self->create_child_actor(test_producer);
//		self->child_actor_run(actorProducer1);
//		self->child_actor_run(actorProducer2);
	}
	{
		actor_mutex amutex(self->self_strand());
		auto actorMutexH = [amutex](my_actor* self)
		{//模拟actor_mutex互斥特性
			while (true)
			{
				my_actor::quit_guard qg(self);//保护上锁期间不让Actor强制退出
				//actor_lock_guard lg(amutex, self);
				if (amutex.timed_lock(800, self))
				{
					for (int i = 0; i < 10 && !self->quit_msg(); i++)
					{
						printf("%d--%d\n", i, (int)self->self_id());
						self->sleep(100);
					}
					amutex.unlock(self);
				}
				else
				{
					printf("---\n");
				}
			}
		};
		actorMutex1 = self->create_child_actor(actorMutexH);
		actorMutex2 = self->create_child_actor(actorMutexH);
		actorMutex3 = self->create_child_actor(actorMutexH);
		//self->child_actor_run(actorMutex1);//模拟actor_mutex互斥特性
		//self->child_actor_run(actorMutex2);
		//self->child_actor_run(actorMutex3);
	}
	{//条件变量测试
		std::shared_ptr<bool> waiting(new bool(false));
		actor_mutex amutex(self->self_strand());
		actor_condition_variable aconVar(self->self_strand());
		actorConVarWait = self->create_child_actor([waiting, amutex, aconVar](my_actor* self)
		{
			while (true)
			{
				actor_lock_guard lg(amutex, self);
				assert(!*waiting);
				*waiting = true;
				if (aconVar.timed_wait(90, self, lg))
				{
					printf("notify\n");
				}
				else
				{
					*waiting = false;
					printf("tm\n");
				}
			}
		});
		actorConVarNtf = self->create_child_actor([&](my_actor* self)
		{
			while (true)
			{
				{
					actor_lock_guard lg(amutex, self);
					if (*waiting)
					{
						*waiting = false;
						aconVar.notify_one(self);
					}
				}
				self->sleep(100);
			}
		});
		//self->child_actor_run(actorConVarWait);
		//self->child_actor_run(actorConVarNtf);
	}
	async_buffer<passing_test> abuff(2, self->self_strand());
	{//异步队列测试
		buffPush = self->create_child_actor(self->self_strand(), [&](my_actor* self)
		{
			int i = 0;
			try
			{
				while (true)
				{
					abuff.push(self, passing_test(i++));
				}
			}
			catch (async_buffer<int>::close_exception)
			{
				printf("--\n");
			}
		});
		buffPop = self->create_child_actor(self->self_strand(), [&](my_actor* self)
		{
			int i = 0;
			try
			{
				while (true)
				{
					passing_test id = abuff.pop(self);
					printf("%d\n", id._count->_id);
					self->sleep(1000);
				}
			}
			catch (async_buffer<int>::close_exception)
			{
				printf("!--\n");
			}
		});
		//self->child_actor_run(buffPush);
		//self->child_actor_run(buffPop);
	}
	list<actor_handle> chs;//需要被挂起的Actor对象，可以从下方注释几个测试
	chs.push_back(actorLeft.get_actor());
	chs.push_back(actorRight.get_actor());
	chs.push_back(actorUp.get_actor());
	chs.push_back(actorDown.get_actor());
	chs.push_back(actorPrint.get_actor());
	chs.push_back(actorShift.get_actor());
	chs.push_back(actorTwo.get_actor());
//	chs.push_back(actorConsumer.get_actor());
//	chs.push_back(actorPerfor.get_actor());
	chs.push_back(actorProducer1.get_actor());
	chs.push_back(actorProducer2.get_actor());
	chs.push_back(actorCreate.get_actor());
	child_actor_handle actorSuspend = self->create_child_actor(boost::bind(&actor_suspend, _1, boost::ref(chs)), 32 kB);//点击鼠标右键暂停按键检测
	child_actor_handle actorResume = self->create_child_actor(boost::bind(&actor_resume, _1, boost::ref(chs)), 32 kB);//点击鼠标左键恢复按键检测
	self->child_actor_run(actorLeft);
	self->child_actor_run(actorRight);
	self->child_actor_run(actorUp);
	self->child_actor_run(actorDown);
	self->child_actor_run(actorPrint);
	self->child_actor_run(actorShift);
	self->child_actor_run(actorTwo);
	self->child_actor_run(actorPerfor);
//	self->child_actor_run(actorCreate);
	self->child_actor_run(actorSuspend);
	self->child_actor_run(actorResume);
	self->child_actor_suspend(actorPerfor);
	check_key_down(self, VK_ESCAPE);//ESC键退出
	self->child_actor_force_quit(actorLeft);
	self->child_actor_force_quit(actorRight);
	self->child_actor_force_quit(actorUp);
	self->child_actor_force_quit(actorDown);
	self->child_actor_force_quit(actorPrint);
	self->child_actor_force_quit(actorShift);
	self->child_actor_force_quit(actorTwo);
	self->child_actor_force_quit(actorPerfor);
	self->child_actor_force_quit(actorConsumer);
	self->child_actor_force_quit(actorProducer1);
	self->child_actor_force_quit(actorProducer2);
	self->child_actor_force_quit(actorCreate);
	self->child_actor_force_quit(actorSuspend);
	self->child_actor_force_quit(actorResume);
	actorMutex1.get_actor()->notify_quit();
	actorMutex2.get_actor()->notify_quit();
	actorMutex3.get_actor()->notify_quit();
	self->child_actor_wait_quit(actorMutex1);
	self->child_actor_wait_quit(actorMutex2);
	self->child_actor_wait_quit(actorMutex3);
	self->child_actor_force_quit(actorConVarWait);
	self->child_actor_force_quit(actorConVarNtf);
	abuff.close(self);
	self->child_actor_force_quit(buffPush);
	self->child_actor_force_quit(buffPop);
	perforIos.stop();
}


	/*
	逻辑控制测试程序
	上下左右方向键检测，按下后1000ms内弹起打印ok，否则打印timeout;
	空格键检测，按下后打印 _ ，500ms内没有弹起，每隔50ms打印 - ;
	D,F键检测，10ms内两个键"同时"按下(必须"同时"，而不是一前一后)打印success，否则打印failure;
	鼠标按键检测，右键按下挂起动作，左键按下恢复动作;
	生产者/消费者模型测试;
	ESC键退出;
	注意：某些键盘可能不支持2个以上按键同时操作
	*/
int main(int argc, char* argv[])
{
	enable_high_resolution();
	my_actor::enable_stack_pool();
	ios_proxy ios;
	ios.run();
	{
		actor_handle actorTest = my_actor::create(boost_strand::create(ios), boost::bind(&actor_test, _1));
		actorTest->notify_run();
		actorTest->outside_wait_quit();
	}
 	ios.stop();
	return 0;
}
