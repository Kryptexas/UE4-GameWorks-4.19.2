// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Serialization/Formatters/BinaryArchiveFormatter.h"
#include "Serialization/Archive.h"

FBinaryArchiveFormatter::FBinaryArchiveFormatter(FArchive& InInner) 
	: Inner(InInner)
{
}

FBinaryArchiveFormatter::~FBinaryArchiveFormatter()
{
}

