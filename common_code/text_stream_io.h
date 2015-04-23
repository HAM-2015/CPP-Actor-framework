#ifndef __TEXT_STREAM_H
#define __TEXT_STREAM_H

#include "stream_io_base.h"
#include "shared_data.h"
#include "actor_framework.h"

/*!
@brief 文本数据流解析类
*/
class text_stream_io
{
private:
	text_stream_io();
	~text_stream_io();
public:
	static std::shared_ptr<text_stream_io> create(shared_strand strand, std::shared_ptr<stream_io_base> ioObj, const std::function<void (shared_data)>& h);
public:
	void close();
	bool write(shared_data msg);
private:
	void readActor(my_actor* self);
	void writeActor(my_actor* self);
private:
	bool _closed;
	std::shared_ptr<stream_io_base> _ioObj;
	std::function<void (shared_data)> _msgNotify;
	std::function<void (shared_data)> _writerPipeIn;
	actor_msg_handle<shared_data> _writerPipeOut;
};

#endif