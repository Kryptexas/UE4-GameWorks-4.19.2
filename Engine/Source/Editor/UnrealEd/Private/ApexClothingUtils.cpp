// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ApexClothingUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Misc/MessageDialog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "EditorDirectories.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "SlateApplication.h"
#include "Rendering/SkeletalMeshModel.h"

#include "ClothingAssetFactory.h"
#include "PhysicsPublic.h"
#include "PhysXIncludes.h"

DEFINE_LOG_CATEGORY_STATIC(LogApexClothingUtils, Log, All);

#define LOCTEXT_NAMESPACE "ApexClothingUtils"

namespace ApexClothingUtils
{

//enforces a call of "OnRegister" to update vertex factories
void ReregisterSkelMeshComponents(USkeletalMesh* SkelMesh)
{
	for( TObjectIterator<USkeletalMeshComponent> It; It; ++It )
	{
		USkeletalMeshComponent* MeshComponent = *It;
		if( MeshComponent && 
			!MeshComponent->IsTemplate() &&
			MeshComponent->SkeletalMesh == SkelMesh )
		{
			MeshComponent->ReregisterComponent();
		}
	}
}

void RefreshSkelMeshComponents(USkeletalMesh* SkelMesh)
{
	for( TObjectIterator<USkeletalMeshComponent> It; It; ++It )
	{
		USkeletalMeshComponent* MeshComponent = *It;
		if( MeshComponent && 
			!MeshComponent->IsTemplate() &&
			MeshComponent->SkeletalMesh == SkelMesh )
		{
			MeshComponent->RecreateRenderState_Concurrent();
		}
	}
}

FString PromptForClothingFile()
{
	if(IDesktopPlatform* Platform = FDesktopPlatformModule::Get())
{
		const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

		TArray<FString> OpenFilenames;
		FString OpenFilePath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::MESH_IMPORT_EXPORT);

		if(Platform->OpenFileDialog(
			ParentWindowWindowHandle,
			*LOCTEXT("ImportClothing_ChooseFile", "Choose clothing asset source file").ToString(),
			OpenFilePath,
			TEXT(""),
			TEXT("APEX clothing asset(*.apx,*.apb)|*.apx;*.apb|All files (*.*)|*.*"),
			EFileDialogFlags::None,
			OpenFilenames
			))
	{
			return OpenFilenames[0];
		}
	}

	// Nothing picked, empty path
	return FString();
}

void PromptAndImportClothing(USkeletalMesh* SkelMesh)
{
	ensure(SkelMesh);

	FString Filename = PromptForClothingFile();

	if(!Filename.IsEmpty())
{
		FEditorDirectories::Get().SetLastDirectory(ELastDirectory::MESH_IMPORT_EXPORT, Filename);

		UClothingAssetFactory* Factory = UClothingAssetFactory::StaticClass()->GetDefaultObject<UClothingAssetFactory>();

		if(Factory && Factory->CanImport(Filename))
	{					
			Factory->Import(Filename, SkelMesh);
	}
	}
}

#if WITH_APEX_CLOTHING
apex::ClothingAsset* CreateApexClothingAssetFromPxStream(physx::PxFileBuf& Stream)
{
	// Peek into the buffer to see what kind of data it is (binary or xml)
	NvParameterized::Serializer::SerializeType SerializeType = GApexSDK->getSerializeType(Stream);
	// Create an NvParameterized serializer for the correct data type
	NvParameterized::Serializer* Serializer = GApexSDK->createSerializer(SerializeType);

	if(!Serializer)
	{
		return NULL;
	}
	// Deserialize into a DeserializedData buffer
	NvParameterized::Serializer::DeserializedData DeserializedData;
	NvParameterized::Serializer::ErrorType Error = Serializer->deserialize(Stream, DeserializedData);

	apex::Asset* ApexAsset = NULL;
	if( DeserializedData.size() > 0 )
	{
		// The DeserializedData has something in it, so create an APEX asset from it
		ApexAsset = GApexSDK->createAsset( DeserializedData[0], NULL);
	}
	// Release the serializer
	Serializer->release();

	return (apex::ClothingAsset*)ApexAsset;
}

apex::ClothingAsset* CreateApexClothingAssetFromBuffer(const uint8* Buffer, int32 BufferSize)
{
	apex::ClothingAsset* ApexClothingAsset = NULL;

	// Wrap Buffer with the APEX read stream class
	physx::PxFileBuf* Stream = GApexSDK->createMemoryReadStream(Buffer, BufferSize);

	if (Stream != NULL)
	{
		ApexClothingAsset = ApexClothingUtils::CreateApexClothingAssetFromPxStream(*Stream);
		// Release our stream
		GApexSDK->releaseMemoryReadStream(*Stream);
	}

	return ApexClothingAsset;
}

#endif// #if WITH_APEX_CLOTHING

void RestoreAllClothingSections(USkeletalMesh* SkelMesh, uint32 LODIndex, uint32 AssetIndex)
{
	if(FSkeletalMeshModel* Resource = SkelMesh->GetImportedModel())
	{
		for(FSkeletalMeshLODModel& LodModel : Resource->LODModels)
		{
			for(FSkelMeshSection& Section : LodModel.Sections)
			{
				if(Section.HasClothingData())
				{
					ClothingAssetUtils::ClearSectionClothingData(Section);
				}
			}
		}
	}
}

void RemoveAssetFromSkeletalMesh(USkeletalMesh* SkelMesh, uint32 AssetIndex, bool bReleaseAsset, bool bRecreateSkelMeshComponent)
{
	FSkeletalMeshModel* ImportedResource= SkelMesh->GetImportedModel();
	int32 NumLODs = ImportedResource->LODModels.Num();

	for(int32 LODIdx=0; LODIdx < NumLODs; LODIdx++)
	{
		RestoreAllClothingSections(SkelMesh, LODIdx, AssetIndex);
	}

#if WITH_APEX_CLOTHING
	apex::ClothingAsset* ApexClothingAsset = SkelMesh->ClothingAssets_DEPRECATED[AssetIndex].ApexClothingAsset;	//Can't delete apex asset until after apex actors so we save this for now and reregister component (which will trigger the actor delete)
#endif	// WITH_APEX_CLOTHING
	SkelMesh->ClothingAssets_DEPRECATED.RemoveAt(AssetIndex);	//have to remove the asset from the array so that new actors are not created for asset pending deleting

	SkelMesh->PostEditChange(); // update derived data

	ReregisterSkelMeshComponents(SkelMesh);

#if WITH_APEX_CLOTHING
	if(bReleaseAsset)
	{
		//Now we can actually delete the asset
		GPhysCommandHandler->DeferredRelease(ApexClothingAsset);
	}
#endif	// WITH_APEX_CLOTHING

	if(bRecreateSkelMeshComponent)
	{
		// Refresh skeletal mesh components
		RefreshSkelMeshComponents(SkelMesh);
	}
}

} // namespace ApexClothingUtils

#undef LOCTEXT_NAMESPACE
