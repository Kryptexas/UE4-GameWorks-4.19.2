/* Copyright 2017 Google Inc.

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



#include "Components/ActorComponent.h"

#include "Components/StaticMeshComponent.h"

#include "GoogleVRLaserPlaneComponent.generated.h"



DEFINE_LOG_CATEGORY_STATIC(LogGoogleVRLaserPlaneComponent, Log, All);



/**

 * UGoogleVRLaserPlaneComponent is a helper class used to render the targeting laser.

 * It maintains a dynamic material instance used to customize the visual appearance of

 * the targeting laser, and overrides CalcBounds() to place it's bounding sphere around

 * the rendered geometry.  The extrusion/billboarding happens in the vertex shader on

 * the mesh material.

 */

UCLASS(ClassGroup=(GoogleVRController))

class GOOGLEVRCONTROLLER_API UGoogleVRLaserPlaneComponent : public UStaticMeshComponent

{

	GENERATED_BODY()


public:

	UGoogleVRLaserPlaneComponent();



	/** Material parameter name for controlling laser length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	FName LaserPlaneLengthParameterName;

	/** Material parameter name for controlling laser correction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	FName LaserCorrectionParameterName;

	void UpdateLaserDistance(float Distance);
	void UpdateLaserCorrection(FVector Correction);
	UMaterialInstanceDynamic* GetLaserMaterial() const;

	/** ActorComponent Overrides */
	virtual void OnRegister() override;

	/** SceneComponent Overrides */
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;


private:



	float CurrentLaserDistance;

	UMaterialInstanceDynamic* LaserPlaneMaterial;

};