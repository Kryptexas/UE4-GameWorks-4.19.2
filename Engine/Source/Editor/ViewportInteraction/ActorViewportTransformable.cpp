// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ActorViewportTransformable.h"


void FActorViewportTransformable::ApplyTransform( const FTransform& NewTransform, const bool bSweep )
{
	AActor* Actor = ActorWeakPtr.Get();
	if( Actor != nullptr )
	{
		const FTransform& ExistingTransform = Actor->GetActorTransform();
		if( !ExistingTransform.Equals( NewTransform, 0.0f ) )
		{
			GEditor->BroadcastBeginObjectMovement(*Actor);
			const bool bOnlyTranslationChanged =
				ExistingTransform.GetRotation() == NewTransform.GetRotation() &&
				ExistingTransform.GetScale3D() == NewTransform.GetScale3D();

			Actor->SetActorTransform( NewTransform, bSweep );
			//GWarn->Logf( TEXT( "SMOOTH: Actor %s to %s" ), *Actor->GetName(), *Transformable.TargetTransform.ToString() );

			Actor->InvalidateLightingCacheDetailed( bOnlyTranslationChanged );

			const bool bFinished = false;	// @todo gizmo now: PostEditChange never called; and bFinished=true never known!!
			Actor->PostEditMove( bFinished );
			GEditor->BroadcastEndObjectMovement(*Actor);
		}
	}
}


const FTransform FActorViewportTransformable::GetTransform() const
{
	AActor* Actor = ActorWeakPtr.Get();
	if( Actor != nullptr )
	{
		return Actor->GetTransform();
	}
	else
	{
		return FTransform::Identity;
	}
}


FBox FActorViewportTransformable::GetLocalSpaceBoundingBox() const
{
	FBox LocalSpaceBounds( 0 );

	AActor* Actor = Cast<AActor>( ActorWeakPtr.Get() );
	if( Actor != nullptr )
	{
		const bool bIncludeNonCollidingComponents = false;	// @todo gizmo: Disabled this because it causes lights to have huge bounds
		LocalSpaceBounds = Actor->CalculateComponentsBoundingBoxInLocalSpace( bIncludeNonCollidingComponents );
	}
	return LocalSpaceBounds;
}


bool FActorViewportTransformable::IsPhysicallySimulated() const
{
	bool bIsPhysicallySimulated = false;

	AActor* Actor = Cast<AActor>( ActorWeakPtr.Get() );
	if( Actor != nullptr )
	{
		UPrimitiveComponent* RootPrimitiveComponent = Cast<UPrimitiveComponent>( Actor->GetRootComponent() );
		if( RootPrimitiveComponent != nullptr )
		{
			if( RootPrimitiveComponent->IsSimulatingPhysics() )
			{
				bIsPhysicallySimulated = true;
			}
		}
	}

	return bIsPhysicallySimulated;
}


void FActorViewportTransformable::SetLinearVelocity( const FVector& NewVelocity )
{
	AActor* Actor = Cast<AActor>( ActorWeakPtr.Get() );
	if( Actor != nullptr )
	{
		UPrimitiveComponent* RootPrimitiveComponent = Cast<UPrimitiveComponent>( Actor->GetRootComponent() );
		if( RootPrimitiveComponent != nullptr )
		{
			RootPrimitiveComponent->SetAllPhysicsLinearVelocity( NewVelocity );
		}
	}
}


void FActorViewportTransformable::UpdateIgnoredActorList( TArray<AActor*>& IgnoredActors )
{
	AActor* Actor = Cast<AActor>( ActorWeakPtr.Get() );
	if( Actor != nullptr )
	{
		IgnoredActors.Add( Actor );
	}
}
