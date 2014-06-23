// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UniquePtr.h"

// This is essentially a reference version of TUniquePtr
// Useful to force heap allocation of a value, e.g. in a TMap to give similar behaviour to TIndirectArray

template <typename T>
class TUniqueObj
{
public:
	TUniqueObj()
		: Obj(MakeUnique<T>())
	{
	}

	TUniqueObj(const TUniqueObj& other)
		: Obj(MakeUnique<T>(*other))
	{
	}

	template <typename Arg>
	explicit TUniqueObj(Arg&& arg)
		: Obj(MakeUnique<T>(Forward<Arg>(arg)))
	{
	}

#if PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES
	template <typename... Args>
	explicit TUniqueObj(Args&&... args)
		: Obj(MakeUnique<T>(Forward<Args>(args)...))
	{
	}
#endif

#if PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS
	TUniqueObj& operator=(const TUniqueObj&) = delete;
#else
private:
	TUniqueObj& operator=(const TUniqueObj&);
public:
#endif

	template <typename Arg>
	TUniqueObj& operator=(Arg&& other)
	{
		*Obj = Forward<Arg>(other);
		return *this;
	}

	      T& Get()       { return *Obj; }
	const T& Get() const { return *Obj; }

	      T* operator->()       { return Obj.Get(); }
	const T* operator->() const { return Obj.Get(); }

	      T& operator*()       { return *Obj; }
	const T& operator*() const { return *Obj; }

	friend FArchive& operator<<(FArchive& Ar, TUniqueObj& P)
	{
		Ar << *P.Obj;
		return Ar;
	}

private:
	TUniquePtr<T> Obj;
};
