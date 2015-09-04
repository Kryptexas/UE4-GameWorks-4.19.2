// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ClothManager.h
	In charge of passing data between skeletal meshes and cloth solver
=============================================================================*/

#pragma once

class FPhysScene;
class USkeletalMeshComponent;

struct FClothManagerTickFunction : public FTickFunction
{
	class FClothManager* Target;

	// FTickFunction interface
	virtual void ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
	virtual FString DiagnosticMessage() override;
	// End of FTickFunction interface
};


class FClothManager
{
public:

	FClothManager(UWorld* AssociatedWorld, FPhysScene* InPhysScene);

	/** Go through all SkeletalMeshComponents registered for work this frame and call the solver as needed*/
	void Tick(float DeltaTime);

	/** Register the SkeletalMeshComponent so that it does cloth solving work this frame */
	void RegisterForClothThisFrame(USkeletalMeshComponent* SkeletalMeshComponent);

	/** Whether the cloth manager is currently doing work in an async thread */
	bool IsPreparingClothAsync() const { return IsPreparingCloth; }
	
private:
	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	FClothManagerTickFunction ClothManagerTickFunction;
	FPhysScene* PhysScene;

	FThreadSafeBool IsPreparingCloth;
};