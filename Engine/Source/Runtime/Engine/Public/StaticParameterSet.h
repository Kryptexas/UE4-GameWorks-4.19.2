// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NameTypes.h"
#include "Misc/Guid.h"
#include "UObject/RenderingObjectVersion.h"
#include "UObject/ReleaseObjectVersion.h"
#include "Materials/MaterialLayersFunctions.h"
#include "StaticParameterSet.generated.h"

class FSHA1;

/**
* Holds the information for a static switch parameter
*/
USTRUCT()
struct FStaticSwitchParameter
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	FMaterialParameterInfo ParameterInfo;

	UPROPERTY()
	bool Value;

	UPROPERTY()
	bool bOverride;

	UPROPERTY()
	FGuid ExpressionGUID;

	FStaticSwitchParameter() :
		Value(false),
		bOverride(false),
		ExpressionGUID(0, 0, 0, 0)
	{ }

	FStaticSwitchParameter(const FMaterialParameterInfo& InInfo, bool InValue, bool InOverride, FGuid InGuid) :
		ParameterInfo(InInfo),
		Value(InValue),
		bOverride(InOverride),
		ExpressionGUID(InGuid)
	{ }

	friend FArchive& operator<<(FArchive& Ar, FStaticSwitchParameter& P)
	{
		if (Ar.CustomVer(FRenderingObjectVersion::GUID) < FRenderingObjectVersion::MaterialAttributeLayerParameters)
		{
			Ar << P.ParameterInfo.Name;
		}
		else
		{
			Ar << P.ParameterInfo;
		}
		Ar << P.Value << P.bOverride << P.ExpressionGUID;
		return Ar;
	}
};

/**
* Holds the information for a static component mask parameter
*/
USTRUCT()
struct FStaticComponentMaskParameter
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	FMaterialParameterInfo ParameterInfo;

	UPROPERTY()
	bool R;
	
	UPROPERTY()
	bool G;

	UPROPERTY()
	bool B;

	UPROPERTY()
	bool A; 

	UPROPERTY()
	bool bOverride;

	UPROPERTY()
	FGuid ExpressionGUID;

	FStaticComponentMaskParameter() :
		R(false),
		G(false),
		B(false),
		A(false),
		bOverride(false),
		ExpressionGUID(0, 0, 0, 0)
	{ }

	FStaticComponentMaskParameter(const FMaterialParameterInfo& InInfo, bool InR, bool InG, bool InB, bool InA, bool InOverride, FGuid InGuid) :
		ParameterInfo(InInfo),
		R(InR),
		G(InG),
		B(InB),
		A(InA),
		bOverride(InOverride),
		ExpressionGUID(InGuid)
	{ }

	friend FArchive& operator<<(FArchive& Ar, FStaticComponentMaskParameter& P)
	{
		if (Ar.CustomVer(FRenderingObjectVersion::GUID) < FRenderingObjectVersion::MaterialAttributeLayerParameters)
		{
			Ar << P.ParameterInfo.Name;
		}
		else
		{
			Ar << P.ParameterInfo;
		}
		Ar << P.R << P.G << P.B << P.A << P.bOverride << P.ExpressionGUID;
		return Ar;
	}
};

/**
* Holds the information for a static switch parameter
*/
USTRUCT()
struct FStaticTerrainLayerWeightParameter
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	FMaterialParameterInfo ParameterInfo;

	UPROPERTY()
	bool bOverride;

	UPROPERTY()
	FGuid ExpressionGUID;

	UPROPERTY()
	int32 WeightmapIndex;

	FStaticTerrainLayerWeightParameter() :
		bOverride(false),
		ExpressionGUID(0, 0, 0, 0),
		WeightmapIndex(INDEX_NONE)
	{ }

	FStaticTerrainLayerWeightParameter(const FMaterialParameterInfo& InInfo, int32 InWeightmapIndex, bool InOverride, FGuid InGuid) :
		ParameterInfo(InInfo),
		bOverride(InOverride),
		ExpressionGUID(InGuid),
		WeightmapIndex(InWeightmapIndex)
	{ }

	friend FArchive& operator<<(FArchive& Ar, FStaticTerrainLayerWeightParameter& P)
	{
		if (Ar.CustomVer(FRenderingObjectVersion::GUID) < FRenderingObjectVersion::MaterialAttributeLayerParameters)
		{
			Ar << P.ParameterInfo.Name;
		}
		else
		{
			Ar << P.ParameterInfo;
		}
		Ar << P.WeightmapIndex<< P.bOverride << P.ExpressionGUID;
		return Ar;
	}
};

/**
* Holds the information for a static material layers parameter
*/
USTRUCT()
struct FStaticMaterialLayersParameter
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	FMaterialParameterInfo ParameterInfo;

	UPROPERTY()
	FMaterialLayersFunctions Value;

	UPROPERTY()
	bool bOverride;

	UPROPERTY()
	FGuid ExpressionGUID;

	FStaticMaterialLayersParameter() :
		bOverride(false),
		ExpressionGUID(0, 0, 0, 0)
	{ }

	FStaticMaterialLayersParameter(const FMaterialParameterInfo& InInfo, const FMaterialLayersFunctions& InValue, bool InOverride, FGuid InGuid) :
		ParameterInfo(InInfo),
		Value(InValue),
		bOverride(InOverride),
		ExpressionGUID(InGuid)
	{ }
	
	UMaterialFunctionInterface* GetParameterAssociatedFunction(const FMaterialParameterInfo& InParameterInfo) const;
	void GetParameterAssociatedFunctions(const FMaterialParameterInfo& InParameterInfo, TArray<UMaterialFunctionInterface*>& AssociatedFunctions) const;
	
	void AppendKeyString(FString& InKeyString) const
	{
		InKeyString += ParameterInfo.ToString() + ExpressionGUID.ToString() + Value.GetStaticPermutationString();
	}

	friend FArchive& operator<<(FArchive& Ar, FStaticMaterialLayersParameter& P)
	{
		Ar << P.ParameterInfo << P.bOverride << P.ExpressionGUID;
		
		Ar.UsingCustomVersion(FReleaseObjectVersion::GUID);
		if (Ar.CustomVer(FReleaseObjectVersion::GUID) >= FReleaseObjectVersion::MaterialLayersParameterSerializationRefactor)
		{
			P.Value.SerializeForDDC(Ar);
		}
		return Ar;
	}
};

/** Contains all the information needed to identify a single permutation of static parameters. */
USTRUCT()
struct FStaticParameterSet
{
	GENERATED_USTRUCT_BODY();

	/** An array of static switch parameters in this set */
	UPROPERTY()
	TArray<FStaticSwitchParameter> StaticSwitchParameters;

	/** An array of static component mask parameters in this set */
	UPROPERTY()
	TArray<FStaticComponentMaskParameter> StaticComponentMaskParameters;

	/** An array of terrain layer weight parameters in this set */
	UPROPERTY()
	TArray<FStaticTerrainLayerWeightParameter> TerrainLayerWeightParameters;

	/** An array of function call parameters in this set */
	UPROPERTY()
	TArray<FStaticMaterialLayersParameter> MaterialLayersParameters;

	/** 
	* Checks if this set contains any parameters
	* 
	* @return	true if this set has no parameters
	*/
	bool IsEmpty() const
	{
		return StaticSwitchParameters.Num() == 0 && StaticComponentMaskParameters.Num() == 0 && TerrainLayerWeightParameters.Num() == 0 && MaterialLayersParameters.Num() == 0;
	}

	void Serialize(FArchive& Ar)
	{
		Ar.UsingCustomVersion(FRenderingObjectVersion::GUID);
		Ar.UsingCustomVersion(FReleaseObjectVersion::GUID);
		// Note: FStaticParameterSet is saved both in packages (UMaterialInstance) and the DDC (FMaterialShaderMap)
		// Backwards compatibility only works with FStaticParameterSet's stored in packages.  
		// You must bump MATERIALSHADERMAP_DERIVEDDATA_VER as well if changing the serialization of FStaticParameterSet.
		Ar << StaticSwitchParameters;
		Ar << StaticComponentMaskParameters;
		Ar << TerrainLayerWeightParameters;
		if (Ar.CustomVer(FReleaseObjectVersion::GUID) >= FReleaseObjectVersion::MaterialLayersParameterSerializationRefactor)
		{
			Ar << MaterialLayersParameters;
		}
	}

	/** Updates the hash state with the static parameter names and values. */
	void UpdateHash(FSHA1& HashState) const;

	/** 
	* Indicates whether this set is equal to another, copying override settings.
	* 
	* @param ReferenceSet	The set to compare against
	* @return				true if the sets are not equal
	*/
	bool ShouldMarkDirty(const FStaticParameterSet* ReferenceSet);

	/** 
	* Tests this set against another for equality, disregarding override settings.
	* 
	* @param ReferenceSet	The set to compare against
	* @return				true if the sets are equal
	*/
	bool operator==(const FStaticParameterSet& ReferenceSet) const;

	bool operator!=(const FStaticParameterSet& ReferenceSet) const
	{
		return !(*this == ReferenceSet);
	}

	bool Equivalent(const FStaticParameterSet& ReferenceSet) const;

	void SortForEquivalent();


	FString GetSummaryString() const;

	void AppendKeyString(FString& KeyString) const;
};
