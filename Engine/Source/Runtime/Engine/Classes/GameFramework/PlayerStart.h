// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// Player start location.
//=============================================================================

#pragma once
#include "PlayerStart.generated.h"

UCLASS(ClassGroup=Common, hidecategories=Collision)
class ENGINE_API APlayerStart : public ANavigationObjectBase
{
	GENERATED_UCLASS_BODY()

	/** Used when searching for which playerstart to use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Object)
	FName PlayerStartTag;

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	class UArrowComponent* ArrowComponent;
public:
#endif

	virtual void PostInitializeComponents() override;
	
	virtual void PostUnregisterAllComponents() override;

#if WITH_EDITORONLY_DATA
	/** Returns ArrowComponent subobject **/
	FORCEINLINE class UArrowComponent* GetArrowComponent() const { return ArrowComponent; }
#endif
};



