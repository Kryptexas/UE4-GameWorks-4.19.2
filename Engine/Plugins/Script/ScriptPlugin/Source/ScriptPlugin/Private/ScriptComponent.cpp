// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved. 
#include "ScriptPluginPrivatePCH.h"


//////////////////////////////////////////////////////////////////////////

UScriptComponent::UScriptComponent(const FPostConstructInitializeProperties& PCIP)
	: Super( PCIP )
{	
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	bAutoActivate = true;

	Context = NULL;
}


void UScriptComponent::OnRegister()
{
	Super::OnRegister();
	if (Script)
	{
		// @todo Create context here
#if WITH_LUA
		Context = new FLuaContext();
#endif
		if (Context)
		{
			bool bDoNotTick = true;
			if (Context->Initialize(this))
			{
				bDoNotTick = !Context->CanTick();
			}
			if (bDoNotTick)
			{
				bTickInEditor = false;
				bAutoActivate = false;
				PrimaryComponentTick.bCanEverTick = false;
			}
		}
	}
}

void UScriptComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Context)
	{
		Context->Tick(DeltaTime);
	}
};

void UScriptComponent::OnUnregister()
{
	if (Context)
	{
		Context->Destroy();
		delete Context;
		Context = NULL;
	}

	Super::OnUnregister();
}