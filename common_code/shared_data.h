#ifndef __MSG_DATA_H
#define __MSG_DATA_H

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

/*!
@brief ÏûÏ¢°ü
*/
class msg_data;
typedef boost::shared_ptr<msg_data> shared_data;
class msg_data
{
public:
	struct pool_memory_exception 
	{
		pool_memory_exception(){};
	};
	msg_data(size_t s);
private:
	msg_data(msg_data& s){};
	msg_data& operator =(msg_data& s){return *this;};
public:
	~msg_data();
	static shared_data create(size_t s);
	static shared_data create(const void* bf, size_t s);
	static shared_data create(const char* bf);
	static shared_data create(const std::string& s);
	static shared_data create(msg_data* s, boost::function<void (msg_data*)> deleter);
	static shared_data create(size_t s, boost::function<void (msg_data*)> deleter);
	static shared_data create_ref(const void* bf, size_t s);
public:
	size_t size();
	void* data();
	char* c_str();
	void resize(size_t s);
	void setZero();
	void moveCur(size_t cur);
	size_t getCur();
	char& operator [](size_t i);
private:
	bool _isRef;
	char* _buff;
	size_t _size;
	size_t _cur;
};

#endif