// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CompositeFont.h"
#include "FontProviderInterface.generated.h"

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UFontProviderInterface : public UInterface
{
	GENERATED_BODY()
public:
	SLATECORE_API UFontProviderInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

class IFontProviderInterface
{
	GENERATED_BODY()
public:

	virtual const FCompositeFont* GetCompositeFont() const = 0;
};
