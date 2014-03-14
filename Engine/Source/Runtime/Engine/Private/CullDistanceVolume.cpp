// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

ACullDistanceVolume::ACullDistanceVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BrushComponent->BodyInstance.bEnableCollision_DEPRECATED = false;
	BrushComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	BrushComponent->bAlwaysCreatePhysicsState = true;

	CullDistances.Add(FCullDistanceSizePair(0,0));
	CullDistances.Add(FCullDistanceSizePair(10000,0));

	bEnabled = true;
}

void ACullDistanceVolume::Destroyed()
{
	Super::Destroyed();

#if WITH_EDITOR
	if (GetWorld())
	{
		GetWorld()->bDoDelayedUpdateCullDistanceVolumes = true;
	}
#endif
}

#if WITH_EDITOR
void ACullDistanceVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	GetWorld()->UpdateCullDistanceVolumes();
}

void ACullDistanceVolume::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if( bFinished )
	{
		GetWorld()->UpdateCullDistanceVolumes();
	}
}
#endif // WITH_EDITOR

bool ACullDistanceVolume::CanBeAffectedByVolumes( UPrimitiveComponent* PrimitiveComponent )
{
	AActor* Owner = PrimitiveComponent ? PrimitiveComponent->GetOwner() : NULL;

	// Require an owner so we can use its location
	if(	Owner
	// Disregard dynamic actors
	&& (PrimitiveComponent->Mobility == EComponentMobility::Static)
	// Disregard prefabs.
	&& !PrimitiveComponent->IsTemplate()
	// Skip primitives that is hidden set as we don't want to cull out brush rendering or other helper objects.
	&&	PrimitiveComponent->IsVisible()
	// Only operate on primitives attached to the owners world.			
	&&	PrimitiveComponent->GetScene() == Owner->GetWorld()->Scene 
	// Require cull distance volume support to be enabled.
	&&	PrimitiveComponent->bAllowCullDistanceVolume )
	{
		return true;
	}
	else
	{
		return false;
	}	
}

void ACullDistanceVolume::GetPrimitiveMaxDrawDistances(TMap<UPrimitiveComponent*,float>& OutCullDistances)
{
	// Nothing to do if there is no brush component or no cull distances are set
	if( BrushComponent && CullDistances.Num() > 0 && bEnabled )
	{
		// Test center of mesh bounds to see whether it is encompassed by volume
		// and propagate cull distance if it is.
		for( FActorIterator It(GetWorld()); It; ++It )
		{
			AActor* Owner = *It;
			UPrimitiveComponent* PrimitiveComponent = Owner->FindComponentByClass<UPrimitiveComponent>();
			if (PrimitiveComponent)
			{
				// Check whether primitive can be affected by cull distance volumes.
				if( Owner && ACullDistanceVolume::CanBeAffectedByVolumes( PrimitiveComponent ) )
				{
					// Check whether primitive supports cull distance volumes and its center point is being encompassed by this volume.
					if( EncompassesPoint( Owner->GetActorLocation() ) )
					{		
						// Find best match in CullDistances array.
						float PrimitiveSize			= PrimitiveComponent->Bounds.SphereRadius * 2;
						float CurrentError			= FLT_MAX;
						float CurrentCullDistance	= 0;
						for( int32 CullDistanceIndex=0; CullDistanceIndex<CullDistances.Num(); CullDistanceIndex++ )
						{
							const FCullDistanceSizePair& CullDistancePair = CullDistances[CullDistanceIndex];
							if( FMath::Abs( PrimitiveSize - CullDistancePair.Size ) < CurrentError )
							{
								CurrentError		= FMath::Abs( PrimitiveSize - CullDistancePair.Size );
								CurrentCullDistance = CullDistancePair.CullDistance;
							}
						}

						float* CurrentDistPtr = OutCullDistances.Find(PrimitiveComponent);
						check(CurrentDistPtr);

						// LD or other volume specified cull distance, use minimum of current and one used for this volume.
						if( *CurrentDistPtr > 0 )
						{
							OutCullDistances.Add(PrimitiveComponent, FMath::Min( *CurrentDistPtr, CurrentCullDistance ));
						}
						// LD didn't specify cull distance, use current setting directly.
						else
						{
							OutCullDistances.Add(PrimitiveComponent, CurrentCullDistance);
						}
					}
				}
			}
		}
	}
}
