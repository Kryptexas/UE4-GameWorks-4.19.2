// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MaterialInstanceConstant.cpp: MaterialInstanceConstant implementation.
=============================================================================*/

#include "Materials/MaterialInstanceConstant.h"


UMaterialInstanceConstant::UMaterialInstanceConstant(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMaterialInstanceConstant::PostLoad()
{
	LLM_SCOPE(ELLMTag::Materials);
	Super::PostLoad();
}

FLinearColor UMaterialInstanceConstant::K2_GetVectorParameterValue(FName ParameterName)
{
	FLinearColor Result(0,0,0);
	Super::GetVectorParameterValue(ParameterName, Result);
	return Result;
}

float UMaterialInstanceConstant::K2_GetScalarParameterValue(FName ParameterName)
{
	float Result = 0.f;
	Super::GetScalarParameterValue(ParameterName, Result);
	return Result;
}

UTexture* UMaterialInstanceConstant::K2_GetTextureParameterValue(FName ParameterName)
{
	UTexture* Result = NULL;
	Super::GetTextureParameterValue(ParameterName, Result);
	return Result;
}

#if WITH_EDITOR
void UMaterialInstanceConstant::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ParameterStateId = FGuid::NewGuid();
}

void UMaterialInstanceConstant::SetParentEditorOnly(UMaterialInterface* NewParent)
{
	check(GIsEditor);
	SetParentInternal(NewParent, true);
}

void UMaterialInstanceConstant::SetVectorParameterValueEditorOnly(const FMaterialParameterInfo& ParameterInfo, FLinearColor Value)
{
	check(GIsEditor);
	SetVectorParameterValueInternal(ParameterInfo,Value);
}

void UMaterialInstanceConstant::SetScalarParameterValueEditorOnly(const FMaterialParameterInfo& ParameterInfo, float Value)
{
	check(GIsEditor);
	SetScalarParameterValueInternal(ParameterInfo,Value);
}

void UMaterialInstanceConstant::SetTextureParameterValueEditorOnly(const FMaterialParameterInfo& ParameterInfo, UTexture* Value)
{
	check(GIsEditor);
	SetTextureParameterValueInternal(ParameterInfo,Value);
}

void UMaterialInstanceConstant::SetFontParameterValueEditorOnly(const FMaterialParameterInfo& ParameterInfo,class UFont* FontValue,int32 FontPage)
{
	check(GIsEditor);
	SetFontParameterValueInternal(ParameterInfo,FontValue,FontPage);
}

void UMaterialInstanceConstant::ClearParameterValuesEditorOnly()
{
	check(GIsEditor);
	ClearParameterValuesInternal();
}
#endif // #if WITH_EDITOR
