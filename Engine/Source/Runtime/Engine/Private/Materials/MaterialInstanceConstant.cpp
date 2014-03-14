// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MaterialInstanceConstant.cpp: MaterialInstanceConstant implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineMaterialClasses.h"
#include "MaterialInstance.h"


UMaterialInstanceConstant::UMaterialInstanceConstant(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

#if WITH_EDITOR
void UMaterialInstanceConstant::SetParentEditorOnly(UMaterialInterface* NewParent)
{
	check(GIsEditor);
	SetParentInternal(NewParent);
}

void UMaterialInstanceConstant::SetVectorParameterValueEditorOnly(FName ParameterName, FLinearColor Value)
{
	check(GIsEditor);
	SetVectorParameterValueInternal(ParameterName,Value);
}

void UMaterialInstanceConstant::SetScalarParameterValueEditorOnly(FName ParameterName, float Value)
{
	check(GIsEditor);
	SetScalarParameterValueInternal(ParameterName,Value);
}

void UMaterialInstanceConstant::SetTextureParameterValueEditorOnly(FName ParameterName, UTexture* Value)
{
	check(GIsEditor);
	SetTextureParameterValueInternal(ParameterName,Value);
}

void UMaterialInstanceConstant::SetFontParameterValueEditorOnly(FName ParameterName,class UFont* FontValue,int32 FontPage)
{
	check(GIsEditor);
	SetFontParameterValueInternal(ParameterName,FontValue,FontPage);
}

void UMaterialInstanceConstant::ClearParameterValuesEditorOnly()
{
	check(GIsEditor);
	ClearParameterValuesInternal();
}
#endif // #if WITH_EDITOR
