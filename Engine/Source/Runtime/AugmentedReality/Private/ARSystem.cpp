// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ARSystem.h"
#include "IXRTrackingSystem.h"
#include "Features/IModularFeatures.h"
#include "ARBlueprintLibrary.h"
#include "ARSessionConfig.h"
#include "GeneralProjectSettings.h"
#include "Engine/Engine.h"



FARSystemBase::FARSystemBase()
: AlignmentTransform(FTransform::Identity)
, ARSettings( NewObject<UARSessionConfig>() )
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

void FARSystemBase::StartARSession(UARSessionConfig* InSessionConfig)
{
	static const TCHAR* NotARApp_Warning = TEXT("To use AR, enable bIsARApp under Project Settings.");
	
	const bool bIsARApp = GetDefault<UGeneralProjectSettings>()->bSupportAR;
	if (ensureAlwaysMsgf(bIsARApp, NotARApp_Warning))
	{
		if (GetARSessionStatus().Status != EARSessionStatus::Running)
		{
			OnStartARSession(InSessionConfig);
			ARSettings = InSessionConfig;
		}
	}
	else
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		// Ensures don't show up on iOS, but we definitely want a developer to see this
		// Their AR project just doesn't make sense unless they have enabled the AR setting.
		GEngine->AddOnScreenDebugMessage((uint64)((PTRINT)this), 3600.0f, FColor(255,48,16),NotARApp_Warning);
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	}
}

void FARSystemBase::PauseARSession()
{
	if (GetARSessionStatus().Status == EARSessionStatus::Running)
	{
		OnPauseARSession();
	}
}

void FARSystemBase::StopARSession()
{
	if (GetARSessionStatus().Status == EARSessionStatus::Running)
	{
		OnStopARSession();
	}
}

FARSessionStatus FARSystemBase::GetARSessionStatus() const
{
	return OnGetARSessionStatus();
}

TArray<FARTraceResult> FARSystemBase::LineTraceTrackedObjects( const FVector2D ScreenCoord, EARLineTraceChannels TraceChannels )
{
	return OnLineTraceTrackedObjects(ScreenCoord, TraceChannels);
}

TArray<UARTrackedGeometry*> FARSystemBase::GetAllTrackedGeometries() const
{
	return OnGetAllTrackedGeometries();
}

TArray<UARPin*> FARSystemBase::GetAllPins() const
{
	return OnGetAllPins();
}

bool FARSystemBase::IsSessionTypeSupported(EARSessionType SessionType) const
{
	return OnIsTrackingTypeSupported(SessionType);
}

void FARSystemBase::SetAlignmentTransform( const FTransform& InAlignmentTransform )
{
	return OnSetAlignmentTransform(InAlignmentTransform);
}


UARLightEstimate* FARSystemBase::GetCurrentLightEstimate() const
{
	return OnGetCurrentLightEstimate();
}

UARPin* FARSystemBase::PinComponent( USceneComponent* ComponentToPin, const FTransform& PinToWorldTransform, UARTrackedGeometry* TrackedGeometry, const FName DebugName )
{
	return OnPinComponent( ComponentToPin, PinToWorldTransform, TrackedGeometry, DebugName );
}

UARPin* FARSystemBase::PinComponent( USceneComponent* ComponentToPin, const FARTraceResult& HitResult, const FName DebugName )
{
	return OnPinComponent( ComponentToPin, HitResult.GetLocalToWorldTransform(), HitResult.GetTrackedGeometry(), DebugName );
}

void FARSystemBase::RemovePin( UARPin* PinToRemove )
{
	OnRemovePin( PinToRemove );
}

const FTransform& FARSystemBase::GetAlignmentTransform() const
{
	return AlignmentTransform;
}

const UARSessionConfig& FARSystemBase::GetSessionConfig() const
{
	check(ARSettings != nullptr);
	return *ARSettings;
}

UARSessionConfig& FARSystemBase::AccessSessionConfig()
{
	check(ARSettings != nullptr);
	return *ARSettings;
}

void FARSystemBase::SetAlignmentTransform_Internal(const FTransform& NewAlignmentTransform)
{
	AlignmentTransform = NewAlignmentTransform;
}

void FARSystemBase::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (ARSettings != nullptr)
	{
		Collector.AddReferencedObject(ARSettings);
	}
}

