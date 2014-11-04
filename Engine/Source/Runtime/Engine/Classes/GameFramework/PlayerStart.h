// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// Player start location.
//=============================================================================

#pragma once
#include "Engine/NavigationObjectBase.h"
#include "PlayerStart.generated.h"

UCLASS(ClassGroup=Common, hidecategories=Collision)
class ENGINE_API APlayerStart : public ANavigationObjectBase
{
	GENERATED_UCLASS_BODY()

	/** Used when searching for which playerstart to use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Object)
	FName PlayerStartTag;

#if WITH_EDITORONLY_DATA
private_subobject:
	DEPRECATED_FORGAME(4.6, "ArrowComponent should not be accessed directly, please use GetArrowComponent() function instead. ArrowComponent will soon be private and your code will not compile.")
	UPROPERTY()
	class UArrowComponent* ArrowComponent;
public:
#endif

	virtual void PostInitializeComponents() override;
	
	virtual void PostUnregisterAllComponents() override;

#if WITH_EDITORONLY_DATA
	/** Returns ArrowComponent subobject **/
	class UArrowComponent* GetArrowComponent() const;
#endif
};



