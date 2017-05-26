// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

FORCEINLINE uint16 HashPage( uint32 vLevel, uint32 vIndex, uint8 vDimensions )
{
	// Mix level into top 4 bits 
	return ( vLevel << 6 ) ^ ( vIndex >> ( vDimensions * vLevel ) );
}


struct FPageUpdate
{
	uint32	vAddress;
	uint16	pAddress;
	uint8	vLevel;
	uint8	vLogSize;

	FPageUpdate()
	{}

	FPageUpdate( const FPageUpdate& Update, uint32 Offset, uint8 vDimensions )
		: vAddress( Update.vAddress + ( Offset << ( vDimensions * Update.vLogSize ) ) )
		, pAddress( Update.pAddress )
		, vLevel( Update.vLevel )
		, vLogSize( Update.vLogSize )
	{}

	inline void Check( uint8 vDimensions )
	{
		uint32 LowBitMask = ( 1 << ( vDimensions * vLogSize ) ) - 1;
		checkSlow( (vAddress & LowBitMask) == 0 );

		checkSlow( vLogSize <= vLevel );
	}
};