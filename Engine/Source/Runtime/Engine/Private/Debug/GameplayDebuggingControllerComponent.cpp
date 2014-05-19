// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
#include "AI/BehaviorTreeDelegates.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogGameplayDebugging, Log, All);

#define BUGIT_VIEWS (1<<EAIDebugDrawDataView::Basic) | (1 << EAIDebugDrawDataView::OverHead)
#define BREAK_LINE_TEXT TEXT("________________________________________________________________")

uint32 UGameplayDebuggingControllerComponent::StaticActiveViews = 0;
UGameplayDebuggingControllerComponent::UGameplayDebuggingControllerComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, DebugComponentKey(EKeys::Quote)
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	bAutoActivate = false;
	bTickInEditor=true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	ActiveViews = (1 << EAIDebugDrawDataView::Basic) | (1 << EAIDebugDrawDataView::OverHead);
	DebugComponentHUDClass = NULL;
	DebugAITargetActor = NULL;
	
	bDisplayAIDebugInfo = false;
	bWaitingForOwnersComponent = false;

	ControlKeyPressedTime = 0;
	bWasDisplayingInfo = false;
}

void UGameplayDebuggingControllerComponent::BeginDestroy()
{
	// Register for debug drawing
	UDebugDrawService::Unregister(FDebugDrawDelegate::CreateUObject(this, &UGameplayDebuggingControllerComponent::OnDebugAI));

	APlayerController* PC = Cast<APlayerController>(GetOuter());
	AHUD* GameHUD = PC ? PC->GetHUD() : NULL;
	if (GameHUD)
	{
		GameHUD->bShowHUD = true;
	}

	if (PC && PC->InputComponent)
	{
		for (int32 Index = PC->InputComponent->KeyBindings.Num() - 1; Index >= 0; --Index)
		{
			const FInputKeyBinding& KeyBind = PC->InputComponent->KeyBindings[Index];
			if (KeyBind.KeyDelegate.IsBoundToObject(this))
			{
				PC->InputComponent->KeyBindings.RemoveAtSwap(Index);
			}
		}
	}

	if(PC)
	{
		APawn* Pawn = PC->GetPawn();
		if (Pawn)
		{
			PC->ServerReplicateMessageToAIDebugView(Pawn, EDebugComponentMessage::DeactivateReplilcation, 0);
		}

		if (DebugAITargetActor != NULL)
		{
			if (DebugAITargetActor->GetDebugComponent(false))
			{
				DebugAITargetActor->GetDebugComponent(false)->EnableDebugDraw(false);
			}
			if (PC)
			{
				for (uint32 Index = 0; Index < EAIDebugDrawDataView::MAX; ++Index)
				{
					PC->ServerReplicateMessageToAIDebugView(DebugAITargetActor, EDebugComponentMessage::DeactivateDataView, (EAIDebugDrawDataView::Type)Index);
				}
				PC->ServerReplicateMessageToAIDebugView(DebugAITargetActor, EDebugComponentMessage::DisableExtendedView);
			}
		}
	}

	Super::BeginDestroy();
}

void UGameplayDebuggingControllerComponent::InitializeComponent()
{
	Super::InitializeComponent();
	InitBasicFuncionality();
}

void UGameplayDebuggingControllerComponent::InitBasicFuncionality()
{
	if (DebugComponentHUDClass == NULL)
	{
		DebugComponentHUDClass = StaticLoadClass(AHUD::StaticClass(), NULL, *DebugComponentHUDClassName, NULL, LOAD_None, NULL);
	}

	BindActivationKeys();

	// Register for debug drawing
	UDebugDrawService::Register(TEXT("DebugAI"), FDebugDrawDelegate::CreateUObject(this, &UGameplayDebuggingControllerComponent::OnDebugAI));
}

bool UGameplayDebuggingControllerComponent::ToggleStaticView(EAIDebugDrawDataView::Type View)
{
	StaticActiveViews ^= 1 << View;
	const bool bIsActive = (StaticActiveViews & (1 << View)) != 0;

#if WITH_EDITOR
	if (View == EAIDebugDrawDataView::EQS && GCurrentLevelEditingViewportClient)
	{
		GCurrentLevelEditingViewportClient->EngineShowFlags.SetSingleFlag(UGameplayDebuggingComponent::ShowFlagIndex, bIsActive);
	}
#endif

	return bIsActive;
}

uint32 UGameplayDebuggingControllerComponent::GetActiveViews()
{
	return ActiveViews;
}

void UGameplayDebuggingControllerComponent::CycleActiveView()
{
	int32 Index = 0;
	for (Index = 0; Index < 32; ++Index)
	{
		if (ActiveViews & (1 << Index))
			break;
	}
	if (++Index >= EAIDebugDrawDataView::EditorDebugAIFlag)
	{
		Index = EAIDebugDrawDataView::Basic;
	}

	ActiveViews = (1 << EAIDebugDrawDataView::OverHead) | (1 << Index);
}

void UGameplayDebuggingControllerComponent::SetActiveViews(uint32 InActiveViews)
{
	ActiveViews = InActiveViews;

	if (DebugAITargetActor)
	{
		APlayerController* MyPC = Cast<APlayerController>(GetOuter());
		for (uint32 Index = 0; Index < EAIDebugDrawDataView::MAX; ++Index)
		{
			MyPC->ServerReplicateMessageToAIDebugView(DebugAITargetActor, IsViewActive((EAIDebugDrawDataView::Type)Index) ? EDebugComponentMessage::ActivateDataView : EDebugComponentMessage::DeactivateDataView, (EAIDebugDrawDataView::Type)Index);
		}
	}
}

void UGameplayDebuggingControllerComponent::SetActiveView(EAIDebugDrawDataView::Type NewView)
{
	ActiveViews = 1 << NewView;
	APlayerController* MyPC = Cast<APlayerController>(GetOuter());
	if (DebugAITargetActor)
	{
		MyPC->ServerReplicateMessageToAIDebugView(DebugAITargetActor, EDebugComponentMessage::ActivateDataView, NewView);
	}
}

void UGameplayDebuggingControllerComponent::EnableActiveView(EAIDebugDrawDataView::Type View, bool bEnable)
{
	ActiveViews = bEnable ? ActiveViews | (1 << View) : ActiveViews & ~(1 << View);

	APlayerController* MyPC = Cast<APlayerController>(GetOuter());
	if (DebugAITargetActor)
	{
		MyPC->ServerReplicateMessageToAIDebugView(DebugAITargetActor, bEnable ? EDebugComponentMessage::ActivateDataView : EDebugComponentMessage::DeactivateDataView, View);
	}
}

void UGameplayDebuggingControllerComponent::ToggleActiveView(EAIDebugDrawDataView::Type NewView)
{
	ActiveViews ^= 1 << NewView;

	APawn* MyPawn = Cast<APawn>(GetOwner());
	APlayerController* MyPC = Cast<APlayerController>(GetOuter());
	if (DebugAITargetActor)
	{
		MyPC->ServerReplicateMessageToAIDebugView(DebugAITargetActor, (ActiveViews & (1 << NewView)) ? EDebugComponentMessage::ActivateDataView : EDebugComponentMessage::DeactivateDataView, NewView);
		if (DebugAITargetActor->GetDebugComponent() && NewView == EAIDebugDrawDataView::EQS)
		{
			DebugAITargetActor->GetDebugComponent()->EnableClientEQSSceneProxy(IsViewActive(EAIDebugDrawDataView::EQS));
		}
	}
}

void UGameplayDebuggingControllerComponent::BindActivationKeys()
{
	APlayerController* PC = Cast<APlayerController>(GetOuter());

	if (PC && PC->InputComponent)
	{
		PC->InputComponent->BindKey(FInputChord(DebugComponentKey, false, false, false), IE_Pressed, this, &UGameplayDebuggingControllerComponent::OnControlPressed);
		PC->InputComponent->BindKey(FInputChord(DebugComponentKey, false, false, false), IE_Released, this, &UGameplayDebuggingControllerComponent::OnControlReleased);
	}
}

void UGameplayDebuggingControllerComponent::OnControlPressed()
{
	ControlKeyPressedTime = GetWorld()->GetTimeSeconds();
	bWasDisplayingInfo = bDisplayAIDebugInfo;
	StartAIDebugView();
}

void UGameplayDebuggingControllerComponent::OnControlReleased()
{
	const float DownTime = GetWorld()->GetTimeSeconds() - ControlKeyPressedTime;
	if (DownTime < 0.5f && bWasDisplayingInfo)
	{
		EndAIDebugView();
	}
	else
	{
		LockAIDebugView();
	}
}

void UGameplayDebuggingControllerComponent::OnDebugAI(class UCanvas* Canvas, class APlayerController* PC)
{
#if !(UE_BUILD_SHIPPING && UE_BUILD_TEST)
	if (World == NULL || IsPendingKill() || Canvas == NULL || Canvas->IsPendingKill())
	{
		return;
	}

	if (Canvas->SceneView != NULL && !Canvas->SceneView->bIsGameView)
	{
		return;
	}

	APlayerController* MyPC = Cast<APlayerController>(GetOuter());
	if (MyPC == NULL || MyPC->IsPendingKill() )
	{
		return;
	}

	for (FConstPawnIterator Iterator = World->GetPawnIterator(); Iterator; ++Iterator )
	{
		APawn *Pawn = *Iterator;
		if (Pawn == NULL || Pawn->IsPendingKill())
		{
			continue;
		}

		UGameplayDebuggingComponent* DebuggingComponent = Pawn->GetDebugComponent(false);
		if (DebuggingComponent == NULL && GetNetMode() != NM_Client && Canvas->SceneView->Family->EngineShowFlags.DebugAI && !Pawn->IsPendingKill())
		{
			//it's probably Simulate so we can safely create component here
			DebuggingComponent = Pawn->GetDebugComponent(true);
		}

		if (DebuggingComponent)
		{
			if (DebuggingComponent->IsPendingKill() == false)
			{
				DebuggingComponent->OnDebugAI(Canvas, MyPC);
			}
		}
	}

	if (OnDebugAIHUD == NULL && MyPC != NULL && DebugComponentHUDClass != NULL)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = MyPC;
		SpawnInfo.Instigator = MyPC->GetPawnOrSpectator();
		SpawnInfo.bNoCollisionFail = true;

		OnDebugAIHUD = GetWorld()->SpawnActor<AHUD>( DebugComponentHUDClass, SpawnInfo );
		OnDebugAIHUD->SetCanvas(Canvas, Canvas);

	}
	
	if (OnDebugAIHUD != NULL && OnDebugAIHUD->PlayerOwner)
	{
		OnDebugAIHUD->SetCanvas(Canvas, Canvas);
		OnDebugAIHUD->PostRender();
	}
#endif
}

void UGameplayDebuggingControllerComponent::StartAIDebugView()
{
	BeginAIDebugView();
}

void UGameplayDebuggingControllerComponent::BeginAIDebugView()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APlayerController* PC = Cast<APlayerController>(GetOuter());
	APawn* Pawn = PC ? PC->GetPawn() : NULL;

	if (!World || !PC || !Pawn)
	{
		return;
	}

	Activate();
	SetComponentTickEnabled(true);
	
	UDebugDrawService::Unregister(FDebugDrawDelegate::CreateUObject(this, &UGameplayDebuggingControllerComponent::OnDebugAI));
	UDebugDrawService::Register(TEXT("Game"), FDebugDrawDelegate::CreateUObject(this, &UGameplayDebuggingControllerComponent::OnDebugAI));

	AHUD* GameHUD = PC->GetHUD();
	if (GameHUD)
	{
		GameHUD->bShowHUD = false;
	}

	bWaitingForOwnersComponent = false;
	UGameplayDebuggingComponent* OwnerComp = Pawn->GetDebugComponent(GetNetMode() < NM_Client  ? true : false);
	if (!OwnerComp)
	{
		UE_VLOG(GetOwner(), LogGDT, Log, TEXT("UGameplayDebuggingControllerComponent: Requesting server to create player's component"));
		PC->ServerReplicateMessageToAIDebugView(Pawn, EDebugComponentMessage::ActivateReplication, 0); //create component for my player's pawn, to replicate non pawn's related data
		OwnerComp = Pawn->GetDebugComponent(false);
		if (!OwnerComp)
		{
			PlayersComponentRequestTime = GetWorld()->TimeSeconds;
			bWaitingForOwnersComponent = true;
			return;
		}
	}

	OwnerComp->ServerEnableTargetSelection(true);
	SetGameplayDebugFlag(true, PC);

	if (DebugAITargetActor != NULL && DebugAITargetActor->GetDebugComponent() == NULL)
	{
		DebugAITargetActor = NULL;
	}

	GEngine->bEnableOnScreenDebugMessages = false;

	if (!bDisplayAIDebugInfo)
	{
		bDisplayAIDebugInfo = true;
		
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void UGameplayDebuggingControllerComponent::LockAIDebugView()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APlayerController* MyPC = Cast<APlayerController>(GetOuter());
	APawn* Pawn = MyPC ? MyPC->GetPawn() : NULL;
	if (Pawn)
	{
		if (UGameplayDebuggingComponent* OwnerComp = Pawn->GetDebugComponent(false))
		{
			OwnerComp->ServerEnableTargetSelection(false);
		}
	}

	BindAIDebugViewKeys();

	FBehaviorTreeDelegates::OnDebugLocked.Broadcast(DebugAITargetActor);
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void UGameplayDebuggingControllerComponent::EndAIDebugView()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// disable debug flag

	APlayerController* MyPC = Cast<APlayerController>(GetOuter());

	SetGameplayDebugFlag(false, MyPC);

	if (AIDebugViewInputComponent && MyPC)
	{
		AIDebugViewInputComponent->UnregisterComponent();
		MyPC->PopInputComponent(AIDebugViewInputComponent);
	}
	AIDebugViewInputComponent = NULL;

	APawn* Pawn = MyPC ? MyPC->GetPawn() : NULL;
	if (Pawn)
	{
		if (UGameplayDebuggingComponent* OwnerComp = Pawn->GetDebugComponent(false))
		{
			if (!OwnerComp->IsPendingKill())
			{
				OwnerComp->ServerEnableTargetSelection(false);
				if (IsViewActive(EAIDebugDrawDataView::NavMesh))
				{
					ToggleAIDebugView_SetView0();
				}
			}
		}
	}

	if (DebugAITargetActor != NULL)
	{
		if (DebugAITargetActor->GetDebugComponent(false))
		{
			DebugAITargetActor->GetDebugComponent(false)->EnableDebugDraw(false);
		}
		if (MyPC)
		{
			for (uint32 Index = 0; Index < EAIDebugDrawDataView::MAX; ++Index)
			{
				MyPC->ServerReplicateMessageToAIDebugView(DebugAITargetActor, EDebugComponentMessage::DeactivateDataView, (EAIDebugDrawDataView::Type)Index);
			}
			MyPC->ServerReplicateMessageToAIDebugView(DebugAITargetActor, EDebugComponentMessage::DisableExtendedView);
		}
	}

	if (MyPC)
	{
		UWorld* World = MyPC->GetWorld();
		for (FConstPawnIterator Iterator = World->GetPawnIterator(); Iterator; ++Iterator )
		{
			APawn* NewTarget = *Iterator;
			if (NewTarget && NewTarget != MyPC->GetPawn())
			{
				MyPC->ServerReplicateMessageToAIDebugView(NewTarget, EDebugComponentMessage::DeactivateReplilcation, 1 << EAIDebugDrawDataView::Empty);
			}
		}
	}

	SetComponentTickEnabled(false);
	Deactivate();

	if (MyPC)
	{
		if (AHUD* GameHUD = MyPC->GetHUD())
		{
			GameHUD->bShowHUD = true;
		}
	}
	UDebugDrawService::Unregister(FDebugDrawDelegate::CreateUObject(this, &UGameplayDebuggingControllerComponent::OnDebugAI));
	UDebugDrawService::Register(TEXT("DebugAI"), FDebugDrawDelegate::CreateUObject(this, &UGameplayDebuggingControllerComponent::OnDebugAI));

	DebugAITargetActor = NULL;
	bDisplayAIDebugInfo = false;

	GEngine->bEnableOnScreenDebugMessages = true;
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void UGameplayDebuggingControllerComponent::SetGameplayDebugFlag(bool bNewValue, APlayerController* PlayerController)
{
	if (PlayerController == NULL)
	{
		PlayerController = Cast<APlayerController>(GetOwner());
	}

	if (PlayerController && Cast<ULocalPlayer>(PlayerController->Player) && ((ULocalPlayer*)(PlayerController->Player))->ViewportClient && UGameplayDebuggingComponent::ShowFlagIndex != INDEX_NONE)
	{
		((ULocalPlayer*)(PlayerController->Player))->ViewportClient->EngineShowFlags.SetSingleFlag(UGameplayDebuggingComponent::ShowFlagIndex, bNewValue);
	}
}

void UGameplayDebuggingControllerComponent::ToggleAIDebugView()
{
	if (!bDisplayAIDebugInfo)
	{
		StartAIDebugView();
		LockAIDebugView();
	}
	else
	{
		EndAIDebugView();
	}
}

void UGameplayDebuggingControllerComponent::BindAIDebugViewKeys()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!AIDebugViewInputComponent)
	{
		AIDebugViewInputComponent = ConstructObject<UInputComponent>(UInputComponent::StaticClass(), GetOwner(), TEXT("AIDebugViewInputComponent0"));
		AIDebugViewInputComponent->RegisterComponent();

		AIDebugViewInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_CycleView);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadZero, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView0);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView1);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView2);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView3);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadFour, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView4);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadFive, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView5);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadSix, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView6);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadSeven, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView7);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadEight, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView8);
		AIDebugViewInputComponent->BindKey(EKeys::NumPadNine, IE_Pressed, this, &UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView9);
	}
	APlayerController* PC = Cast<APlayerController>(GetOuter());
	PC->PushInputComponent(AIDebugViewInputComponent);
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

bool UGameplayDebuggingControllerComponent::CanToggleView()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APlayerController* MyPC = Cast<APlayerController>(GetOuter());
	APawn* Pawn = MyPC->GetPawn();
	return  (MyPC && bDisplayAIDebugInfo && DebugAITargetActor);
#else
	return false;
#endif
}

void UGameplayDebuggingControllerComponent::ToggleAIDebugView_CycleView()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CanToggleView())
	{
		CycleActiveView();
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void UGameplayDebuggingControllerComponent::ToggleAIDebugView_SetView0()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APlayerController* MyPC = Cast<APlayerController>(GetOuter());
	APawn* Pawn = MyPC ? MyPC->GetPawn() : NULL;

	if (MyPC && Pawn && bDisplayAIDebugInfo)
	{
		if (UGameplayDebuggingComponent* OwnerComp = Pawn->GetDebugComponent(false))
		{
			ActiveViews ^= 1 << EAIDebugDrawDataView::NavMesh;

			if (IsViewActive(EAIDebugDrawDataView::NavMesh))
			{
				GetWorld()->GetTimerManager().SetTimer(this, &UGameplayDebuggingControllerComponent::UpdateNavMeshTimer, 5.0f, true);
				UpdateNavMeshTimer();

				MyPC->ServerReplicateMessageToAIDebugView(Pawn, EDebugComponentMessage::ActivateDataView, EAIDebugDrawDataView::NavMesh);
				MyPC->ServerReplicateMessageToAIDebugView(Pawn, EDebugComponentMessage::ActivateReplication, EAIDebugDrawDataView::Empty);
				OwnerComp->SetVisibility(true, true);
				OwnerComp->SetHiddenInGame(false, true);
				OwnerComp->MarkRenderStateDirty();
			}
			else
			{
				GetWorld()->GetTimerManager().ClearTimer(this, &UGameplayDebuggingControllerComponent::UpdateNavMeshTimer);

				MyPC->ServerReplicateMessageToAIDebugView(Pawn, EDebugComponentMessage::DeactivateDataView, EAIDebugDrawDataView::NavMesh);
				MyPC->ServerReplicateMessageToAIDebugView(Pawn, EDebugComponentMessage::DeactivateReplilcation, EAIDebugDrawDataView::Empty);
				OwnerComp->ServerDiscardNavmeshData();
				OwnerComp->SetVisibility(false, true);
				OwnerComp->SetHiddenInGame(true, true);
				OwnerComp->MarkRenderStateDirty();
			}
		}
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#	define DEFAULT_TOGGLE_HANDLER(__func_name__, __DataView__) \
	void UGameplayDebuggingControllerComponent::__func_name__() \
	{ \
		if (CanToggleView()) \
		{ \
			ToggleActiveView(__DataView__); \
		} \
	}

#else
#	define DEFAULT_TOGGLE_HANDLER(__func_name__, __DataView__) \
	void UGameplayDebuggingControllerComponent::__func_name__() { }
#endif 

DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView1, EAIDebugDrawDataView::Basic);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView2, EAIDebugDrawDataView::BehaviorTree);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView3, EAIDebugDrawDataView::EQS);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView4, EAIDebugDrawDataView::Perception);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView5, EAIDebugDrawDataView::GameView1);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView6, EAIDebugDrawDataView::GameView2);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView7, EAIDebugDrawDataView::GameView3);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView8, EAIDebugDrawDataView::GameView4);
DEFAULT_TOGGLE_HANDLER(ToggleAIDebugView_SetView9, EAIDebugDrawDataView::GameView5);

#ifdef DEFAULT_TOGGLE_HANDLER
# undef DEFAULT_TOGGLE_HANDLER
#endif

APawn* UGameplayDebuggingControllerComponent::GetCurrentDebugTarget() const
{
	return DebugAITargetActor;
}

void UGameplayDebuggingControllerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APlayerController* MyPC = Cast<APlayerController>(GetOuter());
	APawn* MyPawn = MyPC ? MyPC->GetPawn() : NULL;

	UGameplayDebuggingComponent* OwnerComp = MyPawn ? MyPawn->GetDebugComponent(false) : NULL;
	if (MyPC && MyPawn && !OwnerComp && bWaitingForOwnersComponent && GetWorld()->TimeSeconds - PlayersComponentRequestTime > 5)
	{
		// repeat replication request from server
		UE_VLOG(GetOwner(), LogGDT, Warning, TEXT("UGameplayDebuggingControllerComponent: Requesting server to create again player's component after 5 seconds!"));
		MyPC->ServerReplicateMessageToAIDebugView(MyPawn, EDebugComponentMessage::ActivateReplication, 0); //create component for my player's pawn, to replicate non pawn's related data
		PlayersComponentRequestTime = GetWorld()->TimeSeconds;
	}

	if (MyPC && OwnerComp && bDisplayAIDebugInfo)
	{
		if(bWaitingForOwnersComponent)
		{
			BeginAIDebugView();
			LockAIDebugView();
		}

		if (DebugAITargetActor != OwnerComp->TargetActor)
		{
			FBehaviorTreeDelegates::OnDebugSelected.Broadcast(OwnerComp->TargetActor);
			if (DebugAITargetActor != NULL)
			{
				for (uint32 Index = 0; Index < EAIDebugDrawDataView::MAX; ++Index)
				{
					MyPC->ServerReplicateMessageToAIDebugView(DebugAITargetActor, EDebugComponentMessage::DeactivateDataView, (EAIDebugDrawDataView::Type)Index);
				}

				if (DebugAITargetActor->GetDebugComponent(false))
				{
					DebugAITargetActor->GetDebugComponent(false)->EnableDebugDraw(true, false);
				}
				MyPC->ServerReplicateMessageToAIDebugView(DebugAITargetActor, EDebugComponentMessage::DisableExtendedView);
			}

			if (OwnerComp->TargetActor != NULL && OwnerComp->TargetActor->GetDebugComponent(false))
			{
				DebugAITargetActor = OwnerComp->TargetActor;
				SetActiveViews(GetActiveViews()); //update replication flags for new target
				DebugAITargetActor->GetDebugComponent(false)->EnableDebugDraw(true, true);
				MyPC->ServerReplicateMessageToAIDebugView(DebugAITargetActor, EDebugComponentMessage::EnableExtendedView);
			}
		}
	}

	if (GetWorld()->bPlayersOnly && GetOwnerRole() == ROLE_Authority)
	{
		for (FConstPawnIterator Iterator = World->GetPawnIterator(); Iterator; ++Iterator )
		{
			APawn* NewTarget = *Iterator;
			if (UGameplayDebuggingComponent* DebuggingComponent = NewTarget->GetDebugComponent(false))
			{
				DebuggingComponent->CollectDataToReplicate();
			}
		}
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
}

void UGameplayDebuggingControllerComponent::UpdateNavMeshTimer()
{
	APlayerController* MyPC = Cast<APlayerController>(GetOuter());
	APawn* Pawn = MyPC ? MyPC->GetPawn() : NULL;
	UGameplayDebuggingComponent* OwnerComp = Pawn ? Pawn->GetDebugComponent(false) : NULL;
	if (OwnerComp)
	{
		const FVector AdditionalTargetLoc =
			DebugAITargetActor ? DebugAITargetActor->GetActorLocation() :
			Pawn ? Pawn->GetActorLocation() : 
			FVector::ZeroVector;

		if (AdditionalTargetLoc != FVector::ZeroVector)
		{
			OwnerComp->ServerCollectNavmeshData(AdditionalTargetLoc);
		}
	}
}
