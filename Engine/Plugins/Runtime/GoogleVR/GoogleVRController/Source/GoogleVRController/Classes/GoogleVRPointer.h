// Copyright 2017 Google Inc.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Interface.h"
#include "Engine/EngineTypes.h"
#include "GoogleVRPointer.generated.h"

UENUM(BlueprintType)
enum class EGoogleVRPointerInputMode : uint8
{
	/**
	*  Sweep a sphere based on the pointer's radius from the camera through the target of the pointer.
	*  This is ideal for reticles that are always rendered on top.
	*  The object that is selected will always be the object that appears
	*  underneath the reticle from the perspective of the camera.
	*  This also prevents the reticle from appearing to "jump" when it starts/stops hitting an object.
	*
	*  Note: This will prevent the user from pointing around an object to hit something that is out of sight.
	*  This isn't a problem in a typical use case.
	*/
	Camera,
	/**
	*  Sweep a sphere based on the pointer's radius directly from the pointer origin.
	*  This is ideal for full-length laser pointers.
	*/
	Direct,
	/*
	*  Default method for casting ray.
	*  Combines the Camera and Direct raycast modes.
	*  Uses a Direct ray up until the CameraRayIntersectionDistance, and then switches to use
	*  a Camera ray starting from the point where the two rays intersect.
	*
	*  This is the most versatile raycast mode. Like Camera mode, this prevents the reticle
	*  appearing jumpy. Additionally, it still allows the user to target objects that are close
	*  to them by using the laser as a visual reference.
	*/
	HybridExperimental
};

/**
 * IGoogleVRPointer is an interface for pointer based input used with UGoogleVRPointerInputComponent.
 */
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class GOOGLEVRCONTROLLER_API UGoogleVRPointer : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class GOOGLEVRCONTROLLER_API IGoogleVRPointer
{
	GENERATED_IINTERFACE_BODY()

public:

	/** Called when the pointer begins hitting an actor. */
	virtual void OnPointerEnter(const FHitResult& HitResult, bool IsHitInteractive) = 0;

	/** Called every frame the pointer is pointing at an actor. */
	virtual void OnPointerHover(const FHitResult& HitResult, bool IsHitInteractive) = 0;

	/** Called when the pointer stops hitting an actor. */
	virtual void OnPointerExit(const FHitResult& HitResult) = 0;

	/** Returns the origin of the ray in world space. */
	virtual FVector GetOrigin() const = 0;

	/** Returns the normalized direction of the ray in world space. */
	virtual FVector GetDirection() const = 0;

	/** Return the radius of the ray. It is used by GoogleVRPointerInputComponent
	 *  when searching for valid targets. If a radius is 0, then
	 *  a ray is used to find a valid target. Otherwise it will use a sphere trace.
	 *  The *EnterRadius* is used for finding new targets while the *ExitRadius*
	 *  is used to see if you are still nearby the object currently selected
	 *  to avoid a flickering effect when just at the border of the intersection.
	 */
	virtual void GetRadius(float& OutEnterRadius, float& OutExitRadius) const = 0;

	/** Returns the max distance this ray will be rendered at from the camera.
	 *  This is used by GoogleVRPointerInputComponent to calculate the ray when using
	 *  the default "Camera" EGoogleVRPointerInputMode. See GoogleVRPointerInputComponent.h for details.
	 */
	virtual float GetMaxPointerDistance() const = 0;

	/** Returns the default distance to render the reticle when no collision is detected. */
	virtual float GetDefaultReticleDistance() const = 0;

	/** Returns true if the pointer is active.
	 *  If the pointer is inactive, then the UGoogleVRPointerInputComponent won't perform hit detection.
	 */
	virtual bool IsPointerActive() const = 0;

	/** Return the method used to detect what the pointer hits. */
	virtual EGoogleVRPointerInputMode GetPointerInputMode() const = 0;
};
