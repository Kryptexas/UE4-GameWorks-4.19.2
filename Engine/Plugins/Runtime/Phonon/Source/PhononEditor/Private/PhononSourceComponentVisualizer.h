//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "ComponentVisualizer.h"
#include "ShowFlags.h"
#include "SceneView.h"
#include "SceneManagement.h"

namespace Phonon
{
	class FPhononSourceComponentVisualizer : public FComponentVisualizer
	{
	public:
		virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	};
}