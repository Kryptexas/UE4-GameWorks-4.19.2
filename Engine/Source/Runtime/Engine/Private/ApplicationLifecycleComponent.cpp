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

	FCoreDelegates::ApplicationWillDeactivateDelegate.Add( FCoreDelegates::FApplicationLifetimeDelegate::FDelegate::CreateUObject(this, &UApplicationLifecycleComponent::ApplicationWillDeactivateDelegate_Handler) );
	FCoreDelegates::ApplicationHasReactivatedDelegate.Add( FCoreDelegates::FApplicationLifetimeDelegate::FDelegate::CreateUObject(this, &UApplicationLifecycleComponent::ApplicationHasReactivatedDelegate_Handler) );
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Add( FCoreDelegates::FApplicationLifetimeDelegate::FDelegate::CreateUObject(this, &UApplicationLifecycleComponent::ApplicationWillEnterBackgroundDelegate_Handler) );
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Add( FCoreDelegates::FApplicationLifetimeDelegate::FDelegate::CreateUObject(this, &UApplicationLifecycleComponent::ApplicationHasEnteredForegroundDelegate_Handler) );
	FCoreDelegates::ApplicationWillTerminateDelegate.Add( FCoreDelegates::FApplicationLifetimeDelegate::FDelegate::CreateUObject(this, &UApplicationLifecycleComponent::ApplicationWillTerminateDelegate_Handler) );
}

void UApplicationLifecycleComponent::OnUnregister()
{
	Super::OnUnregister();
	
 	FCoreDelegates::ApplicationWillDeactivateDelegate.RemoveUObject(this, &UApplicationLifecycleComponent::ApplicationWillDeactivateDelegate_Handler);
 	FCoreDelegates::ApplicationHasReactivatedDelegate.RemoveUObject(this, &UApplicationLifecycleComponent::ApplicationHasReactivatedDelegate_Handler);
 	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveUObject(this, &UApplicationLifecycleComponent::ApplicationWillEnterBackgroundDelegate_Handler);
 	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.RemoveUObject(this, &UApplicationLifecycleComponent::ApplicationHasEnteredForegroundDelegate_Handler);
 	FCoreDelegates::ApplicationWillTerminateDelegate.RemoveUObject(this, &UApplicationLifecycleComponent::ApplicationWillTerminateDelegate_Handler);
}
