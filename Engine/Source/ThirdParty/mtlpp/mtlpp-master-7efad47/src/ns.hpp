/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Object.hpp"

MTLPP_BEGIN

template<typename Interpose>
struct IMPTable<IOSurfaceRef, Interpose>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	{
	}
	
	void RegisterInterpose(Class C) {}
	
	void Retain(IOSurfaceRef Surface) { CFRetain(Surface); }
	void Release(IOSurfaceRef Surface) { CFRelease(Surface); }
};

namespace ns
{
#if __OBJC__
	template<typename T>
	struct Protocol
	{
		typedef T type;
	};
#else
	template<typename t>
	struct id
	{
		typedef t* ptr;
	};
	
	template<typename T>
	struct Protocol
	{
		typedef typename T::ptr type;
	};
#endif
	
	template<typename T, bool bAutoReleased = false>
	class Object
    {
    public:
		typedef T Type;
		typedef IMPTable<T, void> ITable;
		
        inline const T GetPtr() const { return m_ptr; }
		inline T* GetInnerPtr() { return &m_ptr; }

        inline operator bool() const { return m_ptr != nullptr; }
		operator T() const { return m_ptr; }
		
		void Retain()
		{
#if __OBJC__
			__sync_fetch_and_add(&RefCount, 1);
#endif
		}
		
		void Release()
		{
#if __OBJC__
			if ((__sync_fetch_and_sub(&RefCount, 1) - 1) == 0)
			{
				delete this;
			}
#endif
		}

    protected:
        Object();
        Object(T const handle, bool const retain = true, ITable* table = nullptr);
        Object(const Object& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
        Object(Object&& rhs);
#endif
        virtual ~Object();

        Object& operator=(const Object& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
        Object& operator=(Object&& rhs);
#endif

#if MTLPP_CONFIG_VALIDATE
        inline void Validate() const
        {
			assert(m_ptr);
			assert(m_table);
        }
#else
	#define Validate()
#endif

        T m_ptr = nullptr;
		ITable* m_table = nullptr;
		volatile uint64_t RefCount = 0;
    };
	
	template<class T>
	class Ref
	{
		T* Ptr;
	public:
		typedef typename T::Type Type;
		
		Ref() : Ptr(nullptr) {}
		Ref(Type inner) : Ptr(new T(inner)) { Ptr->Retain(); }
		Ref(T* Pointer) : Ptr(Pointer) { Ptr->Retain(); }
		Ref(const Ref& rhs) : Ptr(nullptr) { operator=(rhs); }
#if MTLPP_CONFIG_RVALUE_REFERENCES
		Ref(Ref&& rhs) : Ptr(rhs.Ptr) { rhs.Ptr = nullptr; }
#endif
		virtual ~Ref() { if(Ptr) { Ptr->Release(); Ptr = nullptr; } }
		
		inline operator bool() const { return (Ptr != nullptr && *Ptr); }
		
		Ref& operator=(const Ref& rhs)
		{
			if (this != &rhs)
			{
				if (rhs.Ptr)
				{
					rhs.Ptr->Retain();
				}
				if (Ptr)
				{
					Ptr->Release();
				}
				Ptr = rhs.Ptr;
			}
			return *this;
		}
		
#if MTLPP_CONFIG_RVALUE_REFERENCES
		Ref& operator=(Ref&& rhs)
		{
			if (this != &rhs)
			{
				if (Ptr)
				{
					Ptr->Release();
				}
				Ptr = rhs.Ptr;
				rhs.Ptr = nullptr;
			}
			return *this;
		}
#endif
		
		T* operator->() const
		{
			return Ptr;
		}
		
		operator T*() const
		{
			return Ptr;
		}
		
		operator T const&() const
		{
			assert(Ptr);
			return *Ptr;
		}
		
		T* operator*() const
		{
			return Ptr;
		}
	};

    struct Range
    {
        inline Range(NSUInteger location, NSUInteger length) :
            Location(location),
            Length(length)
        { }

        NSUInteger Location;
        NSUInteger Length;
    };

	class ArrayBase : public Object<NSArray<id<NSObject>>*>
    {
    public:
        ArrayBase() { }
        ArrayBase(NSArray<id<NSObject>>* const handle, bool const bRetain) : Object<NSArray<id<NSObject>>*>(handle, bRetain) { }

        NSUInteger GetSize() const;

    protected:
        void* GetItem(NSUInteger index) const;
    };

    template<typename T>
    class Array : public ArrayBase
    {
    public:
        Array() { }
		Array(NSArray<typename T::Type>* const handle, bool const bRetain = true) : ArrayBase(handle, bRetain) { }

        const T operator[](NSUInteger index) const
        {
			typedef typename T::Type InnerType;
			return (InnerType)GetItem(index);
        }

        T operator[](NSUInteger index)
        {
			typedef typename T::Type InnerType;
			return (InnerType)GetItem(index);
        }
		
		class Iterator
		{
		public:
			Iterator(Array const& ptr, NSUInteger In): array(ptr), index(In) {}
			Iterator operator++() { if (index < array.GetSize()) { ++index; } return *this; }
			
			bool operator!=(Iterator const& other) const { return (&array != &other.array) || (index != other.index); }
			
			const T operator*() const
			{
				if (index < array.GetSize())
					return array[index];
				else
					return T();
			}
		private:
			Array const& array;
			NSUInteger index;
		};
		
		Iterator begin() const
		{
			return Iterator(*this, 0);
		}
		
		Iterator end() const
		{
			return Iterator(*this, GetSize());
		}
    };

    class DictionaryBase : public Object<NSDictionary<id<NSObject>, id<NSObject>>*>
    {
    public:
        DictionaryBase() { }
        DictionaryBase(NSDictionary<id<NSObject>, id<NSObject>>* const handle) : Object<NSDictionary<id<NSObject>, id<NSObject>>*>(handle) { }

    protected:

    };

    template<typename KeyT, typename ValueT>
    class Dictionary : public DictionaryBase
    {
    public:
        Dictionary() { }
        Dictionary(NSDictionary<typename KeyT::Type, typename ValueT::Type>* const handle) : DictionaryBase(handle) { }
    };

    class String : public Object<NSString*>
    {
    public:
        String() { }
        String(NSString* handle) : Object<NSString*>(handle) { }
        String(const char* cstr);

        const char* GetCStr() const;
        NSUInteger    GetLength() const;
    };
	
	class URL : public Object<NSURL*>
	{
	public:
		URL() { }
		URL(NSURL* const handle) : Object<NSURL*>(handle) { }
	};

	/**
	 * Auto-released classes are used to hold Objective-C out-results that are implicitly placed in an auto-release pool
	 * We can't retain these objects safely unless done explicitly after the function that assigns them returns (or is called for handlers).
	 * Thus you need to have a non-released variant of the class that takes ownership of the pointer.
	 */
    class AutoReleasedError : public Object<NSError*, true>
    {
		AutoReleasedError(const AutoReleasedError& rhs) = delete;
#if MTLPP_CONFIG_RVALUE_REFERENCES
		AutoReleasedError(AutoReleasedError&& rhs) = delete;
#endif
		AutoReleasedError& operator=(const AutoReleasedError& rhs) = delete;
#if MTLPP_CONFIG_RVALUE_REFERENCES
		AutoReleasedError& operator=(AutoReleasedError&& rhs) = delete;
#endif
		
		friend class Error;
    public:
        AutoReleasedError();
        AutoReleasedError(NSError* const handle) : Object<NSError*, true>(handle) { }
		AutoReleasedError& operator=(NSError* const handle);
		
        String   GetDomain() const;
        NSUInteger GetCode() const;
        //@property (readonly, copy) NSDictionary *userInfo;
        String   GetLocalizedDescription() const;
        String   GetLocalizedFailureReason() const;
        String   GetLocalizedRecoverySuggestion() const;
        Array<String>   GetLocalizedRecoveryOptions() const;
        //@property (nullable, readonly, strong) id recoveryAttempter;
        String   GetHelpAnchor() const;
    };
	
	class Error : public Object<NSError*>
	{
	public:
		Error();
		Error(NSError* const handle) : Object<NSError*>(handle) { }
		Error(const AutoReleasedError& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		Error(const AutoReleasedError&& rhs);
#endif
		Error& operator=(const AutoReleasedError& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
		Error& operator=(AutoReleasedError&& rhs);
#endif
		
		String   GetDomain() const;
		NSUInteger GetCode() const;
		//@property (readonly, copy) NSDictionary *userInfo;
		String   GetLocalizedDescription() const;
		String   GetLocalizedFailureReason() const;
		String   GetLocalizedRecoverySuggestion() const;
		Array<String>   GetLocalizedRecoveryOptions() const;
		//@property (nullable, readonly, strong) id recoveryAttempter;
		String   GetHelpAnchor() const;
	};
	
	class IOSurface : public Object<IOSurfaceRef>
	{
	public:
		IOSurface() {}
		IOSurface(IOSurfaceRef const handle) : Object<IOSurfaceRef>(handle) { }
	};
	
	class Bundle : public Object<NSBundle*>
	{
	public:
		Bundle() {}
		Bundle(NSBundle* const handle) : Object<NSBundle*>(handle) { }
	};
}

#include "ns.inl"

MTLPP_END
