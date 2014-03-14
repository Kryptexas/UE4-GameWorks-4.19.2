// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// TexAlignerFit
// Fits the texture to a face
// 
//=============================================================================

#pragma once
#include "TexAlignerFit.generated.h"

UCLASS(hidecategories=Object)
class UTexAlignerFit : public UTexAligner
{
	GENERATED_UCLASS_BODY()


	// Begin UObject Interface
	virtual void PostInitProperties() OVERRIDE;
	// End UObject Interface

	// Begin UTexAligner Interface
	virtual void AlignSurf( ETexAlign InTexAlignType, UModel* InModel, FBspSurfIdx* InSurfIdx, FPoly* InPoly, FVector* InNormal ) OVERRIDE;
	// End UTexAligner Interface
};

