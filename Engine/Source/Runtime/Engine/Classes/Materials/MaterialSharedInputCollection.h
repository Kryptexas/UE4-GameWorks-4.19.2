// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/**
 * MaterialSharedInputCollection.h - defines an asset that has a list of inputs, which can be referenced by any material to connect supply defined inputs to layers and blends
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Misc/Guid.h"
#include "MaterialSharedInputCollection.generated.h"

struct FPropertyChangedEvent;

UENUM()
namespace EMaterialSharedInputType
{
	enum Type
	{
		Scalar,
		Vector2,
		Vector3,
		Vector4,
		Texture2D,
		TextureCube
	};
}

/** Base struct for collection inputs */
USTRUCT()
struct FMaterialSharedInputInfo
{
	GENERATED_USTRUCT_BODY()
	
	FMaterialSharedInputInfo()
	{
		InputName = FName(TEXT("Input"));
		InputType = EMaterialSharedInputType::Scalar;
		FPlatformMisc::CreateGuid(Id);
	}

	/** The name of the input. Changing this name will break any blueprints that reference the input. */
	UPROPERTY(EditAnywhere, Category=Input)
	FName InputName;

	UPROPERTY(EditAnywhere, Category=Input)
	TEnumAsByte<EMaterialSharedInputType::Type> InputType;

	/** Uniquely identifies the input, used for fixing up materials that reference this input when renaming. */
	UPROPERTY()
	FGuid Id;
};

/** 
 * Asset class that contains a list of typed inputs. 
 * Any number of materials, layers and blends can reference this collection.
 */
UCLASS(hidecategories=object, MinimalAPI)
class UMaterialSharedInputCollection : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Used by materials using this collection to know when to recompile. */
	UPROPERTY(duplicatetransient)
	FGuid StateId;

	UPROPERTY(EditAnywhere, Category=Material)
	TArray<FMaterialSharedInputInfo> Inputs;

	//~ Begin UObject Interface.
#if WITH_EDITOR
	virtual void PreEditChange(class FEditPropertyChain& PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	virtual void PostLoad() override;
	//~ End UObject Interface.

	/** Finds an input name given an Id, returns NAME_None if the input was not found. */
	FName GetInputName(const FGuid& Id) const;

	/** Finds an input id given a name, returns the default guid if the input was not found. */
	FGuid GetInputId(FName InputName) const;

	/** Finds an input type given a name, returns scalar if the input was not found. */
	EMaterialSharedInputType::Type GetInputType(const FGuid& Id) const;

	/** Returns the name of the input type given a name, returns the "Unknown" if the input was not found. */
	FString GetTypeName(const FGuid& Id) const;
};
