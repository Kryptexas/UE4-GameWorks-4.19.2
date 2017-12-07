// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Features/IModularFeature.h"
#include "XRTrackingSystemBase.h"
#include "ARSystem.generated.h"

class FARSystemBase;
class USceneComponent;
class IXRTrackingSystem;

class IARSystemPin
{
	
};

USTRUCT( BlueprintType, Category="AR")
struct AUGMENTEDREALITY_API FARPin
{
	GENERATED_BODY()
	
public:
	FTransform GetLocalToTrackingTransform();
	FTransform GetLocalToWorldTransform();
	void SetOnTarckingStateChanged();
	void SetOnPoseUpdated();
	
	
private:
	TSharedPtr<class IARSystemPin> SystemPin;
};

UENUM(BlueprintType, Category="AR", meta=(Experimental))
enum class EARTrackingQuality : uint8
{
	/** The tracking quality is not available. */
	NotAvailable,
	
	/** The tracking quality is limited, relying only on the device's motion. */
	Limited,
	
	/** The tracking quality is good. */
	Normal
};


UCLASS(BlueprintType)
class AUGMENTEDREALITY_API UARTrackedGeometry : public UObject
{
	GENERATED_BODY()
	
public:
	virtual void InitTrackedGeometry( const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystemOwner );
	
	virtual void DebugDraw( UWorld* World, const FLinearColor& OutlineColor, float OutlineThickness, float PersistForSeconds = 0.0f) const;
protected:
	TSharedPtr<FARSystemBase, ESPMode::ThreadSafe> GetARSystem() const;
private:
	TWeakPtr<FARSystemBase, ESPMode::ThreadSafe> ARSystem;
};

UCLASS(BlueprintType)
class AUGMENTEDREALITY_API UARPlaneGeometry : public UARTrackedGeometry
{
	GENERATED_BODY()

public:
	void UpdateTrackedGeometry( const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystem, const FTransform& InTransform, const FVector InCenter, const FVector InExtent );
	
	virtual void DebugDraw( UWorld* World, const FLinearColor& OutlineColor, float OutlineThickness, float PersistForSeconds = 0.0f) const override;
	
public:
	UFUNCTION(BlueprintPure, Category="Plane Geometry")
	FTransform GetLocalToTrackingTransform() const;
	
	UFUNCTION(BlueprintPure, Category="Plane Geometry")
	FTransform GetLocalToWorldTransform() const;
	
	UFUNCTION(BlueprintPure, Category="Plane Geometry")
	FVector GetCenter() const { return Center; }
	
	UFUNCTION(BlueprintPure, Category="Plane Geometry")
	FVector GetExtent() const { return Extent; }
	
private:
	UPROPERTY()
	FTransform LocalToTrackingTransform;
	
	UPROPERTY()
	FVector Center;
	
	UPROPERTY()
	FVector Extent;
};


/**
 * A result of an intersection found during a hit-test.
 */
USTRUCT( BlueprintType, Category="AR", meta=(Experimental))
struct AUGMENTEDREALITY_API FARTraceResult
{
	GENERATED_BODY();
	
	FARTraceResult();
	
	FARTraceResult( const TSharedPtr<FARSystemBase, ESPMode::ThreadSafe>& InARSystem, const FTransform& InLocalToTrackingTransform, UARTrackedGeometry* InTrackedGeometry );
	
	
	FTransform GetLocalToTrackingTransform() const;
	
	
	FTransform GetLocalToWorldTransform() const;
	
	
	UARTrackedGeometry* GetTrackedGeometry() const;
	
private:
	
	/**
	 * The transformation matrix that defines the intersection's rotation, translation and scale
	 * relative to the world.
	 */
	UPROPERTY()
	FTransform LocalToTrackingTransform;
	
	UPROPERTY()
	UARTrackedGeometry* TrackedGeometry;
	
	/** A reference to the AR system that creates this hit test result. */
	TSharedPtr<IXRTrackingSystem, ESPMode::ThreadSafe> ARSystem;
};


/** Implement IARSystemSupport for any platform that wants to be an Unreal AR System. e.g. AppleARKit, GoogleARCore. */
class AUGMENTEDREALITY_API IARSystemSupport
{
public:
	/** Invoked after the base AR system has been initialized. */
	virtual void OnARSystemInitialized(){};

	/** @return the tracking quality; if unable to determine tracking quality, return EARTrackingQuality::NotAvailable */
	virtual EARTrackingQuality OnGetTrackingQuality() const = 0;
	/**
	 * Start the AR system.
	 *
	 * @return true if the system was successfully started
	 */
	virtual bool OnStartAR() = 0;
	
	/** Stop the AR system; this task must succeed. */
	virtual void OnStopAR() = 0;
	
	/**
	 * Trace all the tracked geometries and determine which have been hit by a ray cast from `ScreenCoord`
	 *
	 * @return a list of all the geometries that were hit, sorted by distance
	 */
	virtual TArray<FARTraceResult> OnLineTraceTrackedObjects( const FVector2D ScreenCoord ) = 0;
	
	/** @return a TArray of all the tracked geometries known to your ar system */
	virtual TArray<UARTrackedGeometry*> OnGetAllTrackedGeometries() const = 0;
	
	
public:
	virtual ~IARSystemSupport(){}
};


class AUGMENTEDREALITY_API FARSystemBase : public IARSystemSupport, public FXRTrackingSystemBase, public TSharedFromThis<FARSystemBase, ESPMode::ThreadSafe>
{
public:
	//
	// MODULAR FEATURE SUPPORT
	//
	
	/**
	 * Part of the pattern for supporting modular features.
	 *
	 * @return the name of the feature.
	 */
	static FName GetModularFeatureName()
	{
		static const FName ModularFeatureName = FName(TEXT("ARSystem"));
		return ModularFeatureName;
	}
	
public:
	FARSystemBase();
	void InitializeARSystem();
	virtual ~FARSystemBase();
	
	EARTrackingQuality GetTrackingQuality() const;
	bool StartAR();
	void StopAR();
	bool IsARActive() const;
	TArray<FARTraceResult> LineTraceTrackedObjects( const FVector2D NormalizedScreenCoord );
	TArray<UARTrackedGeometry*> GetAllTrackedGeometries() const;
	
//	virtual FARPin PinComponent( USceneComponent* ComponentToPin, const FTransform& PinToWorldTransform, const FARTrackedGeometry& TrackedGeometry = FARTrackedGeometry() );
//	virtual FARPin PinComponent( USceneComponent* ComponentToPin, const FARTraceResult& HitResult );
//
//	virtual void RemovePin( const USceneComponent* ComponentToUnpin );
//	virtual void RemovePin( const FARPin& PinToRemove );
//
//	virtual TArray<FARTrackedGeometry> GetAllTrackedGeometry() const;
//
//	virtual TArray<FARPin> GetAllPins() const;
//
//	virtual TArray<FARTraceResult> LineTraceFromScreenPoint( const FVector2D ScreenPosition );
	
protected:

	
private:
	bool bIsActive;
};

template<class T, typename... ArgTypes>
TSharedRef<T, ESPMode::ThreadSafe> NewARSystem( ArgTypes&&... Args )
{
	auto NewARSystem = MakeShared<T, ESPMode::ThreadSafe>( Forward<ArgTypes>(Args)... );
	NewARSystem->InitializeARSystem();
	return NewARSystem;
}
