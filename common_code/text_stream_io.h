#ifndef __TEXT_STREAM_H
#define __TEXT_STREAM_H

#include "stream_io_base.h"
#include "shared_data.h"
#include "msg_pipe.h"
#include "boost_coroutine.h"

/*!
@brief 文本数据流解析类
*/
class text_stream_io
{
private:
	text_stream_io();
public:
	~text_stream_io();
	static boost::shared_ptr<text_stream_io> create(shared_strand strand, boost::shared_ptr<stream_io_base> ioObj, const boost::function<void (shared_data)>& h);
public:
	void close();
	bool write(shared_data msg);
private:
	void readCoro(boost_coro* coro);
	void writeCoro(boost_coro* coro);
private:
	bool _closed;
	boost::shared_ptr<stream_io_base> _ioObj;
	boost::function<void (shared_data)> _msgNotify;
	msg_pipe<shared_data>::writer_type _writerPipeIn;
	msg_pipe<shared_data>::regist_reader _writerPipeRegOut;
};

#endif