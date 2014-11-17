// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// ApplicationLifecycleComponent.cpp: Component to handle receiving notifications from the OS about application state (activated, suspended, termination, etc)

#include "EnginePrivate.h"
#include "CallbackDevice.h"

UApplicationLifecycleComponent::UApplicationLifecycleComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UApplicationLifecycleComponent::OnRegister()
{
	Super::OnRegister();

	FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &UApplicationLifecycleComponent::ApplicationWillDeactivateDelegate_Handler);
	FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(this, &UApplicationLifecycleComponent::ApplicationHasReactivatedDelegate_Handler);
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UApplicationLifecycleComponent::ApplicationWillEnterBackgroundDelegate_Handler);
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &UApplicationLifecycleComponent::ApplicationHasEnteredForegroundDelegate_Handler);
	FCoreDelegates::ApplicationWillTerminateDelegate.AddUObject(this, &UApplicationLifecycleComponent::ApplicationWillTerminateDelegate_Handler);
}

void UApplicationLifecycleComponent::OnUnregister()
{
	Super::OnUnregister();
	
 	FCoreDelegates::ApplicationWillDeactivateDelegate.RemoveAll(this);
 	FCoreDelegates::ApplicationHasReactivatedDelegate.RemoveAll(this);
 	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);
 	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.RemoveAll(this);
 	FCoreDelegates::ApplicationWillTerminateDelegate.RemoveAll(this);
}
