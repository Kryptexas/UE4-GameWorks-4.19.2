// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#endif

//----------------------------------------------------------------------//
// AEQSTestingPawn
//----------------------------------------------------------------------//
AEQSTestingPawn::AEQSTestingPawn(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, TimeLimitPerStep(-1)
	, StepToDebugDraw(0)
	, bDrawLabels(true)
	, bDrawFailedItems(true)
	, bReRunQueryOnlyOnFinishedMove(true)
{
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> TextureObject;
		FName ID_Misc;
		FText NAME_Misc;
		FConstructorStatics()
			: TextureObject(TEXT("/Engine/EditorResources/S_Pawn"))
			, ID_Misc(TEXT("Misc"))
			, NAME_Misc(NSLOCTEXT( "SpriteCategory", "Misc", "Misc" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	static FName CollisionProfileName(TEXT("NoCollision"));
	CapsuleComponent->SetCollisionProfileName(CollisionProfileName);

#if WITH_EDITORONLY_DATA
	UArrowComponent* ArrowComponent = FindComponentByClass<UArrowComponent>();
	if (ArrowComponent != NULL)
	{
		ArrowComponent->SetRelativeScale3D(FVector(2,2,2));
	}

	TSubobjectPtr<UBillboardComponent> SpriteComponent = PCIP.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.TextureObject.Get();
		SpriteComponent->bHiddenInGame = true;
		//SpriteComponent->Mobility = EComponentMobility::Static;
#if WITH_EDITORONLY_DATA
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Misc;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Misc;
#endif // WITH_EDITORONLY_DATA
		SpriteComponent->AttachParent = RootComponent;
		SpriteComponent->SetRelativeScale3D(FVector(6,6,6));
	}

	EdRenderComp = PCIP.CreateEditorOnlyDefaultSubobject<UEQSRenderingComponent>(this, TEXT("EQSRender"));
#endif // WITH_EDITORONLY_DATA

	// Default to no tick function, but if we set 'never ticks' to false (so there is a tick function) it is enabled by default
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

#if WITH_EDITOR
	if (HasAnyFlags(RF_ClassDefaultObject) && GetClass() == StaticClass())
	{
		USelection::SelectObjectEvent.AddStatic(&AEQSTestingPawn::OnEditorSelectionChanged);
	}
#endif // WITH_EDITOR
}

void AEQSTestingPawn::OnEditorSelectionChanged(UObject* NewSelection)
{
	bool bEQSPawnSelected = Cast<AEQSTestingPawn>(NewSelection) != NULL;
	if (bEQSPawnSelected == false)
	{
		USelection* Selection = Cast<USelection>(NewSelection);
		if (Selection != NULL)
		{
			TArray<AEQSTestingPawn*> SelectedPawns;
			Selection->GetSelectedObjects<AEQSTestingPawn>(SelectedPawns);
			bEQSPawnSelected = SelectedPawns.Num() > 0;
		}
	}

#if WITH_EDITOR
	if (GCurrentLevelEditingViewportClient != NULL && UGameplayDebuggingComponent::ShowFlagIndex != INDEX_NONE)
	{
		GCurrentLevelEditingViewportClient->EngineShowFlags.SetSingleFlag(UGameplayDebuggingComponent::ShowFlagIndex, bEQSPawnSelected);
	}
#endif // WITH_EDITOR
}

void AEQSTestingPawn::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::Tick(DeltaTime);

	if (QueryInstance.IsValid() && QueryInstance->Status == EEnvQueryStatus::Processing)
	{
		MakeOneStep();
	}
}

void AEQSTestingPawn::PostLoad() 
{
	Super::PostLoad();

	UBillboardComponent* SpriteComponent = FindComponentByClass<UBillboardComponent>();
	if (SpriteComponent != NULL)
	{
		SpriteComponent->bHiddenInGame = !bShouldBeVisibleInGame;
	}

	UEnvQueryManager* EQS = GetWorld() ? GetWorld()->GetEnvironmentQueryManager() : NULL;
	if (EQS)
	{
		RunEQSQuery();
	}
	else
	{
		QueryTemplate = NULL;
		QueryParams.Reset();
	}
}

void AEQSTestingPawn::RunEQSQuery()
{
	Reset();
	
	// make one step if TimeLimitPerStep > 0.f, else all steps
	do
	{
		MakeOneStep();
	} while (TimeLimitPerStep <= 0.f && QueryInstance.IsValid() == true && QueryInstance->Status == EEnvQueryStatus::Processing);	
}

void AEQSTestingPawn::Reset()
{
	QueryInstance = NULL;
	StepToDebugDraw = 0;
	StepResults.Reset();

	if (Controller == NULL)
	{
		SpawnDefaultController();
	}
}

void AEQSTestingPawn::MakeOneStep()
{
	UEnvQueryManager* EQS = GetWorld()->GetEnvironmentQueryManager();
	if (EQS == NULL)
	{
		return;
	}

	if (QueryInstance.IsValid() == false && QueryTemplate != NULL)
	{
		FEnvQueryRequest QueryRequest(QueryTemplate, this);
		QueryRequest.SetNamedParams(QueryParams);
		QueryInstance = EQS->PrepareQueryInstance(QueryRequest, EEnvQueryRunMode::AllMatching);
	}

	// possible still not valid 
	if (QueryInstance.IsValid() == true && QueryInstance->Status == EEnvQueryStatus::Processing)
	{
		QueryInstance->ExecuteOneStep(double(TimeLimitPerStep));
		StepResults.Add(*(QueryInstance.Get()));

		if (QueryInstance->Status != EEnvQueryStatus::Processing)
		{
			StepToDebugDraw = StepResults.Num() - 1;
			UpdateDrawing();
		}
	}
}

void AEQSTestingPawn::UpdateDrawing()
{
#if WITH_EDITORONLY_DATA

	UBillboardComponent* SpriteComponent = FindComponentByClass<UBillboardComponent>();
	if (SpriteComponent != NULL)
	{
		SpriteComponent->MarkRenderStateDirty();
	}	

	if (EdRenderComp != NULL && EdRenderComp->bVisible)
	{
		EdRenderComp->MarkRenderStateDirty();

#if WITH_EDITOR
		if (GEditor != NULL)
		{
			GEditor->RedrawLevelEditingViewports();
		}
#endif // WITH_EDITOR
	}
#endif // WITH_EDITORONLY_DATA
}

const FEnvQueryResult* AEQSTestingPawn::GetQueryResult() const 
{
	return StepResults.Num() > 0 ? &StepResults[StepToDebugDraw] : NULL;
}

const FEnvQueryInstance* AEQSTestingPawn::GetQueryInstance() const 
{
	return StepResults.Num() > 0 ? &StepResults[StepToDebugDraw] : NULL;
}

#if WITH_EDITOR

void AEQSTestingPawn::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName NAME_QueryTemplate = GET_MEMBER_NAME_CHECKED(AEQSTestingPawn, QueryTemplate);
	static const FName NAME_StepToDebugDraw = GET_MEMBER_NAME_CHECKED(AEQSTestingPawn, StepToDebugDraw);
	static const FName NAME_QueryParams = GET_MEMBER_NAME_CHECKED(AEQSTestingPawn, QueryParams);
	static const FName NAME_DrawFailedItems = GET_MEMBER_NAME_CHECKED(AEQSTestingPawn, bDrawFailedItems);
	static const FName NAME_ShouldBeVisibleInGame = GET_MEMBER_NAME_CHECKED(AEQSTestingPawn, bShouldBeVisibleInGame);

	if (PropertyChangedEvent.Property != NULL)
	{
		const FName PropName = PropertyChangedEvent.MemberProperty->GetFName();
		if (PropName == NAME_QueryTemplate || PropName == NAME_QueryParams)
		{
			if (QueryTemplate)
			{
				QueryTemplate->CollectQueryParams(QueryParams);
			}

			RunEQSQuery();
		}
		else if (PropName == NAME_StepToDebugDraw)
		{
			StepToDebugDraw  = FMath::Clamp(StepToDebugDraw, 0, StepResults.Num() - 1 );
			UpdateDrawing();
		}
		else if (PropName == NAME_DrawFailedItems)
		{
			UpdateDrawing();
		}
		else if (PropName == NAME_ShouldBeVisibleInGame)
		{
			UBillboardComponent* SpriteComponent = FindComponentByClass<UBillboardComponent>();
			if (SpriteComponent != NULL)
			{
				SpriteComponent->bHiddenInGame = !bShouldBeVisibleInGame;
			}
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AEQSTestingPawn::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished || !bReRunQueryOnlyOnFinishedMove)
	{
		RunEQSQuery();
	}
}

#endif // WITH_EDITOR