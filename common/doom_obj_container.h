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

template <typename ObjType, typename IdxType> class DoomObjectContainer;

//----------------------------------------------------------------------------------------------
// DoomObjectContainer replaces the global doom object pointers (states, mobjinfo,
// sprnames) with functor objects that can handle negative indices. It also auto-resizes,
// similar to vector, and provides a way to get the size and capcity Additionally, it has
// implicit type coercion operators to the base pointer type to be compatible with
// existing code.
//----------------------------------------------------------------------------------------------

template <typename ObjType, typename IdxType>
class DoomObjectContainer
{
  public:   	
	typedef void (*ResetObjType)(ObjType*,IdxType);

  private:
	typedef OHashTable<int, int> IndexTable;
	typedef DoomObjectContainer<ObjType, IdxType> DoomObjectContainerType;

	ObjType* container;
	size_t num_types;
	size_t _capacity;
	IndexTable indices_map;
	ResetObjType rf;
	static void noop(ObjType* p, IdxType idx) { return; }
	
  public:

	DoomObjectContainer(ResetObjType f = nullptr);
	DoomObjectContainer(size_t num_types, ResetObjType f = nullptr);
	DoomObjectContainer(ObjType* pointer, size_t num_types, ResetObjType f = nullptr);
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
	void insert(const ObjType& pointer, int idx = 0);
	void append(const DoomObjectContainer<ObjType, IdxType>& container);
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

// TODO: typename parameter for reset function such as D_ResetStates(int from, int to)
// TODO: operator[](int) resizes and returns the reset value

template <typename ObjType, typename IdxType>
DoomObjectContainer<ObjType, IdxType>::DoomObjectContainer(ResetObjType f)
    : container(nullptr), num_types(0), _capacity(0), rf(f == nullptr ? &noop : f)
{
}

template <typename ObjType, typename IdxType>
DoomObjectContainer<ObjType, IdxType>::DoomObjectContainer(size_t count, ResetObjType f)
    : num_types(count), _capacity(count), rf(f == nullptr ? &noop : f)
{
	this->container = (ObjType*)M_Calloc(count, sizeof(ObjType));
}

template <typename ObjType, typename IdxType>
DoomObjectContainer<ObjType, IdxType>::DoomObjectContainer(ObjType* pointer, size_t count,
                                                           ResetObjType f)
{
	this->container = M_Calloc(count, sizeof(ObjType));
	this->num_types = count;
	if (pointer != nullptr)
	{
		for (int i = 0; i < count; i++)
		{
			this->container[i] = pointer[i];
		}
		this->num_types = count;
		this->_capacity = count;
	}
	this->rf = f == nullptr ? &noop : f;
#if defined _DEBUG
	// Printf(PRINT_HIGH, "D_Initialize_states:: allocated %d states.\n", count);
#endif
}

template <typename ObjType, typename IdxType>
DoomObjectContainer<ObjType, IdxType>::~DoomObjectContainer()
{
	if (this->container != nullptr)
	{
		M_Free_Ref(this->container);
	}
}

template <typename ObjType, typename IdxType>
ObjType& DoomObjectContainer<ObjType, IdxType>::operator[](int idx)
{
	// similar to std::unordered_map::operator[] we re-size and return the zero'd out
	// ObjType
	// if ((size_t)idx > this->capacity())
	//{
	//	this->resize(this->capacity() * 2);
	//}
	return this->container[(size_t)idx];
}

template <typename ObjType, typename IdxType>
const ObjType& DoomObjectContainer<ObjType, IdxType>::operator[](int idx) const
{
	return const_cast<ObjType&>(this->container[(size_t)idx]);
}

template <typename ObjType, typename IdxType>
bool DoomObjectContainer<ObjType, IdxType>::operator==(const ObjType* p) const
{
	return this->pointer == p;
}

template <typename ObjType, typename IdxType>
bool DoomObjectContainer<ObjType, IdxType>::operator!=(const ObjType* p) const
{
	return this->pointer != p;
}

template <typename ObjType, typename IdxType>
DoomObjectContainer<ObjType, IdxType>::operator const ObjType*(void) const
{
	return const_cast<ObjType*>(this->container);
}
template <typename ObjType, typename IdxType>
DoomObjectContainer<ObjType, IdxType>::operator ObjType*(void)
{
	return this->container;
}

template <typename ObjType, typename IdxType>
size_t DoomObjectContainer<ObjType, IdxType>::size() const
{
	return this->num_types;
}

template <typename ObjType, typename IdxType>
size_t DoomObjectContainer<ObjType, IdxType>::capacity() const
{
	return this->_capacity;
}

template <typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::clear()
{
	memset(this->container, 0, this->capacity());
	this->num_types = 0;
}

template <typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::resize(size_t count)
{
	if (this->capacity() > count)
	{
		this->container = (ObjType*)M_Realloc(this->container, count * sizeof(ObjType));
		if (this->size() > count)
		{
			this->num_types = count;
		}
		this->_capacity = count;
	}
	else if (this->capacity() <= count)
	{
		reserve(count);
	}
}

template<typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::reserve(size_t new_cap)
{
	if (this->capacity() <= new_cap)
	{
		this->container = (ObjType*)M_Realloc(this->container, new_cap * sizeof(ObjType));
		memset(this->container + this->capacity(), 0,
		       (new_cap - this->capacity()) * sizeof(ObjType));
		for (int i = this->_capacity; i < new_cap; i++)
		{
			this->rf(&this->container[i], (IdxType)i);
		}
		this->_capacity = new_cap;
	}
}

template <typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::insert(const ObjType& obj, int index)
{
	if (this->size() + 1 > this->capacity())
	{
		this->resize(this->capacity() * 2);
	}
	this->container[index] = obj;
	this->num_types++;
}

template <typename ObjType, typename IdxType>
void DoomObjectContainer<ObjType, IdxType>::append(
    const DoomObjectContainer<ObjType, IdxType>& container)
{
	int c_capacity = container.capacity();
	for (int i = 0; i < c_capacity; i++)
	{
		this->insert(container[i]);
	}
}

template <typename ObjType, typename IdxType>
ObjType* DoomObjectContainer<ObjType, IdxType>::ptr(void)
{
	return this->container;
}

template <typename ObjType, typename IdxType>
const ObjType* DoomObjectContainer<ObjType, IdxType>::ptr(void) const
{
	return const_cast<ObjType*>(this->container);
}

//----------------------------------------------------------------------------------------------