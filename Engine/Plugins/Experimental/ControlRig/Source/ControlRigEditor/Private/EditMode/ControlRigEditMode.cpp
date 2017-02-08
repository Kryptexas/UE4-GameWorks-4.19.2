// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ControlRigEditMode.h"
#include "ControlRigEditModeToolkit.h"
#include "Toolkits/ToolkitManager.h"
#include "SControlRigEditModeTools.h"
#include "Containers/Algo/Transform.h"
#include "ControlRig.h"
#include "HitProxies.h"
#include "HierarchicalRig.h"
#include "HumanRig.h"
#include "ControlRigEditModeSettings.h"
#include "ISequencer.h"
#include "ControlRigSequence.h"
#include "ControlRigBindingTrack.h"
#include "Sections/MovieSceneSpawnSection.h"
#include "MovieScene.h"
#include "EditorViewportClient.h"
#include "EditorModeManager.h"

FName FControlRigEditMode::ModeName("EditMode.ControlRig");

#define LOCTEXT_NAMESPACE "ControlRigEditMode"

/** Base class for ControlRig hit proxies */
struct HControlRigProxy : public HHitProxy
{
	DECLARE_HIT_PROXY();

	HControlRigProxy(UControlRig* InControlRig)
		: HHitProxy(HPP_Wireframe)
		, ControlRig(InControlRig)
	{}

	virtual EMouseCursor::Type GetMouseCursor() override
	{
		return EMouseCursor::Crosshairs;
	}

	TWeakObjectPtr<UControlRig> ControlRig;
};

IMPLEMENT_HIT_PROXY(HControlRigProxy, HHitProxy);

/** Proxy for a manipulator */
struct HManipulatorNodeProxy : public HControlRigProxy
{
	DECLARE_HIT_PROXY();

	HManipulatorNodeProxy(UControlRig* InControlRig, FName InNodeName)
		: HControlRigProxy(InControlRig)
		, NodeName(InNodeName)
	{}

	FName NodeName;
};

IMPLEMENT_HIT_PROXY(HManipulatorNodeProxy, HControlRigProxy);

FControlRigEditMode::FControlRigEditMode()
	: SelectedNode(NAME_None)
	, bIsTransacting(false)
	, bSelectedNode(false)
{
	Settings = NewObject<UControlRigEditModeSettings>(GetTransientPackage(), *LOCTEXT("SettingsName", "Settings").ToString());
	Settings->AddToRoot();
}

FControlRigEditMode::~FControlRigEditMode()
{
	Settings->RemoveFromRoot();
}

void FControlRigEditMode::SetSequencer(TSharedPtr<ISequencer> InSequencer)
{
	Settings->Sequence = nullptr;

	WeakSequencer = InSequencer;
	StaticCastSharedPtr<SControlRigEditModeTools>(Toolkit->GetInlineContent())->SetSequencer(InSequencer);
	if (InSequencer.IsValid())
	{
		if (UControlRigSequence* Sequence = ExactCast<UControlRigSequence>(InSequencer->GetFocusedMovieSceneSequence()))
		{
			Settings->Sequence = Sequence;
		}
	}
}

void FControlRigEditMode::SetObjects(const TArray<TWeakObjectPtr<>>& InSelectedObjects, const TArray<FGuid>& InObjectBindings)
{
	ControlRigs.Reset();

	check(InSelectedObjects.Num() == InObjectBindings.Num());

	ControlRigGuids = InObjectBindings;
	Algo::Transform(InSelectedObjects, ControlRigs, [](TWeakObjectPtr<> Object) { return TWeakObjectPtr<UControlRig>(Cast<UControlRig>(Object.Get())); });

	SetObjects_Internal();
}

void FControlRigEditMode::SetObjects_Internal()
{
	TArray<TWeakObjectPtr<>> SelectedObjects;
	Algo::Transform(ControlRigs, SelectedObjects, [](TWeakObjectPtr<> Object) { return TWeakObjectPtr<>(Object.Get()); });
	SelectedObjects.Insert(Settings, 0);

	if (ControlRigs.Num() > 0 && ControlRigs[0].IsValid())
	{
		if (AActor* HostingActor = ControlRigs[0]->GetHostingActor())
		{
			// @TODO: handle multi-select better here?
			Settings->Actor = HostingActor;
		}
	}

	StaticCastSharedPtr<SControlRigEditModeTools>(Toolkit->GetInlineContent())->SetDetailsObjects(SelectedObjects);
}

void FControlRigEditMode::HandleBindToActor(AActor* InActor)
{
	if (WeakSequencer.IsValid())
	{
		TSharedRef<ISequencer> Sequencer = WeakSequencer.Pin().ToSharedRef();

		UControlRigBindingTrack::SetObjectBinding(InActor);

		// Modify the sequence
		if (UControlRigSequence* Sequence = ExactCast<UControlRigSequence>(Sequencer->GetFocusedMovieSceneSequence()))
		{
			Sequence->Modify();

			// Also modify the binding tracks in the sequence, so bindings get regenerated to this actor
			UMovieScene* MovieScene = Sequence->GetMovieScene();
			for (UMovieSceneSection* Section : MovieScene->GetAllSections())
			{
				if (UMovieSceneSpawnSection* SpawnSection = Cast<UMovieSceneSpawnSection>(Section))
				{
					SpawnSection->TryModify();
				}
			}

			// now notify the sequence (will rebind when it re-evaluates)
			Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::TrackValueChangedRefreshImmediately);

			// Select the first spawnable to expose our animation UI
			if (InActor)
			{
				if (MovieScene->GetSpawnableCount() > 0)
				{
					Sequencer->SelectObject(MovieScene->GetSpawnable(0).GetGuid());
				}
			}
		}
	}
}

bool FControlRigEditMode::UsesToolkits() const
{
	return true;
}

void FControlRigEditMode::Enter()
{
	// Call parent implementation
	FEdMode::Enter();

	if (!Toolkit.IsValid())
	{
		Toolkit = MakeShareable(new FControlRigEditModeToolkit);
	}

	Toolkit->Init(Owner->GetToolkitHost());

	SetObjects_Internal();
}

void FControlRigEditMode::Exit()
{
	if (bIsTransacting)
	{
		GEditor->EndTransaction();
		bIsTransacting = false;
	}

	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
	}

	// Call parent implementation
	FEdMode::Exit();
}

static ETransformComponent WidgetModeToTransformComponent(FWidget::EWidgetMode WidgetMode)
{
	switch (WidgetMode)
	{
	case FWidget::WM_Translate:
		return ETransformComponent::Translation;
	case FWidget::WM_Rotate:
		return ETransformComponent::Rotation;
	case FWidget::WM_Scale:
		return ETransformComponent::Scale;
	case FWidget::WM_2D:
	case FWidget::WM_TranslateRotateZ:
	default:
		return ETransformComponent::None;
	}
}

void FControlRigEditMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	FEdMode::Tick(ViewportClient, DeltaTime);

	if (bSelectedNode)
	{
		if (ControlRigs.Num() > 0 && ControlRigs[0].Get())
		{
			if (UHierarchicalRig* HierarchicalRig = Cast<UHierarchicalRig>(ControlRigs[0].Get()))
			{
				UControlManipulator* Manipulator = HierarchicalRig->FindManipulator(SelectedNode);
				if (Manipulator)
				{
					// cycle the widget mode if it is not supported on this selection
					if (!Manipulator->SupportsTransformComponent(WidgetModeToTransformComponent(GetModeManager()->GetWidgetMode())))
					{
						GetModeManager()->CycleWidgetMode();
						ViewportClient->Invalidate();
					}
				}
			}
		}
		
		bSelectedNode = false;
	}
}

FTransform GetParentTransform(UControlManipulator* Manipulator, UHierarchicalRig* HierarchicalRig)
{
	if (Manipulator->bInLocalSpace)
	{
		const FAnimationHierarchy& Hierarchy = HierarchicalRig->GetHierarchy();
		int32 NodeIndex = Hierarchy.GetNodeIndex(Manipulator->Name);
		if (NodeIndex != INDEX_NONE)
		{
			FName ParentName = Hierarchy.GetParentName(NodeIndex);
			if (ParentName != NAME_None)
			{
				return HierarchicalRig->GetMappedGlobalTransform(ParentName);
			}
		}
	}

	return FTransform::Identity;
}

void FControlRigEditMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	for(TWeakObjectPtr<UControlRig> ControlRig : ControlRigs)
	{
		if (UHierarchicalRig* HierarchicalRig = Cast<UHierarchicalRig>(ControlRig.Get()))
		{
			// now get all node data
			const FAnimationHierarchy& Hierarchy = HierarchicalRig->GetHierarchy();
			const TArray<FNodeObject>& NodeObjects = Hierarchy.GetNodes();

			AActor* Actor = HierarchicalRig->GetHostingActor();

			const FColor NormalColor = FColor(255, 255, 255, 255);
			const FColor SelectedColor = FColor(255, 0, 255, 255);
			const float GrabHandleSize = 5.0f;

			const FTransform RootTransform = Actor ? Actor->GetActorTransform() : FTransform::Identity;

			// each hierarchy node
			for (int32 NodeIndex = 0; NodeIndex < NodeObjects.Num(); ++NodeIndex)
			{
				const FNodeObject& CurrentNode = NodeObjects[NodeIndex];
				const FVector Location = RootTransform.TransformPosition(HierarchicalRig->GetMappedGlobalTransform(CurrentNode.Name).GetLocation());
				if (CurrentNode.ParentName != NAME_None)
				{
					const FVector ParentLocation = RootTransform.TransformPosition(HierarchicalRig->GetMappedGlobalTransform(CurrentNode.ParentName).GetLocation());
					PDI->DrawLine(Location, ParentLocation, SelectedColor, SDPG_Foreground);
				}

				PDI->DrawPoint(Location, NormalColor, GrabHandleSize, SDPG_Foreground);
			}

			// Draw each manipulator
			for (UControlManipulator* Manipulator : HierarchicalRig->Manipulators)
			{
				if (Manipulator && HierarchicalRig->IsManipulatorEnabled(Manipulator))
				{
					PDI->SetHitProxy(new HManipulatorNodeProxy(HierarchicalRig, Manipulator->Name));
					FTransform ManipulatorTransform = Manipulator->GetTransform(HierarchicalRig);
					FTransform ParentTransform = GetParentTransform(Manipulator, HierarchicalRig);
					FTransform DisplayTransform = ManipulatorTransform*ParentTransform*RootTransform;
					Manipulator->Draw(DisplayTransform, View, PDI, Manipulator->Name == SelectedNode);
					PDI->SetHitProxy(nullptr);
				}
			}
		}
	}
}

bool FControlRigEditMode::InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent)
{
	return false;
}

bool FControlRigEditMode::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if (bIsTransacting)
	{
		GEditor->EndTransaction();
		bIsTransacting = false;
		return true;
	}

	return false;
}

bool FControlRigEditMode::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if (!bIsTransacting)
	{
		for (TWeakObjectPtr<UControlRig>& ControlRig : ControlRigs)
		{
			if (UHierarchicalRig* HierarchicalRig = Cast<UHierarchicalRig>(ControlRig.Get()))
			{
				GEditor->BeginTransaction(LOCTEXT("MoveManipulatorTransaction", "Move Manipulator"));
				HierarchicalRig->SetFlags(RF_Transactional);
				HierarchicalRig->Modify();
				bIsTransacting = true;
			}
		}

		return bIsTransacting;
	}

	return false;
}

bool FControlRigEditMode::UsesTransformWidget() const
{
	return (SelectedNode != NAME_None);
}

bool FControlRigEditMode::UsesTransformWidget(FWidget::EWidgetMode CheckMode) const
{
	if(ControlRigs.Num() > 0 && ControlRigs[0].Get())
	{
		if (UHierarchicalRig* HierarchicalRig = Cast<UHierarchicalRig>(ControlRigs[0].Get()))
		{
			if (SelectedNode != NAME_None)
			{
				UControlManipulator* Manipulator = HierarchicalRig->FindManipulator(SelectedNode);
				if (Manipulator)
				{
					return Manipulator->SupportsTransformComponent(WidgetModeToTransformComponent(CheckMode));
				}
			}
		}
	}

	return false;
}

FVector FControlRigEditMode::GetWidgetLocation() const
{
	if (ControlRigs.Num() > 0 && ControlRigs[0].Get())
	{
		if (UHierarchicalRig* HierarchicalRig = Cast<UHierarchicalRig>(ControlRigs[0].Get()))
		{
			if (SelectedNode != NAME_None)
			{
				AActor* HostingActor = HierarchicalRig->GetHostingActor();
				FTransform ActorTransform = HostingActor ? HostingActor->GetActorTransform() : FTransform::Identity;
				return ActorTransform.TransformPosition(HierarchicalRig->GetGlobalLocation(SelectedNode));
			}
		}
	}

	return FVector::ZeroVector;
}

bool FControlRigEditMode::GetCustomDrawingCoordinateSystem(FMatrix& OutMatrix, void* InData)
{
	if (ControlRigs.Num() > 0 && ControlRigs[0].Get())
	{
		if (UHierarchicalRig* HierarchicalRig = Cast<UHierarchicalRig>(ControlRigs[0].Get()))
		{
			if (SelectedNode != NAME_None)
			{
				AActor* HostingActor = HierarchicalRig->GetHostingActor();
				FTransform ActorTransform = HostingActor ? HostingActor->GetActorTransform() : FTransform::Identity;
				FTransform WorldTransform = HierarchicalRig->GetMappedGlobalTransform(SelectedNode) * ActorTransform;

				OutMatrix = WorldTransform.ToMatrixNoScale().RemoveTranslation();

				return true;
			}
		}
	}

	return false;
}

bool FControlRigEditMode::GetCustomInputCoordinateSystem(FMatrix& OutMatrix, void* InData)
{
	return GetCustomDrawingCoordinateSystem(OutMatrix, InData);
}

bool FControlRigEditMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy *HitProxy, const FViewportClick &Click)
{
	if (HitProxy && HitProxy->IsA(HManipulatorNodeProxy::StaticGetType()))
	{
		HManipulatorNodeProxy* NodeProxy = static_cast<HManipulatorNodeProxy*>(HitProxy);
		SetNodeSelection(NodeProxy->NodeName);
		return true;
	}

	// clear Selected Node
	SetNodeSelection(NAME_None);

	return false;
}

bool FControlRigEditMode::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (ControlRigs.Num() > 0 && ControlRigs[0].Get())
	{
		FVector Drag = InDrag;
		FRotator Rot = InRot;
		FVector Scale = InScale;

		const bool bCtrlDown = InViewport->KeyState(EKeys::LeftControl) || InViewport->KeyState(EKeys::RightControl);
		const bool bShiftDown = InViewport->KeyState(EKeys::LeftShift) || InViewport->KeyState(EKeys::RightShift);
		const bool bAltDown = InViewport->KeyState(EKeys::LeftAlt) || InViewport->KeyState(EKeys::RightAlt);
		const bool bMouseButtonDown = InViewport->KeyState(EKeys::LeftMouseButton);

		const FWidget::EWidgetMode WidgetMode = InViewportClient->GetWidgetMode();
		const ECoordSystem CoordSystem = InViewportClient->GetWidgetCoordSystemSpace();

		if (UHierarchicalRig* HierarchicalRig = Cast<UHierarchicalRig>(ControlRigs[0].Get()))
		{
			if (SelectedNode != NAME_None && bIsTransacting && bMouseButtonDown && !bCtrlDown && !bShiftDown && !bAltDown)
			{
				const bool bDoRotation = !Rot.IsZero() && (WidgetMode == FWidget::WM_Rotate || WidgetMode == FWidget::WM_TranslateRotateZ);
				const bool bDoTranslation = !Drag.IsZero() && (WidgetMode == FWidget::WM_Translate || WidgetMode == FWidget::WM_TranslateRotateZ);
				const bool bDoScale = !Scale.IsZero() && WidgetMode == FWidget::WM_Scale;

				// manipulator transform is always on actor base - (actor origin being 0)
				UControlManipulator* Manipulator = HierarchicalRig->FindManipulator(SelectedNode);
				if (Manipulator)
				{
					AActor* HostingActor = HierarchicalRig->GetHostingActor();
					FTransform ActorTransform = HostingActor ? HostingActor->GetActorTransform() : FTransform::Identity;
					FTransform NewTransform = HierarchicalRig->GetMappedGlobalTransform(SelectedNode);
					if (bDoRotation)
					{
						if (CoordSystem == COORD_Local)
						{
							FQuat CurrentRotation = NewTransform.GetRotation();
							CurrentRotation = (Rot.Quaternion() * CurrentRotation);
							NewTransform.SetRotation(CurrentRotation);
						}
						else
						{
							FQuat CurrentRotation = NewTransform.GetRotation();
							FQuat ActorRotation = ActorTransform.GetRotation();
							FQuat ManipulatorLocalRotation = ActorRotation.Inverse() * Rot.Quaternion() * CurrentRotation * ActorRotation;
							NewTransform.SetRotation(ManipulatorLocalRotation);
						}
					}

					if (bDoTranslation)
					{
						if (CoordSystem == COORD_Local)
						{
							FVector ManipulatorLocation = NewTransform.GetLocation();;
							ManipulatorLocation = ManipulatorLocation + Drag;
							NewTransform.SetLocation(ManipulatorLocation);
						}
						else
						{
							FVector ManipulatorWorldLocation = ActorTransform.TransformPosition(NewTransform.GetLocation());
							ManipulatorWorldLocation = ManipulatorWorldLocation + Drag;
							NewTransform.SetLocation(ActorTransform.InverseTransformPosition(ManipulatorWorldLocation));
						}
					}

					if (bDoScale)
					{
						if (CoordSystem == COORD_Local)
						{
							FVector ManipulatorScale = NewTransform.GetScale3D();
							ManipulatorScale = ManipulatorScale + Scale;
							NewTransform.SetScale3D(ManipulatorScale);
						}
						else
						{
							FVector ManipulatorWorldScale = ActorTransform.TransformVector(NewTransform.GetScale3D());
							ManipulatorWorldScale = ManipulatorWorldScale + Scale;
							NewTransform.SetScale3D(ActorTransform.TransformVector(ManipulatorWorldScale));
						}
					}

					if (bDoRotation || bDoTranslation || bDoScale)
					{
						HierarchicalRig->SetMappedGlobalTransform(SelectedNode, NewTransform);
						
						if (Manipulator->bInLocalSpace)
						{
							FTransform ParentTransform = GetParentTransform(Manipulator, HierarchicalRig);
							Manipulator->SetTransform(NewTransform.GetRelativeTransform(ParentTransform), HierarchicalRig);
						}
						else
						{
							Manipulator->SetTransform(NewTransform, HierarchicalRig);
						}
				
						// have to update manipulator to node when children modifies from set global transform
						HierarchicalRig->UpdateManipulatorToNode();
					}
				}

				return true;
			}
		}
	}

	return false;
}

bool FControlRigEditMode::ShouldDrawWidget() const
{
	return SelectedNode != NAME_None;
}

bool FControlRigEditMode::IsCompatibleWith(FEditorModeID OtherModeID) const
{
	if (OtherModeID == FBuiltinEditorModes::EM_Placement)
	{
		return false;
	}
	return true;
}

void FControlRigEditMode::SetNodeSelection(FName NodeName)
{
	SelectedNode = NodeName;
	bSelectedNode = true;
}

FName FControlRigEditMode::GetNodeSelection() const
{
	return SelectedNode;
}

void FControlRigEditMode::HandleObjectSpawned(FGuid InObjectBinding, UObject* SpawnedObject)
{
	// check if the object is being displayed currently
	check(ControlRigs.Num() == ControlRigGuids.Num());
	for (int32 ObjectIndex = 0; ObjectIndex < ControlRigGuids.Num(); ObjectIndex++)
	{
		if (ControlRigGuids[ObjectIndex] == InObjectBinding)
		{
			ControlRigs[ObjectIndex] = Cast<UControlRig>(SpawnedObject);
			SetObjects_Internal();
			return;
		}
	}
}

#undef LOCTEXT_NAMESPACE