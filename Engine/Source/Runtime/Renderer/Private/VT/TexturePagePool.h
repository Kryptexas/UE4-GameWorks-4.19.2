// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VirtualTextureShared.h"
#include "BinaryHeap.h"
#include "HashTable.h"

#define VT_MERGE_RESORT		1
#define VT_RADIX_SORT		0

// 64k x 64k virtual pages
// 256 x 256 physical pages
struct FTexturePage
{
	// Address is Morton order, relative to mip 0
	uint32	vAddress;
	uint16	pAddress;
	uint8	vLevel;
	uint8	ID;
};

class FTexturePagePool
{
public:
				FTexturePagePool( uint32 Size, uint32 Dimensions );
				~FTexturePagePool();

	uint32				GetSize() const						{ return Pages.Num(); }
	const FTexturePage&	GetPage( uint16 pAddress ) const	{ return Pages[ pAddress ]; }
					
	bool		AnyFreeAvailable( uint32 Frame ) const;
	uint32		Alloc( uint32 Frame );
	void		Free( uint32 Frame, uint32 PageIndex );
	void		UpdateUsage( uint32 Frame, uint32 PageIndex );

	uint32		FindPage( uint8 ID, uint8 vLevel, uint32 vAddress ) const;
	uint32		FindNearestPage( uint8 ID, uint8 vLevel, uint32 vAddress ) const;

	void		UnmapPage( uint16 pAddress );
	void		MapPage( uint8 ID, uint8 vLevel, uint32 vAddress, uint16 pAddress );

	void		RefreshEntirePageTable( uint8 ID, TArray< FPageUpdate >* Output );
	void		ExpandPageTableUpdatePainters( uint8 ID, FPageUpdate Update, TArray< FPageUpdate >* Output );
	void		ExpandPageTableUpdateMasked( uint8 ID, FPageUpdate Update, TArray< FPageUpdate >* Output );

private:
	void		BuildSortedKeys();
	uint32		LowerBound( uint32 Min, uint32 Max, uint64 SearchKey, uint64 Mask ) const;
	uint32		UpperBound( uint32 Min, uint32 Max, uint64 SearchKey, uint64 Mask ) const;
	uint32		EqualRange( uint32 Min, uint32 Max, uint64 SearchKey, uint64 Mask ) const;

	uint32		vDimensions;

	TArray< FTexturePage >	Pages;
	
	FHashTable						HashTable;
	FBinaryHeap< uint32, uint16 >	FreeHeap;

	TArray< uint64 >	UnsortedKeys;
	TArray< uint64 >	SortedKeys;
	bool				SortedKeysDirty;
	
	TArray< uint32 >	SortedSubIndexes;
	TArray< uint32 >	SortedAddIndexes;
};