// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "WindDirectionalSource.generated.h"

UCLASS(ClassGroup=Wind, showcategories=(Rendering, "Utilities|Orientation"))
class ENGINE_API AWindDirectionalSource : public AInfo
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=WindDirectionalSource, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UWindDirectionalSourceComponent> Component;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TSubobjectPtr<class UArrowComponent> ArrowComponent;
#endif
};



