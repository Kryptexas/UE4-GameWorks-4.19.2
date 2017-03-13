// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ControlRigComponentVisualizer.h"
#include "ControlRigComponent.h"
#include "HierarchicalRig.h"
#include "ActorEditorUtils.h"
#include "SceneManagement.h"
#include "EditorViewportClient.h"
#include "Editor.h"

IMPLEMENT_HIT_PROXY(HControlVisProxy, HComponentVisProxy);
IMPLEMENT_HIT_PROXY(HControlNodeProxy, HControlVisProxy);

#define LOCTEXT_NAMESPACE "ControlRigComponentVisualizer"

FControlRigComponentVisualizer::FControlRigComponentVisualizer()
	: FComponentVisualizer()
	, SelectedNode(NAME_None)
	, bIsTransacting(false)
{
}

static float GetDashSize(const FSceneView* View, const FVector& Start, const FVector& End, float Scale)
{
	const float StartW = View->WorldToScreen(Start).W;
	const float EndW = View->WorldToScreen(End).W;

	const float WLimit = 10.0f;
	if (StartW > WLimit || EndW > WLimit)
	{
		return FMath::Max(StartW, EndW) * Scale;
	}

	return 0.0f;
}

void FControlRigComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const UControlRigComponent* ControlComp = Cast<const UControlRigComponent>(Component))
	{
		if (const UHierarchicalRig* ControlRig = ControlComp->GetControlRig<UHierarchicalRig>())
		{
			// now get all node data
			const TArray<FNodeObject>& NodeObjects = ControlRig->GetHierarchy().GetNodes();

			AActor* Owner = ControlComp->GetOwner();

			const FColor NormalColor = FColor(255, 255, 255, 255);
			const FColor SelectedColor = FColor(255, 0, 255, 255);
			const float GrabHandleSize = 12.0f;

			const FTransform RootTransform = Owner->GetActorTransform();

			// each hierarchy node
			for (int32 NodeIndex = 0; NodeIndex < NodeObjects.Num(); ++NodeIndex)
			{
				const FNodeObject& CurrentNode = NodeObjects[NodeIndex];
				const FVector Location = RootTransform.TransformPosition(ControlRig->GetMappedGlobalTransform(CurrentNode.Name).GetLocation());
				if (CurrentNode.ParentName != NAME_None)
				{
					const FVector ParentLocation = RootTransform.TransformPosition(ControlRig->GetMappedGlobalTransform(CurrentNode.ParentName).GetLocation());

					PDI->SetHitProxy(nullptr);

					const float DashSize = GetDashSize(View, Location, ParentLocation, 0.01f);
					if (DashSize > 0.0f)
					{
						DrawDashedLine(PDI, Location, ParentLocation, SelectedColor, DashSize, SDPG_Foreground);
					}
				}

				PDI->SetHitProxy(new HControlNodeProxy(Component, CurrentNode.Name));
				PDI->DrawPoint(Location, (CurrentNode.Name == SelectedNode) ? SelectedColor : NormalColor, GrabHandleSize, SDPG_Foreground);
				PDI->SetHitProxy(nullptr);
			}

			// Draw each manipulator
			for (UControlManipulator* Manipulator : ControlRig->Manipulators)
			{
				if (Manipulator)
				{
					PDI->SetHitProxy(new HControlNodeProxy(Component, Manipulator->Name));
					Manipulator->Draw(Manipulator->GetTransform(ControlRig) * RootTransform, View, PDI, Manipulator->Name == SelectedNode);
					PDI->SetHitProxy(nullptr);
				}
			}
		}
	}
}

bool FControlRigComponentVisualizer::VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click)
{
	if(VisProxy && VisProxy->Component.IsValid())
	{
		const UControlRigComponent* ControlComp = CastChecked<const UControlRigComponent>(VisProxy->Component.Get());
		if (const UHierarchicalRig* ControlRig = ControlComp->GetControlRig<UHierarchicalRig>())
		{
			ControlCompPropName = GetComponentPropertyName(ControlComp);
			if (ControlCompPropName.IsValid())
			{
				AActor* OldControlOwningActor = ControlOwningActor.Get();
				ControlOwningActor = ControlComp->GetOwner();

				if (OldControlOwningActor != ControlOwningActor)
				{
					// Reset selection state if we are selecting a different actor to the one previously selected
					SelectedNode = NAME_None;
				}

				if (VisProxy->IsA(HControlNodeProxy::StaticGetType()))
				{
					// Control point clicked
					HControlNodeProxy* NodeProxy = (HControlNodeProxy*)VisProxy;

					// Modify the selection state, unless right-clicking on an already selected key
					if (SelectedNode != NodeProxy->NodeName)
					{
						SelectedNode = NodeProxy->NodeName;
					}
					return true;
				}
			}
			else
			{
				ControlOwningActor = nullptr;
			}
		}
		else
		{
			ControlOwningActor = nullptr;
		}
	}
	else
	{
		ControlOwningActor = nullptr;
	}

	return false;
}


UControlRigComponent* FControlRigComponentVisualizer::GetEditedControlRigComponent() const
{
	return Cast<UControlRigComponent>(GetComponentFromPropertyName(ControlOwningActor.Get(), ControlCompPropName));
}

bool FControlRigComponentVisualizer::GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const
{
	UControlRigComponent* ControlComp = GetEditedControlRigComponent();
	if (ControlComp != nullptr)
	{
		if (SelectedNode != NAME_None)
		{
			if (const UHierarchicalRig* ControlRig = ControlComp->GetControlRig<UHierarchicalRig>())
			{
				FTransform CompTransform = ControlComp->GetOwner() ? ControlComp->GetOwner()->GetActorTransform() : FTransform::Identity;
				FTransform WorldTransform = ControlRig->GetMappedGlobalTransform(SelectedNode) * CompTransform;

				OutLocation = WorldTransform.GetLocation();

				return true;
			}
		}
	}

	return false;
}


bool FControlRigComponentVisualizer::GetCustomInputCoordinateSystem(const FEditorViewportClient* ViewportClient, FMatrix& OutMatrix) const
{
	UControlRigComponent* ControlComp = GetEditedControlRigComponent();
	if (ControlComp != nullptr)
	{
		if (SelectedNode != NAME_None)
		{
			if (const UHierarchicalRig* ControlRig = ControlComp->GetControlRig<UHierarchicalRig>())
			{
				if (ViewportClient->GetWidgetCoordSystemSpace() == COORD_Local)
				{
					FTransform LocalTransform = ControlRig->GetLocalTransform(SelectedNode);
					OutMatrix = LocalTransform.ToMatrixNoScale().RemoveTranslation();
				}
				else if (ViewportClient->GetWidgetCoordSystemSpace() == COORD_World)
				{
					FTransform CompTransform = ControlComp->GetOwner() ? ControlComp->GetOwner()->GetActorTransform() : FTransform::Identity;
					FTransform WorldTransform = ControlRig->GetMappedGlobalTransform(SelectedNode) * CompTransform;

					OutMatrix = WorldTransform.ToMatrixNoScale().RemoveTranslation();
				}

				return true;
			}
		}
	}

	return false;
}


bool FControlRigComponentVisualizer::IsVisualizingArchetype() const
{
	UControlRigComponent* ControlComp = GetEditedControlRigComponent();
	return (ControlComp && ControlComp->GetOwner() && FActorEditorUtils::IsAPreviewOrInactiveActor(ControlComp->GetOwner()));
}


bool FControlRigComponentVisualizer::HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	UControlRigComponent* ControlComp = GetEditedControlRigComponent();
	if (ControlComp != nullptr)
	{
		if (UHierarchicalRig* ControlRig = ControlComp->GetControlRig<UHierarchicalRig>())
		{
			if (Key == EKeys::LeftMouseButton)
			{
				if (Event == EInputEvent::IE_Pressed)
				{
					if (!bIsTransacting)
					{
						GEditor->BeginTransaction(LOCTEXT("MoveManipulatorTransaction", "Move Manipulator"));
						ControlRig->SetFlags(RF_Transactional);
						ControlRig->Modify();
						bIsTransacting = true;
					}
				}
				else if (Event == EInputEvent::IE_Released)
				{
					if (bIsTransacting)
					{
						GEditor->EndTransaction();
						bIsTransacting = false;
					}
				}
			}
		}
	}

	return false;
}


bool FControlRigComponentVisualizer::HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale)
{
	const FWidget::EWidgetMode WidgetMode = ViewportClient->GetWidgetMode();
	const ECoordSystem CoordSystem = ViewportClient->GetWidgetCoordSystemSpace();

	UControlRigComponent* ControlComp = GetEditedControlRigComponent();
	if (ControlComp != nullptr)
	{
		if (SelectedNode != NAME_None && bIsTransacting)
		{
			if (UHierarchicalRig* ControlRig = ControlComp->GetControlRig<UHierarchicalRig>())
			{
				const bool bDoRotation = !DeltaRotate.IsZero() && (WidgetMode == FWidget::WM_Rotate || WidgetMode == FWidget::WM_TranslateRotateZ);
				const bool bDoTranslation = !DeltaTranslate.IsZero() && (WidgetMode == FWidget::WM_Translate || WidgetMode == FWidget::WM_TranslateRotateZ);
				const bool bDoScale = !DeltaScale.IsZero() && WidgetMode == FWidget::WM_Scale;

				FTransform CompTransform = ControlComp->GetOwner() ? ControlComp->GetOwner()->GetActorTransform() : FTransform::Identity;
				UControlManipulator* Manipulator = ControlRig->FindManipulator(SelectedNode);
				if (Manipulator)
				{
					if (bDoRotation)
					{
						FRotator ManipulatorRotation = Manipulator->GetRotation(ControlRig);
						ManipulatorRotation = (ManipulatorRotation.Quaternion() * DeltaRotate.Quaternion()).Rotator();
						Manipulator->SetRotation(ManipulatorRotation, ControlRig);
					}

					if (bDoTranslation)
					{
						FVector ManipulatorLocation = Manipulator->GetLocation(ControlRig);
						ManipulatorLocation = ManipulatorLocation + DeltaTranslate;
						Manipulator->SetLocation(ManipulatorLocation, ControlRig);
					}

					if (bDoScale)
					{
						FVector ManipulatorScale = Manipulator->GetScale(ControlRig);
						ManipulatorScale = ManipulatorScale + DeltaScale;
						Manipulator->SetScale(ManipulatorScale, ControlRig);
					}

					if (bDoRotation || bDoTranslation || bDoScale)
					{
						ControlRig->SetMappedGlobalTransform(SelectedNode, Manipulator->GetTransform(ControlRig));

						// copy back the node transform, that way hierarchical node can update their transform back
						ControlRig->UpdateManipulatorToNode();
					}
				}
			}
		}

		return true;
	}

	return false;
}

void FControlRigComponentVisualizer::EndEditing()
{
	ControlOwningActor = nullptr;
	ControlCompPropName.Clear();
	SelectedNode = NAME_None;
	if (bIsTransacting)
	{
		GEditor->EndTransaction();
		bIsTransacting = false;
	}
}

#undef LOCTEXT_NAMESPACE
