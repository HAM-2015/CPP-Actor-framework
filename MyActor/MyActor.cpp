#include <iostream>
#include "./actor/my_actor.h"
#include "./actor/async_buffer.h"
#include "./actor/actor_socket.h"
#include "./actor/async_timer.h"
#include "./actor/msg_queue.h"
#include "./actor/sync_msg.h"
#include "./actor/trace.h"

void wait_multi_msg()
{
	trace_line("begin wait_multi_msg");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		csp_invoke<void(move_test)> csp(self->self_strand());
		async_buffer<move_test> buf(self->self_strand(), 1);
		actor_handle act1 = my_actor::create(self->self_strand(), [&](my_actor* self)
		{
			my_actor::quit_guard qg(self);
			trig_handle<> cspAth;
			trig_handle<> bufAth;
			csp.regist_take_ntf(self, self->make_trig_notifer_to_self(cspAth));
			buf.regist_pop_ntf(self, self->make_trig_notifer_to_self(bufAth));
			self->run_mutex_blocks_safe(mutex_block_trig<>(cspAth, [&]()
			{
				trace_comma(self->self_id(), "begin wait csp");
				csp.wait_invoke(self, [&](const move_test& mt)
				{
					trace_comma(self->self_id(), "csp msg ", mt);
				});
				trace_comma(self->self_id(), "end wait csp");
				csp.regist_take_ntf(self, self->make_trig_notifer_to_self(cspAth));
				return false;
			}), mutex_block_trig<>(bufAth, [&]()
			{
				trace_comma(self->self_id(), "begin wait buf");
				move_test mt = buf.pop(self);
				trace_comma(self->self_id(), "buf msg ", mt);
				trace_comma(self->self_id(), "end wait buf");
				buf.regist_pop_ntf(self, self->make_trig_notifer_to_self(bufAth));
				return false;
			}), mutex_block_pump_check_state<int>(self, [&](int msg)
			{
				trace_comma(self->self_id(), "begin msg int");
				self->sleep(500);
				trace_comma(self->self_id(), msg);
				self->sleep(500);
				trace_comma(self->self_id(), "end msg int");
				return false;
			}, [](pump_check_state s)
			{
				trace_line("msg int state ", s);
				return false;
			}), mutex_block_pump<move_test>(self, [&](const move_test& msg)
			{
				trace_comma(self->self_id(), "begin msg int");
				self->sleep(500);
				trace_comma(self->self_id(), msg);
				self->sleep(500);
				trace_comma(self->self_id(), "end msg int");
				return false;
			}), mutex_block_quit(self, [&]()
			{
				msg_pump_handle<int> pump1 = self->connect_msg_pump<int>(true);
				msg_pump_handle<move_test> pump2 = self->connect_msg_pump<move_test>(true);
				while (true)
				{
					try
					{
						trace_comma(self->self_id(), "lost msg", self->pump_msg(pump1));
					}
					catch (ntf_lost_exception&)
					{
						break;
					}
				}
				while (true)
				{
					try
					{
						trace_comma(self->self_id(), "lost msg", self->pump_msg(pump2));
					}
					catch (ntf_lost_exception&)
					{
						break;
					}
				}
				trace_comma(self->self_id(), "begin end");
				self->sleep(1000);
				return true;
			}));
		});
		child_handle ch2 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				buf.push(self, move_test(i));
				self->sleep(300);
			}
			auto ntf = self->connect_msg_notifer_to<int>(act1, true);
			if (ntf)
			{
				self->sleep(1000);
				for (int i = 0; i < 3; i++)
				{
					ntf(i);
					self->sleep(500);
				}
			}
		});
		child_handle ch3 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				csp.invoke(self, move_test(i));
				self->sleep(1000);
			}
			auto ntf = self->connect_msg_notifer_to<move_test>(act1, true);
			if (ntf)
			{
				for (int i = 0; i < 3; i++)
				{
					ntf(move_test(i));
					self->sleep(500);
				}
			}
		});
		act1->run();
		self->child_run(ch2, ch3);
		self->child_wait_quit(ch2, ch3);
		act1->force_quit();
		self->actor_wait_quit(act1);
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end wait_multi_msg");
}

void async_buffer_test()
{
	trace_line("begin async_buffer_test");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		async_buffer<move_test> asyncMsg(self->self_strand(), 1);
		child_handle ch1 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				self->sleep(1000);
				trace_comma(self->self_id(), asyncMsg.pop(self));
			}
		});
		child_handle ch2 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				trace_comma(self->self_id(), "begin push", i);
				asyncMsg.push(self, move_test(i));
				trace_comma(self->self_id(), "push ok", i);
			}
		});
		self->child_run(ch1, ch2);
		self->child_wait_quit(ch1, ch2);
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end async_buffer_test");
}

void sync_msg_test()
{
	trace_line("begin sync_msg_test");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		sync_msg<move_test> syncMsg(self->self_strand());
		child_handle ch1 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				self->sleep(1000);
				trace_comma(self->self_id(), syncMsg.take(self));
			}
		});
		child_handle ch2 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				trace_comma(self->self_id(), "begin send", i);
				syncMsg.send(self, move_test(i));
				trace_comma(self->self_id(), "send ok", i);
			}
		});
		self->child_run(ch1, ch2);
		self->child_wait_quit(ch1, ch2);
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end sync_msg_test");
}

void csp_test()
{
	trace_line("begin csp_test");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		csp_invoke<int(move_test&, bool)> csp(self->self_strand());
		child_handle ch1 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 6; i++)
			{
				self->sleep(1000);
				csp.wait_invoke(self, [&](move_test& msg, bool rval)->int
				{
					trace_comma(self->self_id(), "csp msg", msg, "rval", rval);
					return -i;
				});
			}
		});
		child_handle ch2 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				int r = csp.invoke_rval(self, move_test(i));
				trace_comma(self->self_id(), "csp return ", r);
			}
			for (int i = 0; i < 3; i++)
			{
				move_test t(i);
				int r = csp.invoke_rval(self, t);
				trace_comma(self->self_id(), "csp return ", r);
			}
		});
		self->child_run(ch1, ch2);
		self->child_wait_quit(ch1, ch2);
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end csp_test");
}

void mutex_test()
{
	trace_line("begin mutex_test");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		actor_mutex mtx(self->self_strand());
		child_handle ch1 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 10; i++)
			{
				mtx.lock(self);
				trace_comma(self->self_id(), "A", "mutex in");
				trace_comma(self->self_id(), "A", i, 0);
				self->sleep(100);
				trace_comma(self->self_id(), "A", i, 1);
				self->sleep(100);
				trace_comma(self->self_id(), "A", i, 2);
				self->sleep(100);
				trace_comma(self->self_id(), "A", "mutex out");
				mtx.unlock(self);
			}
		});
		child_handle ch2 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 10; i++)
			{
				mtx.lock(self);
				trace_comma(self->self_id(), "B", "mutex in");
				trace_comma(self->self_id(), "B", i, 0);
				self->sleep(100);
				trace_comma(self->self_id(), "B", i, 1);
				self->sleep(100);
				trace_comma(self->self_id(), "B", i, 2);
				self->sleep(100);
				trace_comma(self->self_id(), "B", "mutex out");
				mtx.unlock(self);
			}
		});
		self->child_run(ch1, ch2);
		self->child_wait_quit(ch1, ch2);
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end mutex_test");
}

void shared_mutex_test()
{
	trace_line("begin shared_mutex_test");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		actor_shared_mutex smtx(self->self_strand());
		child_handle ch1 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 10; i++)
			{
				trace_comma(self->self_id(), "A-", "mutex in");
				smtx.lock(self);
				trace_comma(self->self_id(), "A-", ">mutex in");
				trace_comma(self->self_id(), "A-", i, 0);
				self->sleep(10);
				trace_comma(self->self_id(), "A-", i, 1);
				self->sleep(10);
				trace_comma(self->self_id(), "A-", i, 2);
				self->sleep(10);
				trace_comma(self->self_id(), "A-", "mutex out");
				smtx.unlock(self);
				self->sleep(10);
			}
		});
		child_handle ch2 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 10; i++)
			{
				trace_comma(self->self_id(), "A+", "mutex in");
				smtx.lock(self);
				trace_comma(self->self_id(), "A+", ">mutex in");
				trace_comma(self->self_id(), "A+", i, 0);
				self->sleep(10);
				trace_comma(self->self_id(), "A+", i, 1);
				self->sleep(10);
				trace_comma(self->self_id(), "A+", i, 2);
				self->sleep(10);
				trace_comma(self->self_id(), "A+", i, 3);
				self->sleep(10);
				trace_comma(self->self_id(), "A+", i, 4);
				self->sleep(10);
				trace_comma(self->self_id(), "A+", i, 5);
				self->sleep(10);
				trace_comma(self->self_id(), "A+", "mutex out");
				smtx.unlock(self);
				self->sleep(10);
			}
		});
		child_handle ch3 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 10; i++)
			{
				trace_comma(self->self_id(), "B", "shared_mutex in");
				smtx.lock_shared(self);
				trace_comma(self->self_id(), "B", ">shared_mutex in");
				trace_comma(self->self_id(), "B", i, 0);
				self->sleep(10);
				trace_comma(self->self_id(), "B", i, 1);
				self->sleep(10);
				trace_comma(self->self_id(), "B", i, 2);
				self->sleep(10);
				trace_comma(self->self_id(), "B-", "--upgrade_mutex in");
				smtx.lock_upgrade(self);
				trace_comma(self->self_id(), "B-", ">upgrade_mutex in");
				trace_comma(self->self_id(), "B-", i, 3);
				self->sleep(10);
				trace_comma(self->self_id(), "B-", i, 4);
				self->sleep(10);
				trace_comma(self->self_id(), "B-", i, 5);
				self->sleep(10);
				trace_comma(self->self_id(), "B-", i, 6);
				self->sleep(10);
				trace_comma(self->self_id(), "B-", "-upgrade_mutex out");
				smtx.unlock_upgrade(self);
				trace_comma(self->self_id(), "B-", ">-upgrade_mutex out");
				trace_comma(self->self_id(), "B", i, 7);
				self->sleep(10);
				trace_comma(self->self_id(), "B", i, 8);
				self->sleep(10);
				trace_comma(self->self_id(), "B", i, 9);
				self->sleep(10);
				trace_comma(self->self_id(), "B", "shared_mutex out");
				smtx.unlock_shared(self);
				self->sleep(10);
			}
		});
		child_handle ch4 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 10; i++)
			{
				trace_comma(self->self_id(), "C", "shared_mutex in");
				smtx.lock_shared(self);
				trace_comma(self->self_id(), "C", ">shared_mutex in");
				trace_comma(self->self_id(), "C", i, 0);
				self->sleep(10);
				trace_comma(self->self_id(), "C", i, 1);
				self->sleep(10);
				trace_comma(self->self_id(), "C-", "--upgrade_mutex in");
				smtx.lock_upgrade(self);
				trace_comma(self->self_id(), "C-", ">--upgrade_mutex in");
				trace_comma(self->self_id(), "C-", i, 2);
				self->sleep(10);
				trace_comma(self->self_id(), "C-", i, 3);
				self->sleep(10);
				trace_comma(self->self_id(), "C-", i, 4);
				self->sleep(10);
				trace_comma(self->self_id(), "C-", "-upgrade_mutex out");
				smtx.unlock_upgrade(self);
				trace_comma(self->self_id(), "C-", ">-upgrade_mutex out");
				trace_comma(self->self_id(), "C", i, 5);
				self->sleep(10);
				trace_comma(self->self_id(), "C", i, 6);
				self->sleep(10);
				trace_comma(self->self_id(), "C", i, 7);
				self->sleep(10);
				trace_comma(self->self_id(), "C", i, 8);
				self->sleep(10);
				trace_comma(self->self_id(), "C", i, 9);
				self->sleep(10);
				trace_comma(self->self_id(), "C", "shared_mutex out");
				smtx.unlock_shared(self);
				self->sleep(10);
			}
		});
		self->child_run(ch1, ch2, ch3, ch4);
		self->child_wait_quit(ch1, ch2, ch3, ch4);
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end shared_mutex_test");
}

void agent_test()
{
	trace_line("begin agent_test");
	io_engine ios;
	ios.run(4);
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		child_handle ch = self->create_child(self->self_strand()->clone(), [](my_actor* self)
		{
			child_handle ch1 = self->create_child(self->self_strand()->clone(), [](my_actor* self)
			{
				msg_pump_handle<move_test> pp = self->connect_msg_pump<move_test>(true);
				try
				{
					self->wait_connect(pp);
					while (true)
					{
						trace_comma(self->self_id(), self->pump_msg(true, pp));
					}
				}
				catch (ntf_lost_exception&)
				{
					trace_comma(self->self_id(), "notifer lost");
				}
				catch (pump_disconnected_exception&)
				{
					trace_comma(self->self_id(), "pump disconnected");
				}
			});
			self->child_run(ch1);
			self->msg_agent_to<move_test>(ch1);
			self->sleep(1000);
			msg_pump_handle<move_test> pp = self->connect_msg_pump<move_test>(true);
			try
			{
				while (true)
				{
					trace_comma(self->self_id(), self->pump_msg(true, pp));
				}
			}
			catch (ntf_lost_exception&)
			{
				trace_comma(self->self_id(), "notifer lost");
			}
			self->child_wait_quit(ch1);
		});
		self->child_run(ch);
		{
			auto ntf = self->connect_msg_notifer_to<move_test>(ch, true);
			for (int i = 0; i < 15; i++)
			{
				ntf(move_test(i));
				self->sleep(100);
			}
		}
		self->child_wait_quit(ch);
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end agent_test");
}

void pump_test()
{
	trace_line("begin pump_test");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		child_handle ch = self->create_child([&](my_actor* self)
		{
			msg_pump_handle<move_test> pp = self->connect_msg_pump<move_test>(true);
			try
			{
				while (true)
				{
					trace_comma(self->self_id(), self->pump_msg(pp));
				}
			}
			catch (ntf_lost_exception&)
			{
				trace_comma(self->self_id(), "notifer lost");
			}
		});
		self->child_run(ch);
		{
			auto ntf = self->connect_msg_notifer_to<move_test>(ch, true);
			for (int i = 0; i < 3; i++)
			{
				ntf(move_test(i));
				self->sleep(1000);
			}
		}
		self->child_wait_quit(ch);
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end pump_test");
}

void msg_test()
{
	trace_line("begin msg_test");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		msg_handle<move_test> amh;
		child_handle ch = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				trace_comma(self->self_id(), self->wait_msg(amh));
			}
		});
		auto ntf = self->make_msg_notifer_to(ch, amh);
		self->child_run(ch);
		for (int i = 0; i < 3; i++)
		{
			ntf(move_test(i));
			self->sleep(1000);
		}
		self->child_wait_quit(ch);
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end msg_test");
}

void trig_test()
{
	trace_line("begin trig_test");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		trig_handle<move_test> ath;
		child_handle ch = self->create_child([&](my_actor* self)
		{
			trace_comma(self->self_id(), "waiting trig");
			move_test msg = self->wait_trig(ath);
			trace_comma(self->self_id(), "trig ok", msg);
		});
		auto trig = self->make_trig_notifer_to(ch, ath);
		self->child_run(ch);
		trace_comma(self->self_id(), "begin trig");
		self->sleep(500);
		trace_comma(self->self_id(), "begin trig.");
		self->sleep(500);
		trace_comma(self->self_id(), "begin trig..");
		self->sleep(500);
		trace_comma(self->self_id(), "begin trig...");
		self->sleep(500);
		trace_comma(self->self_id(), "begin triged!!!");
		trig(move_test(123));
		self->child_wait_quit(ch);
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end trig_test");
}

void socket_test()
{
	trace_line("begin socket_test");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		child_handle srv = self->create_child([&](my_actor* self)
		{
			tcp_acceptor acc(self->self_io_service());
			if (!acc.open("127.0.0.1", 1234))
			{
				trace_line("server port conflict");
				return;
			}
			tcp_socket sck(self->self_io_service());
			bool timed = false;
			if (acc.timed_accept(self, 1500, timed, sck))
			{
				trace_line("new client connected");
				acc.close();
				char buf[128];
				while (true)
				{
					bool timed = false;
					tcp_socket::result res = sck.timed_read_some(self, 2000, timed, buf, sizeof(buf)-1);
					if (res.ok)
					{
						buf[res.s] = 0;
						trace_comma(self->self_id(), "received", buf);
					} 
					else if (timed)
					{
						trace_comma(self->self_id(), "receive timeout");
						break;
					}
					else
					{
						trace_comma(self->self_id(), "client disconnect");
						break;
					}
				}
			}
			sck.close();
		});
		child_handle cli = self->create_child([&](my_actor* self)
		{
			tcp_socket sck(self->self_io_service());
			if (sck.connect(self, "127.0.0.1", 1234))
			{
				char buf[128];
				for (int i = 0; i < 10; i++)
				{
					int l = snPrintf(buf, sizeof(buf), "msg %d", i);
					if (!sck.write(self, buf, l).ok)
					{
						break;
					}
					self->sleep(1000);
				}
				self->sleep(2000);
			}
			sck.close();
		});
		self->child_run(srv);
		self->sleep(1000);
		self->child_run(cli);
		self->child_wait_quit(srv, cli);
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end socket_test");
}

void udp_test()
{
	trace_line("begin udp_test");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		child_handle sender = self->create_child([](my_actor* self)
		{
			self->sleep(100);
			udp_socket udp(self->self_io_service());
			udp.connect(self, "127.0.0.1", 1234);
			char buf[128];
			for (int i = 0; i < 3; i++)
			{
				int l = snPrintf(buf, sizeof(buf), "udp msg %d", i);
				udp.send(self, buf, l);
				self->sleep(1000);
			}
			udp.close();
		});
		child_handle receiver = self->create_child([](my_actor* self)
		{
			udp_socket udp(self->self_io_service());
			udp.open_bind_v4(1234);
			char buf[128];
			for (int i = 0; i < 3; i++)
			{
				udp_socket::result res = udp.receive(self, buf, sizeof(buf));
				if (!res.ok)
				{
					break;
				}
				buf[res.s] = 0;
				trace_line(buf);
			}
			udp.close();
		});
		self->child_run(sender, receiver);
		self->child_wait_quit(sender, receiver);
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end udp_test");
}

void perfor_test()
{
	trace_line("begin perfor_test");
	io_engine ios;
	ios.run(8);
	actor_handle ah = my_actor::create(boost_strand::create(ios), [&](my_actor* self)
	{
		self->check_stack();
		std::vector<shared_strand> strands = boost_strand::create_multi(ios.threadNumber(), ios);
		for (int n = 1; n < 200; n++)
		{
			int num = n*n;
			std::list<child_handle> childList;
			std::vector<int> count(num);
			for (int i = 0; i < num; i++)
			{
				count[i] = 0;
				childList.push_front(self->create_child(strands[i%strands.size()], [&count, i](my_actor* self)
				{
					while (true)
					{
						count[i]++;
						self->tick_yield();
					}
				}));
			}
			long long tk = get_tick_us();
			self->children_run(childList);
			self->sleep(1000);
			self->children_force_quit(childList);
			int ct = 0;
			for (int i = 0; i < num; i++)
			{
				ct += count[i];
			}
			double f = (double)ct * 1000000 / (get_tick_us() - tk);
			trace_line("Actor number=", num, ", ", "switching frequency=", (int)f);
		}
	});
	ah->run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end perfor_test");
}

void async_timer_test()
{
	trace_line("begin async_timer_test");
	io_engine ios;
	ios.run();
	{
		shared_strand strand = boost_strand::create(ios);
		async_timer timer1 = strand->make_timer();
		async_timer timer2 = strand->make_timer();
		async_timer timer3 = strand->make_timer();
		strand->post([=]
		{
			const shared_strand& lockStrand = strand;
			timer1->timeout(500, [timer1]
			{
				info_trace_line("timer1 ", 1);
				timer1->timeout(1000, [timer1]
				{
					info_trace_line("timer1 ", 2);
					timer1->timeout(1000, []
					{
						info_trace_line("timer1 ", 3);
					});
				});
			});
			timer2->timeout(1000, [timer2]
			{
				info_trace_line("timer2 ", 1);
				timer2->timeout(1000, [timer2]
				{
					info_trace_line("timer2 ", 2);
					timer2->timeout(1000, []
					{
						info_trace_line("timer2 ", 3);
					});
				});
			});
			timer3->timeout(1500, [timer3]
			{
				info_trace_line("timer3 ", 1);
				timer3->timeout(1000, [timer3]
				{
					info_trace_line("timer3 ", 2);
					timer3->timeout(1000, []
					{
						info_trace_line("timer3 ", 3);
					});
				});
			});
		});
	}
	ios.stop();
	trace_line("end async_timer_test");
}

void create_child_test()
{
	trace_line("begin create_child_test");
	io_engine ios;
	ios.run();
	int count = 0;
	BEGIN_RECURSIVE_ACTOR(createActor, self);
	if (++count < 100)
	{
		info_trace_line("create_child ", count);
		child_handle child1 = self->create_child([&](my_actor* self)
		{
			self->sleep(1000);
			createActor(self);
		});
		child_handle child2 = self->create_child([&](my_actor* self)
		{
			self->sleep(1000);
			createActor(self);
		});
		self->child_run(child1, child2);
		self->child_wait_quit(child1, child2);
	}
	self->after_exit_clean_stack();
	END_RECURSIVE_ACTOR(createActor);
	my_actor::create(boost_strand::create(ios), wrap_ref_handler(createActor))->run();
	ios.stop();
	trace_line("end create_child_test");
}

void suspend_test()
{
	trace_line("begin suspend_test");
	io_engine ios;
	ios.run();
	go(ios)[](my_actor* self)
	{
		child_handle child = self->create_child([](my_actor* self)
		{
			child_handle child = self->create_child([](my_actor* self)
			{
				int i = 0;
				self->sleep(300);
				info_trace_comma("---child2", i++);
				info_trace_line("---lock_suspend");
				self->lock_suspend();
				for (int j = 0; j < 10; j++)
				{
					self->sleep(100);
					info_trace_comma("---child2", i++);
				}
				info_trace_line("---begin_unlock_suspend");
				self->unlock_suspend();
				info_trace_line("---end_unlock_suspend");
				info_trace_comma("---child2", i++);
				self->sleep(100);
			});
			self->child_run(child);
			int i = 0;
			self->sleep(100);
			info_trace_comma("--child1", i++);
			self->sleep(300);
			info_trace_comma("--child1", i++);
			self->sleep(300);
			info_trace_comma("--child1", i++);
			self->child_wait_quit(child);
		});
		self->child_run(child);
		self->sleep(500);
		info_trace_line("-begin suspend");
		self->child_suspend(child);
		info_trace_line("-end suspend");
		self->sleep(1500);
		info_trace_line("-begin resume");
		self->child_resume(child);
		info_trace_line("-end resume");
		self->child_wait_quit(child);
	};
	ios.stop();
	trace_line("end suspend_test");
}

void auto_stack_test()
{
	trace_line("begin auto_stack_test");
	io_engine ios;
	ios.run();
	for (int i = 0; i < 3; i++)
	{
		actor_handle ah = my_actor::create(boost_strand::create(ios), auto_stack_[i](my_actor* self)
		{
			int count = 0;
			HEARTBEAT_EXEC(self, 300, trace_space("heartbeat", i, count++));
			self->sleep(1000);
		});
		ah->run();
		ah->outside_wait_quit();
		trace_line("stack size:", ah->stack_size(), ", using size:", ah->using_stack_size());
	}
	ios.stop();
	trace_line("end auto_stack_test");
}

void go_test()
{
	io_engine ios;
	ios.run();

	go(ios) [](my_actor* self)
	{
		trace_line("go");
		self->sleep(500);
		trace_line("go.");
		self->sleep(500);
		trace_line("go..");
		self->sleep(500);
		trace_line("go...");
	};

	ios.stop();
}

int main(int argc, char *argv[])
{
	init_my_actor();
	enable_high_resolution();
	go_test();
	trace("\n");
	auto_stack_test();
	trace("\n");
	suspend_test();
	trace("\n");
	create_child_test();
	trace("\n");
	async_timer_test();
	trace("\n");
	trig_test();
	trace("\n");
	msg_test();
	trace("\n");
	pump_test();
	trace("\n");
	agent_test();
	trace("\n");
	mutex_test();
	trace("\n");
	shared_mutex_test();
	trace("\n");
	csp_test();
	trace("\n");
	sync_msg_test();
	trace("\n");
	async_buffer_test();
	trace("\n");
	socket_test();
	trace("\n");
	udp_test();
	trace("\n");
	wait_multi_msg();
	trace("\n");
// 	perfor_test();
// 	trace("\n");
	trace_line("end");
	getchar();
	return 0;
}