// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	MaterialInstanceDynamic.cpp: MaterialInstanceDynamic implementation.
==============================================================================*/

#include "EnginePrivate.h"
#include "MaterialInstanceSupport.h"

UMaterialInstanceDynamic::UMaterialInstanceDynamic(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UMaterialInstanceDynamic* UMaterialInstanceDynamic::Create(UMaterialInterface* ParentMaterial, UObject* InOuter)
{
	UObject* Outer = InOuter ? InOuter : GetTransientPackage();
	UMaterialInstanceDynamic* MID = NewObject<UMaterialInstanceDynamic>(Outer);
	MID->SetParentInternal(ParentMaterial, false);
	return MID;
}

void UMaterialInstanceDynamic::SetVectorParameterValue(FName ParameterName, FLinearColor Value)
{
	SetVectorParameterValueInternal(ParameterName,Value);
}

FLinearColor UMaterialInstanceDynamic::K2_GetVectorParameterValue(FName ParameterName)
{
	FLinearColor Result(0,0,0);
	Super::GetVectorParameterValue(ParameterName, Result);
	return Result;
}

void UMaterialInstanceDynamic::SetScalarParameterValue(FName ParameterName, float Value)
{
	SetScalarParameterValueInternal(ParameterName,Value);
}

float UMaterialInstanceDynamic::K2_GetScalarParameterValue(FName ParameterName)
{
	float Result = 0.f;
	Super::GetScalarParameterValue(ParameterName, Result);
	return Result;
}

void UMaterialInstanceDynamic::SetTextureParameterValue(FName ParameterName, UTexture* Value)
{
	SetTextureParameterValueInternal(ParameterName,Value);
}

UTexture* UMaterialInstanceDynamic::K2_GetTextureParameterValue(FName ParameterName)
{
	UTexture* Result = NULL;
	Super::GetTextureParameterValue(ParameterName, Result);
	return Result;
}

void UMaterialInstanceDynamic::SetFontParameterValue(FName ParameterName,class UFont* FontValue,int32 FontPage)
{
	SetFontParameterValueInternal(ParameterName,FontValue,FontPage);
}

void UMaterialInstanceDynamic::ClearParameterValues()
{
	ClearParameterValuesInternal();
}


// could be optimized but surely faster than GetAllVectorParameterNames()
void GameThread_FindAllScalarParameterNames(UMaterialInstance* MaterialInstance, TArray<FName>& InOutNames)
{
	while(MaterialInstance)
	{
		for(int32 i = 0, Num = MaterialInstance->ScalarParameterValues.Num(); i < Num; ++i)
		{
			InOutNames.AddUnique(MaterialInstance->ScalarParameterValues[i].ParameterName);
		}

		MaterialInstance = Cast<UMaterialInstance>(MaterialInstance->Parent);
	}
}

// could be optimized but surely faster than GetAllVectorParameterNames()
void GameThread_FindAllVectorParameterNames(UMaterialInstance* MaterialInstance, TArray<FName>& InOutNames)
{
	while(MaterialInstance)
	{
		for(int32 i = 0, Num = MaterialInstance->VectorParameterValues.Num(); i < Num; ++i)
		{
			InOutNames.AddUnique(MaterialInstance->VectorParameterValues[i].ParameterName);
		}

		MaterialInstance = Cast<UMaterialInstance>(MaterialInstance->Parent);
	}
}

// Finds a parameter by name from the game thread, traversing the chain up to the BaseMaterial.
FScalarParameterValue* GameThread_GetScalarParameterValue(UMaterialInstance* MaterialInstance, FName Name)
{
	UMaterialInterface* It = 0;

	while(MaterialInstance)
	{
		if(FScalarParameterValue* Ret = GameThread_FindParameterByName(MaterialInstance->ScalarParameterValues, Name))
		{
			return Ret;
		}

		It = MaterialInstance->Parent;
		MaterialInstance = Cast<UMaterialInstance>(It);
	}

	return 0;
}

// Finds a parameter by name from the game thread, traversing the chain up to the BaseMaterial.
FVectorParameterValue* GameThread_GetVectorParameterValue(UMaterialInstance* MaterialInstance, FName Name)
{
	UMaterialInterface* It = 0;

	while(MaterialInstance)
	{
		if(FVectorParameterValue* Ret = GameThread_FindParameterByName(MaterialInstance->VectorParameterValues, Name))
		{
			return Ret;
		}

		It = MaterialInstance->Parent;
		MaterialInstance = Cast<UMaterialInstance>(It);
	}

	return 0;
}

void UMaterialInstanceDynamic::K2_InterpolateMaterialInstanceParams(UMaterialInstance* MaterialInstanceA, UMaterialInstance* MaterialInstanceB, float Alpha)
{
	if(MaterialInstanceA && MaterialInstanceB)
	{
		UMaterial* BaseMaterialInstanceA = MaterialInstanceA->GetBaseMaterial();
		UMaterial* BaseMaterialInstanceB = MaterialInstanceB->GetBaseMaterial();

		if(BaseMaterialInstanceA == BaseMaterialInstanceB)
		{
			// todo: can be optimized, at least we can reserve
			TArray<FName> Names;

			GameThread_FindAllScalarParameterNames(MaterialInstanceA, Names);
			GameThread_FindAllScalarParameterNames(MaterialInstanceB, Names);

			// Interpolate the scalar parameters common to both materials
			for(int32 Idx = 0, Count = Names.Num(); Idx < Count; ++Idx)
			{
				FName Name = Names[Idx];

				auto ParamValueA = GameThread_GetScalarParameterValue(MaterialInstanceA, Name);
				auto ParamValueB = GameThread_GetScalarParameterValue(MaterialInstanceB, Name);

				if(!ParamValueA && !ParamValueB)
				{
					auto Default = 0.0f;

					if(!ParamValueA || !ParamValueB)
					{
						BaseMaterialInstanceA->GetScalarParameterValue(Name, Default);
					}

					auto ValueA = ParamValueA ? ParamValueA->ParameterValue : Default;
					auto ValueB = ParamValueB ? ParamValueB->ParameterValue : Default;

					SetScalarParameterValue(Name, FMath::Lerp(ValueA, ValueB, Alpha));
				}
			}

			// reused array to minimize further allocations
			Names.Empty();
			GameThread_FindAllVectorParameterNames(MaterialInstanceA, Names);
			GameThread_FindAllVectorParameterNames(MaterialInstanceB, Names);

			// Interpolate the vector parameters common to both
			for(int32 Idx = 0, Count = Names.Num(); Idx < Count; ++Idx)
			{
				FName Name = Names[Idx];

				auto ParamValueA = GameThread_GetVectorParameterValue(MaterialInstanceA, Name);
				auto ParamValueB = GameThread_GetVectorParameterValue(MaterialInstanceB, Name);

				if(!ParamValueA && !ParamValueB)
				{
					auto Default = FLinearColor();

					if(!ParamValueA || !ParamValueB)
					{
						BaseMaterialInstanceA->GetVectorParameterValue(Name, Default);
					}

					auto ValueA = ParamValueA ? ParamValueA->ParameterValue : Default;
					auto ValueB = ParamValueB ? ParamValueB->ParameterValue : Default;

					SetVectorParameterValue(Name, FMath::Lerp(ValueA, ValueB, Alpha));
				}
			}
		}
		else
		{
			// to find bad usage of this method
			ensure(BaseMaterialInstanceA == BaseMaterialInstanceB);
		}
	}
}

void UMaterialInstanceDynamic::K2_CopyMaterialInstanceParameters(UMaterialInterface* SourceMaterialToCopyFrom)
{
	CopyMaterialInstanceParameters(SourceMaterialToCopyFrom);
}

void UMaterialInstanceDynamic::CopyParameterOverrides(UMaterialInstance* MaterialInstance)
{
	ClearParameterValues();
	VectorParameterValues = MaterialInstance->VectorParameterValues;
	ScalarParameterValues = MaterialInstance->ScalarParameterValues;
	TextureParameterValues = MaterialInstance->TextureParameterValues;
	FontParameterValues = MaterialInstance->FontParameterValues;
	InitResources();
}
