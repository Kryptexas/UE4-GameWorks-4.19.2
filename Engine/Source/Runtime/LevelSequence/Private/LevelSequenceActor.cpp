// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LevelSequencePCH.h"
#include "LevelSequenceActor.h"


ALevelSequenceActor::ALevelSequenceActor(const FObjectInitializer& Init)
	: Super(Init)
{
#if WITH_EDITORONLY_DATA
	UBillboardComponent* SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));

	if (!IsRunningCommandlet())
	{
		// Structure to hold one-time initialization
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> DecalTexture;
			FConstructorStatics() : DecalTexture(TEXT("/Engine/EditorResources/SceneManager")) {}
		};
		static FConstructorStatics ConstructorStatics;

		if (SpriteComponent)
		{
			SpriteComponent->Sprite = ConstructorStatics.DecalTexture.Get();
			SpriteComponent->AttachParent = RootComponent;
			SpriteComponent->bIsScreenSizeScaled = true;
			SpriteComponent->bAbsoluteScale = true;
			SpriteComponent->bReceivesDecals = false;
			SpriteComponent->bHiddenInGame = true;
			SetRootComponent(SpriteComponent);
		}
	}
#endif // WITH_EDITORONLY_DATA

	PrimaryActorTick.bCanEverTick = true;
	bAutoPlay = false;
}

void ALevelSequenceActor::BeginPlay()
{
	Super::BeginPlay();
	InitializePlayer();
}

void ALevelSequenceActor::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		CreateSequenceInstance();
	}
}

void ALevelSequenceActor::PostLoad()
{
	Super::PostLoad();
	
	UpdateAnimationInstance();
}

#if WITH_EDITOR
void ALevelSequenceActor::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ALevelSequenceActor, LevelSequence))
	{
		UpdateAnimationInstance();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool ALevelSequenceActor::GetReferencedContentObjects(TArray<UObject*>& Objects) const
{
	if (UObject* Asset = SequenceInstance ? SequenceInstance->GetLevelSequence() : nullptr)
	{
		// @todo: Enable editing of animation instances
		Objects.Add(Asset);
	}

	Super::GetReferencedContentObjects(Objects);

	return true;
}

#endif // WITH_EDITOR

void ALevelSequenceActor::Tick(float DeltaSeconds)
{
	if (SequencePlayer)
	{
		SequencePlayer->Update(DeltaSeconds);
	}
}

void ALevelSequenceActor::SetSequence(ULevelSequence* InSequence)
{
	if (!SequencePlayer || !SequencePlayer->IsPlaying())
	{
		LevelSequence = InSequence;
		InitializePlayer();
	}
}

void ALevelSequenceActor::InitializePlayer()
{
	UpdateAnimationInstance();

	if (GetWorld()->IsGameWorld())
	{
		SequencePlayer = NewObject<ULevelSequencePlayer>(this, "AnimationPlayer");
		SequencePlayer->Initialize(SequenceInstance, GetWorld(), PlaybackSettings);

		if (bAutoPlay)
		{
			SequencePlayer->Play();
		}
	}
}

void ALevelSequenceActor::CreateSequenceInstance()
{
	if (!SequenceInstance)
	{
		// Make an instance of our asset so that we can keep hard references to actors we are using
		SequenceInstance = NewObject<ULevelSequenceInstance>(this, "AnimationInstance");
	}
}

void ALevelSequenceActor::UpdateAnimationInstance()
{
	CreateSequenceInstance();
	
	ULevelSequence* LevelSequenceAsset = Cast<ULevelSequence>(LevelSequence.TryLoad());
	SequenceInstance->Initialize(LevelSequenceAsset, GetWorld(), true);
}