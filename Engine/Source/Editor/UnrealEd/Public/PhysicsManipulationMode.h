// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/** 
 * Editor mode to manipulate physics objects in level editor viewport simulation
 **/
class FPhysicsManipulationEdMode : public FEdMode
{
public:
	FPhysicsManipulationEdMode();
	~FPhysicsManipulationEdMode();

	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	virtual void Enter();
	virtual void Exit();
	
	virtual void PeekAtSelectionChangedEvent(UObject* ItemUndergoingChange);

	virtual bool InputDelta(FLevelEditorViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale);
	virtual bool StartTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport);
	virtual bool EndTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport);

	virtual bool ShouldDrawWidget() const		{ return true; };
	virtual bool UsesTransformWidget() const	{ return true; };

private:
	class UPhysicsHandleComponent* HandleComp;

	FVector HandleTargetLocation;
	FRotator HandleTargetRotation;

};