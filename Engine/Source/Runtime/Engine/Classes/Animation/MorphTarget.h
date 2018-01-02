// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "PackedNormal.h"
#include "UObject/EditorObjectVersion.h"

#include "MorphTarget.generated.h"

class USkeletalMesh;
class UStaticMesh;

/** Morph mesh vertex data used for rendering */
struct FMorphTargetDelta
{
	/** change in position */
	FVector			PositionDelta;

	/** Tangent basis normal */
	FVector			TangentZDelta;

	/** index of source vertex to apply deltas to */
	uint32			SourceIdx;

	/** pipe operator */
	friend FArchive& operator<<(FArchive& Ar, FMorphTargetDelta& V)
	{
		if (Ar.UE4Ver() < VER_UE4_MORPHTARGET_CPU_TANGENTZDELTA_FORMATCHANGE)
		{
			/** old format of change in tangent basis normal */
			FPackedNormal	TangentZDelta_DEPRECATED;

			if (Ar.IsSaving())
			{
				TangentZDelta_DEPRECATED = FPackedNormal(V.TangentZDelta);
			}

			Ar << V.PositionDelta << TangentZDelta_DEPRECATED << V.SourceIdx;

			if (Ar.IsLoading())
			{
				V.TangentZDelta = TangentZDelta_DEPRECATED;
			}
		}
		else
		{
			Ar << V.PositionDelta << V.TangentZDelta << V.SourceIdx;
		}
		return Ar;
	}
};


/**
* Mesh data for a single LOD model of a morph target
*/
struct FMorphTargetLODModel
{
	/** vertex data for a single LOD morph mesh */
	TArray<FMorphTargetDelta> Vertices;

	/** number of original verts in the base mesh */
	int32 NumBaseMeshVerts;
	
	/** list of sections this morph is used */
	TArray<int32> SectionIndices;

	/** Get Resource Size */
	void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const;

	FMorphTargetLODModel()
		: NumBaseMeshVerts(0)
	{ }

	/** pipe operator */
	friend FArchive& operator<<(FArchive& Ar, FMorphTargetLODModel& M)
	{
		Ar.UsingCustomVersion(FEditorObjectVersion::GUID);

		if (Ar.IsLoading() && Ar.CustomVer(FEditorObjectVersion::GUID) < FEditorObjectVersion::AddedMorphTargetSectionIndices)
		{
			Ar << M.Vertices << M.NumBaseMeshVerts;
		}
		else
		{
			Ar << M.Vertices << M.NumBaseMeshVerts << M.SectionIndices;
		}
		return Ar;
	}
};


UCLASS(hidecategories=Object, MinimalAPI)
class UMorphTarget
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** USkeletalMesh that this vertex animation works on. */
	UPROPERTY(AssetRegistrySearchable)
	class USkeletalMesh* BaseSkelMesh;

	/** morph mesh vertex data for each LOD */
	TArray<FMorphTargetLODModel>	MorphLODModels;

public:

	/** Get Morphtarget Delta array for the given input Index */
	FMorphTargetDelta* GetMorphTargetDelta(int32 LODIndex, int32& OutNumDeltas);
	ENGINE_API bool HasDataForLOD(int32 LODIndex);
	/** return true if this morphtarget contains valid vertices */
	ENGINE_API bool HasValidData() const;

#if WITH_EDITOR
	/** Populates the given morph target LOD model with the provided deltas */
	ENGINE_API void PopulateDeltas(const TArray<FMorphTargetDelta>& Deltas, const int32 LODIndex, const TArray<struct FSkelMeshSection>& Sections, const bool bCompareNormal = false);
#endif // WITH_EDITOR

public:

	//~ UObject interface

	virtual void Serialize(FArchive& Ar) override;
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	virtual void PostLoad() override;

};
