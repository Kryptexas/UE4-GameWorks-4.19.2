// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SteamEndpoint.h: Implements the FSteamEndpoint class.
=============================================================================*/

#include "NetworkingPrivatePCH.h"


/* FSteamEndpoint interface
 *****************************************************************************/

FString FSteamEndpoint::ToString( ) const
{
	return FString::Printf(TEXT("0x%llX:%i"), UniqueNetId, SteamChannel);
}