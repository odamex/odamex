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

#include "hashtable.h"
#include "i_system.h"
#include <vector>
#include <cstddef>
#include <typeinfo>

template <typename ObjType, typename IdxType>
class DoomObjectContainer;

//----------------------------------------------------------------------------------------------
// DoomObjectContainer replaces the global doom object pointers (states, mobjinfo,
// sprnames) with multi-use objects that can handle negative indices. It also
// auto-resizes, similar to vector, and provides a way to get the size and capacity.
// Existing code cannot rely on an index being greater than the number of types now
// because dehacked does not enforce contiguous indices i.e. frame 405 could jump to frame
// 1055
//----------------------------------------------------------------------------------------------

template <typename ObjType, typename IdxType = int32_t>
class DoomObjectContainer
{

	typedef OHashTable<int, ObjType> LookupTable;
	typedef std::vector<ObjType> DoomObjectContainerData;
	typedef DoomObjectContainer<ObjType, IdxType> DoomObjectContainerType;

	DoomObjectContainerData container;
	LookupTable lookup_table;

	static void noop(ObjType p, IdxType idx) { }

  public:
	using ObjReference = typename DoomObjectContainerData::reference;
	using ConstObjReference = typename DoomObjectContainerData::const_reference;
	using iterator = typename LookupTable::iterator;
	using const_iterator = typename LookupTable::const_iterator;

	typedef void (*ResetObjType)(ObjType, IdxType);

	explicit DoomObjectContainer(ResetObjType f = nullptr);
	explicit DoomObjectContainer(size_t count, ResetObjType f = nullptr);
	~DoomObjectContainer();

	ObjReference operator[](int);
	ConstObjReference operator[](int) const;
	bool operator==(const ObjType* p) const;
	bool operator!=(const ObjType* p) const;
	// convert to ObjType* to allow pointer arithmetic
	operator const ObjType*() const;
	operator ObjType*();
	// direct access similar to STL data()
	ObjType* data();
	const ObjType* data() const;

	size_t capacity() const;
	size_t size() const;
	void clear();
	void resize(size_t count);
	void reserve(size_t new_cap);
	void insert(const ObjType& obj, IdxType idx);
	void append(const DoomObjectContainerType& dObjContainer);

	iterator begin();
	iterator end();
	const_iterator cbegin();
	const_iterator cend();
	iterator find(IdxType index);
	const_iterator find(IdxType index) const;

	friend ObjType operator-(ObjType obj, DoomObjectContainerType& container);
	friend ObjType operator+(DoomObjectContainerType& container, WORD ofs);

  private:
	ResetObjType rf;
};

//----------------------------------------------------------------------------------------------

// Construction and Destruction

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
{
	clear();
}

// Operators

template <typename ObjType, typename IdxType>
typename DoomObjectContainer<ObjType, IdxType>::ObjReference DoomObjectContainer<
    ObjType, IdxType>::operator[](int idx)
{
	iterator it = this->lookup_table.find(idx);
    if (it == this->end())
    {
	    I_Error("Attempt to access invalid %s at idx %d", typeid(ObjType).name(), idx);
    }
    return it->second;
}

template <typename ObjType, typename IdxType>
typename DoomObjectContainer<ObjType, IdxType>::ConstObjReference DoomObjectContainer<
    ObjType, IdxType>::operator[](int idx) const
{
    const_iterator it = this->lookup_table.find(idx);
    if (it == this->end())
    {
    	I_Error("Attempt to access invalid %s at idx %d", typeid(ObjType).name(), idx);
    }
    return it->second;
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
DoomObjectContainer<ObjType, IdxType>::operator const ObjType*() const
{
	return const_cast<ObjType>(this->container.data());
}
template <typename ObjType, typename IdxType>
DoomObjectContainer<ObjType, IdxType>::operator ObjType*()
{
	return this->container.data();
}

template <typename ObjType, typename IdxType>
ObjType operator-(ObjType obj, DoomObjectContainer<ObjType, IdxType>& container)
{
	return obj - container.data();
}
template <typename ObjType, typename IdxType>
ObjType operator+(DoomObjectContainer<ObjType, IdxType>& container, WORD ofs)
{
	return container.data() + ofs;
}

// PTR functions for quicker access to all

template <typename ObjType, typename IdxType>
ObjType* DoomObjectContainer<ObjType, IdxType>::data()
{
	return this->container.data();
}

template <typename ObjType, typename IdxType>
const ObjType* DoomObjectContainer<ObjType, IdxType>::data() const
{
	return this->container.data();
}

// Capacity and Size

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
	this->container.erase(container.begin(), container.end());
	this->lookup_table.erase(lookup_table.begin(), lookup_table.end());
}

// Allocation changes

template <typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::resize(size_t count)
{
	this->container.resize(count);
	// this->lookup_table.resize(count);
}

template <typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::reserve(size_t new_cap)
{
	this->container.reserve(new_cap);
	// this->lookup_table.resize(new_cap);
}

// Insertion

template <typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::insert(const ObjType& obj, IdxType idx)
{
	this->container.push_back(obj);
	this->lookup_table[static_cast<int>(idx)] = obj;
}

// TODO: more of a copy construct in a sense
template <typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::append(
    const DoomObjectContainer<ObjType, IdxType>& dObjContainer)
{
	for (auto it = dObjContainer.lookup_table.begin();
	     it != dObjContainer.lookup_table.end(); ++it)
	{
		int idx = it->first;
		ObjType obj = it->second;
		this->insert(static_cast<IdxType>(idx), obj);
	}
}

// Iterators

template <typename ObjType, typename IdxType>
typename DoomObjectContainer<ObjType, IdxType>::iterator DoomObjectContainer<ObjType, IdxType>::begin()
{
	return lookup_table.begin();
}

template <typename ObjType, typename IdxType>
typename DoomObjectContainer<ObjType, IdxType>::iterator DoomObjectContainer<ObjType, IdxType>::end()
{
	return lookup_table.end();
}

template <typename ObjType, typename IdxType>
typename DoomObjectContainer<ObjType, IdxType>::const_iterator DoomObjectContainer<ObjType, IdxType>::cbegin()
{
	return lookup_table.begin();
}

template <typename ObjType, typename IdxType>
typename DoomObjectContainer<ObjType, IdxType>::const_iterator DoomObjectContainer<ObjType, IdxType>::cend()
{
	return lookup_table.end();
}

// Lookup

template <typename ObjType, typename IdxType>
typename DoomObjectContainer<ObjType, IdxType>::iterator DoomObjectContainer<
    ObjType, IdxType>::find(IdxType idx)
{
	typename LookupTable::iterator it = this->lookup_table.find(idx);
	if (it != this->lookup_table.end())
	{
		return it;
	}
	return this->lookup_table.end();
}

template <typename ObjType, typename IdxType>
typename DoomObjectContainer<ObjType, IdxType>::const_iterator DoomObjectContainer<
    ObjType, IdxType>::find(IdxType idx) const
{
	typename LookupTable::iterator it = this->lookup_table.find(idx);
	if (it != this->lookup_table.end())
	{
		return it;
	}
	return this->lookup_table.end();
}

//----------------------------------------------------------------------------------------------
