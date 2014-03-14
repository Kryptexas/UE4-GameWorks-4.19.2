// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AssetEditorMessages.h: Declares message types used by asset editor.
=============================================================================*/

#pragma once

#include "AssetEditorMessages.generated.h"


/**
 * Implements a message for opening an asset in the asset browser.
 */
USTRUCT()
struct FAssetEditorRequestOpenAsset
{
	GENERATED_USTRUCT_BODY()
	
	/**
	 * Holds the name of the asset to open.
	 */
	UPROPERTY()
	FString AssetName;


	/**
	 * Default constructor.
	 */
	FAssetEditorRequestOpenAsset( ) { }

	/**
	 * Creates and initializes a new instance.
	 */
	FAssetEditorRequestOpenAsset( const FString& InAssetName )
		: AssetName(InAssetName)
	{ }
};


template<>
struct TStructOpsTypeTraits<FAssetEditorRequestOpenAsset> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithMessageHandling = true
	};
};


/* Dummy class
 *****************************************************************************/

UCLASS()
class UAssetEditorMessages
	: public UObject
{
public:

	GENERATED_UCLASS_BODY()
};
