// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// TexAlignerPlanar
// Aligns according to which axis the poly is most facing.
// 
//=============================================================================

#pragma once
#include "TexAlignerPlanar.generated.h"

UCLASS(hidecategories=Object)
class UTexAlignerPlanar : public UTexAligner
{
	GENERATED_BODY()
public:
	UTexAlignerPlanar(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UObject Interface
	virtual void PostInitProperties() override;
	// End UObject Interface

	// Begin UTexAligner Interface
	virtual void AlignSurf( ETexAlign InTexAlignType, UModel* InModel, FBspSurfIdx* InSurfIdx, FPoly* InPoly, FVector* InNormal ) override;
	// End UTexAligner Interface
};

