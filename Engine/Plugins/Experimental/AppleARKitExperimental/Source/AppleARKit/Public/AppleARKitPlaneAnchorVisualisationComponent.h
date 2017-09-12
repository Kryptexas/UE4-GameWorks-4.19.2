#pragma once

// Unreal
#include "Components/InstancedStaticMeshComponent.h"

// AppleARKit
#include "AppleARKitSession.h"
#include "AppleARKitPlaneAnchorVisualisationComponent.generated.h"

/**
 * A specialized version of a camera component that updates its transform
 * on tick, using a ARKit tracking session.
 */
UCLASS( ClassGroup=AppleARKit, BlueprintType, Blueprintable, Hidecategories=(InstancedStaticMeshComponent,Instances), ShowCategories=(Activation,"Components|Activation"), meta=(BlueprintSpawnableComponent) )
class APPLEARKITEXPERIMENTAL_API UAppleARKitPlaneAnchorVisualisationComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:

	// Default constructor
	UAppleARKitPlaneAnchorVisualisationComponent();

	// AActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay( const EEndPlayReason::Type EndPlayReason ) override;
	virtual void TickComponent( float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction ) override;

private:

	// Session
	UPROPERTY( Transient )
	UAppleARKitSession* Session;
};
