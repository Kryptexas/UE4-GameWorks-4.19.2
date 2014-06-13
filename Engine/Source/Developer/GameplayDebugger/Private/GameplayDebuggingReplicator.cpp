// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayDebuggerPrivate.h"
#include "GameplayDebuggingComponent.h"
#include "GameplayDebuggingHUDComponent.h"
#include "GameplayDebuggingReplicator.h"
#include "BehaviorTreeDelegates.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif // WITH_EDITOR
#include "UnrealNetwork.h"

AGameplayDebuggingReplicator::AGameplayDebuggingReplicator(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, LastDrawAtFrame(0)
{
	TSubobjectPtr<USceneComponent> SceneComponent = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComponent"));
	RootComponent = SceneComponent;
#if WITH_EDITORONLY_DATA
	SetTickableWhenPaused(true);
	SetIsTemporarilyHiddenInEditor(true);
	SetActorHiddenInGame(false);
	bHiddenEdLevel = true;
	bHiddenEdLayer = true;
	bHiddenEd = true;
	bEditable = false;
#endif
}

void AGameplayDebuggingReplicator::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	DOREPLIFETIME(AGameplayDebuggingReplicator, DebugComponent);
#endif
}

bool AGameplayDebuggingReplicator::IsNetRelevantFor(class APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation)
{
	if (Clients.Num() == 0)
	{
		return true;// relevant to everyone for now
	}

	return Clients.Contains(FDebugContext(RealViewer));
}

void AGameplayDebuggingReplicator::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (!HasAnyFlags(RF_ClassDefaultObject) && GetWorld() && GetNetMode() != ENetMode::NM_DedicatedServer)
	{
		if (GIsEditor)
		{
			UDebugDrawService::Register(TEXT("DebugAI"), FDebugDrawDelegate::CreateUObject(this, &AGameplayDebuggingReplicator::OnDebugAIDelegate));
		}
		UDebugDrawService::Register(TEXT("Game"), FDebugDrawDelegate::CreateUObject(this, &AGameplayDebuggingReplicator::DrawDebugDataDelegate));

		if (!DebugComponentHUDClass.IsValid())
		{
			DebugComponentHUDClass = StaticLoadClass(AGameplayDebuggingHUDComponent::StaticClass(), NULL, *DebugComponentHUDClassName, NULL, LOAD_None, NULL);
			if (!DebugComponentHUDClass.IsValid())
			{
				DebugComponentHUDClass = AGameplayDebuggingHUDComponent::StaticClass();
			}
		}

		if (!DebugComponentClass.IsValid())
		{
			DebugComponentClass = StaticLoadClass(UGameplayDebuggingComponent::StaticClass(), NULL, *DebugComponentClassName, NULL, LOAD_None, NULL);
			if (!DebugComponentClass.IsValid())
			{
				DebugComponentClass = UGameplayDebuggingComponent::StaticClass();
			}
		}
	}

	SetActorTickEnabled(true);
}

void AGameplayDebuggingReplicator::BeginPlay()
{
	Super::BeginPlay();
	if (Role == ROLE_Authority)
	{
		bReplicates = false;
		SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
		SetReplicates(true);

		if (DebugComponent == NULL)
		{
			DebugComponent = ConstructObject<UGameplayDebuggingComponent>(DebugComponentClass.Get(), this);
			DebugComponent->SetIsReplicated(true);
			DebugComponent->RegisterComponent();
			DebugComponent->Activate();
		}
	}
}

class UNetConnection* AGameplayDebuggingReplicator::GetNetConnection()
{
	if (GetWorld() && GetWorld()->GetFirstPlayerController())
	{
		return GetWorld()->GetFirstPlayerController()->GetNetConnection();
	}

	return NULL;
}

bool AGameplayDebuggingReplicator::ServerEnableTargetSelection_Validate(bool, APlayerController* )
{
	return true;
}

void AGameplayDebuggingReplicator::ServerEnableTargetSelection_Implementation(bool bEnable, APlayerController* Context)
{
	if (DebugComponent)
	{
		uint32 Index = Clients.Find(FDebugContext(Context));
		if (Index != INDEX_NONE)
		{
			Clients[Index].bEnabledTargetSelection = bEnable;
		}

		DebugComponent->ServerEnableTargetSelection(true);
	}
}

bool AGameplayDebuggingReplicator::ClientReplicateMessage_Validate(class AActor* Actor, uint32 InMessage, uint32 DataView)
{
	return true;
}

void AGameplayDebuggingReplicator::ClientReplicateMessage_Implementation(class  AActor* Actor, uint32 InMessage, uint32 DataView)
{

}

bool AGameplayDebuggingReplicator::ServerReplicateMessage_Validate(class AActor* Actor, uint32 InMessage, uint32 DataView)
{
	return true;
}

void AGameplayDebuggingReplicator::ServerReplicateMessage_Implementation(class  AActor* Actor, uint32 InMessage, uint32 DataView)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (DebugComponent)
	{
		DebugComponent->ServerReplicateData((EDebugComponentMessage::Type)InMessage, (EAIDebugDrawDataView::Type)DataView);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

bool AGameplayDebuggingReplicator::ServerRegisterClient_Validate(APlayerController*)
{
	return true;
}

void AGameplayDebuggingReplicator::ServerRegisterClient_Implementation(APlayerController* NewClient)
{
	if (!Clients.Contains(FDebugContext(NewClient)))
	{
		Clients.AddUnique(FDebugContext(NewClient));
	}
}

bool AGameplayDebuggingReplicator::ServerUnregisterClient_Validate(APlayerController*)
{
	return true;
}

void AGameplayDebuggingReplicator::ServerUnregisterClient_Implementation(APlayerController* OldClient)
{
	if (Clients.Contains(FDebugContext(OldClient)))
	{
		Clients.Remove(FDebugContext(OldClient));
	}
}

bool AGameplayDebuggingReplicator::IsDrawEnabled()
{
	return bEnabledDraw && GetWorld() && GetNetMode() != ENetMode::NM_DedicatedServer;
}

void AGameplayDebuggingReplicator::EnableDraw(bool bEnable)
{
	bEnabledDraw = bEnable;

	if (AHUD* const GameHUD = LocalPlayerOwner.IsValid() ? LocalPlayerOwner->GetHUD() : NULL)
	{
		GameHUD->bShowHUD = bEnable ? false : true;
	}
	GEngine->bEnableOnScreenDebugMessages = bEnable ? false : true;
}

bool AGameplayDebuggingReplicator::IsToolCreated()
{
	UGameplayDebuggingControllerComponent*  GDC = FindComponentByClass<UGameplayDebuggingControllerComponent>();
	return LocalPlayerOwner.IsValid() && GDC;
}

void AGameplayDebuggingReplicator::CreateTool(APlayerController *PlayerController)
{
	if (GetWorld() && GetNetMode() != ENetMode::NM_DedicatedServer)
	{
		LocalPlayerOwner = PlayerController;
		ServerRegisterClient( PlayerController );
		UGameplayDebuggingControllerComponent*  GDC = FindComponentByClass<UGameplayDebuggingControllerComponent>();
		if (!GDC)
		{
			GDC = ConstructObject<UGameplayDebuggingControllerComponent>(UGameplayDebuggingControllerComponent::StaticClass(), this);
			GDC->SetPlayerOwner(PlayerController);
			GDC->RegisterComponent();
		}
	}
}

void AGameplayDebuggingReplicator::EnableTool()
{
	if (GetWorld() && GetNetMode() != ENetMode::NM_DedicatedServer)
	{
		UGameplayDebuggingControllerComponent*  GDC = FindComponentByClass<UGameplayDebuggingControllerComponent>();
		if (GDC)
		{
			// simulate key press
			GDC->OnActivationKeyPressed();
			GDC->OnActivationKeyReleased();
		}
	}
}

void AGameplayDebuggingReplicator::OnDebugAIDelegate(class UCanvas* Canvas, class APlayerController* PC)
{
#if WITH_EDITOR
	if (!GIsEditor)
	{
		return;
	}
	UEditorEngine* EEngine = Cast<UEditorEngine>(GEngine);
	if (GFrameNumber == LastDrawAtFrame || !EEngine || !EEngine->bIsSimulatingInEditor)
	{
		return;
	}
	LastDrawAtFrame = GFrameNumber;

	FEngineShowFlags EngineShowFlags = Canvas && Canvas->SceneView && Canvas->SceneView->Family ? Canvas->SceneView->Family->EngineShowFlags : FEngineShowFlags(GIsEditor ? EShowFlagInitMode::ESFIM_Editor : EShowFlagInitMode::ESFIM_Game);
	if (!EngineShowFlags.DebugAI)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World && DebugComponent && DebugComponent->GetOwnerRole() == ROLE_Authority)
	{
		bool bBroadcastedNewSelection = false;

		// looks like Simulate in UE4 Editor - let's find selected Pawn to debug
		for (FConstPawnIterator Iterator = World->GetPawnIterator(); Iterator; ++Iterator)
		{
			AActor* NewTarget = Cast<AActor>(*Iterator);

			//We needs to collect data manually in Simulate
			FDebugContext Context(LocalPlayerOwner.Get());
			Context.DebugTarget = NewTarget;
			DebugComponent->SetActorToDebug(NewTarget);
			DebugComponent->SelectForDebugging(NewTarget->IsSelected());
			DebugComponent->CollectDataToReplicate(NewTarget->IsSelected());

			if (NewTarget->IsSelected() && LastSelectedActorInSimulation.Get() != NewTarget)
			{
				LastSelectedActorInSimulation = NewTarget;
				UGameplayDebuggingComponent::OnDebuggingTargetChangedDelegate.Broadcast(NewTarget, NewTarget->IsSelected());
				APawn* TargetPawn = Cast<APawn>(NewTarget);
				if (TargetPawn)
				{
					FBehaviorTreeDelegates::OnDebugSelected.Broadcast(TargetPawn);
				}
				bBroadcastedNewSelection = true;
			}
			DrawDebugData(Canvas, PC);
		}

		if (!bBroadcastedNewSelection && LastSelectedActorInSimulation.IsValid() && !LastSelectedActorInSimulation->IsSelected() )
		{
			UGameplayDebuggingComponent::OnDebuggingTargetChangedDelegate.Broadcast(LastSelectedActorInSimulation.Get(), false);
			LastSelectedActorInSimulation = NULL;
		}
	}
#endif
}

void AGameplayDebuggingReplicator::DrawDebugDataDelegate(class UCanvas* Canvas, class APlayerController* PC)
{
#if !(UE_BUILD_SHIPPING && UE_BUILD_TEST)
	if (GetWorld() == NULL || IsPendingKill() || Canvas == NULL || Canvas->IsPendingKill())
	{
		return;
	}

	if (Canvas->SceneView != NULL && !Canvas->SceneView->bIsGameView)
	{
		return;
	}

	if (GFrameNumber == LastDrawAtFrame)
	{
		return;
	}
	LastDrawAtFrame = GFrameNumber;

	if (!IsDrawEnabled())
	{
		return;
	}

	if (GetWorld()->bPlayersOnly && Role == ROLE_Authority)
	{
		for (FConstPawnIterator Iterator = GetWorld()->GetPawnIterator(); Iterator; ++Iterator)
		{
			AActor* NewTarget = Cast<AActor>(*Iterator);
			FDebugContext Context(LocalPlayerOwner.Get());
			Context.DebugTarget = NewTarget;
			DebugComponent->SetActorToDebug(NewTarget);
			DebugComponent->CollectDataToReplicate(true);
		}
	}

	DrawDebugData(Canvas, PC);
}

void AGameplayDebuggingReplicator::DrawDebugData(class UCanvas* Canvas, class APlayerController* PC)
{
	if (!DebugRenderer.IsValid() && DebugComponentHUDClass.IsValid())
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = NULL;
		SpawnInfo.Instigator = NULL;
		SpawnInfo.bNoCollisionFail = true;

		DebugRenderer = GetWorld()->SpawnActor<AGameplayDebuggingHUDComponent>(DebugComponentHUDClass.Get(), SpawnInfo);
		DebugRenderer->SetCanvas(Canvas);
		DebugRenderer->SetWorld(GetWorld());
	}

	if (DebugRenderer != NULL)
	{
		DebugRenderer->SetCanvas(Canvas);
		DebugRenderer->Render();
	}

#endif
}
