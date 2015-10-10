#ifndef __MSG_QUEUE_H
#define __MSG_QUEUE_H

#include "mem_pool.h"
#include "scattered.h"
#include <map>
#include <set>
#include <list>

template <typename T>
class msg_queue
{
	struct node 
	{
		char _data[sizeof(T)];
		node* _next;
	};
public:
	msg_queue(size_t poolSize = sizeof(void*))
		:_alloc(poolSize)
	{
		_size = 0;
		_head._next = NULL;
		_tail = &_head;
	}

	~msg_queue()
	{
		clear();
	}

	void push_back(const T& p)
	{
		assert(_size < 256);
		_size++;
		new(new_back()->_data)T(p);
	}

	void push_back(T&& p)
	{
		assert(_size < 256);
		_size++;
		new(new_back()->_data)T(std::move(p));
	}

	void push_front(const T& p)
	{
		assert(_size < 256);
		_size++;
		new(new_front()->_data)T(p);
	}

	void push_front(T&& p)
	{
		assert(_size < 256);
		_size++;
		new(new_front()->_data)T(std::move(p));
	}

	T& front()
	{
		assert(_size && _head._next);
		return *(T*)_head._next->_data;
	}

	T& back()
	{
		assert(_size);
		return *(T*)_tail->_data;
	}

	void pop_front()
	{
		assert(_size);
		if (0 == --_size)
		{
			_tail = &_head;
		}
		node* frontNode = _head._next;
		_head._next = frontNode->_next;
		((T*)frontNode->_data)->~T();
		_alloc.deallocate(frontNode);
	}

	size_t size()
	{
		return _size;
	}

	bool empty()
	{
		return !_size;
	}

	void clear()
	{
		node* pIt = _head._next;
		while (pIt)
		{
			_size--;
			assert((int)_size >= 0);
			((T*)pIt->_data)->~T();
			node* t = pIt;
			pIt = pIt->_next;
			_alloc.deallocate(t);
		}
		assert(0 == _size);
		_head._next = NULL;
		_tail = &_head;
	}

	void expand_fixed(size_t fixedSize)
	{
		if (fixedSize > _alloc._poolMaxSize)
		{
			_alloc._poolMaxSize = fixedSize;
		}
	}
private:
	node* new_back()
	{
		node* newNode = (node*)_alloc.allocate();
		newNode->_next = NULL;
		_tail->_next = newNode;
		_tail = newNode;
		return newNode;
	}

	node* new_front()
	{
		node* newNode = (node*)_alloc.allocate();
		newNode->_next = _head._next;
		_head._next = newNode;
		if (&_head == _tail)
		{
			_tail = newNode;
		}
		return newNode;
	}
private:
	node _head;
	node* _tail;
	size_t _size;
	mem_alloc<node> _alloc;
};

template <typename T, typename AlNod = pool_alloc<T>>
class msg_list : public std::list<T, AlNod>
{
public:
	typedef std::_List_node<T, typename AlNod::pointer> node_type;
	typedef pool_alloc<node_type> node_alloc;
public:
	typedef std::list<T, AlNod> base_list;
public:
	explicit msg_list(size_t poolSize = sizeof(void*))
		:base_list(AlNod(poolSize)) {}

	explicit msg_list(AlNod& Al)
		:base_list(Al) {}

	msg_list(const msg_list& s)
		:base_list(s) {}

	msg_list(msg_list&& s)
		:base_list(std::move((base_list&)s)) {}

	template <typename Al>
	msg_list(const std::list<T, Al>& s, size_t poolSize = -1)
		: base_list(AlNod(-1 == poolSize ? s.size() : poolSize))
	{
		*this = s;
	}
public:
	msg_list& operator=(const msg_list& s)
	{
		(base_list&)*this = (const base_list&)s;
		return *this;
	}

	msg_list& operator=(msg_list&& s)
	{
		(base_list&)*this = std::move((base_list&)s);
		return *this;
	}

	template <typename Al>
	msg_list& operator=(const std::list<T, Al>& s)
	{
		clear();
		for (auto it = s.begin(); s.end() != it; it++)
		{
			push_back(*it);
		}
		return *this;
	}

	template <typename Al>
	void copy_to(std::list<T, Al>& d)
	{
		for (auto it = begin(); end() != it; it++)
		{
			d.push_back(*it);
		}
	}
};

template <typename Tkey, typename Tval, typename AlNod = pool_alloc<std::pair<const Tkey, Tval>> >
class msg_map : public std::map<Tkey, Tval, std::less<Tkey>, AlNod>
{
	typedef std::less<Tkey> Tcmp;
public:
	typedef std::_Tree_node<std::pair<Tkey const, Tval>, typename AlNod::pointer> node_type;
	typedef pool_alloc<node_type> node_alloc;
public:
	typedef std::map<Tkey, Tval, Tcmp, AlNod> base_map;
public:
	explicit msg_map(size_t poolSize = sizeof(void*))
		:base_map(Tcmp(), AlNod(poolSize)){}

	explicit msg_map(AlNod& al)
		:base_map(Tcmp(), al) {}

	msg_map(const msg_map& s)
		:base_map(s) {}

	msg_map(msg_map&& s)
		:base_map(std::move((base_map&)s)) {}

	template <typename Al>
	msg_map(const std::map<Tkey, Tval, Tcmp, Al>& s, size_t poolSize = -1)
		: base_map(Tcmp(), AlNod(-1 == poolSize ? s.size() : poolSize))
	{
		*this = s;
	}
public:
	msg_map& operator=(const msg_map& s)
	{
		(base_map&)*this = (const base_map&)s;
		return *this;
	}

	msg_map& operator=(msg_map&& s)
	{
		(base_map&)*this = std::move((base_map&)s);
		return *this;
	}

	template <typename Al>
	msg_map& operator=(const std::map<Tkey, Tval, Tcmp, Al>& s)
	{
		clear();
		for (auto it = s.begin(); s.end() != it; it++)
		{
			insert(make_pair((const Tkey&)it->first, (const Tval&)it->second));
		}
		return *this;
	}

	template <typename Al>
	void copy_to(std::map<Tkey, Tval, Tcmp, Al>& d)
	{
		for (auto it = begin(); end() != it; it++)
		{
			d.insert(make_pair((const Tkey&)it->first, (const Tval&)it->second));
		}
	}
};

template <typename Tkey, typename AlNod = pool_alloc<Tkey> >
class msg_set : public std::set<Tkey, std::less<Tkey>, AlNod>
{
	typedef std::less<Tkey> Tcmp;
public:
	typedef std::_Tree_node<Tkey, typename AlNod::pointer> node_type;
	typedef pool_alloc<node_type> node_alloc;
public:
	typedef std::set<Tkey, Tcmp, AlNod> base_set;
public:
	explicit msg_set(size_t poolSize = sizeof(void*))
		:base_set(Tcmp(), AlNod(poolSize)){}

	explicit msg_set(AlNod& al)
		:base_set(Tcmp(), al) {}

	msg_set(const msg_set& s)
		:base_set(s) {}

	msg_set(msg_set&& s)
		:base_set(std::move((base_set&)s)) {}

	template <typename Al>
	msg_set(const std::set<Tkey, Tcmp, Al>& s, size_t poolSize = -1)
		: base_set(Tcmp(), AlNod(-1 == poolSize ? s.size() : poolSize))
	{
		*this = s;
	}
public:
	msg_set& operator=(const msg_set& s)
	{
		(base_set&)*this = (const base_set&)s;
		return *this;
	}

	msg_set& operator=(msg_set&& s)
	{
		(base_set&)*this = std::move((base_set&)s);
		return *this;
	}

	template <typename Al>
	msg_set& operator=(const std::set<Tkey, Tcmp, Al>& s)
	{
		clear();
		for (auto it = s.begin(); s.end() != it; it++)
		{
			insert(*it);
		}
		return *this;
	}

	template <typename Al>
	void copy_to(std::set<Tkey, Tcmp, Al>& d)
	{
		for (auto it = begin(); end() != it; it++)
		{
			d.insert(*it);
		}
	}
};

template <typename T, typename TMtx = boost::mutex>
class msg_list_shared_alloc : public msg_list<T, pool_alloc_mt<typename msg_list<T>::node_type, TMtx>>
{
	typedef pool_alloc_mt<typename msg_list<T>::node_type, TMtx> AlNod;
public:
	typedef AlNod shared_node_alloc;
public:
	explicit msg_list_shared_alloc(size_t poolSize = sizeof(void*))
	:msg_list(poolSize) {}

	explicit msg_list_shared_alloc(AlNod& Al)
		: msg_list(Al) {}

	msg_list_shared_alloc(const msg_list& s)
		:msg_list(s) {}

	msg_list_shared_alloc(msg_list_shared_alloc&& s)
		:msg_list(std::move((msg_list&)s)) {}

	template <typename Al>
	msg_list_shared_alloc(const std::list<T, Al>& s, size_t poolSize = -1)
		: msg_list(s, poolSize) {}

	msg_list_shared_alloc& operator=(msg_list_shared_alloc&& s)
	{
		(msg_list&)*this = std::move((msg_list&)s);
		return *this;
	}
};

template <typename Tkey, typename Tval, typename TMtx = boost::mutex>
class msg_map_shared_alloc : public msg_map<Tkey, Tval, pool_alloc_mt<typename msg_map<Tkey, Tval>::node_type, TMtx>>
{
	typedef pool_alloc_mt<typename msg_map<Tkey, Tval>::node_type, TMtx> AlNod;
public:
	typedef AlNod shared_node_alloc;
public:
	explicit msg_map_shared_alloc(size_t poolSize = sizeof(void*))
		:msg_map(poolSize){}

	explicit msg_map_shared_alloc(AlNod& al)
		:msg_map(al) {}

	msg_map_shared_alloc(const msg_map& s)
		:msg_map(s) {}

	msg_map_shared_alloc(msg_map_shared_alloc&& s)
		:msg_map(std::move((msg_map&)s)) {}

	template <typename Al>
	msg_map_shared_alloc(const std::map<Tkey, Tval, Tcmp, Al>& s, size_t poolSize = -1)
		: msg_map(s, poolSize) {}

	msg_map_shared_alloc& operator=(msg_map_shared_alloc&& s)
	{
		(msg_map&)*this = std::move((msg_map&)s);
		return *this;
	}
};

template <typename Tkey, typename TMtx = boost::mutex>
class msg_set_shared_alloc : public msg_set<Tkey, pool_alloc_mt<typename msg_set<Tkey>::node_type, TMtx>>
{
	typedef pool_alloc_mt<typename msg_set<Tkey>::node_type, TMtx> AlNod;
public:
	typedef AlNod shared_node_alloc;
public:
	explicit msg_set_shared_alloc(size_t poolSize = sizeof(void*))
		:msg_set(poolSize){}

	explicit msg_set_shared_alloc(AlNod& al)
		:msg_set(al) {}

	msg_set_shared_alloc(const msg_set& s)
		:msg_set(s) {}

	msg_set_shared_alloc(msg_set_shared_alloc&& s)
		:msg_set(std::move((msg_set&)s)) {}

	template <typename Al>
	msg_set_shared_alloc(const std::set<Tkey, Tcmp, Al>& s, size_t poolSize = -1)
		: msg_set(s, poolSize) {}

	msg_set_shared_alloc& operator=(msg_set_shared_alloc&& s)
	{
		(msg_set&)*this = std::move((msg_set&)s);
		return *this;
	}
};

#endif