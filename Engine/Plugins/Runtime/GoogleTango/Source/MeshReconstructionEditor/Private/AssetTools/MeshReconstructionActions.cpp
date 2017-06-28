// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MeshReconstructionActions.h"
#include "TangoMeshReconstructor.h"


#define LOCTEXT_NAMESPACE "AssetTypeActions"


/* FAssetTypeActions overrides
 *****************************************************************************/

bool FMeshReconstructionActions::CanFilter()
{
	return true;
}


uint32 FMeshReconstructionActions::GetCategories()
{
	return EAssetTypeCategories::Sounds;
}


FText FMeshReconstructionActions::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MeshReconstruction", "Tango Mesh Reconstruction");
}


UClass* FMeshReconstructionActions::GetSupportedClass() const
{
	return UTangoMeshReconstructor::StaticClass();
}


FColor FMeshReconstructionActions::GetTypeColor() const
{
	return FColor(77, 93, 239);
}


bool FMeshReconstructionActions::HasActions ( const TArray<UObject*>& InObjects ) const
{
	return false;
}


#undef LOCTEXT_NAMESPACE
