// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Serialization/ArchiveFromStructuredArchive.h"
#include "Serialization/ArchiveUObject.h"

class FArchiveUObjectFromStructuredArchive : public FArchiveFromStructuredArchive
{
public:

	FArchiveUObjectFromStructuredArchive(FStructuredArchive::FSlot Slot)
		: FArchiveFromStructuredArchive(Slot)
	{

	}

	using FArchive::operator<<; // For visibility of the overloads we don't override

	//~ Begin FArchive Interface
	virtual FArchive& operator<<(FLazyObjectPtr& Value) override { return FArchiveUObject::SerializeLazyObjectPtr(*this, Value); }
	virtual FArchive& operator<<(FSoftObjectPtr& Value) override { return FArchiveUObject::SerializeSoftObjectPtr(*this, Value); }
	virtual FArchive& operator<<(FSoftObjectPath& Value) override { return FArchiveUObject::SerializeSoftObjectPath(*this, Value); }
	virtual FArchive& operator<<(FWeakObjectPtr& Value) override { return FArchiveUObject::SerializeWeakObjectPtr(*this, Value); }
	//~ End FArchive Interface
};