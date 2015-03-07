#include "shared_data.h"

msg_data::msg_data( size_t s )
{
	assert(s >= 0 && s <= 1*1024*1024);
	_buff = NULL;
	if (s)
	{
		_buff = (char*)malloc(s);
		if (!_buff)
		{
			throw pool_memory_exception();
		}
	}
	_size = s;
	_cur = 0;
	_isRef = false;
}

msg_data::~msg_data()
{
	if (!_isRef && _buff)
	{
		free(_buff);
	}
}

shared_data msg_data::create( size_t s )
{
	return shared_data(new msg_data(s));
}

shared_data msg_data::create( const void* bf, size_t s )
{
	shared_data r = shared_data(new msg_data(s));
	memcpy(r->_buff, bf, s);
	return r;
}

shared_data msg_data::create( const char* bf )
{
	size_t s = strlen(bf);
	return create(bf, s+1);
}

shared_data msg_data::create( const std::string& s )
{
	return create(s.c_str(), s.size()+1);
}

shared_data msg_data::create(msg_data* s, boost::function<void (msg_data*)> deleter)
{
#ifdef _DEBUG
	memset(s->data(), 0xCD, s->size());
#endif
	return shared_data(s, deleter);
}

shared_data msg_data::create(size_t s, boost::function<void (msg_data*)> deleter)
{
	return shared_data(new msg_data(s), deleter);
}

shared_data msg_data::create_ref(const void* bf, size_t s)
{
	shared_data r = shared_data(new msg_data(0));
	r->_buff = (char*)bf;
	r->_size = s;
	r->_isRef = true;
	return r;
}

size_t msg_data::size()
{
	return _size;
}

void* msg_data::data()
{
	return _buff;
}

char* msg_data::c_str()
{
	return _buff;
}

void msg_data::resize( size_t s )
{
	assert(s > 0 && s <= 1*1024*1024);
	if (s == _size)
	{
		return;
	}

	char* nb = (char*)realloc(_buff, s);
	if (!nb)
	{
		throw pool_memory_exception();
	}
	_buff = nb;
	_size = s;
}

void msg_data::setZero()
{
	memset(_buff, 0, _size);
}

char& msg_data::operator[]( size_t i )
{
	assert(i >= 0 && i < _size);
	return _buff[i];
}

void msg_data::moveCur( size_t cur )
{
	assert(cur < _size);
	_cur = cur;
}

size_t msg_data::getCur()
{
	return _cur;
}
