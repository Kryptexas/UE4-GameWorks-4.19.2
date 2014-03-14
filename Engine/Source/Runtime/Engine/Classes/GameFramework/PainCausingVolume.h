// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// PhysicsVolume:  a bounding volume which affects actor physics
// Each  AActor  is affected at any time by one PhysicsVolume
//=============================================================================

#pragma once
#include "PainCausingVolume.generated.h"


UCLASS()
class ENGINE_API APainCausingVolume : public APhysicsVolume
{
	GENERATED_UCLASS_BODY()

	/** Whether volume currently causes damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PainCausingVolume)
	uint32 bPainCausing:1;

	/** Damage done per second to actors in this volume when bPainCausing=true */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PainCausingVolume)
	float DamagePerSec;

	/** Type of damage done */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PainCausingVolume)
	TSubclassOf<UDamageType> DamageType;

	/** If pain causing, time between damage applications. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PainCausingVolume)
	float PainInterval;

	/** if bPainCausing, cause pain when something enters the volume in addition to damage each second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PainCausingVolume)
	uint32 bEntryPain:1;

	/** Checkpointed bPainCausing value */
	UPROPERTY()
	uint32 BACKUP_bPainCausing:1;

	/** Controller that gets credit for any damage caused by this volume */
	UPROPERTY()
	class AController* DamageInstigator;

	/** Damage overlapping actors if pain-causing. */
	virtual void PainTimer();

#if WITH_EDITOR
	//Begin AVolume Interface
	virtual void CheckForErrors() OVERRIDE;
	//End AVolume Interface
#endif

	//Begin AActor Interface
	virtual void PostInitializeComponents() OVERRIDE;

	/* reset actor to initial state - used when restarting level without reloading. */
	virtual void Reset() OVERRIDE;
	//End AActor Interface

	//Being PhysicsVolume Interface
	/** If bEntryPain is true, call CausePainTo() on entering actor. */
	virtual void ActorEnteredVolume(class AActor* Other) OVERRIDE;
	//End PhysicsVolume Interface

	/** damage overlapping actors if pain causing. */
	virtual void CausePainTo(class AActor* Other);
};



