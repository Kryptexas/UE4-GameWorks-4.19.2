// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LevelSequencePCH.h"
#include "MovieSceneActor.h"


AMovieSceneActor::AMovieSceneActor(const FObjectInitializer& Init)
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

void AMovieSceneActor::BeginPlay()
{
	Super::BeginPlay();
	InitializePlayer();
}

void AMovieSceneActor::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// Make an instance of our asset so that we can keep hard references to actors we are using
		SequenceInstance = NewObject<ULevelSequenceInstance>(this, "AnimationInstance");
	}
}

void AMovieSceneActor::PostLoad()
{
	Super::PostLoad();
	
	UpdateAnimationInstance();
}

#if WITH_EDITOR
void AMovieSceneActor::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMovieSceneActor, LevelSequence))
	{
		UpdateAnimationInstance();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool AMovieSceneActor::GetReferencedContentObjects(TArray<UObject*>& Objects) const
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

void AMovieSceneActor::Tick(float DeltaSeconds)
{
	if (AnimationPlayer)
	{
		AnimationPlayer->Update(DeltaSeconds);
	}
}

void AMovieSceneActor::SetAnimation(ULevelSequence* InAnimation)
{
	if (!AnimationPlayer || !AnimationPlayer->IsPlaying())
	{
		LevelSequence = InAnimation;
		InitializePlayer();
	}
}

void AMovieSceneActor::InitializePlayer()
{
	UpdateAnimationInstance();

	if (GetWorld()->IsGameWorld())
	{
		AnimationPlayer = NewObject<ULevelSequencePlayer>(this, "AnimationPlayer");
		AnimationPlayer->Initialize(SequenceInstance, GetWorld(), PlaybackSettings);

		if (bAutoPlay)
		{
			AnimationPlayer->Play();
		}
	}
}

void AMovieSceneActor::UpdateAnimationInstance()
{
	ULevelSequence* LevelSequenceAsset = Cast<ULevelSequence>(LevelSequence.TryLoad());
	SequenceInstance->Initialize(LevelSequenceAsset, GetWorld(), true);
}