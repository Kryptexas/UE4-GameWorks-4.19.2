// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "Serialization/Archive.h"

/*-----------------------------------------------------------------------------
	FArchiveProxy implementation.
-----------------------------------------------------------------------------*/

FArchiveProxy::FArchiveProxy(FArchive& InInnerArchive)
: FArchive    (InInnerArchive)
, InnerArchive(InInnerArchive)
{
}

