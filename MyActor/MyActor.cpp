#include <iostream>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffered_write_stream.hpp>
#include "./actor/actor_framework.h"
#include "./actor/async_buffer.h"
#include "./actor/msg_queue.h"
#include "./actor/sync_msg.h"
#include "./actor/trace.h"

using namespace std;

void wait_multi_msg()
{
	trace_line("begin wait_multi_msg");
	io_engine ios;
	ios.run();
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		actor_handle act1 = my_actor::create(self->self_strand(), [](my_actor* self)
		{
			self->run_mutex_blocks1(mutex_block_pump<int>(self, [&](int msg)
			{
				trace_comma(self->self_id(), "begin msg int");
				self->sleep(500);
				trace_comma(self->self_id(), msg);
				self->sleep(500);
				trace_comma(self->self_id(), "end msg int");
				return false;
			}), mutex_block_pump<move_test>(self, [&](const move_test& msg)
			{
				trace_comma(self->self_id(), "begin msg int");
				self->sleep(500);
				trace_comma(self->self_id(), msg);
				self->sleep(500);
				trace_comma(self->self_id(), "end msg int");
				return false;
			}), mutex_block_pump<>(self, [&]()
			{
				trace_comma(self->self_id(), "begin end");
				self->sleep(1000);
				return true;
			}));
		});
		child_actor_handle ch2 = self->create_child_actor([&](my_actor* self)
		{
			auto ntf = self->connect_msg_notifer_to<int>(act1);
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
		child_actor_handle ch3 = self->create_child_actor([&](my_actor* self)
		{
			auto ntf = self->connect_msg_notifer_to<move_test>(act1);
			if (ntf)
			{
				for (int i = 0; i < 3; i++)
				{
					ntf(move_test(i));
					self->sleep(500);
				}
			}
		});
		act1->notify_run();
		self->child_actor_run(ch2, ch3);
		self->child_actor_wait_quit(ch2, ch3);
		self->connect_msg_notifer_to(act1)();
		self->actor_wait_quit(act1);
	});
	ah->notify_run();
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
		child_actor_handle ch1 = self->create_child_actor([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				self->sleep(1000);
				trace_comma(self->self_id(), asyncMsg.pop(self));
			}
		});
		child_actor_handle ch2 = self->create_child_actor([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				trace_comma(self->self_id(), "begin push", i);
				asyncMsg.push(self, move_test(i));
				trace_comma(self->self_id(), "push ok", i);
			}
		});
		self->child_actor_run(ch1, ch2);
		self->child_actor_wait_quit(ch1, ch2);
	});
	ah->notify_run();
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
		child_actor_handle ch1 = self->create_child_actor([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				self->sleep(1000);
				trace_comma(self->self_id(), syncMsg.take(self));
			}
		});
		child_actor_handle ch2 = self->create_child_actor([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				trace_comma(self->self_id(), "begin send", i);
				syncMsg.send(self, move_test(i));
				trace_comma(self->self_id(), "send ok", i);
			}
		});
		self->child_actor_run(ch1, ch2);
		self->child_actor_wait_quit(ch1, ch2);
	});
	ah->notify_run();
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
		csp_invoke<int(move_test)> csp(self->self_strand());
		child_actor_handle ch1 = self->create_child_actor([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				self->sleep(1000);
				csp.wait_invoke(self, [&](move_test msg)->int
				{
					trace_comma(self->self_id(), "csp msg", msg);
					return -i;
				});
			}
		});
		child_actor_handle ch2 = self->create_child_actor([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				int r = csp.invoke(self, move_test(i));
				trace_comma(self->self_id(), "csp return ", r);
			}
		});
		self->child_actor_run(ch1, ch2);
		self->child_actor_wait_quit(ch1, ch2);
	});
	ah->notify_run();
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
		child_actor_handle ch1 = self->create_child_actor([&](my_actor* self)
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
		child_actor_handle ch2 = self->create_child_actor([&](my_actor* self)
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
		self->child_actor_run(ch1, ch2);
		self->child_actor_wait_quit(ch1, ch2);
	});
	ah->notify_run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end mutex_test");
}

void agent_test()
{
	trace_line("begin agent_test");
	io_engine ios;
	ios.run(4);
	actor_handle ah = my_actor::create(boost_strand::create(ios), [](my_actor* self)
	{
		child_actor_handle ch = self->create_child_actor(self->self_strand()->clone(), [](my_actor* self)
		{
			child_actor_handle ch1 = self->create_child_actor(self->self_strand()->clone(), [](my_actor* self)
			{
				msg_pump_handle<move_test> pp = self->connect_msg_pump<move_test>(true);
				try
				{
					while (true)
					{
						trace_comma(self->self_id(), self->pump_msg(pp));
					}
				}
				catch (msg_lost_exception&)
				{
					trace_comma(self->self_id(), "notifer lost");
				}
			});
			self->child_actor_run(ch1);
			self->msg_agent_to<move_test>(ch1);
			self->child_actor_wait_quit(ch1);
		});
		self->child_actor_run(ch);
		{
			auto ntf = self->connect_msg_notifer_to<move_test>(ch, true);
			for (int i = 0; i < 3; i++)
			{
				ntf(move_test(i));
				self->sleep(1000);
			}
		}
		self->child_actor_wait_quit(ch);
	});
	ah->notify_run();
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
		child_actor_handle ch = self->create_child_actor([&](my_actor* self)
		{
			msg_pump_handle<move_test> pp = self->connect_msg_pump<move_test>(true);
			try
			{
				while (true)
				{
					trace_comma(self->self_id(), self->pump_msg(pp));
				}
			}
			catch (msg_lost_exception&)
			{
				trace_comma(self->self_id(), "notifer lost");
			}
		});
		self->child_actor_run(ch);
		{
			auto ntf = self->connect_msg_notifer_to<move_test>(ch, true);
			for (int i = 0; i < 3; i++)
			{
				ntf(move_test(i));
				self->sleep(1000);
			}
		}
		self->child_actor_wait_quit(ch);
	});
	ah->notify_run();
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
		actor_msg_handle<move_test> amh;
		child_actor_handle ch = self->create_child_actor([&](my_actor* self)
		{
			for (int i = 0; i < 3; i++)
			{
				trace_comma(self->self_id(), self->wait_msg(amh));
			}
		});
		auto ntf = self->make_msg_notifer_to(ch, amh);
		self->child_actor_run(ch);
		for (int i = 0; i < 3; i++)
		{
			ntf(move_test(i));
			self->sleep(1000);
		}
		self->child_actor_wait_quit(ch);
	});
	ah->notify_run();
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
		actor_trig_handle<move_test> ath;
		child_actor_handle ch = self->create_child_actor([&](my_actor* self)
		{
			trace_comma(self->self_id(), "waiting trig");
			move_test msg = self->wait_trig(ath);
			trace_comma(self->self_id(), "trig ok", msg);
		});
		auto trig = self->make_trig_notifer_to(ch, ath);
		self->child_actor_run(ch);
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
		self->child_actor_wait_quit(ch);
	});
	ah->notify_run();
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
		child_actor_handle srv = self->create_child_actor([&](my_actor* self)
		{
			boost::asio::ip::tcp::socket sck(self->self_io_service());
			stack_obj<boost::asio::ip::tcp::acceptor> acc;
			RUN_IN_TRHEAD_STACK(self, acc.create(self->self_io_service(), boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1234)));//Actor堆栈内创建acceptor会出错，切换到线程堆栈内创建
			boost::system::error_code ec;
			acc->async_accept(sck, self->make_asio_context(ec));
			if (!ec)
			{
				acc->close(ec);
				char buf[128];
				size_t s = 0;
				while (true)
				{
					//sck.async_read_some(boost::asio::buffer(buf, sizeof(buf)-1), self->make_asio_context(ec, s));
					actor_trig_handle<boost::system::error_code, size_t> ath;
					sck.async_read_some(boost::asio::buffer(buf, sizeof(buf)-1), self->make_trig_notifer_to_self(ath));
					if (self->timed_wait_trig(2000, ath, ec, s) && !ec)
					{
						buf[s] = 0;
						trace_comma(self->self_id(), "received", buf);
					} 
					else if (!ec)
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
			sck.close(ec);
		});
		child_actor_handle cli = self->create_child_actor([&](my_actor* self)
		{
			boost::asio::ip::tcp::socket sck(self->self_io_service());
			boost::system::error_code ec;
			sck.async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1234), self->make_asio_context(ec));
			if (!ec)
			{
				char buf[128];
				for (int i = 0; i < 10; i++)
				{
					int l = snPrintf(buf, sizeof(buf), "msg %d", i);
					size_t s;
					boost::asio::async_write(sck, boost::asio::buffer(buf, l), self->make_asio_context(ec, s));
					if (ec)
					{
						break;
					}
					self->sleep(1000);
				}
				self->sleep(2000);
			}
			sck.close(ec);
		});
		self->child_actor_run(srv);
		self->sleep(1000);
		self->child_actor_run(cli);
		self->child_actor_wait_quit(srv, cli);
	});
	ah->notify_run();
	ah->outside_wait_quit();
	ios.stop();
	trace_line("end socket_test");
}

int main(int argc, char *argv[])
{
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
	csp_test();
	trace("\n");
	sync_msg_test();
	trace("\n");
	async_buffer_test();
	trace("\n");
	socket_test();
	trace("\n");
	wait_multi_msg();
	trace("\n");
	trace_line("end");
	getchar();
	return 0;
}