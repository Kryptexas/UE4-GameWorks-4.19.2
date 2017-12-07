// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ARBlueprintLibrary.h"
#include "Features/IModularFeature.h"
#include "Features/IModularFeatures.h"
#include "Engine/Engine.h"
#include "ARSystem.h"


TSharedPtr<FARSystemBase, ESPMode::ThreadSafe> UARBlueprintLibrary::RegisteredARSystem = nullptr;

//bool UARBlueprintLibrary::ARLineTraceFromScreenPoint(UObject* WorldContextObject, const FVector2D ScreenPosition, TArray<FARTraceResult>& OutHitResults)
//{
//	TArray<IARHitTestingSupport*> Providers = IModularFeatures::Get().GetModularFeatureImplementations<IARHitTestingSupport>(IARHitTestingSupport::GetModularFeatureName());
//	const int32 NumProviders = Providers.Num();
//	ensureMsgf(NumProviders <= 1, TEXT("Expected at most one ARHitTestingSupport provider, but there are %d registered. Using the first."), NumProviders);
//	if ( ensureMsgf(NumProviders > 0, TEXT("Expected at least one ARHitTestingSupport provider.")) )
//	{
//		if (const bool bHit = Providers[0]->ARLineTraceFromScreenPoint(ScreenPosition, OutHitResults))
//		{
//			// Update transform from ARKit (camera) space to UE World Space
//			
//			UWorld* MyWorld = WorldContextObject->GetWorld();
//			APlayerController* MyPC = MyWorld != nullptr ? MyWorld->GetFirstPlayerController() : nullptr;
//			APawn* MyPawn = MyPC != nullptr ? MyPC->GetPawn() : nullptr;
//			
//			if (MyPawn != nullptr)
//			{
//				const FTransform PawnTransform = MyPawn->GetActorTransform();
//				for ( FARTraceResult& HitResult : OutHitResults )
//				{
//					HitResult.Transform *= PawnTransform;
//				}
//				return true;
//			}
//		}
//	}
//
//	return false;
//}


EARTrackingQuality UARBlueprintLibrary::GetTrackingQuality()
{
	auto ARSystem = GetARSystem();
	if (ARSystem.IsValid())
	{
		return ARSystem->GetTrackingQuality();
	}
	else
	{
		return EARTrackingQuality::NotAvailable;
	}
}

bool UARBlueprintLibrary::StartAR()
{
	auto ARSystem = GetARSystem();
	if (ARSystem.IsValid())
	{
		return ARSystem->StartAR();
	}
	else
	{
		return false;
	}
}

void UARBlueprintLibrary::StopAR()
{
	auto ARSystem = GetARSystem();
	if (ARSystem.IsValid())
	{
		ARSystem->StopAR();
	}
}

bool UARBlueprintLibrary::IsARActive()
{
	auto ARSystem = GetARSystem();
	if (ARSystem.IsValid())
	{
		return ARSystem->IsARActive();
	}
	else
	{
		return false;
	}
}

TArray<FARTraceResult> UARBlueprintLibrary::LineTraceTrackedObjects( const FVector2D ScreenCoord )
{
	TArray<FARTraceResult> Result;
	
	auto ARSystem = GetARSystem();
	if (ensure(ARSystem.IsValid()))
	{
		Result = ARSystem->LineTraceTrackedObjects(ScreenCoord);
	}
	
	return Result;
}

TArray<UARTrackedGeometry*> UARBlueprintLibrary::GetAllGeometries()
{
	TArray<UARTrackedGeometry*> Geometries;
	
	auto ARSystem = GetARSystem();
	if (ensure(ARSystem.IsValid()))
	{
		Geometries = ARSystem->GetAllTrackedGeometries();
	}
	return Geometries;
}


FTransform UARBlueprintLibrary::GetLocalToTrackingTransform( const FARTraceResult& TraceResult )
{
	return TraceResult.GetLocalToTrackingTransform();
}

FTransform UARBlueprintLibrary::GetLocalToWorldTransform( const FARTraceResult& TraceResult )
{
	return TraceResult.GetLocalToWorldTransform();
}

UARTrackedGeometry* UARBlueprintLibrary::GetTrackedGeometry( const FARTraceResult& TraceResult )
{
	return TraceResult.GetTrackedGeometry();
}

void UARBlueprintLibrary::DebugDrawTrackedGeometry( UARTrackedGeometry* TrackedGeometry, UObject* WorldContextObject, FLinearColor Color, float OutlineThickness, float PersistForSeconds )
{
	UWorld* MyWorld = WorldContextObject->GetWorld();
	if (ensure(TrackedGeometry != nullptr) && ensure(MyWorld != nullptr))
	{
		TrackedGeometry->DebugDraw(MyWorld, Color, OutlineThickness, PersistForSeconds);
	}
}

void UARBlueprintLibrary::RegisterAsARSystem(const TSharedPtr<FARSystemBase, ESPMode::ThreadSafe>& NewARSystem)
{
	RegisteredARSystem = NewARSystem;
}


const TSharedPtr<FARSystemBase, ESPMode::ThreadSafe>& UARBlueprintLibrary::GetARSystem()
{
	return RegisteredARSystem;
}
