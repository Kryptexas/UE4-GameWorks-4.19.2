// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ActorAnimationPrivatePCH.h"
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
		UpdateAnimationInstance();
	}
}

void AMovieSceneActor::Tick(float DeltaSeconds)
{
	if (AnimationPlayer)
	{
		AnimationPlayer->Update(DeltaSeconds);
	}
}

void AMovieSceneActor::SetAnimation(UActorAnimation* InAnimation)
{
	if (!AnimationPlayer || !AnimationPlayer->IsPlaying())
	{
		ActorAnimation = InAnimation;
		InitializePlayer();
	}
}

void AMovieSceneActor::InitializePlayer()
{
	UpdateAnimationInstance();

	if (GetWorld()->IsGameWorld())
	{
		AnimationPlayer = NewObject<UActorAnimationPlayer>(this, "AnimationPlayer");
		AnimationPlayer->Initialize(AnimationInstance, GetWorld(), PlaybackSettings);

		if (bAutoPlay)
		{
			AnimationPlayer->Play();
		}
	}
}

void AMovieSceneActor::UpdateAnimationInstance()
{
	UActorAnimation* ActorAnimationAsset = Cast<UActorAnimation>(ActorAnimation.TryLoad());
	if (!AnimationInstance)
	{
		// Make an instance of our asset so that we can keep hard references to actors we are using
		AnimationInstance = NewObject<UActorAnimationInstance>(this, "AnimationInstance");
	}

	AnimationInstance->Initialize(ActorAnimationAsset, GetWorld(), true);
}