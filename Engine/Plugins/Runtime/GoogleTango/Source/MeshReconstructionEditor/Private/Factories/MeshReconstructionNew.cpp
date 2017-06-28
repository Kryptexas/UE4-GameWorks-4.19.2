// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MeshReconstructionFactoryNew.h"
#include "TangoMeshReconstructor.h"
#include "AssetTypeCategories.h"

/* UMeshReconstructionFactoryNew structors
 *****************************************************************************/

UMeshReconstructionFactoryNew::UMeshReconstructionFactoryNew( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	SupportedClass = UTangoMeshReconstructor::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}


/* UFactory overrides
 *****************************************************************************/

UObject* UMeshReconstructionFactoryNew::FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn )
{
	UTangoMeshReconstructor* MeshReconstructor = NewObject<UTangoMeshReconstructor>(InParent, InClass, InName, Flags);

	return MeshReconstructor;
}

uint32 UMeshReconstructionFactoryNew::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}



bool UMeshReconstructionFactoryNew::ShouldShowInNewMenu() const
{
	return true;
}
