// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ARBlueprintLibrary.h"
#include "Features/IModularFeature.h"
#include "Features/IModularFeatures.h"
#include "Engine/Engine.h"
#include "ARSystem.h"
#include "ARPin.h"
#include "ARTrackable.h"





//
//
//
//UARPinEventHandlers* UARPinEventHandlers::HandleARPinEvents( UARPin* Pin )
//{
//	UARPinEventHandlers* NewEventHandlers = NewObject<UARPinEventHandlers>();
//	Pin->SetOnTarckingStateChanged(NewEventHandlers->OnARTrackingStateChanged);
//	Pin->SetOnTransformUpdated(NewEventHandlers->OnARTransformUpdated);
//	
//	return NewEventHandlers;
//}




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
		return EARTrackingQuality::NotTracking;
	}
}

void UARBlueprintLibrary::StartARSession(UARSessionConfig* SessionConfig)
{
	static const TCHAR* NotARApp_Warning = TEXT("Attempting to Start an AR session but there is no AR system. To use AR, enable bIsARApp under Project Settings.");
	
	auto ARSystem = GetARSystem();
	if (ensureAlwaysMsgf(ARSystem.IsValid(), NotARApp_Warning))
	{
		ARSystem->StartARSession(SessionConfig);
	}
	else
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		// Ensures don't show up on iOS, but we definitely want a developer to see this
		// Their AR project just doesn't make sense unless they have enabled the AR setting.
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3600.0f, FColor(255,48,16),NotARApp_Warning);
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	}
}

void UARBlueprintLibrary::PauseARSession()
{
	auto ARSystem = GetARSystem();
	if (ARSystem.IsValid())
	{
		ARSystem->PauseARSession();
	}
}

void UARBlueprintLibrary::StopARSession()
{
	auto ARSystem = GetARSystem();
	if (ARSystem.IsValid())
	{
		ARSystem->StopARSession();
	}
}

FARSessionStatus UARBlueprintLibrary::GetARSessionStatus()
{
	auto ARSystem = GetARSystem();
	if (ARSystem.IsValid())
	{
		return ARSystem->GetARSessionStatus();
	}
	else
	{
		return FARSessionStatus(EARSessionStatus::NotStarted);
	}
}
UARSessionConfig* UARBlueprintLibrary::GetSessionConfig()
{
	auto ARSystem = GetARSystem();
	if (ARSystem.IsValid())
	{
		return &ARSystem->AccessSessionConfig();
	}
	else
	{
		return nullptr;
	}
}


void UARBlueprintLibrary::SetAlignmentTransform( const FTransform& InAlignmentTransform )
{
	auto ARSystem = GetARSystem();
	if (ARSystem.IsValid())
	{
		return ARSystem->SetAlignmentTransform( InAlignmentTransform );
	}
}


TArray<FARTraceResult> UARBlueprintLibrary::LineTraceTrackedObjects( const FVector2D ScreenCoord, bool bTestFeaturePoints, bool bTestGroundPlane, bool bTestPlaneExtents, bool bTestPlaneBoundaryPolygon )
{
	TArray<FARTraceResult> Result;
	
	auto ARSystem = GetARSystem();
	if (ensure(ARSystem.IsValid()))
	{
		EARLineTraceChannels ActiveTraceChannels =
		(bTestFeaturePoints ? EARLineTraceChannels::FeaturePoint : EARLineTraceChannels::None) |
			(bTestGroundPlane ? EARLineTraceChannels::GroundPlane : EARLineTraceChannels::None) |
			(bTestPlaneExtents ? EARLineTraceChannels::PlaneUsingExtent : EARLineTraceChannels::None ) |
			(bTestPlaneBoundaryPolygon ? EARLineTraceChannels::PlaneUsingBoundaryPolygon : EARLineTraceChannels::None);
		
		Result = ARSystem->LineTraceTrackedObjects(ScreenCoord, ActiveTraceChannels);
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

TArray<UARPin*> UARBlueprintLibrary::GetAllPins()
{
	TArray<UARPin*> Pins;
	
	auto ARSystem = GetARSystem();
	if (ensure(ARSystem.IsValid()))
	{
		Pins = ARSystem->GetAllPins();
	}
	return Pins;
}

bool UARBlueprintLibrary::IsSessionTypeSupported(EARSessionType SessionType)
{
	auto ARSystem = GetARSystem();
	if (ensure(ARSystem.IsValid()))
	{
		return ARSystem->IsSessionTypeSupported(SessionType);
	}
	return false;
}


void UARBlueprintLibrary::DebugDrawTrackedGeometry( UARTrackedGeometry* TrackedGeometry, UObject* WorldContextObject, FLinearColor Color, float OutlineThickness, float PersistForSeconds )
{
	UWorld* MyWorld = WorldContextObject->GetWorld();
	if (ensure(TrackedGeometry != nullptr) && ensure(MyWorld != nullptr))
	{
		TrackedGeometry->DebugDraw(MyWorld, Color, OutlineThickness, PersistForSeconds);
	}
}

void UARBlueprintLibrary::DebugDrawPin( UARPin* ARPin, UObject* WorldContextObject, FLinearColor Color, float Scale, float PersistForSeconds )
{
	UWorld* MyWorld = WorldContextObject->GetWorld();
	if (ensure(ARPin != nullptr) && ensure(MyWorld != nullptr))
	{
		ARPin->DebugDraw(MyWorld, Color, Scale, PersistForSeconds);
	}
}


UARLightEstimate* UARBlueprintLibrary::GetCurrentLightEstimate()
{
	auto ARSystem = GetARSystem();
	if (ensure(ARSystem.IsValid()))
	{
		return ARSystem->GetCurrentLightEstimate();
	}
	return nullptr;
}

UARPin* UARBlueprintLibrary::PinComponent( USceneComponent* ComponentToPin, const FTransform& PinToWorldTransform, UARTrackedGeometry* TrackedGeometry, const FName DebugName )
{
	auto ARSystem = GetARSystem();
	if (ensure(ARSystem.IsValid()))
	{
		return ARSystem->PinComponent( ComponentToPin, PinToWorldTransform, TrackedGeometry, DebugName );
	}
	return nullptr;
}

UARPin* UARBlueprintLibrary::PinComponentToTraceResult( USceneComponent* ComponentToPin, const FARTraceResult& TraceResult, const FName DebugName )
{
	auto ARSystem = GetARSystem();
	if (ensure(ARSystem.IsValid()))
	{
		return ARSystem->PinComponent( ComponentToPin, TraceResult, DebugName );
	}
	return nullptr;
}

void UARBlueprintLibrary::UnpinComponent( USceneComponent* ComponentToUnpin )
{
	auto ARSystem = GetARSystem();
	if (ensure(ARSystem.IsValid()))
	{
		TArray<UARPin*> AllPins = ARSystem->GetAllPins();
		const int32 AllPinsCount = AllPins.Num();
		for (int32 i=0; i<AllPinsCount; ++i)
		{
			if (AllPins[i]->GetPinnedComponent() == ComponentToUnpin)
			{
				ARSystem->RemovePin( AllPins[i] );
				return;
			}
		}
	}
}

void UARBlueprintLibrary::RemovePin( UARPin* PinToRemove )
{
	auto ARSystem = GetARSystem();
	if (ensure(ARSystem.IsValid()))
	{
		return ARSystem->RemovePin( PinToRemove );
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



float UARTraceResultLibrary::GetDistanceFromCamera( const FARTraceResult& TraceResult )
{
	return TraceResult.GetDistanceFromCamera();
}

FTransform UARTraceResultLibrary::GetLocalToTrackingTransform( const FARTraceResult& TraceResult )
{
	return TraceResult.GetLocalToTrackingTransform();
}

FTransform UARTraceResultLibrary::GetLocalToWorldTransform( const FARTraceResult& TraceResult )
{
	return TraceResult.GetLocalToWorldTransform();
}

UARTrackedGeometry* UARTraceResultLibrary::GetTrackedGeometry( const FARTraceResult& TraceResult )
{
	return TraceResult.GetTrackedGeometry();
}

EARLineTraceChannels UARTraceResultLibrary::GetTraceChannel( const FARTraceResult& TraceResult )
{
	return TraceResult.GetTraceChannel();
}
