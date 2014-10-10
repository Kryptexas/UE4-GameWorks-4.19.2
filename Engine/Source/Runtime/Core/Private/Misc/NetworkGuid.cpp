// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"


/* FNetworkGUID
 *****************************************************************************/

bool FNetworkGUID::NetSerialize( FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess )
{
	Ar << Value;
	return true;
}
