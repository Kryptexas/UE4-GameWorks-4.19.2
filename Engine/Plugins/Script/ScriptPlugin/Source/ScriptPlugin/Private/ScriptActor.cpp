// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved. 
#include "ScriptPluginPrivatePCH.h"
#include "ScriptActor.h"


//////////////////////////////////////////////////////////////////////////

AScriptActor::AScriptActor(const FPostConstructInitializeProperties& PCIP)
: Super( PCIP )
{
	PrimaryActorTick.bCanEverTick = true;
}

void AScriptActor::PostLoad()
{
	Super::PostLoad();
}

void AScriptActor::BeginDestroy()
{
	if (ScriptContext)
	{
		ScriptContext->Destroy();
		delete ScriptContext;
		ScriptContext = NULL;
	}

	Super::BeginDestroy();
}

void AScriptActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!ScriptContext)
	{
		UScriptBlueprintGeneratedClass* ScriptClass = NULL;
		for (UClass* MyClass = GetClass(); MyClass && !ScriptClass; MyClass = MyClass->GetSuperClass())
		{
			ScriptClass = Cast<UScriptBlueprintGeneratedClass>(MyClass);
		}

		bool Result = true;
		if (ScriptClass)
		{
			ScriptContext = ScriptClass->CreateContext();
		}

		if (ScriptContext && GetWorld() && GetWorld()->WorldType != EWorldType::Editor)
		{
			if (!ScriptContext->Initialize(ScriptClass->SourceCode, this))
			{
				delete ScriptContext;
				ScriptContext = NULL;
			}
		}
	}
}

void AScriptActor::BeginPlay()
{
	Super::BeginPlay();

	if (ScriptContext && GetWorld() && GetWorld()->WorldType != EWorldType::Editor)
	{
		ScriptContext->BeginPlay();
		SetActorTickEnabled(ScriptContext->CanTick());
	}
}

void AScriptActor::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);
	if (ScriptContext && ScriptContext->CanTick())
	{
		ScriptContext->Tick(DeltaTime);
	}
}

void AScriptActor::ReceiveDestroyed()
{
	Super::ReceiveDestroyed();
}

void AScriptActor::ReceiveEndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::ReceiveEndPlay(EndPlayReason);
}

void AScriptActor::InvokeScriptFunction(FFrame& Stack, RESULT_DECL)
{
	check(ScriptContext);
	ScriptContext->InvokeScriptFunction(Stack, Result);
}