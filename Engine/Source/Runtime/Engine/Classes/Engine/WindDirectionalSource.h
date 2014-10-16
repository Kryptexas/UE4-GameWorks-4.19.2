// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "WindDirectionalSource.generated.h"

UCLASS(ClassGroup=Wind, showcategories=(Rendering, "Utilities|Transformation"))
class ENGINE_API AWindDirectionalSource : public AInfo
{
	GENERATED_UCLASS_BODY()

private:
	UPROPERTY(Category = WindDirectionalSource, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWindDirectionalSourceComponent* Component;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UArrowComponent* ArrowComponent;
#endif

public:
	/** Returns Component subobject **/
	FORCEINLINE class UWindDirectionalSourceComponent* GetComponent() const { return Component; }

#if WITH_EDITORONLY_DATA
	/** Returns ArrowComponent subobject **/
	FORCEINLINE class UArrowComponent* GetArrowComponent() const { return ArrowComponent; }
#endif
};



