/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once

#include "defines.hpp"

MTLPP_CLASS(NSDictionary);
MTLPP_CLASS(NSString);
MTLPP_CLASS(NSURL);
MTLPP_CLASS(NSError);
MTLPP_CLASS(NSBundle);
MTLPP_CLASS(NSArray);
typedef struct __IOSurface* IOSurfaceRef;

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
	
	template<typename T>
	class Object
    {
    public:
		typedef T Type;
		
        inline const T GetPtr() const { return m_ptr; }

        inline operator bool() const { return m_ptr != nullptr; }
		
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
        Object(T const handle, bool const retain = true);
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
        }
#else
	#define Validate()
#endif

        T m_ptr = nullptr;
		volatile uint64_t RefCount = 0;
    };
	
	template<class T>
	class Ref
	{
		T* Ptr;
	public:
		typedef typename T::Type Type;
		
		Ref() : Ptr(nullptr) {}
		Ref(Type inner) : Ptr(new T(inner)) {}
		Ref(T* Pointer) : Ptr(Pointer) {}
		Ref(const Ref& rhs) : Ptr(nullptr) { operator=(rhs); }
#if MTLPP_CONFIG_RVALUE_REFERENCES
		Ref(Ref&& rhs) : Ptr(rhs.Ptr) { rhs.Ptr = nullptr; }
#endif
		virtual ~Ref() { if(Ptr) { Ptr->Release(); Ptr = nullptr; } }
		
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
        inline Range(uint32_t location, uint32_t length) :
            Location(location),
            Length(length)
        { }

        uint32_t Location;
        uint32_t Length;
    };

	class ArrayBase : public Object<NSArray*>
    {
    public:
        ArrayBase() { }
        ArrayBase(NSArray* const handle) : Object<NSArray*>(handle) { }

        uint32_t GetSize() const;

    protected:
        void* GetItem(uint32_t index) const;
    };

    template<typename T>
    class Array : public ArrayBase
    {
    public:
        Array() { }
        Array(NSArray* const handle) : ArrayBase(handle) { }

        const T operator[](uint32_t index) const
        {
			typedef typename T::Type InnerType;
			return (InnerType)GetItem(index);
        }

        T operator[](uint32_t index)
        {
			typedef typename T::Type InnerType;
			return (InnerType)GetItem(index);
        }
    };

    class DictionaryBase : public Object<NSDictionary*>
    {
    public:
        DictionaryBase() { }
        DictionaryBase(NSDictionary* const handle) : Object<NSDictionary*>(handle) { }

    protected:

    };

    template<typename KeyT, typename ValueT>
    class Dictionary : public DictionaryBase
    {
    public:
        Dictionary() { }
        Dictionary(NSDictionary* const handle) : DictionaryBase(handle) { }
    };

    class String : public Object<NSString*>
    {
    public:
        String() { }
        String(NSString* handle) : Object<NSString*>(handle) { }
        String(const char* cstr);

        const char* GetCStr() const;
        uint32_t    GetLength() const;
    };
	
	class URL : public Object<NSURL*>
	{
	public:
		URL() { }
		URL(NSURL* const handle) : Object<NSURL*>(handle) { }
	};

    class Error : public Object<NSError*>
    {
    public:
        Error();
        Error(NSError* const handle) : Object<NSError*>(handle) { }

		inline NSError** GetInnerPtr() { return &m_ptr; }
		
        String   GetDomain() const;
        uint32_t GetCode() const;
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
