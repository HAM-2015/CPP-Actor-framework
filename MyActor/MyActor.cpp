#include <iostream>
#include "./actor/my_actor.h"
#include "./actor/actor_socket.h"
#include "./actor/async_timer.h"
#include "./actor/msg_queue.h"
#include "./actor/generator.h"
#include "./actor/channel.h"
#include "./actor/trace.h"

void wait_multi_msg()
{
	trace_line("begin wait_multi_msg");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		actor_handle act1 = my_actor::create(self->self_strand(), [&](my_actor* self)
		{
			my_actor::quit_guard qg(self);
			self->select_msg_blocks_safe(select_block_pump_check_state<int>(self, [&](int msg)
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
			}), select_block_pump<move_test>(self, [&](const move_test& msg)
			{
				trace_comma(self->self_id(), "begin msg int");
				self->sleep(500);
				trace_comma(self->self_id(), msg);
				self->sleep(500);
				trace_comma(self->self_id(), "end msg int");
				return false;
			}), select_block_quit(self, [&]()
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

void sync_msg_test()
{
	trace_line("begin sync_msg_test");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		nil_channel<move_test> syncMsg(self->self_strand());
		child_handle ch1 = self->create_child([&](my_actor* self)
		{
			move_test mt;
			for (int i = 0; i < 3; i++)
			{
				self->sleep(1000);
				syncMsg.take(self, mt);
				trace_comma(self->self_id(), mt);
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
		csp_channel<int(move_test, bool)> csp(self->self_strand());
		co_go(self->self_strand())[&](co_generator)
		{
			co_begin_context;
			int i;
			bool b;
			move_test mt;
			csp_result<int> res;
			co_use_state;
			co_end_context(ctx);
			
			co_begin;
			for (ctx.i = 0; ctx.i < 3; ctx.i++)
			{
				co_sleep(1000);
				co_csp_io(csp, ctx.res) >> co_chan_multi(ctx.mt, ctx.b);
				trace_comma("csp msg", ctx.mt, "generator", ctx.b);
				ctx.res.return_(-ctx.i);
			}
			co_end;
		};
		child_handle ch1 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				csp.wait(self, [&](move_test& msg, bool b)->int
				{
					self->sleep(1000);
					trace_comma(self->self_id(), "csp msg", msg, "generator", b);
					return -i;
				});
			}
		});
		co_go(self->self_strand())[&](co_generator)
		{
			co_begin_context;
			int i;
			int res;
			co_use_state;
			co_end_context(ctx);
			
			co_begin;
			for (ctx.i = 0; ctx.i < 3; ctx.i++)
			{
				co_sleep(100);
				co_csp_io(csp, ctx.res) << co_chan_multi(move_test(ctx.i), true);
				trace_comma("csp return ", ctx.res);
			}
			co_end;
		};
		child_handle ch2 = self->create_child([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				int r = csp.send(self, move_test(i), false);
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

void co_socket_test()
{
	trace_line("begin co_socket_test");
	io_engine ios;
	ios.run();
	shared_strand strand = boost_strand::create(ios);
	co_go(strand)[](co_generator)
	{
		co_begin_context;
		size_t s;
		char buf[128];
		bool overtime;
		boost::system::error_code ec;
		boost::asio::ip::tcp::socket socket;
		stack_obj<boost::asio::ip::tcp::acceptor> acceptor;
		co_use_timer;
		co_end_context_init(ctx, (co_self), socket(co_strand->get_io_service()), overtime(false), s(0));

		co_begin;
		try
		{
			ctx.acceptor.create(co_strand->get_io_service(), boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(), 1235), false);
		}
		catch (...) { co_stop; }
		co_await ctx.acceptor->async_accept(ctx.socket, co_async_result(ctx.ec));
		if (!ctx.ec)
		{
			ctx.acceptor->close(ctx.ec);
			while (true)
			{
				ctx.overtime = false;
				co_timed_await(1500, { ctx.overtime = true; ctx.socket.close(ctx.ec); }) ctx.socket.async_read_some(boost::asio::buffer(ctx.buf, sizeof(ctx.buf)), co_async_result(ctx.ec, ctx.s));
				if (ctx.overtime)
				{
					trace_comma("co_socket", "receive overtime");
					break;
				}
				else if (ctx.ec)
				{
					trace_comma("co_socket", "receive disconnect");
					break;
				}
				ctx.buf[ctx.s] = 0;
				trace_comma("co_socket", "received", ctx.buf);
			}
			ctx.socket.close(ctx.ec);
		}
		else
		{
			ctx.acceptor->close(ctx.ec);
		}
		co_end;
	};
	co_go(strand)[](co_generator)
	{
		co_begin_context;
		int i;
		size_t s;
		char buf[128];
		boost::system::error_code ec;
		boost::asio::ip::tcp::socket socket;
		co_end_context_init(ctx, (co_self), socket(co_strand->get_io_service()), s(0), i(0));

		co_begin;
		co_await ctx.socket.async_connect(boost::asio::ip::tcp::endpoint(
			boost::asio::ip::address::from_string("127.0.0.1"), 1235), co_async_result(ctx.ec));
		if (!ctx.ec)
		{
			for (ctx.i = 0; ctx.i < 10; ctx.i++)
			{
				co_await {
					int l = snprintf(ctx.buf, sizeof(ctx.buf), "msg %d", ctx.i);
					boost::asio::async_write(ctx.socket, boost::asio::buffer(ctx.buf, l), co_async_result(ctx.ec, ctx.s));
				}
				co_sleep(1000);
			}
			co_sleep(2000);
		}
		co_end;
	};
	ios.stop();
	trace_line("end co_socket_test");
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
			bool overtime = false;
			if (acc.timed_accept(self, 1500, overtime, sck))
			{
				trace_line("new client connected");
				acc.close();
				char buf[128];
				while (true)
				{
					bool overtime = false;
					tcp_socket::result res = sck.timed_read_some(self, 2000, overtime, buf, sizeof(buf)-1);
					if (res.ok)
					{
						buf[res.s] = 0;
						trace_comma(self->self_id(), "received", buf);
					} 
					else if (overtime)
					{
						trace_comma(self->self_id(), "receive overtime");
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
					int l = snprintf(buf, sizeof(buf), "msg %d", i);
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
				int l = snprintf(buf, sizeof(buf), "udp msg %d", i);
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
	ios.run(run_thread::cpu_thread_number());
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

void co_perfor_test()
{
	trace_line("begin co_perfor_test");
	io_engine ios;
	ios.run(run_thread::cpu_thread_number());
	std::vector<size_t> count(ios.threadNumber());
	std::vector<shared_strand> strands = boost_strand::create_multi(ios.threadNumber(), ios);
	std::list<generator_handle> gens;
	size_t num = 1000;
	for (size_t i = 0; i < ios.threadNumber(); i++)
	{
		for (size_t j = 0; j < num; j++)
		{
			generator_handle gen = co_go(strands[i])[&count, i](co_generator)
			{
				co_no_context;

				co_begin;
				while (true)
				{
					++count[i];
					co_tick;
				}
				co_end;
			};
			gens.push_back(gen);
		}
	}
	long long tk = get_tick_us();
	run_thread::sleep(3000);
	size_t ct = 0;
	for (size_t i = 0; i < count.size(); i++)
	{
		ct += count[i];
	}
	double f = (double)ct * 1000000 / (get_tick_us() - tk);
	while (!gens.empty())
	{
		gens.front()->stop();
		gens.pop_front();
	}
	trace_line("generator number=", ios.threadNumber()*num, ", ", "switching frequency=", (int)f);
	ios.stop();
	trace_line("end co_perfor_test");
}

void co_convar_test()
{
	trace_line("begin co_convar_test");
	io_engine ios;
	ios.run();
	bool sign = false;
	bool over = false;
	co_mutex mutex(boost_strand::create(ios));
	co_condition_variable conVar(boost_strand::create(ios));

	co_go(ios)[&](co_generator)
	{
		co_begin_context;
		int i;
		co_use_id;
		co_end_context(ctx);

		co_begin;
		for (ctx.i = 0; ctx.i < 3; ctx.i++)
		{
			co_sleep(500);
			co_mutex_lock_guard(mutex)
			{
				co_sleep(500);
				sign = true;
				trace_line("wait notify");
				co_convar_wait(conVar, mutex);
				trace_line("notified");
			}
		}
		co_mutex_lock_guard(mutex)
		{
			over = true;
		}
		co_end;
	};
	co_go(ios)[&](co_generator)
	{
		co_begin_context;
		int i;
		bool check;
		co_use_id;
		co_end_context(ctx);

		co_begin;
		ctx.check = true;
		while (ctx.check)
		{
			co_sleep(600);
			co_mutex_lock_guard(mutex)
			{
				if (!over)
				{
					if (sign)
					{
						sign = false;
						conVar.notify_one();
					}
				}
				else
				{
					ctx.check = false;
				}
			}
		}
		co_end;
	};

	ios.stop();
	trace_line("end co_convar_test");
}

void co_mutex_test()
{
	trace_line("begin co_mutex_test");
	io_engine ios;
	ios.run();
	co_mutex mutex(boost_strand::create(ios));
	co_go(ios)[&](co_generator)
	{
		co_begin_context;
		int i;
		co_use_id;
		co_end_context(ctx);

		co_begin;
		for (ctx.i = 0; ctx.i < 10; ctx.i++)
		{
			co_mutex_lock_guard(mutex)
			{
				info_trace_space("a", ctx.i);
				co_sleep(100);
				info_trace_space("a", ctx.i);
				co_sleep(100);
				info_trace_space("a", ctx.i);
				co_sleep(100);
			}
		}
		co_end;
	};
	co_go(ios)[&](co_generator)
	{
		co_begin_context;
		int i;
		move_test mt;
		co_use_id;
		co_end_context(ctx);

		co_begin;
		for (ctx.i = 0; ctx.i < 10; ctx.i++)
		{
			co_mutex_lock_guard(mutex)
			{
				info_trace_space("b", ctx.i);
				co_sleep(100);
				info_trace_space("b", ctx.i);
				co_sleep(100);
				info_trace_space("b", ctx.i);
				co_sleep(100);
			}
		}
		co_end;
	};
	ios.stop();
	trace_line("end co_mutex_test");
}

void co_select_msg_test()
{
	trace_line("begin co_select_msg_test");
	io_engine ios;
	ios.run();
	co_msg_buffer<move_test> msgBuff(boost_strand::create(ios));
	co_channel<move_test> msgChan(boost_strand::create(ios), 1);
	co_nil_channel<void> doneMsg(boost_strand::create(ios));
	co_csp_channel<move_test(move_test)> csp(boost_strand::create(ios));
	co_go(ios)[&](co_generator)
	{
		co_begin_context;
		move_test mt;
		csp_result<move_test> cspRes;
		co_use_select;
		co_end_context_init(ctx, (co_self), co_select_init);

		co_begin;
		co_begin_select0;
		co_select_case_to(msgBuff) >> ctx.mt;
		if (co_select_state_is_ok)
		{
			info_trace_line("buff: ", ctx.mt);
			co_sleep(10);
		}
		co_select_case_to(msgChan) >> ctx.mt;
		if (co_select_state_is_ok)
		{
			info_trace_line("chan: ", ctx.mt);
			co_sleep(10);
		}
		co_select_case_csp_to(csp, ctx.cspRes) >> ctx.mt;
		if (co_select_state_is_ok)
		{
			info_trace_line("csp: ", ctx.mt);
			co_sleep(10);
			ctx.cspRes.return_(move_test(456));
		}
		co_select_case_void(doneMsg);
		{
			assert(co_select_state_is_ok);
			info_trace_line("select msg done");
			co_select_done0;
		}
		co_end_select;
		co_end;
	};
	co_go(ios)[&](co_generator)
	{
		co_begin_context;
		int i;
		move_test mt;
		co_use_state;
		co_end_context(ctx);

		co_begin;
		co_csp_io(csp, ctx.mt) << move_test(123);
		info_trace_line("csp result: ", ctx.mt);
		for (ctx.i = 0; ctx.i < 5; ctx.i++)
		{
			co_chan_io(msgBuff) << move_test(ctx.i);
			co_sleep(100);
			co_chan_io(msgChan) << move_test(ctx.i);
			co_sleep(100);
		}
		co_chan_io(doneMsg) << void_type();
		co_chan_close(msgBuff);
		co_chan_close(msgChan);
		co_chan_close(doneMsg);
		co_end;
	};
	ios.stop();
	trace_line("end co_select_msg_test");
}

void co_chan_perfor_test()
{
	trace_line("begin co_chan_perfor_test");
	io_engine ios;
	const int msgNum = 10000000;
	for (int i = 1; i <= 4; i++)
	{
		ios.run(i);
		trace_line(i, " threads, msg number", msgNum);
		std::atomic<int> msgCount(0);
		std::atomic<int> recCount(0);
		long long beginTick = get_tick_ms();
		for (int j = 1; j <= i; j++)
		{
			std::shared_ptr<co_channel<int>> channel(new co_channel<int>(boost_strand::create(ios), 3));
			for (int k = 0; k < 100; k++)
			{
				co_go(channel->self_strand())[&, channel](co_generator)
				{
					co_begin_context;
					co_use_state;
					co_end_context(ctx);

					co_begin;
					while (++msgCount <= msgNum)
					{
						co_chan_aff_io(*channel) << msgCount;
					}
					co_end;
				};
				co_go(channel->self_strand())[&, channel](co_generator)
				{
					co_begin_context;
					int res;
					co_use_state;
					co_end_context(ctx);

					co_begin;
					while (++recCount <= msgNum)
					{
						co_chan_aff_io(*channel) >> ctx.res;
					}
					co_end;
				};
			}
		}
		ios.stop();
		long long time = get_tick_ms() - beginTick;
		trace_line("time ", time, ", perfor ", (size_t)((double)msgNum * 1000.0 / (double)time), "/s");
	}
	trace_line("end co_chan_perfor_test");
}

void co_msg_test()
{
	trace_line("begin co_msg_test");
	io_engine ios;
	ios.run();
	co_msg_buffer<int, move_test> msgBuff(boost_strand::create(ios));
	co_go(ios)[&](co_generator)
	{
		co_begin_context;
		int i;
		co_use_state;
		co_end_context(ctx);

		co_begin;
		for (ctx.i = 0; ctx.i < 10; ctx.i++)
		{
			co_chan_io(msgBuff) << co_chan_multi(ctx.i, move_test(ctx.i));
			co_sleep(100);
		}
		co_end;
	};
	co_go(ios)[&](co_generator)
	{
		co_begin_context;
		int i;
		int id;
		move_test mt;
		co_use_state;
		co_end_context(ctx);

		co_begin;
		for (ctx.i = 0; ctx.i < 10; ctx.i++)
		{
			co_chan_io(msgBuff) >> co_chan_multi(ctx.id, ctx.mt);
			info_trace_comma(ctx.id, ctx.mt);
			co_sleep(300);
		}
		co_end;
	};
	ios.stop();
	trace_line("end co_msg_test");
}

void co_channel_test()
{
	trace_line("begin co_channel_test");
	io_engine ios;
	ios.run();
	co_channel<int> channel(boost_strand::create(ios), 3);
	co_go(ios)[&](co_generator)
	{
		co_begin_context;
		int i;
		co_use_state;
		co_end_context(ctx);

		co_begin;
		for (ctx.i = 0; ctx.i < 10; ctx.i++)
		{
			co_chan_io(channel) << ctx.i;
			co_sleep(100);
		}
		co_end;
	};
	co_go(ios)[&](co_generator)
	{
		co_begin_context;
		int i;
		int id;
		co_use_state;
		co_end_context(ctx);

		co_begin;
		for (ctx.i = 0; ctx.i < 10; ctx.i++)
		{
			co_chan_io(channel) >> ctx.id;
			info_trace_line(ctx.id);
			co_sleep(300);
		}
		co_end;
	};
	ios.stop();
	trace_line("end co_channel_test");
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
	co_go(ios) [](co_generator)
	{
		static auto recursionSleep = [](co_generator, const int& ms)
		{
			co_no_context;
			co_begin;
			co_sleep(ms);
			co_end;
		};
		co_no_context;
		co_begin;
		co_call(recursionSleep, 250);
		trace_line("co go");
		co_call(recursionSleep, 500);
		trace_line("co go.");
		co_call(recursionSleep, 500);
		trace_line("co go..");
		co_call(recursionSleep, 500);
		trace_line("co go...");
		co_end;
	};
	ios.stop();
}

int main(int argc, char *argv[])
{
	init_my_actor();
	enable_high_resolution();
	go_test();
	trace("\n");
	co_channel_test();
	trace("\n");
	co_msg_test();
	trace("\n");
#ifdef NDEBUG
	co_chan_perfor_test();
	trace("\n");
#endif
	co_select_msg_test();
	trace("\n");
	co_mutex_test();
	trace("\n");
	co_convar_test();
	trace("\n");
#ifdef NDEBUG
	co_perfor_test();
	trace("\n");
#endif
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
	socket_test();
	trace("\n");
	co_socket_test();
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