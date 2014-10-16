// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 */

#pragma once
#include "TargetPoint.generated.h"

UCLASS(MinimalAPI)
class ATargetPoint : public AActor
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Display, meta = (AllowPrivateAccess = "true"))
	class UBillboardComponent* SpriteComponent;

	UPROPERTY()
	class UArrowComponent* ArrowComponent;

public:
	/** Returns SpriteComponent subobject **/
	FORCEINLINE class UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }
	/** Returns ArrowComponent subobject **/
	FORCEINLINE class UArrowComponent* GetArrowComponent() const { return ArrowComponent; }
#endif
};



