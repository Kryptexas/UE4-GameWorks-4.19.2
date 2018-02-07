// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetData.h"
#include "MaterialLayersFunctions.generated.h"

#define LOCTEXT_NAMESPACE "MaterialLayersFunctions"

class FArchive;


UENUM()
enum EMaterialParameterAssociation
{
	LayerParameter,
	BlendParameter,
	GlobalParameter,
};

USTRUCT(BlueprintType)
struct ENGINE_API FMaterialParameterInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ParameterInfo)
	FName Name;

	/** Whether this is a global parameter, or part of a layer or blend */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ParameterInfo)
	TEnumAsByte<EMaterialParameterAssociation> Association;

	/** Layer or blend index this parameter is part of. INDEX_NONE for global parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ParameterInfo)
	int32 Index;

	FMaterialParameterInfo(const TCHAR* InName, EMaterialParameterAssociation InAssociation = EMaterialParameterAssociation::GlobalParameter, int32 InIndex = INDEX_NONE)
		: Name(InName)
		, Association(InAssociation)
		, Index(InIndex)
	{
	}
	FMaterialParameterInfo(FName InName = FName(), EMaterialParameterAssociation InAssociation = EMaterialParameterAssociation::GlobalParameter, int32 InIndex = INDEX_NONE)
	: Name(InName)
	, Association(InAssociation)
	, Index(InIndex)
	{
	}

	FString ToString() const
	{
		return *Name.ToString() + FString::FromInt(Association) + FString::FromInt(Index);
	}

	friend FArchive& operator<<(FArchive& Ar, FMaterialParameterInfo& Ref)
	{
		Ar << Ref.Name << Ref.Association << Ref.Index;
		return Ar;
	}

	FORCEINLINE bool operator==(const FMaterialParameterInfo& Other) const
	{
		return Name.IsEqual(Other.Name) && Association == Other.Association && Index == Other.Index;
	}

	FORCEINLINE bool operator!=(const FMaterialParameterInfo& Other) const
	{
		return !Name.IsEqual(Other.Name) || Association != Other.Association || Index != Other.Index;
	}
};

USTRUCT()
struct ENGINE_API FMaterialLayersFunctions
{
	GENERATED_USTRUCT_BODY()

	FMaterialLayersFunctions()
	{
		// Default to a non-blended "background" layer
		Layers.AddDefaulted();
#if WITH_EDITOR
		FText LayerName = FText(LOCTEXT("Background", "Background"));
		LayerNames.Add(LayerName);
		RestrictToLayerRelatives.Push(false);
#endif
		LayerStates.Push(true);

	}

	UPROPERTY(EditAnywhere, Category=MaterialLayers)
	TArray<class UMaterialFunctionInterface*> Layers;

	UPROPERTY(EditAnywhere, Category=MaterialLayers)
	TArray<class UMaterialFunctionInterface*> Blends;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TArray<FText> LayerNames;

	UPROPERTY()
	TArray<bool> RestrictToLayerRelatives;

	UPROPERTY()
	TArray<bool> RestrictToBlendRelatives;
#endif

	UPROPERTY()
	TArray<bool> LayerStates;


	UPROPERTY()
	FString KeyString;

	void AppendBlendedLayer()
	{
		Layers.AddDefaulted();
		Blends.AddDefaulted();
#if WITH_EDITOR
		FText LayerName = FText::Format(LOCTEXT("LayerPrefix", "Layer {0}"), Layers.Num()-1);
		LayerNames.Add(LayerName);
		RestrictToLayerRelatives.Push(false);
		RestrictToBlendRelatives.Push(false);
#endif
		LayerStates.Push(true);
		KeyString = GetStaticPermutationString();
	}

	void RemoveBlendedLayerAt(int32 Index)
	{
		check(Layers.IsValidIndex(Index) && Blends.IsValidIndex(Index-1) && LayerStates.IsValidIndex(Index));
		Layers.RemoveAt(Index);
		Blends.RemoveAt(Index-1);
		LayerStates.RemoveAt(Index);
#if WITH_EDITOR
		LayerNames.RemoveAt(Index);
		RestrictToLayerRelatives.RemoveAt(Index);
		RestrictToBlendRelatives.RemoveAt(Index - 1);
#endif
		KeyString = GetStaticPermutationString();
	}

	void ToggleBlendedLayerVisibility(int32 Index)
	{
		check(LayerStates.IsValidIndex(Index));
		LayerStates[Index] = !LayerStates[Index];
		KeyString = GetStaticPermutationString();
	}

	bool GetLayerVisibility(int32 Index)
	{
		check(LayerStates.IsValidIndex(Index));
		return LayerStates[Index];
	}

#if WITH_EDITORONLY_DATA
	FText GetLayerName(int32 Counter)
	{
		FText LayerName = FText::Format(LOCTEXT("LayerPrefix", "Layer {0}"), Counter);
		if (LayerNames.IsValidIndex(Counter))
		{
			LayerName = LayerNames[Counter];
		}
		return LayerName;
	}

#endif

	/** Lists referenced function packages in a string, intended for use as a static permutation identifier. */
	FString GetStaticPermutationString() const;

	void SerializeForDDC(FArchive& Ar);

	FORCEINLINE bool operator==(const FMaterialLayersFunctions& Other) const
	{
		return Layers == Other.Layers && Blends == Other.Blends && LayerStates == Other.LayerStates;
	}

	FORCEINLINE bool operator!=(const FMaterialLayersFunctions& Other) const
	{
		return Layers != Other.Layers || Blends != Other.Blends || LayerStates != Other.LayerStates;
	}

	FORCEINLINE void UpdateStaticPermutationString()
	{
		KeyString = GetStaticPermutationString();
	}
};

#undef LOCTEXT_NAMESPACE