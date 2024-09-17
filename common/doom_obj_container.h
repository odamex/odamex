//----------------------------------------------------------------------------------------------
// odamex enum definitions with negative indices (similar to id24 specification) are in
// info.h using negative indices prevents overriding during dehacked (though id24 allows
// this)
//
// id24 allows 0x80000000-0x8FFFFFFF (abbreviated as 0x8000-0x8FFF) for negative indices
// for source port implementation id24 spec no longer uses enum to define the indices for
// doom objects, but until that is implemented enums will continue to be used
//----------------------------------------------------------------------------------------------

#pragma once

#include <vector>
#include "hashtable.h"

template <typename ObjType, typename IdxType> class DoomObjectContainer;

//----------------------------------------------------------------------------------------------
// DoomObjectContainer replaces the global doom object pointers (states, mobjinfo,
// sprnames) with functor objects that can handle negative indices. It also auto-resizes,
// similar to vector, and provides a way to get the size and capcity Additionally, it has
// implicit type coercion operators to the base pointer type to be compatible with
// existing code.
// Existing code cannot rely on an index being greater than the number of types now because dehacked does not
// enforce contiguous indices i.e. frame 405 could jump to frame 1055
//----------------------------------------------------------------------------------------------

template <typename ObjType, typename IdxType>
class DoomObjectContainer
{
  public:   	
	typedef void (*ResetObjType)(ObjType*, IdxType);

  private:
	typedef OHashTable<int, int> IndexTable;
	typedef DoomObjectContainer<ObjType, IdxType> DoomObjectContainerType;

	std::vector<ObjType> container;
	IndexTable indices_map;
	ResetObjType rf;
	static void noop(ObjType* p, IdxType idx) { return; }
	
  public:

	DoomObjectContainer(ResetObjType f = nullptr);
	DoomObjectContainer(size_t num_types, ResetObjType f = nullptr);
	~DoomObjectContainer();

	ObjType& operator[](int);
	const ObjType& operator[](int) const;
	bool operator==(const ObjType* p) const;
	bool operator!=(const ObjType* p) const;
	// convert to ObjType* to allow pointer arithmetic
	operator const ObjType*(void) const;
	operator ObjType*(void);

	size_t capacity() const;
	size_t size() const;
	void clear();
	void resize(size_t count);
	void reserve(size_t new_cap);
	void insert(const ObjType& pointer, IdxType idx);
	void append(const DoomObjectContainer<ObjType, IdxType>& container);
    ObjType* find(int);
	// [CMB] TODO: this method needs to go, but for now its provided for compatibility
	ObjType* ptr(void);
	const ObjType* ptr(void) const;

	// ------------------------------------------------------------------------
	// DoomObjectContainer::iterator & const_iterator implementation
	// ------------------------------------------------------------------------

	template <typename IObjType, typename IDOBT> class generic_iterator;
	typedef generic_iterator<ObjType, DoomObjectContainerType> iterator;
	typedef generic_iterator<const ObjType, const DoomObjectContainerType> const_iterator;
};

//----------------------------------------------------------------------------------------------

// inside class

template <typename ObjType, typename IdxType>
DoomObjectContainer<ObjType, IdxType>::DoomObjectContainer(ResetObjType f)
    : rf(f == nullptr ? &noop : f)
{
}

template <typename ObjType, typename IdxType>
DoomObjectContainer<ObjType, IdxType>::DoomObjectContainer(size_t count, ResetObjType f)
    : rf(f == nullptr ? &noop : f)
{
	this->container.reserve(count);
}

template <typename ObjType, typename IdxType>
DoomObjectContainer<ObjType, IdxType>::~DoomObjectContainer()
{}

template <typename ObjType, typename IdxType>
ObjType& DoomObjectContainer<ObjType, IdxType>::operator[](int idx)
{
	IndexTable::iterator it = this->indices_map.find(idx);
	int realidx;
	if (it != this->indices_map.end())
	{
		realidx = it->second;
	}
    // we did not find an element: add a blank element at the end, map it, and return it
	else
    {
        this->container.emplace_back();
        this->rf(&this->container.back(), (IdxType) idx);
        this->indices_map[idx] = this->container.size() - 1;
		realidx = this->container.size() - 1;
    }
	return this->container[realidx];
}

template <typename ObjType, typename IdxType>
const ObjType& DoomObjectContainer<ObjType, IdxType>::operator[](int idx) const
{
    ObjType& o = (*this)[idx];
	return const_cast<ObjType&>(o);
}

template <typename ObjType, typename IdxType>
bool DoomObjectContainer<ObjType, IdxType>::operator==(const ObjType* p) const
{
	return this->container().data() == p;
}

template <typename ObjType, typename IdxType>
bool DoomObjectContainer<ObjType, IdxType>::operator!=(const ObjType* p) const
{
	return this->container().data() != p;
}

template <typename ObjType, typename IdxType>
DoomObjectContainer<ObjType, IdxType>::operator const ObjType*(void) const
{
	return const_cast<ObjType*>(this->container.data());
}
template <typename ObjType, typename IdxType>
DoomObjectContainer<ObjType, IdxType>::operator ObjType*(void)
{
	return this->container.data();
}

template <typename ObjType, typename IdxType>
size_t DoomObjectContainer<ObjType, IdxType>::size() const
{
	return this->container.size();
}

template <typename ObjType, typename IdxType>
size_t DoomObjectContainer<ObjType, IdxType>::capacity() const
{
	return this->container.capacity();
}

template <typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::clear()
{
	this->container.clear();
	this->indices_map.clear();
}

template <typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::resize(size_t count)
{
	int old_size = this->container.size();
	this->container.resize(count);
	if (old_size < this->container.size())
	{
		for (int i = old_size; i < this->container.size(); i++)
		{
			memset(&this->container[i], 0, sizeof(ObjType));
			this->rf(&this->container[i], (IdxType)i);
		}
	}
}

template<typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::reserve(size_t new_cap)
{
	this->container.reserve(new_cap);
}

template <typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::insert(const ObjType& obj, IdxType idx)
{
	this->container.push_back(obj);
	this->indices_map[(int) idx] = this->container.size() - 1;
}

template <typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::append(
    const DoomObjectContainer<ObjType, IdxType>& dObjContainer)
{
	this->container.insert(this->container.end(), dObjContainer.container.begin(),
	                       dObjContainer.container.end());
    // [CMB] TODO: need to append indices_maps together
	// this->indices_map.insert(dObjContainer.indices_map.begin(), dObjContainer.indices_map.end());
}

template<typename ObjType, typename IdxType>
ObjType* DoomObjectContainer<ObjType, IdxType>::find(int idx)
{
    IndexTable::iterator it = this->indices_map.find(idx);
    if (it != this->indices_map.end())
    {
        return &this->container[it->second];
    }
    return NULL;
}

template <typename ObjType, typename IdxType>
ObjType* DoomObjectContainer<ObjType, IdxType>::ptr(void)
{
	return this->container.data();
}

template <typename ObjType, typename IdxType>
const ObjType* DoomObjectContainer<ObjType, IdxType>::ptr(void) const
{
	return const_cast<ObjType*>(this->container.data());
}

//----------------------------------------------------------------------------------------------
