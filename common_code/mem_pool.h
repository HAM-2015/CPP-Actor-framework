#ifndef __MEM_POOL_H
#define __MEM_POOL_H

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

template <typename T, typename CREATER>
class mem_pool;

template <typename T>
class mem_pool_base
{
public:
	virtual ~mem_pool_base(){};
	virtual	T* new_() = 0;
	virtual void delete_(void* p) = 0;
protected:
	boost::mutex _mutex;
};

template <typename T, typename CREATER>
static mem_pool_base<T>* create_pool(size_t poolSize, const CREATER& creater)
{
	return new mem_pool<T, CREATER>(poolSize, creater);
}

template <typename T, typename CREATER>
class mem_pool: public mem_pool_base<T>
{
	struct node 
	{
		BYTE _data[sizeof(T)];
		node* _link;
	};

	friend static mem_pool_base<T>* create_pool<T, CREATER>(size_t, const CREATER&);
private:
	mem_pool(size_t poolSize, const CREATER& creater)
		:_creater(creater), _poolSize(poolSize), _size(0), _link(NULL)
	{
#ifdef _DEBUG
		_nodeNumber = 0;
#endif
	}
public:
	~mem_pool()
	{
		boost::lock_guard<boost::mutex> lg(_mutex);
		assert(0 == _nodeNumber);
		node* it = _link;
		while (it)
		{
			_size--;
			node* t = it;
			it = it->_link;
			((T*)t->_data)->~T();
			free(t);
		}
		assert(0 == _size);
	}
public:
	T* new_()
	{
		{
			boost::lock_guard<boost::mutex> lg(_mutex);
#ifdef _DEBUG
			_nodeNumber++;
#endif
			if (_link)
			{
				_size--;
				node* r = _link;
				_link = _link->_link;
				return (T*)r->_data;
			}
		}
		node* newNode = (node*)malloc(sizeof(node));
		_creater(newNode->_data);
		return (T*)newNode->_data;
	}

	void delete_(void* p)
	{
		{
			boost::lock_guard<boost::mutex> lg(_mutex);
#ifdef _DEBUG
			_nodeNumber--;
#endif
			if (_size < _poolSize)
			{
				_size++;
				((node*)p)->_link = _link;
				_link = (node*)p;
				return;
			}
		}
		((T*)p)->~T();
		free(p);
	}
private:
	CREATER _creater;
	node* _link;
	size_t _poolSize;
	size_t _size;
#ifdef _DEBUG
	size_t _nodeNumber;
#endif
};


#endif