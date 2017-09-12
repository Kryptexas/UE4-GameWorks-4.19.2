// AppleARKit
#include "AppleARKitPlaneAnchorVisualisationComponent.h"
#include "AppleARKit.h"
#include "AppleARKitPlaneAnchor.h"

// UE4
#include "Engine/StaticMesh.h"

UAppleARKitPlaneAnchorVisualisationComponent::UAppleARKitPlaneAnchorVisualisationComponent()
{
	// Tick, but not yet (let auto-activate do it)
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// Set default mesh
	SetStaticMesh( Cast<UStaticMesh>( StaticLoadObject( UStaticMesh::StaticClass(), NULL, TEXT("/Engine/BasicShapes/Plane" ) ) ) );
}

void UAppleARKitPlaneAnchorVisualisationComponent::BeginPlay()
{
	Super::BeginPlay();

	// Get a reference to the shared session
	Session = FAppleARKitExperimentalModule::Get().GetSession();
	check( Session != nullptr );

	// Start the shared session
	UE_LOG( LogAppleARKit, Log, TEXT("%s Starting Session %p ..."), *GetName(), &*Session );
	Session->Run();
}

void UAppleARKitPlaneAnchorVisualisationComponent::EndPlay( const EEndPlayReason::Type EndPlayReason )
{
	Super::EndPlay( EndPlayReason );

	// Stop the active session
	UE_LOG( LogAppleARKit, Log, TEXT("%s Stopping Session %p ..."), *GetName(), &*Session );
	Session->Pause();
}

void UAppleARKitPlaneAnchorVisualisationComponent::TickComponent( float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	// Only update if active
	if ( IsActive() )
	{
		// Get anchors
		TMap< FGuid, UAppleARKitAnchor* > Anchors = Session->GetAnchors();

		// Reset instances
		ClearInstances();

		// Set new instance transforms
		int32 Instance = 0;
		int32 StartingInstanceCount = GetInstanceCount();
		FTransform PlaneWorldTransform;
		bool bMarkDirty = false;
		for ( auto It = Anchors.CreateConstIterator(); It; ++It )
		{
			if ( UAppleARKitPlaneAnchor* PlaneAnchor = Cast< UAppleARKitPlaneAnchor >( It.Value() ) )
			{
				// Build local space transform
				PlaneWorldTransform = FTransform( PlaneAnchor->GetCenter() ) * PlaneAnchor->GetTransform() * Session->GetBaseTransform();

				// Fold in parent transform
				if ( USceneComponent* ParentComponent = GetAttachParent() )
				{
					PlaneWorldTransform *= ParentComponent->GetComponentTransform();
				}

				// Mesh local bounds
				FBox MeshBounds = GetStaticMesh()->GetBoundingBox();
				FVector MeshDimensions = MeshBounds.Max - MeshBounds.Min;

				// Set world scale to represent extent
				FVector PlaneWorldExtent = PlaneAnchor->GetExtent() * PlaneWorldTransform.GetScale3D();
				PlaneWorldTransform.SetScale3D( PlaneWorldExtent / MeshDimensions );

				// Can we just update an existing instance?
				if ( Instance < StartingInstanceCount )
				{
					UpdateInstanceTransform( Instance, PlaneWorldTransform, /*bWorldSpace=*/true, /*bMarkRenderStateDirty=*/false, /*bTeleport=*/true );

					// Reset the last transform again at the end to mark dirty render state
					bMarkDirty = true;
				}
				else
				{
					// Add a new one
					AddInstanceWorldSpace( PlaneWorldTransform );

					// We don't need reset the last transform to mark dirty as the instance addition 
					// should do it for us.
					bMarkDirty = false;
				}

				++Instance;
			}
		}

		// Remove excess instances?
		if ( Instance < StartingInstanceCount - 1 )
		{
			// Trim off excess instances (starting from the last, stopping before the lasy instance we 
			// updated above)
			for ( int32 ExcessInstance = StartingInstanceCount - 1; ExcessInstance > Instance; --ExcessInstance )
			{
				RemoveInstance( ExcessInstance );
			}

			// We don't need to reset the last transform to mark dirty as the instance addition 
			// should do it for us.
			bMarkDirty = false;
		}

		// Re-set last transform to mark dirty render state?
		if ( bMarkDirty )
		{
			UpdateInstanceTransform( Instance, PlaneWorldTransform, /*bWorldSpace=*/true, /*bMarkRenderStateDirty=*/true, /*bTeleport=*/true );
		}
	}
}
