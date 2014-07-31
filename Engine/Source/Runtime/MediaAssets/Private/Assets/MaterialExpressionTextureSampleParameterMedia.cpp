// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetsPrivatePCH.h"


#define LOCTEXT_NAMESPACE "UMaterialExpressionTextureSampleParameterMedia"


/* UMaterialExpressionTextureSampleParameterMedia structors
 *****************************************************************************/

UMaterialExpressionTextureSampleParameterMedia::UMaterialExpressionTextureSampleParameterMedia( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FString NAME_Texture;
		FString NAME_Parameters;
		FConstructorStatics()
			: NAME_Texture(LOCTEXT("Texture", "Texture").ToString())
			, NAME_Parameters(LOCTEXT("Parameters", "Parameters").ToString())
		{ }
	};

	static FConstructorStatics ConstructorStatics;

	MenuCategories.Empty();
	MenuCategories.Add(ConstructorStatics.NAME_Texture);
	MenuCategories.Add(ConstructorStatics.NAME_Parameters);
}


/* UMaterialExpressionTextureSampleParameter interface
 *****************************************************************************/

const TCHAR* UMaterialExpressionTextureSampleParameterMedia::GetRequirements( )
{
	return TEXT("Requires MediaTexture");
}


bool UMaterialExpressionTextureSampleParameterMedia::TextureIsValid( UTexture* InTexture )
{
	bool Result = false;

	if (InTexture)
	{
		Result = (InTexture->GetClass() == UMediaTexture::StaticClass());
	}

	return Result;
}


/* UMaterialExpression interface
 *****************************************************************************/

void UMaterialExpressionTextureSampleParameterMedia::GetCaption( TArray<FString>& OutCaptions ) const
{
	OutCaptions.Add(TEXT("ParamMedia")); 
	OutCaptions.Add(FString::Printf(TEXT("'%s'"), *ParameterName.ToString()));
}


#undef LOCTEXT_NAMESPACE
