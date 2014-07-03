// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved. 
#include "ScriptPluginPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////

UScriptComponent::UScriptComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;
	bAutoActivate = true;
	bWantsInitializeComponent = true;

	Context = NULL;
}

void UScriptComponent::OnRegister()
{
	Super::OnRegister();

	UScriptBlueprintGeneratedClass* ScriptClass = NULL;
	for (UClass* MyClass = GetClass(); MyClass && !ScriptClass; MyClass = MyClass->GetSuperClass())
	{
		ScriptClass = Cast<UScriptBlueprintGeneratedClass>(MyClass);
	}

	if (ScriptClass && GetWorld() && GetWorld()->WorldType != EWorldType::Editor)
	{
		Context = ScriptClass->CreateContext();
		if (Context)
		{
			bool bDoNotTick = true;
			if (Context->Initialize(ScriptClass->SourceCode, this))
			{
				bDoNotTick = !Context->CanTick();
			}
			if (bDoNotTick)
			{
				bAutoActivate = false;
				PrimaryComponentTick.bCanEverTick = false;
			}
		}
	}
}

void UScriptComponent::InitializeComponent()
{
	Super::InitializeComponent();
	if (Context)
	{
		Context->BeginPlay();
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

bool UScriptComponent::CallScriptFunction(FString FunctionName)
{
	bool bSuccess = false;
	if (Context)
	{
		bSuccess = Context->CallFunction(FunctionName);
	}
	return bSuccess;
}
