// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "DebugRenderSceneProxy.h"
#include "EnvironmentQuery/EQSRenderingComponent.h"
#include "EnvironmentQuery/EQSQueryResultSourceInterface.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "VisualLog.h"

static const int32 EQSMaxItemsDrawn = 10000;

namespace FEQSRenderingHelper
{
	FVector ExtractLocation(TSubclassOf<class UEnvQueryItemType> ItemType, TArray<uint8> RawData, const TArray<FEnvQueryItem>& Items, int32 Index)
	{
		if (Items.IsValidIndex(Index) &&
			ItemType->IsChildOf(UEnvQueryItemType_VectorBase::StaticClass()))
		{
			UEnvQueryItemType_VectorBase* DefTypeOb = (UEnvQueryItemType_VectorBase*)ItemType->GetDefaultObject();
			return DefTypeOb->GetLocation(RawData.GetTypedData() + Items[Index].DataOffset);
		}
		return FVector::ZeroVector;
	}
}

//----------------------------------------------------------------------//
// FEQSSceneProxy
//----------------------------------------------------------------------//
const FVector FEQSSceneProxy::ItemDrawRadius(30,30,30);

FEQSSceneProxy::FEQSSceneProxy(const UPrimitiveComponent* InComponent, const FString& InViewFlagName, bool bInDrawOnlyWhenSelected) 
	: FDebugRenderSceneProxy(InComponent)
	, ActorOwner(NULL)
	, QueryDataSource(NULL)
	, bDrawOnlyWhenSelected(bInDrawOnlyWhenSelected)
{
	TextWithoutShadowDistance = 1500;
	ViewFlagIndex = uint32(FEngineShowFlags::FindIndexByName(*InViewFlagName));
	ViewFlagName = InViewFlagName;

	if (InComponent == NULL)
	{
		return;
	}

	ActorOwner = InComponent ? InComponent->GetOwner() : NULL;
	QueryDataSource = InterfaceCast<const IEQSQueryResultSourceInterface>(ActorOwner);
	if (QueryDataSource == NULL)
	{
		QueryDataSource = InterfaceCast<const IEQSQueryResultSourceInterface>(InComponent);
		if (QueryDataSource == NULL)
		{
			return;
		}
	}

	FEQSSceneProxy::CollectEQSData(InComponent, QueryDataSource, SolidSpheres, Texts);
}

FEQSSceneProxy::FEQSSceneProxy(const UPrimitiveComponent* InComponent, const FString& InViewFlagName, bool bInDrawOnlyWhenSelected, const TArray<FSphere>& Spheres, const TArray<FText3d>& InTexts)
	: FDebugRenderSceneProxy(InComponent)
	, ActorOwner(NULL)
	, QueryDataSource(NULL)
	, bDrawOnlyWhenSelected(bInDrawOnlyWhenSelected)
{
	TextWithoutShadowDistance = 1500;
	ViewFlagIndex = uint32(FEngineShowFlags::FindIndexByName(*InViewFlagName));
	ViewFlagName = InViewFlagName;

	if (InComponent == NULL)
	{
		return;
	}

	ActorOwner = InComponent ? InComponent->GetOwner() : NULL;
	QueryDataSource = InterfaceCast<const IEQSQueryResultSourceInterface>(ActorOwner);
	if (QueryDataSource == NULL)
	{
		QueryDataSource = InterfaceCast<const IEQSQueryResultSourceInterface>(InComponent);
		if (QueryDataSource == NULL)
		{
			return;
		}
	}

	SolidSpheres = Spheres;
	Texts = InTexts;
}

void FEQSSceneProxy::CollectEQSData(const UPrimitiveComponent* InComponent, const class IEQSQueryResultSourceInterface* InQueryDataSource, TArray<FSphere>& Spheres, TArray<FText3d>& Texts)
{
	AActor* ActorOwner = InComponent ? InComponent->GetOwner() : NULL;
	IEQSQueryResultSourceInterface* QueryDataSource = const_cast<IEQSQueryResultSourceInterface*>(InQueryDataSource);
	if (QueryDataSource == NULL)
	{
		QueryDataSource = InterfaceCast<IEQSQueryResultSourceInterface>(ActorOwner);
		if (QueryDataSource == NULL)
		{
			QueryDataSource = InterfaceCast<IEQSQueryResultSourceInterface>(const_cast<UPrimitiveComponent*>(InComponent));
			if (QueryDataSource == NULL)
			{
				return;
			}
		}
	}

	const FEnvQueryResult* ResultItems = QueryDataSource->GetQueryResult();
	const FEnvQueryInstance* QueryInstance = QueryDataSource->GetQueryInstance();

	if (ResultItems == NULL)
	{
		if (QueryInstance == NULL)
		{
			return;
		}
		else
		{
			ResultItems = QueryInstance;
		}
	}

#if USE_EQS_DEBUGGER
	// using "mid-results" requires manual normalization
	const bool bUseMidResults = QueryInstance && (QueryInstance->Items.Num() < QueryInstance->DebugData.DebugItems.Num());
	const TArray<FEnvQueryItem>& Items = bUseMidResults ? QueryInstance->DebugData.DebugItems : ResultItems->Items;
	const int32 ItemCountLimit = FMath::Clamp(Items.Num(), 0, EQSMaxItemsDrawn);
	const bool bNoTestsPerformed = QueryInstance != NULL && QueryInstance->CurrentTest <= 0;

	//if (!bUseMidResults && Items.Num() > 0 && Items[0].IsValid())
	//{
	//	const FVector Loc = FEQSRenderingHelper::ExtractLocation(ResultItems->ItemType, ResultItems->RawData, Items, 0);
	//	Cylinders.Add(FWireCylinder(Loc, ItemDrawRadius.X, 100, FColor::White));
	//}

	float MinScore = 0.f;
	float MaxScore = -BIG_NUMBER;
	if (bUseMidResults)
	{
		const FEnvQueryItem* ItemInfo = Items.GetTypedData();
		for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ItemIndex++, ItemInfo++)
		{
			if (ItemInfo->IsValid())
			{
				MinScore = FMath::Min(MinScore, ItemInfo->Score);
				MaxScore = FMath::Max(MaxScore, ItemInfo->Score);
			}
		}
	}
	const float ScoreNormalizer = bUseMidResults && (MaxScore != MinScore) ? 1.f / (MaxScore - MinScore) : 1.f;

	for (int32 ItemIndex = 0; ItemIndex < ItemCountLimit; ++ItemIndex)
	{
		if (Items[ItemIndex].IsValid())
		{
			const float Score = bNoTestsPerformed ? 1 : Items[ItemIndex].Score * ScoreNormalizer;
			const FVector Loc = FEQSRenderingHelper::ExtractLocation(ResultItems->ItemType, ResultItems->RawData, Items, ItemIndex);
			Spheres.Add(FSphere(ItemDrawRadius.X, Loc, !bNoTestsPerformed ? FLinearColor(FColor::MakeRedToGreenColorFromScalar(Score)) : FLinearColor(0.2, 1.0, 1.0, 1)));

			const FString Label = bNoTestsPerformed ? TEXT("") : FString::Printf(TEXT("%.2f"), Score);
			Texts.Add(FText3d(Label, Loc, FLinearColor::White));
		}
	}

	if (QueryDataSource->GetShouldDrawFailedItems() && QueryInstance)
	{
		const FEQSQueryDebugData& InstanceDebugData = QueryInstance->DebugData;
		const TArray<FEnvQueryItem>& DebugQueryItems = InstanceDebugData.DebugItems;
		const TArray<FEnvQueryItemDetails>& Details = InstanceDebugData.DebugItemDetails;

		const int32 DebugItemCountLimit = DebugQueryItems.Num() == Details.Num() ? FMath::Clamp(DebugQueryItems.Num(), 0, EQSMaxItemsDrawn) : 0;

		for (int32 ItemIndex = 0; ItemIndex < DebugItemCountLimit; ++ItemIndex)
		{
			if (DebugQueryItems[ItemIndex].IsValid())
			{
				continue;
			}

			const float Score = bNoTestsPerformed ? 1 : Items[ItemIndex].Score * ScoreNormalizer;
			const FVector Loc = FEQSRenderingHelper::ExtractLocation(QueryInstance->ItemType, InstanceDebugData.RawData, DebugQueryItems, ItemIndex);
			Spheres.Add(FSphere(ItemDrawRadius.X, Loc, FLinearColor(0.0, 0.0, 0.6, 0.6)));

			const FString Label = InstanceDebugData.PerformedTestNames[Details[ItemIndex].FailedTestIndex];
			Texts.Add(FText3d(Label, Loc, FLinearColor::White));
		}
	}
#endif // USE_EQS_DEBUGGER

	if (ActorOwner && Spheres.Num() > EQSMaxItemsDrawn)
	{
		UE_VLOG(ActorOwner, LogEQS, Warning, TEXT("EQS drawing: too much items to draw! Drawing first %d from set of %d")
			, EQSMaxItemsDrawn
			, Spheres.Num()
			);
	}
}

void FEQSSceneProxy::DrawDebugLabels(UCanvas* Canvas, APlayerController* PC)
{
	if ( !ActorOwner
		 || (ActorOwner->IsSelected() == false && bDrawOnlyWhenSelected == true)
		 || QueryDataSource->GetShouldDebugDrawLabels() == false)
	{
		return;
	}

	FDebugRenderSceneProxy::DrawDebugLabels(Canvas, PC);
}

bool FEQSSceneProxy::SafeIsActorSelected() const
{
	if(ActorOwner)
	{
		return ActorOwner->IsSelected();
	}

	return false;
}

FPrimitiveViewRelevance FEQSSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = View->Family->EngineShowFlags.GetSingleFlag(ViewFlagIndex) && IsShown(View) 
		&& (!bDrawOnlyWhenSelected || SafeIsActorSelected());
	Result.bDynamicRelevance = true;
	Result.bNormalTranslucencyRelevance = IsShown(View);
	return Result;
}

//----------------------------------------------------------------------//
// UEQSRenderingComponent
//----------------------------------------------------------------------//
UEQSRenderingComponent::UEQSRenderingComponent(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
	, DrawFlagName("GameplayDebug")
	, bDrawOnlyWhenSelected(true)
{
}

FPrimitiveSceneProxy* UEQSRenderingComponent::CreateSceneProxy()
{
	return new FEQSSceneProxy(this, DrawFlagName, bDrawOnlyWhenSelected);
}

FBoxSphereBounds UEQSRenderingComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	static FBox BoundingBox(FVector(-HALF_WORLD_MAX), FVector(HALF_WORLD_MAX));
	return FBoxSphereBounds(BoundingBox);
}

void UEQSRenderingComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

#if USE_EQS_DEBUGGER
	if (SceneProxy)
	{
		static_cast<FEQSSceneProxy*>(SceneProxy)->RegisterDebugDrawDelgate();
	}
#endif
}

void UEQSRenderingComponent::DestroyRenderState_Concurrent()
{
#if USE_EQS_DEBUGGER
	if (SceneProxy)
	{
		static_cast<FEQSSceneProxy*>(SceneProxy)->UnregisterDebugDrawDelgate();
	}
#endif

	Super::DestroyRenderState_Concurrent();
}

//----------------------------------------------------------------------//
// UEQSQueryResultSourceInterface
//----------------------------------------------------------------------//
UEQSQueryResultSourceInterface::UEQSQueryResultSourceInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}
