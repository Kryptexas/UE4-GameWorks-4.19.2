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

#pragma once

#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "GoogleVRControllerTooltipComponent.generated.h"

class UMotionControllerComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogGoogleVRControllerTooltip, Log, All);

UENUM(BlueprintType)
enum class EGoogleVRControllerTooltipLocation : uint8
{
	TouchPadOutside,
	TouchPadInside,
	AppButtonOutside,
	AppButtonInside,
	None
};

UCLASS(ClassGroup=(GoogleVRController), meta=(BlueprintSpawnableComponent))
class GOOGLEVRCONTROLLER_API UGoogleVRControllerTooltipComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGoogleVRControllerTooltipComponent(const FObjectInitializer& ObjectInitializer);

	/** Determines the location of this tooltip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	EGoogleVRControllerTooltipLocation TooltipLocation;

	/** Text to display for the tooltip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	UTextRenderComponent* TextRenderComponent;

	/** Parameter collection used to set the alpha of the tooltip.
	 *  Must include property named "GoogleVRControllerTooltipAlpha".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialParameterCollection* ParameterCollection;

	/** Called when the tooltip changes sides.
	 *  This happens when the handedness of the user changes.
	 *  Override to update the visual based on which side it is on.
	 */
	virtual void OnSideChanged(bool IsLocationOnLeft);

	/** Blueprint implementable event for when the tooltip changes sides.
	 *  This happens when the handedness of the user changes.
	 *  Override to update the visual based on which side it is on.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "On Side Changed"))
	void ReceiveOnSideChanged(bool IsLocationOnLeft);

	virtual void BeginPlay() override;

	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

private:

	void RefreshTooltipLocation();
	FVector GetRelativeLocation() const;
	bool IsTooltipInside() const;
	bool IsTooltipOnLeft() const;
	float GetWorldToMetersScale() const;

	UMotionControllerComponent* MotionController;
	bool IsOnLeft;
};
