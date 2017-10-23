// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include <assert.h>
#include <objc/runtime.h>

namespace ue4
{
	template<typename ReturnType, bool ClassMethod, typename... Args>
	class Selector
	{
		typedef ReturnType (*TypeSafeIMP)(Args...);
		TypeSafeIMP Implementation;
	public:
		Selector(Class InClass, SEL InSelector)
		{
			assert(InClass && InSelector);
			if (ClassMethod)
			{
				Implementation = (TypeSafeIMP)class_getClassMethod(InClass, InSelector);
			}
			else
			{
				Implementation = (TypeSafeIMP)class_getMethodImplementation(InClass, InSelector);
			}
			assert(Implementation);
		}
		
		ReturnType operator()(Args... InArgs)
		{
			return Implementation(InArgs...);
		}
	};
	
	template<bool ClassMethod, typename... Args>
	class Selector<void, ClassMethod, Args...>
	{
		typedef void (*TypeSafeIMP)(Args...);
		TypeSafeIMP Implementation;
	public:
		Selector(Class InClass, SEL InSelector)
		{
			assert(InClass && InSelector);
			if (ClassMethod)
			{
				Implementation = (TypeSafeIMP)class_getClassMethod(InClass, InSelector);
			}
			else
			{
				Implementation = (TypeSafeIMP)class_getMethodImplementation(InClass, InSelector);
			}
			assert(Implementation);
		}
		
		void operator()(Args... InArgs)
		{
			Implementation(InArgs...);
		}
	};
}

