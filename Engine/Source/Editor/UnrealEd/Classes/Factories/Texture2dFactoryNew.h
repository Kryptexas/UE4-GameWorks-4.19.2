// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// Texture2DFactoryNew
//=============================================================================

#pragma once
#include "Texture2dFactoryNew.generated.h"

UCLASS(hidecategories=Object, MinimalAPI)
class UTexture2DFactoryNew : public UFactory
{
	GENERATED_BODY()
public:
	UNREALED_API UTexture2DFactoryNew(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** width of new texture */
	UPROPERTY()
	int32		Width;

	/** height of new texture */
	UPROPERTY()
	int32		Height;

	virtual bool ShouldShowInNewMenu() const override;
	virtual UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn ) override;
};
