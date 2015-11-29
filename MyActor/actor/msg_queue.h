#ifndef __MSG_QUEUE_H
#define __MSG_QUEUE_H

#include "mem_pool.h"
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
		:_alloc(poolSize), _size(0), _head(NULL), _tail(NULL) {}

	~msg_queue()
	{
		clear();
	}

	void push_back(const T& p)
	{
		_size++;
		new(new_back()->_data)T(p);
	}

	void push_back(T&& p)
	{
		_size++;
		new(new_back()->_data)T(std::move(p));
	}

	void push_front(const T& p)
	{
		_size++;
		new(new_front()->_data)T(p);
	}

	void push_front(T&& p)
	{
		_size++;
		new(new_front()->_data)T(std::move(p));
	}

	T& front()
	{
		assert(_size && _head);
		return *(T*)_head->_data;
	}

	T& back()
	{
		assert(_size && _tail);
		return *(T*)_tail->_data;
	}

	void pop_front()
	{
		assert(_size);
		node* frontNode = _head;
		_head = _head->_next;
		((T*)frontNode->_data)->~T();
		_alloc.deallocate(frontNode);
		if (0 == --_size)
		{
			assert(!_head);
			_tail = NULL;
		}
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
		node* pIt = _head;
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
		_head = NULL;
		_tail = NULL;
	}

	void expand_fixed(size_t fixedSize)
	{
		if (fixedSize > _alloc._poolMaxSize)
		{
			_alloc._poolMaxSize = fixedSize;
		}
	}

	size_t fixed_size()
	{
		return _alloc._poolMaxSize;
	}
private:
	node* new_back()
	{
		node* newNode = (node*)_alloc.allocate();
		newNode->_next = NULL;
		if (!_head)
		{
			_head = newNode;
		}
		else
		{
			_tail->_next = newNode;
		}
		_tail = newNode;
		return newNode;
	}

	node* new_front()
	{
		node* newNode = (node*)_alloc.allocate();
		newNode->_next = _head;
		_head = newNode;
		if (!_tail)
		{
			_tail = newNode;
		}
		return newNode;
	}
private:
	node* _head;
	node* _tail;
	size_t _size;
	mem_alloc<node> _alloc;
};
//////////////////////////////////////////////////////////////////////////

template <typename T, typename AlNod = pool_alloc<T>>
class msg_list : public std::list<T, AlNod>
{
	typedef std::list<T, AlNod> Parent;
public:
#ifdef _MSC_VER
	typedef std::_List_node<T, typename AlNod::pointer> node_type;
#elif __GNUG__
	typedef std::_List_node<T> node_type;
#endif
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
		Parent::clear();
		for (auto& i : s)
		{
			Parent::push_back(i);
		}
		return *this;
	}

	template <typename Al>
	void copy_to(std::list<T, Al>& d)
	{
		for (auto& i : (*this))
		{
			d.push_back(i);
		}
	}
};
//////////////////////////////////////////////////////////////////////////

template <typename Tkey, typename Tval, typename AlNod = pool_alloc<std::pair<const Tkey, Tval>> >
class msg_map : public std::map<Tkey, Tval, std::less<Tkey>, AlNod>
{
	typedef std::less<Tkey> Tcmp;
	typedef std::map<Tkey, Tval, std::less<Tkey>, AlNod> Parent;
public:
#ifdef _MSC_VER
	typedef std::_Tree_node<std::pair<Tkey const, Tval>, typename AlNod::pointer> node_type;
#elif __GNUG__
	typedef std::_Rb_tree_node<std::pair<Tkey const, Tval>> node_type;
#endif
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
		Parent::clear();
		for (auto& i : s)
		{
			Parent::insert(i);
		}
		return *this;
	}

	template <typename Al>
	void copy_to(std::map<Tkey, Tval, Tcmp, Al>& d)
	{
		for (auto& i : (*this))
		{
			d.insert(i);
		}
	}
};
//////////////////////////////////////////////////////////////////////////

template <typename Tkey, typename Tval, typename AlNod = pool_alloc<std::pair<const Tkey, Tval>> >
class msg_multimap : public std::multimap<Tkey, Tval, std::less<Tkey>, AlNod>
{
	typedef std::less<Tkey> Tcmp;
	typedef std::multimap<Tkey, Tval, std::less<Tkey>, AlNod> Parent;
public:
#ifdef _MSC_VER
	typedef std::_Tree_node<std::pair<Tkey const, Tval>, typename AlNod::pointer> node_type;
#elif __GNUG__
	typedef std::_Rb_tree_node<std::pair<Tkey const, Tval>> node_type;
#endif
	typedef pool_alloc<node_type> node_alloc;
public:
	typedef std::multimap<Tkey, Tval, Tcmp, AlNod> base_map;
public:
	explicit msg_multimap(size_t poolSize = sizeof(void*))
		:base_map(Tcmp(), AlNod(poolSize)){}

	explicit msg_multimap(AlNod& al)
		:base_map(Tcmp(), al) {}

	msg_multimap(const msg_multimap& s)
		:base_map(s) {}

	msg_multimap(msg_multimap&& s)
		:base_map(std::move((base_map&)s)) {}

	template <typename Al>
	msg_multimap(const std::multimap<Tkey, Tval, Tcmp, Al>& s, size_t poolSize = -1)
		: base_map(Tcmp(), AlNod(-1 == poolSize ? s.size() : poolSize))
	{
		*this = s;
	}
public:
	msg_multimap& operator=(const msg_multimap& s)
	{
		(base_map&)*this = (const base_map&)s;
		return *this;
	}

	msg_multimap& operator=(msg_multimap&& s)
	{
		(base_map&)*this = std::move((base_map&)s);
		return *this;
	}

	template <typename Al>
	msg_multimap& operator=(const std::multimap<Tkey, Tval, Tcmp, Al>& s)
	{
		Parent::clear();
		for (auto& i : s)
		{
			Parent::insert(i);
		}
		return *this;
	}

	template <typename Al>
	void copy_to(std::multimap<Tkey, Tval, Tcmp, Al>& d)
	{
		for (auto& i : (*this))
		{
			d.insert(i);
		}
	}
};
//////////////////////////////////////////////////////////////////////////

template <typename Tkey, typename AlNod = pool_alloc<Tkey> >
class msg_set : public std::set<Tkey, std::less<Tkey>, AlNod>
{
	typedef std::less<Tkey> Tcmp;
	typedef std::set<Tkey, std::less<Tkey>, AlNod> Parent;
public:
#ifdef _MSC_VER
	typedef std::_Tree_node<Tkey, typename AlNod::pointer> node_type;
#elif __GNUG__
	typedef std::_Rb_tree_node<Tkey> node_type;
#endif
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
		Parent::clear();
		for (auto& i : s)
		{
			Parent::insert(i);
		}
		return *this;
	}

	template <typename Al>
	void copy_to(std::set<Tkey, Tcmp, Al>& d)
	{
		for (auto& i : (*this))
		{
			d.insert(i);
		}
	}
};
//////////////////////////////////////////////////////////////////////////

template <typename Tkey, typename AlNod = pool_alloc<Tkey> >
class msg_multiset : public std::multiset<Tkey, std::less<Tkey>, AlNod>
{
	typedef std::less<Tkey> Tcmp;
	typedef std::multiset<Tkey, std::less<Tkey>, AlNod> Parent;
public:
#ifdef _MSC_VER
	typedef std::_Tree_node<Tkey, typename AlNod::pointer> node_type;
#elif __GNUG__
	typedef std::_Rb_tree_node<Tkey> node_type;
#endif
	typedef pool_alloc<node_type> node_alloc;
public:
	typedef std::multiset<Tkey, Tcmp, AlNod> base_set;
public:
	explicit msg_multiset(size_t poolSize = sizeof(void*))
		:base_set(Tcmp(), AlNod(poolSize)){}

	explicit msg_multiset(AlNod& al)
		:base_set(Tcmp(), al) {}

	msg_multiset(const msg_multiset& s)
		:base_set(s) {}

	msg_multiset(msg_multiset&& s)
		:base_set(std::move((base_set&)s)) {}

	template <typename Al>
	msg_multiset(const std::multiset<Tkey, Tcmp, Al>& s, size_t poolSize = -1)
		: base_set(Tcmp(), AlNod(-1 == poolSize ? s.size() : poolSize))
	{
		*this = s;
	}
public:
	msg_multiset& operator=(const msg_multiset& s)
	{
		(base_set&)*this = (const base_set&)s;
		return *this;
	}

	msg_multiset& operator=(msg_multiset&& s)
	{
		(base_set&)*this = std::move((base_set&)s);
		return *this;
	}

	template <typename Al>
	msg_multiset& operator=(const std::multiset<Tkey, Tcmp, Al>& s)
	{
		Parent::clear();
		for (auto& i : s)
		{
			Parent::insert(i);
		}
		return *this;
	}

	template <typename Al>
	void copy_to(std::multiset<Tkey, Tcmp, Al>& d)
	{
		for (auto& i : (*this))
		{
			d.insert(i);
		}
	}
};
//////////////////////////////////////////////////////////////////////////

template <typename T, typename TMtx = std::mutex>
class msg_list_shared_alloc : public msg_list<T, pool_alloc_mt<typename msg_list<T>::node_type, TMtx>>
{
	typedef pool_alloc_mt<typename msg_list<T>::node_type, TMtx> AlNod;
	typedef msg_list<T, pool_alloc_mt<typename msg_list<T>::node_type, TMtx>> Parent;
public:
	typedef AlNod shared_node_alloc;
public:
	explicit msg_list_shared_alloc(size_t poolSize = sizeof(void*))
		:Parent(poolSize) {}

	explicit msg_list_shared_alloc(AlNod& Al)
		: Parent(Al) {}

	msg_list_shared_alloc(const Parent& s)
		:Parent(s) {}

	msg_list_shared_alloc(msg_list_shared_alloc&& s)
		:Parent(std::move((Parent&)s)) {}

	template <typename Al>
	msg_list_shared_alloc(const std::list<T, Al>& s, size_t poolSize = -1)
		: Parent(s, poolSize) {}

	msg_list_shared_alloc& operator=(msg_list_shared_alloc&& s)
	{
		(Parent&)*this = std::move((Parent&)s);
		return *this;
	}
};
//////////////////////////////////////////////////////////////////////////

template <typename Tkey, typename Tval, typename TMtx = std::mutex>
class msg_map_shared_alloc : public msg_map<Tkey, Tval, pool_alloc_mt<typename msg_map<Tkey, Tval>::node_type, TMtx>>
{
	typedef pool_alloc_mt<typename msg_map<Tkey, Tval>::node_type, TMtx> AlNod;
	typedef msg_map<Tkey, Tval, pool_alloc_mt<typename msg_map<Tkey, Tval>::node_type, TMtx>> Parent;
public:
	typedef AlNod shared_node_alloc;
public:
	explicit msg_map_shared_alloc(size_t poolSize = sizeof(void*))
		:Parent(poolSize){}

	explicit msg_map_shared_alloc(AlNod& al)
		:Parent(al) {}

	msg_map_shared_alloc(const Parent& s)
		:Parent(s) {}

	msg_map_shared_alloc(msg_map_shared_alloc&& s)
		:Parent(std::move((Parent&)s)) {}

	template <typename Al, typename Tcmp>
	msg_map_shared_alloc(const std::map<Tkey, Tval, Tcmp, Al>& s, size_t poolSize = -1)
		: Parent(s, poolSize) {}

	msg_map_shared_alloc& operator=(msg_map_shared_alloc&& s)
	{
		(Parent&)*this = std::move((Parent&)s);
		return *this;
	}
};
//////////////////////////////////////////////////////////////////////////

template <typename Tkey, typename Tval, typename TMtx = std::mutex>
class msg_multimap_shared_alloc : public msg_multimap<Tkey, Tval, pool_alloc_mt<typename msg_multimap<Tkey, Tval>::node_type, TMtx>>
{
	typedef pool_alloc_mt<typename msg_multimap<Tkey, Tval>::node_type, TMtx> AlNod;
	typedef msg_multimap<Tkey, Tval, pool_alloc_mt<typename msg_multimap<Tkey, Tval>::node_type, TMtx>> Parent;
public:
	typedef AlNod shared_node_alloc;
public:
	explicit msg_multimap_shared_alloc(size_t poolSize = sizeof(void*))
		:Parent(poolSize){}

	explicit msg_multimap_shared_alloc(AlNod& al)
		:Parent(al) {}

	msg_multimap_shared_alloc(const Parent& s)
		:Parent(s) {}

	msg_multimap_shared_alloc(msg_multimap_shared_alloc&& s)
		:Parent(std::move((Parent&)s)) {}

	template <typename Al, typename Tcmp>
	msg_multimap_shared_alloc(const std::map<Tkey, Tval, Tcmp, Al>& s, size_t poolSize = -1)
		: Parent(s, poolSize) {}

	msg_multimap_shared_alloc& operator=(msg_multimap_shared_alloc&& s)
	{
		(Parent&)*this = std::move((Parent&)s);
		return *this;
	}
};
//////////////////////////////////////////////////////////////////////////

template <typename Tkey, typename TMtx = std::mutex>
class msg_set_shared_alloc : public msg_set<Tkey, pool_alloc_mt<typename msg_set<Tkey>::node_type, TMtx>>
{
	typedef pool_alloc_mt<typename msg_set<Tkey>::node_type, TMtx> AlNod;
	typedef msg_set<Tkey, pool_alloc_mt<typename msg_set<Tkey>::node_type, TMtx>> Parent;
public:
	typedef AlNod shared_node_alloc;
public:
	explicit msg_set_shared_alloc(size_t poolSize = sizeof(void*))
		:Parent(poolSize){}

	explicit msg_set_shared_alloc(AlNod& al)
		:Parent(al) {}

	msg_set_shared_alloc(const Parent& s)
		:Parent(s) {}

	msg_set_shared_alloc(msg_set_shared_alloc&& s)
		:Parent(std::move((Parent&)s)) {}

	template <typename Al, typename Tcmp>
	msg_set_shared_alloc(const std::set<Tkey, Tcmp, Al>& s, size_t poolSize = -1)
		: Parent(s, poolSize) {}

	msg_set_shared_alloc& operator=(msg_set_shared_alloc&& s)
	{
		(Parent&)*this = std::move((Parent&)s);
		return *this;
	}
};
//////////////////////////////////////////////////////////////////////////

template <typename Tkey, typename TMtx = std::mutex>
class msg_multiset_shared_alloc : public msg_multiset<Tkey, pool_alloc_mt<typename msg_multiset<Tkey>::node_type, TMtx>>
{
	typedef pool_alloc_mt<typename msg_multiset<Tkey>::node_type, TMtx> AlNod;
	typedef msg_multiset<Tkey, pool_alloc_mt<typename msg_multiset<Tkey>::node_type, TMtx>> Parent;
public:
	typedef AlNod shared_node_alloc;
public:
	explicit msg_multiset_shared_alloc(size_t poolSize = sizeof(void*))
		:Parent(poolSize){}

	explicit msg_multiset_shared_alloc(AlNod& al)
		:Parent(al) {}

	msg_multiset_shared_alloc(const Parent& s)
		:Parent(s) {}

	msg_multiset_shared_alloc(msg_multiset_shared_alloc&& s)
		:Parent(std::move((Parent&)s)) {}

	template <typename Al, typename Tcmp>
	msg_multiset_shared_alloc(const std::set<Tkey, Tcmp, Al>& s, size_t poolSize = -1)
		: Parent(s, poolSize) {}

	msg_multiset_shared_alloc& operator=(msg_multiset_shared_alloc&& s)
	{
		(Parent&)*this = std::move((Parent&)s);
		return *this;
	}
};

#endif