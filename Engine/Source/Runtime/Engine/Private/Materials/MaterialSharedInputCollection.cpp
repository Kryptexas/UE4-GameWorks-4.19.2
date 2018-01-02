// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Materials/MaterialSharedInputCollection.h"
#include "UObject/UObjectIterator.h"
#include "MaterialShared.h"
#include "Materials/Material.h"

UMaterialSharedInputCollection::UMaterialSharedInputCollection(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMaterialSharedInputCollection::PostLoad()
{
	Super::PostLoad();
	
	if (!StateId.IsValid())
	{
		StateId = FGuid::NewGuid();
	}
}

#if WITH_EDITOR
template<typename InputType>
FName CreateUniqueInputName(TArray<InputType>& Inputs, int32 RenameInputIndex)
{
	FString RenameString;
	Inputs[RenameInputIndex].InputName.ToString(RenameString);

	int32 NumberStartIndex = RenameString.FindLastCharByPredicate([](TCHAR Letter){ return !FChar::IsDigit(Letter); }) + 1;
	
	int32 RenameNumber = 0;
	if (NumberStartIndex < RenameString.Len() - 1)
	{
		FString RenameStringNumberPart = RenameString.RightChop(NumberStartIndex);
		ensure(RenameStringNumberPart.IsNumeric());

		TTypeFromString<int32>::FromString(RenameNumber, *RenameStringNumberPart);
	}

	FString BaseString = RenameString.Left(NumberStartIndex);

	FName Renamed = FName(*FString::Printf(TEXT("%s%u"), *BaseString, ++RenameNumber));

	bool bMatchFound = false;
	
	do
	{
		bMatchFound = false;

		for (int32 i = 0; i < Inputs.Num(); ++i)
		{
			if (Inputs[i].InputName == Renamed && RenameInputIndex != i)
			{
				Renamed = FName(*FString::Printf(TEXT("%s%u"), *BaseString, ++RenameNumber));
				bMatchFound = true;
				break;
			}
		}
	} while (bMatchFound);
	
	return Renamed;
}

template<typename InputType>
void SanitizeInputs(TArray<InputType>& Inputs)
{
	for (int32 i = 0; i < Inputs.Num() - 1; ++i)
	{
		for (int32 j = i + 1; j < Inputs.Num(); ++j)
		{
			if (Inputs[i].Id == Inputs[j].Id)
			{
				FPlatformMisc::CreateGuid(Inputs[j].Id);
			}

			if (Inputs[i].InputName == Inputs[j].InputName)
			{
				Inputs[j].InputName = CreateUniqueInputName(Inputs, j);
			}
		}
	}
}

TArray<FMaterialSharedInputInfo> PreviousInputs;
FString PreviousName;

void UMaterialSharedInputCollection::PreEditChange(class FEditPropertyChain& PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	PreviousInputs = Inputs;
	PreviousName = GetName();
}

void UMaterialSharedInputCollection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	SanitizeInputs(Inputs);

	// Build set of changed parameter names
	TSet<FName> RenamedInputs;
	for (const FMaterialSharedInputInfo& Input : PreviousInputs)
	{
		RenamedInputs.Add(Input.InputName);
	}

	for (const FMaterialSharedInputInfo& Input : Inputs)
	{
		RenamedInputs.Remove(Input.InputName);
	}

	// If an element has been added, removed or renamed we need to update referencing materials
	if (Inputs.Num() != PreviousInputs.Num() || RenamedInputs.Num() > 0 || PreviousName != GetName())
	{
		// Generate a new Id so that unloaded materials that reference this collection will update correctly on load
		StateId = FGuid::NewGuid();

		// Create a material update context so we can safely update materials using this collection.
		{
			FMaterialUpdateContext UpdateContext;

			// Go through all materials in memory and recompile them if they use this material collection
			for (TObjectIterator<UMaterial> It; It; ++It)
			{
				UMaterial* CurrentMaterial = *It;

				bool bRecompile = false;

				// Preview materials often use expressions for rendering that are not in their Expressions array, 
				// and therefore their MaterialSharedInputCollectionInfo is not up to date.
				if (CurrentMaterial->bIsPreviewMaterial || CurrentMaterial->bIsFunctionPreviewMaterial)
				{
					bRecompile = true;
				}
				else
				{
					for (FMaterialSharedInputCollectionInfo& Info : CurrentMaterial->MaterialSharedInputCollectionInfos)
					{
						if (Info.SharedInputCollection == this && Info.StateId != StateId)
						{
							bRecompile = true;
							break;
						}
					}
				}

				if (bRecompile)
				{
					UpdateContext.AddMaterial(CurrentMaterial);

					// Propagate the change to this material
					CurrentMaterial->PreEditChange(NULL);
					CurrentMaterial->PostEditChange();
					CurrentMaterial->MarkPackageDirty();
				}
			}
		}
	}

	PreviousInputs.Empty();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

FName UMaterialSharedInputCollection::GetInputName(const FGuid& Id) const
{
	if (Id.IsValid())
	{
		for (const FMaterialSharedInputInfo& Input : Inputs)
		{
			if (Input.Id == Id)
			{
				return Input.InputName;
			}
		}
	}

	return NAME_None;
}

FGuid UMaterialSharedInputCollection::GetInputId(FName InputName) const
{
	for (const FMaterialSharedInputInfo& Input : Inputs)
	{
		if (Input.InputName == InputName)
		{
			return Input.Id;
		}
	}

	return FGuid();
}


EMaterialSharedInputType::Type UMaterialSharedInputCollection::GetInputType(const FGuid& Id) const
{
	for (const FMaterialSharedInputInfo& Input : Inputs)
	{
		if (Input.Id == Id)
		{
			return Input.InputType;
		}
	}

	return EMaterialSharedInputType::Scalar;
}

FString UMaterialSharedInputCollection::GetTypeName(const FGuid& Id) const
{
	if (Id.IsValid())
	{
		for (const FMaterialSharedInputInfo& Input : Inputs)
		{
			if (Input.Id == Id)
			{
				switch (Input.InputType)
				{
				case EMaterialSharedInputType::Vector2:
					return TEXT("Vector2");
				case EMaterialSharedInputType::Vector3:
					return TEXT("Vector3");
				case EMaterialSharedInputType::Vector4:
					return TEXT("Vector4");
				case EMaterialSharedInputType::Texture2D:
					return TEXT("Texture2D");
				case EMaterialSharedInputType::TextureCube:
					return TEXT("TextureCube");
				default:
					return TEXT("Scalar");
				}
			}
		}
	}

	return TEXT("Unknown");
}