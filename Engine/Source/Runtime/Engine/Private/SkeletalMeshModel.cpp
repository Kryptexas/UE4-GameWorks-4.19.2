// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Rendering/SkeletalMeshModel.h"

#if WITH_EDITOR
#include "Misc/CoreStats.h"
#include "Engine/SkeletalMesh.h"

FSkeletalMeshModel::FSkeletalMeshModel()
	: bGuidIsHash(false)	
{
}

void FSkeletalMeshModel::Serialize(FArchive& Ar, USkeletalMesh* Owner)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("FSkeletalMeshModel::Serialize"), STAT_SkeletalMeshModel_Serialize, STATGROUP_LoadTime);

	LODModels.Serialize(Ar, Owner);

	Ar.UsingCustomVersion(FSkeletalMeshCustomVersion::GUID);
	// For old content without a GUID, generate one now from the data
	if (Ar.IsLoading() && Ar.CustomVer(FSkeletalMeshCustomVersion::GUID) < FSkeletalMeshCustomVersion::SplitModelAndRenderData)
	{
		GenerateGUIDFromHash(Owner);
	}
	// Serialize the GUID
	else
	{
		Ar << SkeletalMeshModelGUID;
		Ar << bGuidIsHash;
	}
}

void FSkeletalMeshModel::GenerateNewGUID()
{
	SkeletalMeshModelGUID = FGuid::NewGuid();
	bGuidIsHash = false;
}


void FSkeletalMeshModel::GenerateGUIDFromHash(USkeletalMesh* Owner)
{
	// Build the hash from the path name + the contents of the bulk data.
	FSHA1 Sha;
	TArray<TCHAR> OwnerName = Owner->GetPathName().GetCharArray();
	Sha.Update((uint8*)OwnerName.GetData(), OwnerName.Num() * OwnerName.GetTypeSize());

	TArray<uint8> TempBytes;
	FMemoryWriter Ar(TempBytes, /*bIsPersistent=*/ true);
	LODModels.Serialize(Ar, Owner);

	if (TempBytes.Num() > 0)
	{
		Sha.Update(TempBytes.GetData(), TempBytes.Num() * sizeof(uint8));
	}
	Sha.Final();

	// Retrieve the hash and use it to construct a pseudo-GUID. 
	uint32 Hash[5];
	Sha.GetHash((uint8*)Hash);
	SkeletalMeshModelGUID = FGuid(Hash[0] ^ Hash[4], Hash[1], Hash[2], Hash[3]);

	bGuidIsHash = true;
}

FString FSkeletalMeshModel::GetIdString() const
{
	FString GuidString = SkeletalMeshModelGUID.ToString();
	if (bGuidIsHash)
	{
		GuidString += TEXT("X");
	}
	return GuidString;
}


void FSkeletalMeshModel::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	for (int32 LODIndex = 0; LODIndex < LODModels.Num(); ++LODIndex)
	{
		const FSkeletalMeshLODModel& Model = LODModels[LODIndex];
		Model.GetResourceSizeEx(CumulativeResourceSize);
	}
}

#endif // WITH_EDITOR