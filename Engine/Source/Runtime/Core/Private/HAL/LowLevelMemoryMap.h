//-----------------------------------------------------------------------------
// File:		LowLevelMemoryMap.h
//
// Summary:		
//
// Created:		24/03/2015
//
// Author:		mailto:a-slynch@microsoft.com
//
//              Copyright (C) Microsoft. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

//-----------------------------------------------------------------
template<typename TKey, typename TValue>
class VirtualMap
{
public:
// 	class CriticalSectionScope
// 	{
// 	public:
// 		CriticalSectionScope(FCriticalSection& cs) : m_CriticalSection(cs) { EnterCriticalSection(&cs); }
// 		~CriticalSectionScope() { LeaveCriticalSection(&m_CriticalSection); }
// 		FCriticalSection& m_CriticalSection;
// 	};

	//-----------------------------------------------------------------
	struct Pair
	{
		TKey m_Key;
		TValue m_Value;
	};

	//-----------------------------------------------------------------
	// The default capacity of the set. The capacity is the number
	// of elements that the set is expected to hold. The set will resized
	// when the item count is greater than the capacity;
	VirtualMap(const int capacity=m_DefaultCapacity, int alloc_size=m_DefaultAllocSize)
	:	m_AllocatedMemory(0),
		m_Capacity(0),
		mp_Table(NULL),
		m_Count(0),
		m_AllocSize(alloc_size),
		mp_ItemPool(NULL),
		mp_FreePair(NULL)
#ifdef PROFILE_VirtualMap
		,m_IterAcc(0)
		,m_IterCount(0)
#endif
	{
		//InitializeCriticalSection(&m_CriticalSection);

		m_Capacity = GetNextPow2((256 * capacity) / m_Margin);
		check(m_Capacity < m_MaxCapacity);
	}

	//-----------------------------------------------------------------
	~VirtualMap()
	{
		Clear();

		Free(mp_Table, m_Capacity * sizeof(Pair*));
		FreePools();
	}

	int64 GetTotalMemoryUsed()
	{
		return m_AllocatedMemory;
	}

	//-----------------------------------------------------------------
	void Clear()
	{
		RemoveAll();
	}

	//-----------------------------------------------------------------
	// Add a value to this set.
	// If this set already contains the value does nothing.
	void Add(const TKey& key, const TValue& value)
	{
		FScopeLock Lock(&m_CriticalSection);
		//CriticalSectionScope lock(m_CriticalSection);

		if (mp_Table == NULL)
		{
			AllocTable();
		}

		int index = GetItemIndex(key);

		if(IsItemInUse(index))
		{
			mp_Table[index]->m_Value = value;
		}
		else
		{
			if(m_Capacity == 0 || m_Count == (m_Margin * m_Capacity) / 256)
			{
				Grow();
				index = GetItemIndex(key);
			}

			// make a copy of the value
			Pair* p_pair = AllocPair();
			p_pair->m_Key = key;
			p_pair->m_Value = value;

			// add to table
			mp_Table[index] = p_pair;

			++m_Count;
		}
	}

	//-----------------------------------------------------------------
	TValue GetValue(const TKey& key)
	{
		FScopeLock Lock(&m_CriticalSection);
//		CriticalSectionScope lock(m_CriticalSection);

		check(mp_Table);

		int remove_index = GetItemIndex(key);
		check(IsItemInUse(remove_index));

		Pair* p_pair = mp_Table[remove_index];

		return p_pair->m_Value;
	}

	//-----------------------------------------------------------------
	TValue Remove(const TKey& key)
	{
		FScopeLock Lock(&m_CriticalSection);
//		CriticalSectionScope lock(m_CriticalSection);

		check(mp_Table);

		int remove_index = GetItemIndex(key);
		check(IsItemInUse(remove_index));

		Pair* p_pair = mp_Table[remove_index];

		// find first index in this array
		int srch_index = remove_index;
		int first_index = remove_index;
		if(!srch_index)
		{
			srch_index = m_Capacity;
		}
		--srch_index;
		while(IsItemInUse(srch_index))
		{
			first_index = srch_index;
			if(!srch_index)
			{
				srch_index = m_Capacity;
			}
			--srch_index;
		}

		bool found = false;
		for(;;)
		{
			// find the last item in the array that can replace the item being removed
			int srch_index2 = (remove_index + 1) & (m_Capacity-1);

			int swap_index = m_InvalidIndex;
			while(IsItemInUse(srch_index2))
			{
				const unsigned int srch_hash_code = mp_Table[srch_index2]->m_Key.GetHashCode();
				const int srch_insert_index = srch_hash_code & (m_Capacity-1);

				if(InRange(srch_insert_index, first_index, remove_index))
				{
					swap_index = srch_index2;
					found = true;
				}

				srch_index2 = (srch_index2 + 1) & (m_Capacity-1);
			}

			// swap the item
			if(found)
			{
				mp_Table[remove_index] = mp_Table[swap_index];
				remove_index = swap_index;
				found = false;
			}
			else
			{
				break;
			}
		}

		// remove the last item
		mp_Table[remove_index] = NULL;

		TValue value = p_pair->m_Value;

		// free this item
		FreePair(p_pair);

		--m_Count;
		return value;
	}

	//-----------------------------------------------------------------
	int GetCount() const
	{
		return m_Count;
	}

	bool HasKey(const TKey& key)
	{
		FScopeLock Lock(&m_CriticalSection);
		return IsItemInUse(GetItemIndex(key));
	}

	//-----------------------------------------------------------------
	void Resize(int new_capacity)
	{
		FScopeLock Lock(&m_CriticalSection);
//		CriticalSectionScope lock(m_CriticalSection);

		new_capacity = GetNextPow2(new_capacity);

		// keep a copy of the old table
		Pair** const p_old_table = mp_Table;
		const int old_capacity = m_Capacity;

		// allocate the new table
		m_Capacity = new_capacity;
		AllocTable();

		// copy the values from the old to the new table
		Pair** p_old_pair = p_old_table;
		for(int i=0; i<old_capacity; ++i, ++p_old_pair)
		{
			Pair* p_pair = *p_old_pair;
			if(p_pair)
			{
				const int index = GetItemIndex(p_pair->m_Key);
				mp_Table[index] = p_pair;
			}
		}

		Free(p_old_table, old_capacity * sizeof(Pair*));
	}

private:
	//-----------------------------------------------------------------
	void* Alloc(size_t size)
	{
// #if PLATFORM_XBOXONE
// 		int extra_flags = MEM_TITLE;
// #else
// 		int extra_flags = 0;
// #endif
//		return VirtualAlloc(NULL, size, MEM_COMMIT|extra_flags, PAGE_READWRITE);

		m_AllocatedMemory += size;
		return malloc(size);
	}

	//-----------------------------------------------------------------
	void Free(void* p, size_t size)
	{
		m_AllocatedMemory -= size;
//		VirtualFree(p, 0, MEM_RELEASE);
		free(p);
	}

	//-----------------------------------------------------------------
	void RemoveAll()
	{
		FScopeLock Lock(&m_CriticalSection);
		if (mp_Table == nullptr)
		{
			return;
		}
//		CriticalSectionScope lock(m_CriticalSection);

		for(int i=0; i<m_Capacity; ++i)
		{
			Pair* p_pair = mp_Table[i];
			if(p_pair)
			{
				FreePair(p_pair);
				mp_Table[i] = NULL;
			}
		}
		m_Count = 0;
	}

	//-----------------------------------------------------------------
	static int GetNextPow2(int value)
	{
		int p = 2;
		while(p < value)
			p *= 2;
		return p;
	}

	//-----------------------------------------------------------------
	void AllocTable()
	{
		// allocate a block of memory for the table
		if(m_Capacity > 0)
		{
			const int size = m_Capacity * sizeof(Pair*);
			mp_Table = (Pair**)Alloc(size);
			memset(mp_Table, 0, size);
		}
	}

	//-----------------------------------------------------------------
	bool IsItemInUse(const int index) const
	{
		return mp_Table[index] != NULL;
	}

	//-----------------------------------------------------------------
	int GetItemIndex(const TKey& key) const
	{
		check(mp_Table);
		const unsigned int hash = key.GetHashCode();
		int srch_index = hash & (m_Capacity-1);
		while(IsItemInUse(srch_index) && !(mp_Table[srch_index]->m_Key == key))
		{
			srch_index = (srch_index + 1) & (m_Capacity-1);
#ifdef PROFILE_VirtualMap
			++m_IterAcc;
#endif
		}

#ifdef PROFILE_VirtualMap
		++m_IterCount;
		double average = m_IterAcc / (double)m_IterCount;
		if(average > 2.0)
		{
			static int last_write_time = 0;
			int now = Time::Now();
			if(now - last_write_time > 1000)
			{
				last_write_time = now;
				Debug::Write("WARNING: VirtualMap average: %f\n", (float)average);
			}
		}
#endif
		return srch_index;
	}

	//-----------------------------------------------------------------
	// Increase the capacity of the table.
	void Grow()
	{
		const int new_capacity = m_Capacity ? 2*m_Capacity : m_DefaultCapacity;
		Resize(new_capacity);
	}

	//-----------------------------------------------------------------
	static bool InRange(
		const int index,
		const int start_index,
		const int end_index)
	{
		return (start_index <= end_index) ?
			index >= start_index && index <= end_index :
		index >= start_index || index <= end_index;
	}

	//-----------------------------------------------------------------
	void FreePools()
	{
		char* p_pool = mp_ItemPool;
		while(p_pool)
		{
			char* p_next_pool = *(char**)p_pool;
			Free(p_pool, m_DefaultAllocSize);
			p_pool = p_next_pool;
		}
		mp_ItemPool = NULL;
		mp_FreePair = NULL;
	}

	//-----------------------------------------------------------------
	Pair* AllocPair()
	{
		if(!mp_FreePair)
		{
			// allocate a new pool and link to pool list
			char* p_new_pool = (char*)Alloc(m_DefaultAllocSize);
			*(char**)p_new_pool = mp_ItemPool;
			mp_ItemPool = p_new_pool;

			// link all items onto free list
			mp_FreePair = p_new_pool + sizeof(Pair);
			char* p = (char*)mp_FreePair;
			int item_count = m_DefaultAllocSize / sizeof(Pair) - 2;	// subtract 2 for pool pointer and last item
			check(item_count);
			for(int i=0; i<item_count; ++i, p+=sizeof(Pair))
			{
				*(char**)p = p + sizeof(Pair);
			}
			*(char**)p = NULL;
		}

		// take item off free list
		Pair* p_pair = (Pair*)mp_FreePair;
		mp_FreePair = *(char**)mp_FreePair;

		// construct the pair
		new (p_pair)Pair;

		return p_pair;
	}

	//-----------------------------------------------------------------
	void FreePair(Pair* p_pair)
	{
		p_pair->~Pair();

		*(char**)p_pair = mp_FreePair;
		mp_FreePair = (char*)p_pair;
	}

	//-----------------------------------------------------------------
	// data
private:
	enum { m_DefaultCapacity = 32 };
	enum { m_InvalidIndex = 0xffffffff };
	enum { m_MaxCapacity = 0x7fffffff };
	enum { m_Margin = (30 * 256) / 100 };
	enum { m_DefaultAllocSize = 4096 };

//	CRITICAL_SECTION m_CriticalSection;
	FCriticalSection m_CriticalSection;
	
	int64 m_AllocatedMemory;

	int m_Capacity;			// the current capacity of this set, will always be >= m_Margin*m_Count/256
	Pair** mp_Table;		// NULL for a set with capacity 0
	int m_Count;			// the current number of items in this set, will always be <= m_Margin*m_Count/256

	int m_AllocSize;		// size of the blocks of memory to allocate for pairs
	char* mp_ItemPool;
	char* mp_FreePair;

#ifdef PROFILE_VirtualMap
	mutable int64 m_IterAcc;
	mutable int64 m_IterCount;
#endif
};

//-----------------------------------------------------------------
// useful struct for pointer keys
struct PointerKey
{
	PointerKey() : m_p(NULL), m_Hash(0) {}
	PointerKey(const void* p) : m_p(p)
	{
		// 64 bit to 32 bit Hash
		uint64 key = (uint64)p;
		key = (~key) + (key << 18);
		key = key ^ (key >> 31);
		key = key * 21;
		key = key ^ (key >> 11);
		key = key + (key << 6);
		key = key ^ (key >> 22);
		m_Hash = (unsigned int)key;
	}
	unsigned int GetHashCode() const { return m_Hash; }
	bool operator==(const PointerKey& other) const { return m_p == other.m_p; }
	const void* m_p;
	unsigned int m_Hash;
};

