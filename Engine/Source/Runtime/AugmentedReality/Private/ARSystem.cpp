// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ARSystem.h"
#include "IXRTrackingSystem.h"
#include "Features/IModularFeatures.h"
#include "ARBlueprintLibrary.h"
#include "DrawDebugHelpers.h"


//
//
//
void UARTrackedGeometry::InitTrackedGeometry( const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InARSystem )
{
	ARSystem = InARSystem;
}

void UARTrackedGeometry::DebugDraw( UWorld* World, const FLinearColor& OutlineColor, float OutlineThickness, float PersistForSeconds ) const
{
	ensureMsgf( false, TEXT("Unimplemented UARTrackedGeometry::DebugDraw") );
}

TSharedPtr<FARSystemBase, ESPMode::ThreadSafe> UARTrackedGeometry::GetARSystem() const
{
	auto MyARSystem = ARSystem.Pin();
	return MyARSystem;
}

//
//
//
void UARPlaneGeometry::UpdateTrackedGeometry( const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystem, const FTransform& InLocalToTrackingTransform, const FVector InCenter, const FVector InExtent )
{
	InitTrackedGeometry(InTrackingSystem);
	LocalToTrackingTransform = InLocalToTrackingTransform;
	Center = InCenter;
	Extent = InExtent;
}

void UARPlaneGeometry::DebugDraw( UWorld* World, const FLinearColor& OutlineColor, float OutlineThickness, float PersistForSeconds ) const
{
	const FTransform LocalToWorldTransform = GetLocalToWorldTransform();
	DrawDebugBox( World, LocalToWorldTransform.TransformPosition(Center), Extent, LocalToWorldTransform.GetRotation(), OutlineColor.ToFColor(false), false, PersistForSeconds, 0, OutlineThickness );
}

FTransform UARPlaneGeometry::GetLocalToTrackingTransform() const
{
	return LocalToTrackingTransform;
}

FTransform UARPlaneGeometry::GetLocalToWorldTransform() const
{
	return LocalToTrackingTransform * GetARSystem()->GetTrackingToWorldTransform();
}



//
//
//
FARTraceResult::FARTraceResult()
: FARTraceResult(nullptr, FTransform(), nullptr)
{
	
}


FARTraceResult::FARTraceResult( const TSharedPtr<FARSystemBase, ESPMode::ThreadSafe>& InARSystem, const FTransform& InLocalToTrackingTransform, UARTrackedGeometry* InTrackedGeometry )
: LocalToTrackingTransform(InLocalToTrackingTransform)
, TrackedGeometry(InTrackedGeometry)
, ARSystem(InARSystem)
{
	
}


FTransform FARTraceResult::GetLocalToTrackingTransform() const
{
	return LocalToTrackingTransform;
}


FTransform FARTraceResult::GetLocalToWorldTransform() const
{
	return LocalToTrackingTransform * ARSystem->GetTrackingToWorldTransform();
}


UARTrackedGeometry* FARTraceResult::GetTrackedGeometry() const
{
	return TrackedGeometry;
}




FARSystemBase::FARSystemBase()
: bIsActive(false)
{
	// See Initialize(), as we need access to SharedThis()
}

void FARSystemBase::InitializeARSystem()
{
	// Register our ability to support Unreal AR API.
	IModularFeatures::Get().RegisterModularFeature(FARSystemBase::GetModularFeatureName(), this);
	
	UARBlueprintLibrary::RegisterAsARSystem( SharedThis(this) );
	
	OnARSystemInitialized();
}

FARSystemBase::~FARSystemBase()
{
	IModularFeatures::Get().UnregisterModularFeature(FARSystemBase::GetModularFeatureName(), this);
	
	UARBlueprintLibrary::RegisterAsARSystem( nullptr );
}

EARTrackingQuality FARSystemBase::GetTrackingQuality() const
{
	return OnGetTrackingQuality();
}

bool FARSystemBase::StartAR()
{
	if (!IsARActive())
	{
		bIsActive = OnStartAR();
	}
	
	return bIsActive;
}

void FARSystemBase::StopAR()
{
	if (IsARActive())
	{
		OnStopAR();
		bIsActive = false;
	}
}

bool FARSystemBase::IsARActive() const
{
	return bIsActive;
}

TArray<FARTraceResult> FARSystemBase::LineTraceTrackedObjects( const FVector2D ScreenCoord )
{
	return OnLineTraceTrackedObjects(ScreenCoord);
}

TArray<UARTrackedGeometry*> FARSystemBase::GetAllTrackedGeometries() const
{
	return OnGetAllTrackedGeometries();
}




