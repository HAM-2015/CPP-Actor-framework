#include "text_stream_io.h"

text_stream_io::text_stream_io()
{
	_closed = false;
}

text_stream_io::~text_stream_io()
{

}

boost::shared_ptr<text_stream_io> text_stream_io::create( shared_strand strand, boost::shared_ptr<stream_io_base> ioObj, const boost::function<void (shared_data)>& h )
{
	boost::shared_ptr<text_stream_io> res(new text_stream_io);
	res->_ioObj = ioObj;
	res->_msgNotify = h;
	auto wc = boost_coro::create(strand, boost::bind(&text_stream_io::writeCoro, res, _1));
	res->_writerPipeIn = wc->make_msg_notify(res->_writerPipeOut);
	auto rc = boost_coro::create(strand, boost::bind(&text_stream_io::readCoro, res, _1));
	wc->notify_start_run();
	rc->notify_start_run();
	return res;
}

void text_stream_io::close()
{
	_ioObj->close();
}

bool text_stream_io::write( shared_data msg )
{
	if (!_closed)
	{
		_writerPipeIn(msg);
		return true;
	}
	return false;
}

void text_stream_io::readCoro( boost_coro* coro )
{
	unsigned char buff[4096];
	size_t msgLength = 0;
	while (true)
	{
		async_trig_handle<boost::system::error_code, size_t> ath;
		_ioObj->async_read_some(buff+msgLength, sizeof(buff)-msgLength, coro->begin_trig(ath));
		boost::system::error_code ec;
		size_t length;
		coro->wait_trig(ath, ec, length);
		if (ec || 0 == length)
		{
			_msgNotify(shared_data());
			break;
		}
		size_t i = msgLength;
		size_t ei = msgLength+length;
		for (; i < ei; i++)
		{
			char tc = buff[i];
			if (tc == '\r' || tc == '\n')
			{
				if (msgLength)
				{
					shared_data msg = msg_data::create(msgLength+1);
					memcpy(msg->data(), buff+i-msgLength, msgLength);
					msg->c_str()[msgLength] = 0;
					_msgNotify(msg);
					msgLength = 0;
				}
			} 
			else
			{
				msgLength++;
			}
		}
		if (msgLength && i != msgLength)
		{
			memcpy(buff, buff+i-msgLength, msgLength);
		}
	}
	_writerPipeIn(shared_data());
	_msgNotify.clear();
}

void text_stream_io::writeCoro( boost_coro* coro )
{
	string textTail = "\r\n";
	while (true)
	{
		shared_data msg = coro->pump_msg(_writerPipeOut);
		if (!msg)
		{
			break;
		}
		{
			async_trig_handle<boost::system::error_code, size_t> ath;
			_ioObj->async_write((unsigned char*)msg->data(), msg->size(), coro->begin_trig(ath));
			boost::system::error_code ec;
			size_t length;
			coro->wait_trig(ath, ec, length);
			if (ec)
			{
				break;
			}
		}
		{
			async_trig_handle<boost::system::error_code, size_t> ath;
			_ioObj->async_write((unsigned char*)textTail.c_str(), textTail.size(), coro->begin_trig(ath));
			boost::system::error_code ec;
			size_t length;
			coro->wait_trig(ath, ec, length);
			if (ec)
			{
				break;
			}
		}
	}
	coro->close_msg_notify(_writerPipeOut);
	_closed = true;
}
