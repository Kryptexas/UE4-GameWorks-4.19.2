// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// Needed on platforms other than Windows
#ifndef WIN32
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#endif

#include <assert.h>
#define check(x)	assert(x)

// We can't use static_assert in .c files as this is a C++(11) feature
#if __cplusplus && !__clang__
#ifdef _MSC_VER
typedef unsigned long long uint64_t;
#endif // _MSC_VER
static_assert(sizeof(uint64_t) == 8, "Bad!");
#endif

#if __cplusplus
static inline bool isalpha(char c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}
#endif


// Wrap std::vector API's definitions
#include <vector>
template<typename InElementType>
class TArray
{
public:
	typedef InElementType ElementType;
	typedef std::vector<ElementType> TSTLVector;

	void Add(const ElementType& Element)
	{
		Vector.push_back(Element);
	}

	int Num() const
	{
		return (int)Vector.size();
	}

	ElementType& operator[](int Index)
	{
		return Vector[Index];
	}

	const ElementType& operator[](int Index) const
	{
		return Vector[Index];
	}

	typename TSTLVector::iterator begin()
	{
		return Vector.begin();
	}

	typename TSTLVector::iterator end()
	{
		return Vector.end();
	}

	typename TSTLVector::const_iterator begin() const
	{
		return Vector.begin();
	}

	typename TSTLVector::const_iterator end() const
	{
		return Vector.end();
	}

	void Reset(int NewSize)
	{
		Vector.clear();
		Vector.reserve(NewSize);
	}

	void Resize(int NewSize, const ElementType& Element)
	{
		Vector.resize(NewSize, Element);
	}

//private:
	TSTLVector Vector;
};

template<typename T>
void Exchange(TArray<T>& A, TArray<T>& B)
{
	A.Vector.swap(B.Vector);
}
