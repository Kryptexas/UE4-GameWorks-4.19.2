// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Engine/EngineBaseTypes.h"
#include "UObject/GCObject.h"
#include "Editor/ViewportInteraction/ViewportInteractionInputProcessor.h"
#include "IViewportWorldInteractionManager.h"

class FViewportWorldInteractionManager: public FGCObject,  public IViewportWorldInteractionManager
{
public:
	FViewportWorldInteractionManager();
	virtual ~FViewportWorldInteractionManager();

	// FGCObject overrides
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;

	// IViewportWorldInteractionManager overrides
	virtual void SetCurrentViewportWorldInteraction( class UViewportWorldInteraction* WorldInteraction ) override;

	//
	// Input
	//

	/** Passes the input key to the current world interaction */
	bool PreprocessedInputKey( const FKey Key, const EInputEvent Event );

	/** Passes the input axis to the current world interaction */
	bool PreprocessedInputAxis( const int32 ControllerId, const FKey Key, const float Delta, const float DeltaTime );


private:

	/** The current world interaction that is ticking */
	class UViewportWorldInteraction* CurrentWorldInteraction;

	//
	// Input
	//

	/** Slate Input Processor */
	TSharedPtr<class FViewportInteractionInputProcessor> InputProcessor;
};
