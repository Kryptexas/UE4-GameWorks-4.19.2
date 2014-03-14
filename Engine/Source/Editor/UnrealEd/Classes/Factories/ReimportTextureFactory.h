// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// ReimportTextureFactory
//=============================================================================

#pragma once
#include "ReimportTextureFactory.generated.h"

UCLASS(hidecategories=(LightMap, DitherMipMaps, LODGroup), collapsecategories)
class UReimportTextureFactory : public UTextureFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class UTexture* pOriginalTex;


	// Begin FReimportHandler interface
	virtual bool CanReimport( UObject* Obj, TArray<FString>& OutFilenames ) OVERRIDE;
	virtual void SetReimportPaths( UObject* Obj, const TArray<FString>& NewReimportPaths ) OVERRIDE;
	virtual EReimportResult::Type Reimport( UObject* Obj ) OVERRIDE;
	// End FReimportHandler interface

private:
	// Begin UTextureFactory Interface
	virtual UTexture2D* CreateTexture2D( UObject* InParent, FName Name, EObjectFlags Flags ) OVERRIDE;
	virtual UTextureCube* CreateTextureCube( UObject* InParent, FName Name, EObjectFlags Flags ) OVERRIDE;
	// End UTextureFactory Interface
};



