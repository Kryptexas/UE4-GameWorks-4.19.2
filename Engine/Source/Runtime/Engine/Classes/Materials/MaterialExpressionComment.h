// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionComment.generated.h"

UCLASS(HeaderGroup=Material, MinimalAPI)
class UMaterialExpressionComment : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	int32 SizeX;

	UPROPERTY()
	int32 SizeY;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionComment)
	FString Text;

	/** Color to style comment with */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionComment)
	FLinearColor CommentColor;

	// Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual bool Modify( bool bAlwaysMarkDirty=true ) OVERRIDE;
	// End UObject Interface

	// Begin UMaterialExpression Interface
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual bool MatchesSearchQuery( const TCHAR* SearchQuery ) OVERRIDE;
	// End UMaterialExpression Interface
};



