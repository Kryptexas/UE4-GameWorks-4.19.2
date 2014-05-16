// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ComponentVisualizersPrivatePCH.h"
#include "SplineComponentVisualizer.h"

IMPLEMENT_HIT_PROXY(HSplineVisProxy, HComponentVisProxy);
IMPLEMENT_HIT_PROXY(HSplineKeyProxy, HSplineVisProxy);


void FSplineComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const USplineComponent* SplineComp = Cast<const USplineComponent>(Component))
	{
		FColor DrawColor(255,255,255);

		const FInterpCurveVector& SplineInfo = SplineComp->SplineInfo;

		FVector OldKeyPos(0);
		float OldKeyTime = 0.f;
		for (int32 KeyIdx = 0; KeyIdx<SplineInfo.Points.Num(); KeyIdx++)
		{
			float NewKeyTime = SplineInfo.Points[KeyIdx].InVal;
			FVector NewKeyPos = SplineComp->ComponentToWorld.TransformPosition( SplineInfo.Eval(NewKeyTime, FVector::ZeroVector) );

			USplineComponent* EditedSplineComp = GetEditedSplineComponent();
			FColor KeyColor = (SplineComp == EditedSplineComp && KeyIdx == SelectedKeyIndex) ? FColor(255,0,0) : DrawColor;

			// Draw the keypoint
			PDI->SetHitProxy(new HSplineKeyProxy(Component, KeyIdx));

			PDI->DrawPoint(NewKeyPos, KeyColor, 6.f, SDPG_World);

			PDI->SetHitProxy(NULL);


			// If not the first keypoint, draw a line to the last keypoint.
			if (KeyIdx>0)
			{
				// For constant interpolation - don't draw ticks - just draw dotted line.
				if (SplineInfo.Points[KeyIdx - 1].InterpMode == CIM_Constant)
				{
					DrawDashedLine(PDI, OldKeyPos, NewKeyPos, DrawColor, 20, SDPG_World);
				}
				else
				{
					int32 NumSteps = FMath::CeilToInt((NewKeyTime - OldKeyTime) / 0.02f);
					float DrawSubstep = (NewKeyTime - OldKeyTime) / NumSteps;

					// Find position on first keyframe.
					float OldTime = OldKeyTime;
					FVector OldPos = OldKeyPos;

					// Then draw a line for each substep.
					for (int32 StepIdx = 1; StepIdx<NumSteps + 1; StepIdx++)
					{
						float NewTime = OldKeyTime + StepIdx*DrawSubstep;
						FVector NewPos = SplineComp->ComponentToWorld.TransformPosition( SplineInfo.Eval(NewTime, FVector::ZeroVector) );

						PDI->DrawLine(OldPos, NewPos, DrawColor, SDPG_World);

						// Don't draw point for last one - its the keypoint drawn above.
						if (false)// (StepIdx != NumSteps)
						{
							PDI->DrawPoint(NewPos, DrawColor, 3.f, SDPG_World);
						}

						OldTime = NewTime;
						OldPos = NewPos;
					}
				}
			}

			OldKeyTime = NewKeyTime;
			OldKeyPos = NewKeyPos;
		}
	}
}

bool FSplineComponentVisualizer::VisProxyHandleClick(HComponentVisProxy* VisProxy)
{
	bool bEditing = false;

	if(VisProxy && VisProxy->Component.IsValid())
	{
		const USplineComponent* SplineComp = CastChecked<const USplineComponent>(VisProxy->Component.Get());

		SplineCompPropName = GetComponentPropertyName(SplineComp);
		if(SplineCompPropName != NAME_None)
		{
			SplineOwningActor = SplineComp->GetOwner();

			if (VisProxy->IsA(HSplineKeyProxy::StaticGetType()))
			{
				HSplineKeyProxy* KeyProxy = (HSplineKeyProxy*)VisProxy;
				SelectedKeyIndex = KeyProxy->KeyIndex;
				bEditing = true;
			}
		}
		else
		{
			SplineOwningActor = NULL;
		}
	}

	return bEditing;
}

USplineComponent* FSplineComponentVisualizer::GetEditedSplineComponent()
{
	return Cast<USplineComponent>(GetComponentFromPropertyName(SplineOwningActor.Get(), SplineCompPropName));
}


bool FSplineComponentVisualizer::GetWidgetLocation(FVector& OutLocation)
{
	USplineComponent* SplineComp = GetEditedSplineComponent();
	if (SplineComp && SelectedKeyIndex != INDEX_NONE)
	{
		if (SelectedKeyIndex < SplineComp->SplineInfo.Points.Num())
		{
			OutLocation = SplineComp->ComponentToWorld.TransformPosition( SplineComp->SplineInfo.Points[SelectedKeyIndex].OutVal );
			return true;
		}
	}

	return false;
}


bool FSplineComponentVisualizer::HandleInputDelta(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltalRotate, FVector& DeltaScale)
{
	USplineComponent* SplineComp = GetEditedSplineComponent();
	if( SplineComp && 
		SelectedKeyIndex != INDEX_NONE &&
		SelectedKeyIndex < SplineComp->SplineInfo.Points.Num() )
	{
		if (ViewportClient->IsAltPressed() && bAllowDuplication)
		{
			// Duplicate key
			FInterpCurvePoint<FVector> KeyToDupe = SplineComp->SplineInfo.Points[SelectedKeyIndex];
			KeyToDupe.InterpMode = CIM_CurveAuto;
			int32 NewKeyIndex = SplineComp->SplineInfo.Points.Insert(KeyToDupe, SelectedKeyIndex);
			// move selection to 'next' key
			SelectedKeyIndex++;

			// Update Input value for all keys
			SplineComp->RefreshSplineInputs();

			// Don't duplicate again until we release LMB
			bAllowDuplication = false;
		}

		// Find key position in world space
		FVector CurrentWorldPos = SplineComp->ComponentToWorld.TransformPosition(SplineComp->SplineInfo.Points[SelectedKeyIndex].OutVal);
		// Move in world space
		FVector NewWorldPos = CurrentWorldPos + DeltaTranslate;
		// Convert back to local space
		SplineComp->SplineInfo.Points[SelectedKeyIndex].OutVal = SplineComp->ComponentToWorld.InverseTransformPosition(NewWorldPos);

		// Update tangents
		SplineComp->SplineInfo.AutoSetTangents();

		return true;
	}

	return false;
}

bool FSplineComponentVisualizer::HandleInputKey(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	bool bHandled = false;

	if(Key == EKeys::LeftMouseButton && Event == IE_Released)
	{
		// Reset duplication flag on LMB release
		bAllowDuplication = true;
	}
	else if(Key == EKeys::Delete && Event == IE_Pressed)
	{
		// Delete selected key
		USplineComponent* SplineComp = GetEditedSplineComponent();
		if (SplineComp &&
			SelectedKeyIndex != INDEX_NONE &&
			SelectedKeyIndex < SplineComp->SplineInfo.Points.Num())
		{
			SplineComp->SplineInfo.Points.RemoveAt(SelectedKeyIndex);

			SplineComp->RefreshSplineInputs(); // update input value for each key

			SplineComp->SplineInfo.AutoSetTangents(); // update tangents

			SelectedKeyIndex = INDEX_NONE; // deselect any keys

			bHandled = true; // consume key input
		}
	}

	return bHandled;
}


void FSplineComponentVisualizer::EndEditing()
{
	SplineOwningActor = NULL;
	SplineCompPropName = NAME_None;
	SelectedKeyIndex = INDEX_NONE;
}