/* Copyright 2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "Classes/GoogleVRWidgetInteractionComponent.h"
#include "GoogleVRController.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/WidgetComponent.h"

UGoogleVRWidgetInteractionComponent::UGoogleVRWidgetInteractionComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGoogleVRWidgetInteractionComponent::UpdateState(const FHitResult& HitResult)
{
	// Make sure that the interaction source is custom.
	InteractionSource = EWidgetInteractionSource::Custom;

	// Set the custom hit result.
	SetCustomHitResult(HitResult);

	// Simulate the pointer movement.
	SimulatePointerMovement();
}

FWidgetPath UGoogleVRWidgetInteractionComponent::FindHoveredWidgetPath(const FWidgetTraceResult& TraceResult) const
{
	if (TraceResult.HitWidgetComponent != nullptr)
	{
		// This does not need to match the radius of the pointer.
		// LastHitResult.ImpactPoint already represents the location that was hit on the widget
		// based upon the actual radius of the pointer.
		// However, when that impact point is at the very edge the widget, GetHitWidgetPath
		// sometimes fails to find a valid path when the radius is zero.
		// To fix this, we set the radius to a small non-zero value.
		const float WidgetCursorRadius = 5.0f;
		return FWidgetPath(TraceResult.HitWidgetComponent->GetHitWidgetPath(TraceResult.LocalHitLocation, /*bIgnoreEnabledStatus*/ false, WidgetCursorRadius));
	}
	else
	{
		return FWidgetPath();
	}
}