// coro_test.cpp : 定义控制台应用程序的入口点。
//

#include "ios_proxy.h"
#include "boost_coroutine.h"
#include "msg_pipe.h"
#include "async_buffer.h"
#include "time_info.h"
#include <list>
#include <Windows.h>

using namespace std;

void check_key_down(boost_coro* coro, int id)
{//检测按键按下
	while (GetAsyncKeyState(id) >= 0)
	{
		coro->sleep(1);
	}
}

void check_key_up(boost_coro* coro, int id)
{//检测按键弹起
	while (GetAsyncKeyState(id) < 0)
	{
		coro->sleep(1);
	}
}

void check_key_test(boost_coro* coro, int id)
{
	while (true)
	{//按键按下后，检测弹起，1000ms内没有弹起就是超时错误
		check_key_down(coro, id);//检测按下
		child_coro_handle checkUp = coro->create_child_coro(boost::bind(&check_key_up, _1, id), 24 kB);//创建一个检测弹起的子协程
		coro->child_coro_run(checkUp);//开始运行子协程
		coro->delay_trig(1000, boost::bind(&boost_coro::notify_force_quit, checkUp.get_coro()));//启用弹起超时处理，超时后强制关闭checkUp
		bool ok = coro->child_coro_wait_quit(checkUp);//等待，正常退出的返回true，被强制关闭的返回false
		coro->cancel_delay_trig();
		if (ok)
		{
			printf("ok %d\n", id);
		} 
		else
		{//弹起超时
			printf("timeout %d\n", id);
			check_key_up(coro, id);//检测弹起
		}
	}
}

void coro_print(boost_coro* coro, int id)
{//按下后先打印一个字符，500ms后每隔50ms打印一次
	printf("_");
	coro->sleep(500);
	while (true)
	{
		printf("-");
		coro->sleep(50);
	}
}

void coro_test_print(boost_coro* coro, int id)
{
	while (true)
	{
		check_key_down(coro, id);//检测按下
		child_coro_handle ch = coro->create_child_coro(boost::bind(&coro_print, _1, id));
		coro->child_coro_run(ch);
		check_key_up(coro, id);//检测弹起
		coro->child_coro_quit(ch);
	}
}

void coro_suspend(boost_coro* coro, const list<coro_handle>& chs)
{
	while (true)
	{
		check_key_down(coro, VK_RBUTTON);
		coro->another_coros_suspend(chs);
		check_key_up(coro, VK_RBUTTON);
	}
}

void coro_resume(boost_coro* coro, const list<coro_handle>& chs)
{
	while (true)
	{
		check_key_down(coro, VK_LBUTTON);
		coro->another_coros_resume(chs);
		check_key_up(coro, VK_LBUTTON);
	}
}

void check_both_up(boost_coro* coro, int id, bool& own, const bool& another)
{
	goto __start;
	while (!another)
	{
		coro->sleep(1);
		own = false;
__start:
		check_key_up(coro, id);
		own = true;
	}
}

void check_two_down(boost_coro* coro, int dt, int id1, int id2)
{
	while (true)
	{
		{//检测两个按键是否都处于弹起状态
			bool st1 = false;
			bool st2 = false;
			child_coro_handle up1 = coro->create_child_coro(boost::bind(&check_both_up, _1, id1, boost::ref(st1), boost::ref(st2)));
			child_coro_handle up2 = coro->create_child_coro(boost::bind(&check_both_up, _1, id2, boost::ref(st2), boost::ref(st1)));
			async_trig_handle<bool> ath;
			auto nh = coro->begin_trig(ath);
			up1.get_coro()->append_quit_callback(nh);
			up2.get_coro()->append_quit_callback(nh);
			coro->child_coro_run(up1);
			coro->child_coro_run(up2);
			if (coro->timed_wait_trig(ath, st1, 3000))
			{
				coro->child_coro_quit(up1);
				coro->child_coro_quit(up2);
			}
			else
			{
				printf("退出 check_two_down\n");
				coro->child_coro_quit(up1);
				coro->child_coro_quit(up2);
				coro->close_trig(ath);
				break;
			}
		}
		{
			child_coro_handle check1 = coro->create_child_coro(boost::bind(&check_key_down, _1, id1));
			child_coro_handle check2 = coro->create_child_coro(boost::bind(&check_key_down, _1, id2));
			async_trig_handle<coro_handle, bool> ath;
			auto nh = coro->begin_trig(ath);
			check1.get_coro()->append_quit_callback(boost::bind(nh, check2.get_coro(), _1));
			check2.get_coro()->append_quit_callback(boost::bind(nh, check1.get_coro(), _1));
			coro->child_coro_run(check1);
			coro->child_coro_run(check2);
			coro_handle ah;
			bool ok = false;
			coro->wait_trig(ath, ah, ok);
			coro->delay_trig(dt, boost::bind(&boost_coro::notify_force_quit, ah));
			if (coro->another_coro_wait_quit(ah))
			{
				printf("*success*\n");
			} 
			else
			{
				printf("*failure*\n");
			}
			coro->cancel_delay_trig();
			coro->child_coro_quit(check1);
			coro->child_coro_quit(check2);
		}
	}
}

void wait_key(boost_coro* coro, int id, boost::function<void (int)> cb)
{
	check_key_up(coro, id);
	while (true)
	{
		check_key_down(coro, id);
		cb(id);
		check_key_up(coro, id);
	}
}

void shift_key(boost_coro* coro, coro_handle pauseCoro)
{
	list<child_coro_handle::ptr> childs;
	coro_msg_handle<int> cmh;
	auto h = coro->make_msg_notify(cmh);
	for (int i = 'A'; i <= 'Z'; i++)
	{
		auto tp = child_coro_handle::make_ptr();
		*tp = coro->create_child_coro(boost::bind(&wait_key, _1, i, h));
		coro->child_coro_run(*tp);
		childs.push_back(tp);
	}
	while (true)
	{
		int id = coro->pump_msg(cmh);
		if ('P' == id)
		{
			bool isPause = coro->another_coro_switch(pauseCoro);
			printf("%s性能测试\n", isPause? "暂停": "恢复");
		}
		else
		{
			printf("shift+%c\n", id);
		}
	}
	coro->close_msg_notify(cmh);
	coro->child_coros_quit(childs);
}

void test_shift(boost_coro* coro, coro_handle pauseCoro)
{
	child_coro_handle ch;
	while (true)
	{
		check_key_down(coro, VK_SHIFT);
		ch = coro->create_child_coro(boost::bind(&shift_key, _1, pauseCoro));
		coro->child_coro_run(ch);
		check_key_up(coro, VK_SHIFT);
		coro->child_coro_quit(ch);
	}
}

void test_producer(boost_coro* coro, boost::function<void (int, int)> writer)
{//生产者
	for (int i = 0; true; i++)
	{
		writer(i, (int)coro->this_id());
		coro->sleep(2100);
	}
}

void test_consumer(boost_coro* coro, coro_msg_handle<int, int>& cmh)
{//消费者
	while (true)
	{
		int p0;
		int id;
		coro->pump_msg(cmh, p0, id);
		printf("%d-%d id=%d\n", p0, (int)cmh.get_size(), id);
		coro->sleep(1000);
	}
	coro->close_msg_notify(cmh);
}

void count_test(boost_coro* coro, int& ct)
{
	while (true)
	{
		ct++;
		coro->sleep(0);
	}
}

void perfor_test(boost_coro* coro, ios_proxy& ios, int coroNum)
{
	int* count = (int*)_alloca(coroNum*sizeof(int));
	coro->check_stack();
	vector<shared_strand> strands;
	strands.resize(ios.threadNumber());
	for (int i = 0; i < (int)ios.threadNumber(); i++)
	{
		strands[i] = boost_strand::create(ios);
	}

	for (int num = 1; true; num = num % coroNum + 1)
	{
		long long tk = get_tick();
		list<child_coro_handle::ptr> childList;
		for (int i = 0; i < num; i++)
		{
			count[i] = 0;
			auto newCoro = child_coro_handle::make_ptr();
			*newCoro = coro->create_child_coro(strands[i%strands.size()], boost::bind(&count_test, _1, boost::ref(count[i])));
			childList.push_front(newCoro);
			coro->child_coro_run(*childList.front());
		}
		coro->sleep(1000);
		coro->child_coros_quit(childList);
		int ct = 0;
		for (int i = 0; i < num; i++)
		{
			ct += count[i];
		}
		double f = (double)ct * 1000000 / (get_tick()-tk);
		printf("协程数=%d, 切换频率=%d\n", num, (int)f);
	}
}

void create_null_coro(boost_coro* coro, int* count)
{
	while (true)
	{
		size_t yieldTick = coro->yield_count();
		list<child_coro_handle::ptr> childList;
		for (int i = 0; i < 100; i++)
		{
			(*count)++;
			auto newCoro = child_coro_handle::make_ptr();
			*newCoro = coro->create_child_coro([](boost_coro*){});
			childList.push_back(newCoro);
		}
		coro->child_coros_quit(childList);
		if (coro->yield_count() == yieldTick)
		{
			coro->sleep(0);
		}
	}
}

void create_coro_test(boost_coro* coro)
{
	while (true)
	{
		int count = 0;
		child_coro_handle createCoro = coro->create_child_coro(boost::bind(&create_null_coro, _1, &count));
		coro->child_coro_run(createCoro);
		coro->sleep(1000);
		coro->child_coro_quit(createCoro);
		printf("1秒创建协程数=%d\n", count);
	}
}

void async_buffer_read(boost_coro* coro, boost::shared_ptr<async_buffer<int> > buffer, msg_pipe<>::regist_reader regWaitFull, int max)
{
	coro_msg_handle<> cmh;
	regWaitFull(coro, cmh);
	goto __start;
	while (true)
	{
		coro->sleep(500);
		int i;
		bool empty = buffer->pop(i);
		printf("%d\n", i);
		if (max == i)
		{
			break;
		}
		if (empty)
		{
			printf("read 缓存空\n");
			__start:
			coro->pump_msg(cmh);
			printf("read 缓存来数据\n");
		}
	}
	coro->close_msg_notify(cmh);
}

void async_buffer_test(boost_coro* coro)
{
	msg_pipe<>::writer_type fullNotify;
	msg_pipe<>::regist_reader regWaitFull = msg_pipe<>::make(fullNotify);
	boost::shared_ptr<async_buffer<int> > buffer = async_buffer<int>::create(10);
	coro_msg_handle<> cmh;
	int testCyc = 30;
	buffer->setNotify(fullNotify, coro->make_msg_notify(cmh));
	child_coro_handle readCoro = coro->create_child_coro(boost::bind(&async_buffer_read, _1, buffer, regWaitFull, testCyc-1));
	coro->child_coro_run(readCoro);
	for (int i = 0; i < testCyc; i++)
	{
		check_key_down(coro, VK_CONTROL);
		check_key_up(coro, VK_CONTROL);
		if (buffer->push(i))
		{
			printf("writer 缓存满\n");
			coro->pump_msg(cmh);
			printf("writer 缓存半空\n");
		}
	}
	coro->child_coro_wait_quit(readCoro);
	coro->close_msg_notify(cmh);
	printf("缓存测试结束\n");
}

void coro_test(boost_coro* coro)
{
	ios_proxy perforIos;//用于测试多线程下的协程切换性能
	perforIos.run(ios_proxy::hardwareConcurrency());//调度器线程设置为CPU线程数
	perforIos.runPriority(ios_proxy::idle);//调度器优先级设置为最低
	child_coro_handle coroLeft = coro->create_child_coro(boost::bind(&check_key_test, _1, VK_LEFT));
	child_coro_handle coroRight = coro->create_child_coro(boost::bind(&check_key_test, _1, VK_RIGHT));
	child_coro_handle coroUp = coro->create_child_coro(boost::bind(&check_key_test, _1, VK_UP));
	child_coro_handle coroDown = coro->create_child_coro(boost::bind(&check_key_test, _1, VK_DOWN));
	child_coro_handle coroPrint = coro->create_child_coro(boost::bind(&coro_test_print, _1, VK_SPACE));//检测空格键
	child_coro_handle coroTwo = coro->create_child_coro(boost::bind(&check_two_down, _1, 10, 'D', 'F'));//检测D,F键是否同时按下(设置间隔误差10ms)
	child_coro_handle coroPerfor = coro->create_child_coro(boost::bind(&perfor_test, _1, boost::ref(perforIos), 200));//协程切换性能测试
	child_coro_handle coroCreate = coro->create_child_coro(boost::bind(&create_coro_test, _1));//协程创建性能测试
	child_coro_handle coroShift = coro->create_child_coro(boost::bind(&test_shift, _1, coroPerfor.get_coro()));//shift+字母检测
	child_coro_handle coroBuffer = coro->create_child_coro(boost::bind(&async_buffer_test, _1));//异步缓冲队列测试
	child_coro_handle coroProducer1;
	child_coro_handle coroProducer2;
	child_coro_handle coroConsumer;
	//创建生产者/消费者模型测试
	coro_msg_handle<int, int> conCmh;
	{
		coroConsumer = coro->create_child_coro(boost::bind(&test_consumer, _1, boost::ref(conCmh)));
		auto h = coroConsumer.get_coro()->make_msg_notify(conCmh);
		coroProducer1 = coro->create_child_coro(boost::bind(&test_producer, _1, h));
		coroProducer2 = coro->create_child_coro(boost::bind(&test_producer, _1, h));
	}

	list<coro_handle> chs;//需要被挂起的协程对象，可以从下方注释几个测试
	chs.push_back(coroLeft.get_coro());
	chs.push_back(coroRight.get_coro());
	chs.push_back(coroUp.get_coro());
	chs.push_back(coroDown.get_coro());
	chs.push_back(coroPrint.get_coro());
	chs.push_back(coroShift.get_coro());
	chs.push_back(coroTwo.get_coro());
	chs.push_back(coroConsumer.get_coro());
//	chs.push_back(coroPerfor.get_coro());
	chs.push_back(coroProducer1.get_coro());
	chs.push_back(coroProducer2.get_coro());
	chs.push_back(coroCreate.get_coro());
	child_coro_handle coroSuspend = coro->create_child_coro(boost::bind(&coro_suspend, _1, boost::ref(chs)), 32 kB);//点击鼠标右键暂停按键检测
	child_coro_handle coroResume = coro->create_child_coro(boost::bind(&coro_resume, _1, boost::ref(chs)), 32 kB);//点击鼠标左键恢复按键检测
	coro->child_coro_run(coroLeft);
	coro->child_coro_run(coroRight);
	coro->child_coro_run(coroUp);
	coro->child_coro_run(coroDown);
	coro->child_coro_run(coroPrint);
	coro->child_coro_run(coroShift);
	coro->child_coro_run(coroTwo);
	coro->child_coro_run(coroPerfor);
// 	coro->child_coro_run(coroConsumer);
// 	coro->child_coro_run(coroProducer1);
// 	coro->child_coro_run(coroProducer2);
//	coro->child_coro_run(coroCreate);
	coro->child_coro_run(coroSuspend);
	coro->child_coro_run(coroResume);
	coro->child_coro_run(coroBuffer);
	coro->child_coro_suspend(coroPerfor);
	check_key_down(coro, VK_ESCAPE);//ESC键退出
	coro->child_coro_quit(coroLeft);
	coro->child_coro_quit(coroRight);
	coro->child_coro_quit(coroUp);
	coro->child_coro_quit(coroDown);
	coro->child_coro_quit(coroPrint);
	coro->child_coro_quit(coroShift);
	coro->child_coro_quit(coroTwo);
	coro->child_coro_quit(coroPerfor);
	coro->child_coro_quit(coroConsumer);
	coro->child_coro_quit(coroProducer1);
	coro->child_coro_quit(coroProducer2);
	coro->child_coro_quit(coroCreate);
	coro->child_coro_quit(coroSuspend);
	coro->child_coro_quit(coroResume);
	coro->child_coro_quit(coroBuffer);
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
	boost_coro::enable_stack_pool();
	ios_proxy ios;
	ios.run();
	{
		coro_handle coroTest = boost_coro::create(boost_strand::create(ios), boost::bind(&coro_test, _1));
		coroTest->notify_start_run();
		coroTest->outside_wait_quit();
	}
 	ios.stop();
	return 0;
}
