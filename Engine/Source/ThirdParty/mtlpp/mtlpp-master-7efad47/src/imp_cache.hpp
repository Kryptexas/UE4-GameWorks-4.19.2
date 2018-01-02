// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "declare.hpp"
#include "imp_SelectorCache.hpp"
#include "imp_Object.hpp"

MTLPP_BEGIN

namespace ue4
{
	template<typename ObjC, typename Interpose>
	class imp_cache
	{
		imp_cache() {}
		~imp_cache()
		{
			for(auto entry : Impls)
			{
				delete entry.second;
			}
		}
		
	public:
		typedef IMPTable<ObjC, Interpose> table;
		
		static table* Register(ObjC Object)
		{
			static imp_cache Self;
			table* impTable = nullptr;
			if(Object)
			{
				Class c = object_getClass(Object);
				Self.Lock.lock();
				auto it = Self.Impls.find(c);
				if (it != Self.Impls.end())
				{
					impTable = it->second;
				}
				else
				{
					impTable = new table(c);
					Self.Impls.emplace(c, impTable);
				}
				Self.Lock.unlock();
			}
			return impTable;
		}
		
	private:
		std::mutex Lock;
		std::unordered_map<Class, table*> Impls;
	};
	
	template<typename T>
	static inline IMPTable<T, void>* CreateIMPTable(const T handle)
	{
		if (handle)
		{
			return imp_cache<T, void>::Register(handle);
		}
		else
		{
			return nullptr;
		}
	}
	
	template<>
	inline IMPTable<IOSurfaceRef, void>* CreateIMPTable(const IOSurfaceRef handle)
	{
		static IMPTable<IOSurfaceRef, void> Table;
		return &Table;
	}
	
	template<>
	inline IMPTable<NSError*, void>* CreateIMPTable(NSError* handle)
	{
		static IMPTable<NSError*, void> Table(objc_getRequiredClass("NSError"));
		return &Table;
	}
}

MTLPP_END

