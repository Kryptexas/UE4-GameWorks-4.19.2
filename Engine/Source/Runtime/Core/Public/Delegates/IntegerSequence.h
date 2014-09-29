// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

template <uint32... Indices>
struct TIntegerSequence;

template <uint32 ToAdd, uint32... Values>
struct TMakeIntegerSequenceImpl : TMakeIntegerSequenceImpl<ToAdd - 1, Values..., sizeof...(Values)>
{
};

template <uint32... Values>
struct TMakeIntegerSequenceImpl<0, Values...>
{
	typedef TIntegerSequence<Values...> Type;
};

template <uint32 ToAdd>
using TMakeIntegerSequence = typename TMakeIntegerSequenceImpl<ToAdd>::Type;
