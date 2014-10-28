// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraActor.generated.h"

UCLASS()
class ENGINE_API ANiagaraActor : public AActor
{
	GENERATED_UCLASS_BODY()

private:
	/** Pointer to effect component */
	UPROPERTY(VisibleAnywhere, Category=NiagaraActor)
	class UNiagaraComponent* NiagaraComponent;

#if WITH_EDITORONLY_DATA
	// Reference to sprite visualization component
	UPROPERTY()
	class UBillboardComponent* SpriteComponent;

	// Reference to arrow visualization component
	UPROPERTY()
	class UArrowComponent* ArrowComponent;

#endif

public:
	/** Returns NiagaraComponent subobject **/
	FORCEINLINE class UNiagaraComponent* GetNiagaraComponent() const { return NiagaraComponent; }
#if WITH_EDITORONLY_DATA
	/** Returns SpriteComponent subobject **/
	FORCEINLINE class UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }
	/** Returns ArrowComponent subobject **/
	FORCEINLINE class UArrowComponent* GetArrowComponent() const { return ArrowComponent; }
#endif
};
