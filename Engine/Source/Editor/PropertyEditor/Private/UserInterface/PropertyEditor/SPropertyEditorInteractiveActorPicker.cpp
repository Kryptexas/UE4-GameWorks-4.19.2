// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "PropertyEditorPrivatePCH.h"
#include "SPropertyEditorInteractiveActorPicker.h"
#include "PropertyEditor.h"
#include "PropertyNode.h"
#include "ObjectPropertyNode.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/LevelEditor/Public/ILevelViewport.h"

#define LOCTEXT_NAMESPACE "PropertyPicker"

namespace EPickState
{
	enum Type
	{
		NotOverViewport,
		OverViewport,
		OverIncompatibleActor,
		OverActor,
	};
}

/**
 * Editor mode used to interactively pick actors in the level editor viewports.
 */
struct FEdModePicker : public FEdMode
{
	FEdModePicker()
	{
		ID = FBuiltinEditorModes::EM_ActorPicker;
		PickState = EPickState::NotOverViewport;
		HoveredActor.Reset();
	}
	
	/** Begin FEdMode interface */
	virtual void Tick(FLevelEditorViewportClient* ViewportClient, float DeltaTime) OVERRIDE
	{
		if(CursorDecoratorWindow.IsValid())
		{
			CursorDecoratorWindow->MoveWindowTo(FSlateApplication::Get().GetCursorPos() + FSlateApplication::Get().GetCursorSize());
		}

		FEdMode::Tick(ViewportClient, DeltaTime);
	}

	virtual bool MouseEnter(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport,int32 x, int32 y) OVERRIDE
	{
		PickState = EPickState::OverViewport;
		HoveredActor.Reset();
		return FEdMode::MouseEnter(ViewportClient, Viewport, x, y);
	}

	virtual bool MouseLeave(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport) OVERRIDE
	{
		PickState = EPickState::NotOverViewport;
		HoveredActor.Reset();
		return FEdMode::MouseLeave(ViewportClient, Viewport);
	}

	virtual bool MouseMove(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) OVERRIDE
	{
		PickState = EPickState::OverViewport;
		HoveredActor.Reset();

		int32 HitX = Viewport->GetMouseX();
		int32 HitY = Viewport->GetMouseY();
		HHitProxy* HitProxy = Viewport->GetHitProxy(HitX, HitY);
		if (HitProxy != NULL && HitProxy->IsA(HActor::StaticGetType()))
		{
			HActor* ActorHit = static_cast<HActor*>(HitProxy);
			if(ActorHit->Actor != NULL)
			{
				AActor* Actor = ActorHit->Actor;
				HoveredActor = Actor;
				PickState = EPickState::OverIncompatibleActor;

				TArray<const UClass*> AllowedClasses;
				OnGetAllowedClasses.Execute(AllowedClasses);
				for(auto It(AllowedClasses.CreateConstIterator()); It; It++)
				{
					if(Actor->IsA(*It))
					{
						PickState = EPickState::OverActor;
						break;
					}
				}
			}
		}

		return true;
	}

	virtual bool InputKey(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) OVERRIDE
	{
		if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
		{
			// See if we clicked on an actor
			int32 HitX = Viewport->GetMouseX();
			int32 HitY = Viewport->GetMouseY();
			HHitProxy*	HitProxy = Viewport->GetHitProxy(HitX, HitY);
			if (HitProxy != NULL && HitProxy->IsA(HActor::StaticGetType()))
			{
				HActor* ActorHit = static_cast<HActor*>(HitProxy);
				if(ActorHit->Actor != NULL)
				{
					AActor* Actor = ActorHit->Actor;
					TArray<const UClass*> AllowedClasses;
					OnGetAllowedClasses.Execute(AllowedClasses);
					for(auto It(AllowedClasses.CreateConstIterator()); It; It++)
					{
						if(Actor->IsA(*It))
						{
							OnActorSelected.ExecuteIfBound(ActorHit->Actor);
							EndMode();
							break;
						}
					}
				}
			}
			return true;
		}
		else if(Key == EKeys::Escape && Event == IE_Pressed)
		{
			EndMode();
			return true;
		}
		return false;
	}

	virtual bool GetCursor(EMouseCursor::Type& OutCursor) const OVERRIDE
	{
		if(HoveredActor.IsValid())
		{
			OutCursor = EMouseCursor::EyeDropper;
		}
		else
		{
			OutCursor = EMouseCursor::SlashedCircle;
		}
		
		return true;
	}

	virtual bool UsesToolkits() const OVERRIDE
	{
		return false;
	}
	/** End FEdMode interface */

	/** Start up the mode, reset state etc. */
	void BeginMode(FOnGetAllowedClasses InOnGetAllowedClasses, FOnActorSelected InOnActorSelected )
	{
		OnActorSelected = InOnActorSelected;
		OnGetAllowedClasses = InOnGetAllowedClasses;
		GEditorModeTools().ActivateMode(GetID());
		CursorDecoratorWindow = SWindow::MakeCursorDecorator();
		FSlateApplication::Get().AddWindow(CursorDecoratorWindow.ToSharedRef(), true);
		CursorDecoratorWindow->SetContent(
			SNew(SToolTip)
			.Text(this, &FEdModePicker::GetCursorDecoratorText)
			);

		HoveredActor.Reset();
		PickState = EPickState::NotOverViewport;
	}

	/** End the mode */
	void EndMode()
	{
		OnActorSelected = FOnActorSelected();
		OnGetAllowedClasses = FOnGetAllowedClasses();
		GEditorModeTools().DeactivateMode(GetID());

		if (CursorDecoratorWindow.IsValid())
		{
			CursorDecoratorWindow->RequestDestroyWindow();
			CursorDecoratorWindow.Reset();
		}

		HoveredActor.Reset();
		PickState = EPickState::NotOverViewport;
	}

	/** Delegate used to display information about picking near the cursor */
	FString GetCursorDecoratorText() const
	{
		switch(PickState)
		{
		default:
		case EPickState::NotOverViewport:
			return LOCTEXT("PickActor_NotOverViewport", "Pick an actor by clicking on it in a level viewport").ToString();
		case EPickState::OverViewport:
			return LOCTEXT("PickActor_NotOverActor", "Pick an actor by clicking on it").ToString();
		case EPickState::OverIncompatibleActor:
			{
				if(HoveredActor.IsValid())
				{
					FFormatNamedArguments Arguments;
					Arguments.Add(TEXT("Actor"), FText::FromString(HoveredActor.Get()->GetName()));
					return FText::Format(LOCTEXT("PickActor_OverIncompatibleActor", "{Actor} is incompatible"), Arguments).ToString();
				}
				else
				{
					return LOCTEXT("PickActor_NotOverActor", "Pick an actor by clicking on it").ToString();
				}
			}
		case EPickState::OverActor:
			{
				if(HoveredActor.IsValid())
				{
					FFormatNamedArguments Arguments;
					Arguments.Add(TEXT("Actor"), FText::FromString(HoveredActor.Get()->GetName()));
					return FText::Format(LOCTEXT("PickActor_OverActor", "Pick {Actor}"), Arguments).ToString();
				}
				else
				{
					return LOCTEXT("PickActor_NotOverActor", "Pick an actor by clicking on it").ToString();
				}
			}
		}
	}

	/** Flag for display state */
	TWeakObjectPtr<AActor> HoveredActor;

	/** Flag for display state */
	EPickState::Type PickState;

	/** The window that owns the decorator widget */
	TSharedPtr<SWindow> CursorDecoratorWindow;

	/** Delegates used to pick actors */
	FOnActorSelected OnActorSelected;
	FOnGetAllowedClasses OnGetAllowedClasses;
};

/** The one and only picker edit mode */
static TSharedPtr<FEdModePicker> EdModePicker = NULL;

void SPropertyEditorInteractiveActorPicker::RegisterEditMode()
{	
	EdModePicker = MakeShareable(new FEdModePicker);
	GEditorModeTools().RegisterMode(EdModePicker.ToSharedRef());

	// we want to be able to perform this action with all the built-in editor modes
	GEditorModeTools().RegisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Default);
	GEditorModeTools().RegisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Placement);
	GEditorModeTools().RegisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Bsp);
	GEditorModeTools().RegisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Geometry);
	GEditorModeTools().RegisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_InterpEdit);
	GEditorModeTools().RegisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Texture);
	GEditorModeTools().RegisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_MeshPaint);
	GEditorModeTools().RegisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Landscape);
	GEditorModeTools().RegisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Foliage);
	GEditorModeTools().RegisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Level);
	GEditorModeTools().RegisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_StreamingLevel);
	GEditorModeTools().RegisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Physics);
}

void SPropertyEditorInteractiveActorPicker::UnregisterEditMode()
{		
	if(GEditorModeTools().IsModeActive(EdModePicker->GetID()))
	{
		EdModePicker->EndMode();
	}

	GEditorModeTools().UnregisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Default);
	GEditorModeTools().UnregisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Placement);
	GEditorModeTools().UnregisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Bsp);
	GEditorModeTools().UnregisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Geometry);
	GEditorModeTools().UnregisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_InterpEdit);
	GEditorModeTools().UnregisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Texture);
	GEditorModeTools().UnregisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_MeshPaint);
	GEditorModeTools().UnregisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Landscape);
	GEditorModeTools().UnregisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Foliage);
	GEditorModeTools().UnregisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Level);
	GEditorModeTools().UnregisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_StreamingLevel);
	GEditorModeTools().UnregisterCompatibleModes(EdModePicker->GetID(), FBuiltinEditorModes::EM_Physics);
	GEditorModeTools().UnregisterMode(EdModePicker.ToSharedRef());
	EdModePicker = NULL;
}

SPropertyEditorInteractiveActorPicker::~SPropertyEditorInteractiveActorPicker()
{
	// make sure we are unregistered when this widget goes away
	if(GEditorModeTools().IsModeActive(EdModePicker->GetID()))
	{
		EdModePicker->EndMode();
	}	
}

void SPropertyEditorInteractiveActorPicker::Construct( const FArguments& InArgs )
{
	OnActorSelected = InArgs._OnActorSelected;
	OnGetAllowedClasses = InArgs._OnGetAllowedClasses;

	SButton::Construct(
		SButton::FArguments()
		.ButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
		.OnClicked( this, &SPropertyEditorInteractiveActorPicker::OnClicked )
		.ContentPadding(4.0f)
		.ForegroundColor( FSlateColor::UseForeground() )
		.IsFocusable(false)
		[ 
			SNew( SImage )
			.Image( FEditorStyle::GetBrush("PropertyWindow.Button_PickActorInteractive") )
			.ColorAndOpacity( FSlateColor::UseForeground() )
		]
	);
}

FReply SPropertyEditorInteractiveActorPicker::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	if(InKeyboardEvent.GetKey() == EKeys::Escape)
	{
		if(GEditorModeTools().IsModeActive(EdModePicker->GetID()))
		{
			EdModePicker->EndMode();
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

bool SPropertyEditorInteractiveActorPicker::SupportsKeyboardFocus() const
{
	return true;
}

FReply SPropertyEditorInteractiveActorPicker::OnClicked()
{
	if(GEditorModeTools().IsModeActive(EdModePicker->GetID()))
	{
		EdModePicker->EndMode();
	}
	else
	{
		EdModePicker->BeginMode(OnGetAllowedClasses, OnActorSelected);
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE