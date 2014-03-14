// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "DebugRenderSceneProxy.h"

static const int32 EQSMaxItemsDrawn = 10000;

//struct FEQSColors : public TSharedFromThis<FEQSColors>
//{
//	static TWeakObjectPtr<FEQSColors>
//};

namespace FEQSRenderingHelper
{
	FVector ExtractLocation(TSubclassOf<class UEnvQueryItemType> ItemType, TArray<uint8> RawData, const TArray<FEnvQueryItem>& Items, int32 Index)
	{
		if (Items.IsValidIndex(Index) &&
			ItemType->IsChildOf(UEnvQueryItemType_LocationBase::StaticClass()))
		{
			UEnvQueryItemType_LocationBase* DefTypeOb = (UEnvQueryItemType_LocationBase*)ItemType->GetDefaultObject();
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
	, bUntestedItems(false)
	, bDrawOnlyWhenSelected(bInDrawOnlyWhenSelected)
	, ViewFlagIndex(uint32(FEngineShowFlags::FindIndexByName(*InViewFlagName)))
	, ViewFlagName(InViewFlagName)
{
	if (InComponent == NULL)
	{
		return;
	}

	ActorOwner = InComponent->GetOwner();
	QueryDataSource = InterfaceCast<const IEQSQueryResultSourceInterface>(ActorOwner);
	if (QueryDataSource == NULL)
	{
		QueryDataSource = InterfaceCast<const IEQSQueryResultSourceInterface>(InComponent);
		if (QueryDataSource == NULL)
		{
			return;
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

#if WITH_EDITOR
	// using "mid-results" requires manual normalization
	const bool bUseMidResults = QueryInstance && (QueryInstance->Items.Num() < QueryInstance->DebugData.DebugItems.Num());
	const TArray<FEnvQueryItem>& Items =  bUseMidResults ? QueryInstance->DebugData.DebugItems : ResultItems->Items;
	const int32 ItemCountLimit = FMath::Clamp(Items.Num(), 0, EQSMaxItemsDrawn);
	const bool bNoTestsPerformed = QueryInstance != NULL && QueryInstance->CurrentTest <= 0;
	bUntestedItems = bNoTestsPerformed;

	float MinScore = 0.f;
	float MaxScore = -BIG_NUMBER;
	if (bUseMidResults)
	{
		const FEnvQueryItem* ItemInfo = Items.GetTypedData();
		for (int32 ItemIndex = 0; ItemIndex < Items.Num(); ItemIndex++, ItemInfo++)
		{
			if(ItemInfo->IsValid())
			{
				MinScore = FMath::Min(MinScore, ItemInfo->Score);
				MaxScore = FMath::Max(MaxScore, ItemInfo->Score);
			}
		}
	}
	const float ScoreNormalizer = bUseMidResults && (MaxScore != MinScore) ? 1.f/(MaxScore - MinScore) : 1.f;

	for (int32 ItemIndex = 0; ItemIndex < ItemCountLimit; ++ItemIndex)
	{
		if (Items[ItemIndex].IsValid())
		{
			FEQSItemDebugData& DebugData = DebugItems[DebugItems.AddZeroed(1)];
			DebugData.Location = FEQSRenderingHelper::ExtractLocation(ResultItems->ItemType, ResultItems->RawData, Items, ItemIndex);
			const float Score = Items[ItemIndex].Score * ScoreNormalizer;
			if (bNoTestsPerformed)
			{
				DebugData.Score = 1.f;
				DebugData.Label = TEXT("");
			}
			else
			{
				DebugData.Score = Score;
				DebugData.Label = FString::Printf(TEXT("%.2f"), Score);
			}
		}
	}

	if (QueryDataSource->GetShouldDrawFailedItems() && QueryInstance)
	{
		const FEQSQueryDebugData& InstanceDebugData = QueryInstance->DebugData;
		const TArray<FEnvQueryItem>& DebugQueryItems = InstanceDebugData.DebugItems;
		const TArray<FEnvQueryItemDetails>& Details = InstanceDebugData.DebugItemDetails;

		const int32 DebugItemCountLimit = FMath::Clamp(DebugQueryItems.Num(), 0, EQSMaxItemsDrawn);

		for (int32 ItemIndex = 0; ItemIndex < DebugItemCountLimit; ++ItemIndex)
		{
			if (DebugQueryItems[ItemIndex].IsValid())
			{
				continue;
			}

			FEQSItemDebugData& DebugData = FailedDebugItems[FailedDebugItems.AddZeroed(1)];
			DebugData.Location = FEQSRenderingHelper::ExtractLocation(QueryInstance->ItemType, InstanceDebugData.RawData, DebugQueryItems, ItemIndex);
			DebugData.Label = InstanceDebugData.PerformedTestNames[Details[ItemIndex].FailedTestIndex];
		}
	}
#endif // WITH_EDITOR

	if (DebugItems.Num() + FailedDebugItems.Num() > EQSMaxItemsDrawn)
	{
		UE_VLOG(ActorOwner, LogEQS, Warning, TEXT("EQS drawing: too much items to draw! Drawing first %d from set of %d")
			, EQSMaxItemsDrawn
			, DebugItems.Num()
			);
	}
}

void FEQSSceneProxy::RegisterDebugDrawDelgate()
{
	DebugTextDrawingDelegate = FDebugDrawDelegate::CreateRaw(this, &FEQSSceneProxy::DrawDebugLabels);
	UDebugDrawService::Register(*ViewFlagName, DebugTextDrawingDelegate);
}

void FEQSSceneProxy::UnregisterDebugDrawDelgate()
{
	if (DebugTextDrawingDelegate.IsBound())
	{
		UDebugDrawService::Unregister(DebugTextDrawingDelegate);
	}
}

//void FEQSSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
void FEQSSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View)
{
	struct FEQSColor
	{
		enum { NumberOfBuckets = 20 };
		mutable FMaterialRenderProxy* MeshColorInstances[NumberOfBuckets];
		mutable FMaterialRenderProxy* FailedColorInstance;
		mutable FMaterialRenderProxy* GeneratedColorInstance;

		FEQSColor()
			: FailedColorInstance(NULL), GeneratedColorInstance(NULL)
		{
			FMemory::MemZero(MeshColorInstances);				
		}

		FMaterialRenderProxy* operator[](float Score) const
		{
			static const float MinOpacity = 0.0;
			static const float Step = (1.f - MinOpacity)/(NumberOfBuckets);

			const int32 Index = FMath::Clamp<int32>(Score * 0.99 * NumberOfBuckets, 0, NumberOfBuckets-1);
			if (MeshColorInstances[Index] == NULL)
			{				
				MeshColorInstances[Index] = new(FMemStack::Get()) FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(false)
					, FColor::MakeRedToGreenColorFromScalar(Score));
			}

			return MeshColorInstances[Index];
		}

		FMaterialRenderProxy* Failed() const
		{
			if (FailedColorInstance == NULL)
			{
				FailedColorInstance = new(FMemStack::Get()) FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(false)
					, FLinearColor(0.0, 0.0, 0.6, 0.6));
			}

			return FailedColorInstance;
		}

		FMaterialRenderProxy* Generated() const
		{
			if (GeneratedColorInstance == NULL)
			{
				GeneratedColorInstance = new(FMemStack::Get()) FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(false)
					, FLinearColor(0.2, 1.0, 1.0, 1));
			}

			return GeneratedColorInstance;
		}
	};
	const FEQSColor EQSColor;

	if (bUntestedItems)
	{
		for (auto It = DebugItems.CreateConstIterator(); It; ++It)
		{
			DrawSphere(PDI, It->Location, ItemDrawRadius, 10, 7, EQSColor.Generated(), SDPG_World);
		}
	}
	else
	{
		for (auto It = DebugItems.CreateConstIterator(); It; ++It)
		{
			DrawSphere(PDI, It->Location, ItemDrawRadius, 10, 7, EQSColor[It->Score], SDPG_World);
		}

		for (auto It = FailedDebugItems.CreateConstIterator(); It; ++It)
		{
			DrawSphere(PDI, It->Location, ItemDrawRadius, 10, 7, EQSColor.Failed(), SDPG_World);
		}
	}
}

void FEQSSceneProxy::DrawDebugLabels(UCanvas* Canvas, APlayerController*)
{
	if ((ActorOwner->IsSelected() == false && bDrawOnlyWhenSelected == true) 
		|| QueryDataSource->GetShouldDebugDrawLabels() == false)
	{
		return;
	}

	const FColor OldDrawColor = Canvas->DrawColor;
	const FFontRenderInfo FontRenderInfo = Canvas->CreateFontRenderInfo(false, true);

	Canvas->SetDrawColor(FColor::White);

	UFont* RenderFont = GEngine->GetSmallFont();
	for (auto It = DebugItems.CreateConstIterator(); It; ++It)
	{
		const FVector ScreenLoc = Canvas->Project(It->Location);
		Canvas->DrawText(RenderFont, It->Label, ScreenLoc.X, ScreenLoc.Y, 1, 1, FontRenderInfo);
	}

	for (auto It = FailedDebugItems.CreateConstIterator(); It; ++It)
	{
		const FVector ScreenLoc = Canvas->Project(It->Location);
		Canvas->DrawText(RenderFont, It->Label, ScreenLoc.X, ScreenLoc.Y, 1, 1, FontRenderInfo);
	}

	Canvas->SetDrawColor(OldDrawColor);
}

FPrimitiveViewRelevance FEQSSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = !!View->Family->EngineShowFlags.GetSingleFlag(ViewFlagIndex) && IsShown(View) 
		&& (bDrawOnlyWhenSelected == false || ActorOwner->IsSelected());
	Result.bDynamicRelevance = true;
	Result.bNormalTranslucencyRelevance = IsShown(View) && GIsEditor;
	return Result;
}

uint32 FEQSSceneProxy::GetMemoryFootprint( void ) const 
{ 
	return sizeof( *this ) + GetAllocatedSize(); 
}

uint32 FEQSSceneProxy::GetAllocatedSize( void ) const 
{
	return FDebugRenderSceneProxy::GetAllocatedSize() + DebugItems.GetAllocatedSize();
}

//----------------------------------------------------------------------//
// UEQSRenderingComponent
//----------------------------------------------------------------------//
UEQSRenderingComponent::UEQSRenderingComponent(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
	, DrawFlagName("GameplayDebug")
{
}

FPrimitiveSceneProxy* UEQSRenderingComponent::CreateSceneProxy()
{
	return new FEQSSceneProxy(this, DrawFlagName);
}

FBoxSphereBounds UEQSRenderingComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FBox BoundingBox(FVector(-BIG_NUMBER/3), FVector(BIG_NUMBER/3));	
	
	return FBoxSphereBounds(BoundingBox);
}

void UEQSRenderingComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

#if WITH_EDITOR
	if (SceneProxy)
	{
		static_cast<FEQSSceneProxy*>(SceneProxy)->RegisterDebugDrawDelgate();
	}
#endif
}

void UEQSRenderingComponent::DestroyRenderState_Concurrent()
{
#if WITH_EDITOR
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
