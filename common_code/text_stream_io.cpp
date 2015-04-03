#include "text_stream_io.h"
#include "scattered.h"

text_stream_io::text_stream_io()
{
	_closed = false;
}

text_stream_io::~text_stream_io()
{

}

std::shared_ptr<text_stream_io> text_stream_io::create( shared_strand strand, std::shared_ptr<stream_io_base> ioObj, const std::function<void (shared_data)>& h )
{
	std::shared_ptr<text_stream_io> res(new text_stream_io);
	res->_ioObj = ioObj;
	res->_msgNotify = h;
	auto wc = my_actor::create(strand, [res](my_actor* self){res->writeActor(self); });
	res->_writerPipeIn = wc->make_msg_notifer(res->_writerPipeOut);
	auto rc = my_actor::create(strand, [res](my_actor* self){res->readActor(self); });
	wc->notify_run();
	rc->notify_run();
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

void text_stream_io::readActor( my_actor* self )
{
	unsigned char buff[4096];
	size_t msgLength = 0;
	while (true)
	{
		async_trig_handle<boost::system::error_code, size_t> ath;
		_ioObj->async_read_some(buff+msgLength, sizeof(buff)-msgLength, self->begin_trig(ath));
		boost::system::error_code ec;
		size_t length;
		self->wait_trig(ath, ec, length);
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
	clear_function(_msgNotify);
}

void text_stream_io::writeActor( my_actor* self )
{
	string textTail = "\r\n";
	while (true)
	{
		shared_data msg = self->pump_msg(_writerPipeOut);
		if (!msg)
		{
			break;
		}
		{
			async_trig_handle<boost::system::error_code, size_t> ath;
			_ioObj->async_write((unsigned char*)msg->data(), msg->size(), self->begin_trig(ath));
			boost::system::error_code ec;
			size_t length;
			self->wait_trig(ath, ec, length);
			if (ec)
			{
				break;
			}
		}
		{
			async_trig_handle<boost::system::error_code, size_t> ath;
			_ioObj->async_write((unsigned char*)textTail.c_str(), textTail.size(), self->begin_trig(ath));
			boost::system::error_code ec;
			size_t length;
			self->wait_trig(ath, ec, length);
			if (ec)
			{
				break;
			}
		}
	}
	self->close_msg_notifer(_writerPipeOut);
	_closed = true;
}
