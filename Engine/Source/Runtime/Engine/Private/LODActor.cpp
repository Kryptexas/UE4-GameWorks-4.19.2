// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LODActorBase.cpp: Static mesh actor base class implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "Engine/LODActor.h"
#include "MapErrors.h"
#include "MessageLog.h"
#include "UObjectToken.h"

#define LOCTEXT_NAMESPACE "LODActor"

ALODActor::ALODActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LODDrawDistance(5000)
{
	bCanBeDamaged = false;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent0"));
	StaticMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StaticMeshComponent->Mobility = EComponentMobility::Static;
	StaticMeshComponent->bGenerateOverlapEvents = false;
	StaticMeshComponent->bCastDynamicShadow = false;
	StaticMeshComponent->bCastStaticShadow = false;

	RootComponent = StaticMeshComponent;
}

FString ALODActor::GetDetailedInfoInternal() const
{
	return StaticMeshComponent ? StaticMeshComponent->GetDetailedInfoInternal() : TEXT("No_StaticMeshComponent");
}

void ALODActor::PostRegisterAllComponents() 
{
	Super::PostRegisterAllComponents();
#if WITH_EDITOR
	if (StaticMeshComponent->SceneProxy)
	{
		ensure (LODLevel >= 1);
		(StaticMeshComponent->SceneProxy)->SetHierarchicalLOD_GameThread(LODLevel);
	}
#endif
}
#if WITH_EDITOR

void ALODActor::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

bool ALODActor::GetReferencedContentObjects( TArray<UObject*>& Objects ) const
{
	Super::GetReferencedContentObjects(Objects);

	if (StaticMeshComponent && StaticMeshComponent->StaticMesh)
	{
		Objects.Add(StaticMeshComponent->StaticMesh);
	}
	return true;
}

void ALODActor::CheckForErrors()
{
	Super::CheckForErrors();

	FMessageLog MapCheck("MapCheck");

	if( !StaticMeshComponent )
	{
		MapCheck.Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT( "MapCheck_Message_StaticMeshComponent", "Static mesh actor has NULL StaticMeshComponent property - please delete" ) ))
			->AddToken(FMapErrorToken::Create(FMapErrors::StaticMeshComponent));
	}
	else if( StaticMeshComponent->StaticMesh == NULL )
	{
		MapCheck.Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT( "MapCheck_Message_StaticMeshNull", "Static mesh actor has NULL StaticMesh property" ) ))
			->AddToken(FMapErrorToken::Create(FMapErrors::StaticMeshNull));
	}

	// @todo error message when missing Actors
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE

