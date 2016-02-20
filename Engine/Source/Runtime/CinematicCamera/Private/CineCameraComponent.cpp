// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CinematicCameraPrivate.h"
#include "CineCameraComponent.h"

#define LOCTEXT_NAMESPACE "CineCameraComponent"


//////////////////////////////////////////////////////////////////////////
// UCameraComponent

UCineCameraComponent::UCineCameraComponent()
{
	// Super 35mm 4 Perf
	FilmbackSettings.SensorWidth = 24.89f;
	FilmbackSettings.SensorHeight = 18.67;
	LensSettings.MinFocalLength = 50.f;
	LensSettings.MaxFocalLength = 50.f;
	LensSettings.MinFStop = 2.f;
	LensSettings.MaxFStop = 2.f;
	LensSettings.MinimumFocusDistance = 15.f;
	LensSettings.MaxReproductionRatio = 1 / 11.f;

	bConstrainAspectRatio = true;

	RecalcDerivedData();
}

void UCineCameraComponent::PostLoad()
{
	RecalcDerivedData();
	Super::PostLoad();
}

#if WITH_EDITORONLY_DATA
void UCineCameraComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	RecalcDerivedData();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif	// WITH_EDITORONLY_DATA


float UCineCameraComponent::GetHorizontalFieldOfView() const
{
	return (CurrentFocalLength > 0.f)
		? FMath::RadiansToDegrees(2.f * FMath::Atan(FilmbackSettings.SensorWidth / (2.f * CurrentFocalLength)))
		: 0.f;
}

float UCineCameraComponent::GetVerticalFieldOfView() const
{
	return (CurrentFocalLength > 0.f)
		? FMath::RadiansToDegrees(2.f * FMath::Atan(FilmbackSettings.SensorHeight / (2.f * CurrentFocalLength)))
		: 0.f;
}

float UCineCameraComponent::GetWorldToMetersScale() const
{
	UWorld const* const World = GetWorld();
	AWorldSettings const* const WorldSettings = World ? World->GetWorldSettings() : nullptr;
	return WorldSettings ? WorldSettings->WorldToMeters : 100.f;
}

// static
TArray<FNamedFilmbackPreset> const& UCineCameraComponent::GetFilmbackPresets()
{
	return GetDefault<UCineCameraComponent>()->FilmbackPresets;
}

// static
TArray<FNamedLensPreset> const& UCineCameraComponent::GetLensPresets()
{
	return GetDefault<UCineCameraComponent>()->LensPresets;
}

void UCineCameraComponent::RecalcDerivedData()
{
	// respect physical limits of the (simulated) hardware
	CurrentFocalLength = FMath::Clamp(CurrentFocalLength, LensSettings.MinFocalLength, LensSettings.MaxFocalLength);
	CurrentAperture = FMath::Clamp(CurrentAperture, LensSettings.MinFStop, LensSettings.MaxFStop);

	float const MinFocusDistInWorldUnits = LensSettings.MinimumFocusDistance * (GetWorldToMetersScale() / 1000.f);	// convert mm to uu
	FocusSettings.ManualFocusDistance = FMath::Max(FocusSettings.ManualFocusDistance, MinFocusDistInWorldUnits);

	FieldOfView = GetHorizontalFieldOfView();
	FilmbackSettings.SensorAspectRatio = (FilmbackSettings.SensorHeight > 0.f) ? (FilmbackSettings.SensorWidth / FilmbackSettings.SensorHeight) : 0.f;
	AspectRatio = FilmbackSettings.SensorAspectRatio;
}


static const FName NAME_GetDesiredFocusDistance(TEXT("GetDesiredFocusDistance"));
static const float SpotFocusTraceDist = 1000000.f;
static const float FocusDistInfinity = 1000000.f;

float UCineCameraComponent::GetDesiredFocusDistance(FMinimalViewInfo& DesiredView) const
{
	float DesiredFocusDistance = 0.f;

	// get focus distance
	switch (FocusSettings.FocusMethod)
	{
	case ECameraFocusMethod::Manual:
		DesiredFocusDistance = FocusSettings.ManualFocusDistance;
		break;

	case ECameraFocusMethod::Spot:
		{
			// AutoFocus, do a trace
			AActor* const OwningActor = GetOwner();

			// trace to get depth at center of screen
			FCollisionQueryParams TraceParams(NAME_GetDesiredFocusDistance, true, OwningActor);
			FHitResult Hit;

			FVector const TraceStart = DesiredView.Location;
			FVector const TraceEnd = TraceStart + DesiredView.Rotation.Vector() * SpotFocusTraceDist;
			bool const bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams);
			DesiredFocusDistance = bHit ? (TraceStart - Hit.ImpactPoint).Size() : FocusDistInfinity;
		}
		break;

	case ECameraFocusMethod::Tracking:
		{
			AActor const* const TrackedActor = FocusSettings.TrackingFocusSettings.ActorToTrack;

			FVector FocusPoint;
			if (TrackedActor)
			{
				FTransform const BaseTransform = TrackedActor->GetActorTransform();
				FocusPoint = BaseTransform.TransformPosition(FocusSettings.TrackingFocusSettings.RelativeOffset);
			}
			else
			{
				FocusPoint = FocusSettings.TrackingFocusSettings.RelativeOffset;
			}

			DesiredFocusDistance = (FocusPoint - DesiredView.Location).Size();

// 			// try to get the position of the tracked object
// 			UObject* const TrackedObj = FocusSettings.TrackingFocusSettings.ObjectToTrack;
// 
// 			// infinity is the fallback in case none of the other code paths produces a good result
// 			DesiredFocusDistance = FocusDistInfinity;
// 			if (TrackedObj)
// 			{
// 				// we have a socket/bone, see if the object is a scenecomponent
// 				USceneComponent* const TrackedSceneComp = Cast<USceneComponent>(TrackedObj);
// 				if (TrackedSceneComp)
// 				{
// 					if (FocusSettings.TrackingFocusSettings.BoneOrSocketName != NAME_None)
// 					{
// 						FVector const SocketLoc = TrackedSceneComp->GetSocketLocation(FocusSettings.TrackingFocusSettings.BoneOrSocketName);
// 						DesiredFocusDistance = (SocketLoc - DesiredView.Location).Size();
// 					}
// 					else
// 					{
// 						DesiredFocusDistance = (TrackedSceneComp->GetComponentLocation() - DesiredView.Location).Size();
// 					}
// 				}
// 				else
// 				{
// 					AActor* const TrackedActor = Cast<AActor>(TrackedObj);
// 					if (TrackedActor)
// 					{
// 						if (FocusSettings.TrackingFocusSettings.BoneOrSocketName != NAME_None)
// 						{
// 							// user asked for an actor and a bone or socket, try to find a suitable component
// 							TArray<USceneComponent*> OutComponents;
// 							TrackedActor->GetComponents(OutComponents);
// 							for (USceneComponent const* Comp : OutComponents)
// 							{
// 								if (Comp->DoesSocketExist(FocusSettings.TrackingFocusSettings.BoneOrSocketName))
// 								{
// 									FVector const SocketLoc = Comp->GetSocketLocation(FocusSettings.TrackingFocusSettings.BoneOrSocketName);
// 									DesiredFocusDistance = (SocketLoc - DesiredView.Location).Size();
// 									break;
// 								}
// 							}
// 
// 						}
// 						else
// 						{
// 							DesiredFocusDistance = (TrackedActor->GetActorLocation() - DesiredView.Location).Size();
// 						}
// 					}
// 				}
// 			}
		}
		break;
	}
	
	// add in the adjustment offset
	DesiredFocusDistance += FocusSettings.FocusOffset;

	return DesiredFocusDistance;
}

static const FColor DebugFocusPlaneColor(64, 32, 128, 64);		// purple

void UCineCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	RecalcDerivedData();

	Super::GetCameraView(DeltaTime, DesiredView);

	UpdateCameraLens(DeltaTime, DesiredView);

	if (FocusSettings.bDrawDebugFocusPlane)
	{
		FVector const CamDir = DesiredView.Rotation.Vector();
		FVector const FocusPoint = DesiredView.Location + CamDir * CurrentFocusDistance;

		FPlane P(FocusPoint, CamDir);

		::DrawDebugSolidPlane(GetWorld(), P, DesiredView.Location, 10000.f, DebugFocusPlaneColor, false, -1, SDPG_World);
	}
}


void UCineCameraComponent::UpdateCameraLens(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	// Update focus/DoF
	DesiredView.PostProcessBlendWeight = 1.f;
	DesiredView.PostProcessSettings.bOverride_DepthOfFieldMethod = true;
	DesiredView.PostProcessSettings.DepthOfFieldMethod = DOFM_CircleDOF;

	DesiredView.PostProcessSettings.bOverride_DepthOfFieldFstop = true;
	DesiredView.PostProcessSettings.DepthOfFieldFstop = CurrentAperture;

	CurrentFocusDistance = GetDesiredFocusDistance(DesiredView);

	// clamp to min focus distance
	float const MinFocusDistInWorldUnits = LensSettings.MinimumFocusDistance * (GetWorldToMetersScale() / 1000.f);	// convert mm to uu
	CurrentFocusDistance = FMath::Max(CurrentFocusDistance, MinFocusDistInWorldUnits);

	// smoothing, if desired
 	if (FocusSettings.bSmoothFocusChanges)
 	{
		CurrentFocusDistance = FMath::FInterpTo(LastFocusDistance, CurrentFocusDistance, DeltaTime, FocusSettings.FocusSmoothingInterpSpeed);
 	}
	LastFocusDistance = CurrentFocusDistance;

	DesiredView.PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
	DesiredView.PostProcessSettings.DepthOfFieldFocalDistance = CurrentFocusDistance;
}

#undef LOCTEXT_NAMESPACE
