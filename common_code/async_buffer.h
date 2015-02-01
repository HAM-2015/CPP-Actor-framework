#ifndef __ASYNC_BUFFER_H
#define __ASYNC_BUFFER_H

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/circular_buffer.hpp>

/*!
@brief 异步缓冲队列，在同一个shared_strand中使用
*/
template <typename T>
class async_buffer
{
private:
	async_buffer(size_t maxLength)
		:_buffer(maxLength)
	{
		_waitData = true;
		_waitHalf = false;
	}
public:
	~async_buffer()
	{

	}

	static boost::shared_ptr<async_buffer> create(size_t maxLength)
	{
		return boost::shared_ptr<async_buffer>(new async_buffer(maxLength));
	}
public:
	void setNotify(const boost::function<void ()>& newDataNotify, const boost::function<void ()>& emptyNotify)
	{
		_newDataNotify = newDataNotify;
		_halfNotify = emptyNotify;
	}

	bool push(const T& p)
	{
		assert(_buffer.size() < _buffer.capacity());
		_buffer.push_back(p);
		_waitHalf = _buffer.size() == _buffer.capacity();
		if (_waitData)
		{
			_waitData = false;
			_newDataNotify();
		}
		return _waitHalf;
	}

	bool pop(T& p)
	{
		assert(!_buffer.empty());
		p = _buffer.front();
		_buffer.pop_front();
		_waitData = _buffer.empty();
		if (_waitHalf && _buffer.size() <= _buffer.capacity()/2)
		{
			_waitHalf = false;
			_halfNotify();
		}
		return _waitData;
	}
private:
	bool _waitData;
	bool _waitHalf;
	boost::circular_buffer<T> _buffer;
	boost::function<void ()> _newDataNotify;
	boost::function<void ()> _halfNotify;
};

#endif