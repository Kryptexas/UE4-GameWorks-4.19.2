//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#include "PhononSourceComponentVisualizer.h"
#include "PhononSourceComponent.h"

namespace Phonon
{
	void FPhononSourceComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
	{
		const UPhononSourceComponent* AttenuatedComponent = Cast<const UPhononSourceComponent>(Component);
		if (AttenuatedComponent != nullptr)
		{
			const FColor OuterRadiusColor(0, 153, 255);
			auto Translation = AttenuatedComponent->GetComponentTransform().GetTranslation();

			DrawWireSphereAutoSides(PDI, Translation, OuterRadiusColor, AttenuatedComponent->BakeRadius * 100, SDPG_World);
		}
	}
}