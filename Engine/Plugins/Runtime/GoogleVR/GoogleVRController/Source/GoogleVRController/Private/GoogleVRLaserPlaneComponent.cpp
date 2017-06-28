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

#include "GoogleVRLaserPlaneComponent.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Math/BoxSphereBounds.h"


UGoogleVRLaserPlaneComponent::UGoogleVRLaserPlaneComponent()

: LaserPlaneLengthParameterName("LaserLength")
, LaserCorrectionParameterName("LaserCorrection")
, CurrentLaserDistance(100.f)
, LaserPlaneMaterial(nullptr)
{

	bAutoActivate = true;

}


void UGoogleVRLaserPlaneComponent::OnRegister()
{
	Super::OnRegister();

	if (UStaticMesh* LaserPlaneMesh = GetStaticMesh())
	{
		LaserPlaneMaterial = UMaterialInstanceDynamic::Create(LaserPlaneMesh->GetMaterial(0), this);
		SetMaterial(0, LaserPlaneMaterial);
	}
}

void UGoogleVRLaserPlaneComponent::UpdateLaserDistance(float Distance)
{
	CurrentLaserDistance = Distance;

	if (LaserPlaneMaterial != nullptr)
	{
		LaserPlaneMaterial->SetScalarParameterValue(LaserPlaneLengthParameterName, Distance);
	}
}

UMaterialInstanceDynamic* UGoogleVRLaserPlaneComponent::GetLaserMaterial() const
{
	return LaserPlaneMaterial;
}

void UGoogleVRLaserPlaneComponent::UpdateLaserCorrection(FVector Correction)
{
	if (LaserPlaneMaterial != nullptr)
	{
		LaserPlaneMaterial->SetVectorParameterValue(LaserCorrectionParameterName, FLinearColor(Correction));
	}
}

FBoxSphereBounds UGoogleVRLaserPlaneComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	float HalfDistance = CurrentLaserDistance * 0.5f;
	return FBoxSphereBounds(FVector(HalfDistance, 0, 0), FVector(HalfDistance, HalfDistance, HalfDistance), HalfDistance).TransformBy(LocalToWorld);
}
