// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ActorPickerModePrivatePCH.h"

#define LOCTEXT_NAMESPACE "PropertyPicker"

FEdModeActorPicker::FEdModeActorPicker()
{
	ID = FBuiltinEditorModes::EM_ActorPicker;
	PickState = EPickState::NotOverViewport;
	HoveredActor.Reset();
}
	
void FEdModeActorPicker::Tick(FLevelEditorViewportClient* ViewportClient, float DeltaTime)
{
	if(CursorDecoratorWindow.IsValid())
	{
		CursorDecoratorWindow->MoveWindowTo(FSlateApplication::Get().GetCursorPos() + FSlateApplication::Get().GetCursorSize());
	}

	FEdMode::Tick(ViewportClient, DeltaTime);
}

bool FEdModeActorPicker::MouseEnter(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport,int32 x, int32 y)
{
	PickState = EPickState::OverViewport;
	HoveredActor.Reset();
	return FEdMode::MouseEnter(ViewportClient, Viewport, x, y);
}

bool FEdModeActorPicker::MouseLeave(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport)
{
	PickState = EPickState::NotOverViewport;
	HoveredActor.Reset();
	return FEdMode::MouseLeave(ViewportClient, Viewport);
}

bool FEdModeActorPicker::MouseMove(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y)
{
	if (ViewportClient == GCurrentLevelEditingViewportClient)
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
				PickState =  IsActorValid(Actor) ? EPickState::OverActor : EPickState::OverIncompatibleActor;
			}
		}
	}
	else
	{
		PickState = EPickState::NotOverViewport;
		HoveredActor.Reset();
	}

	return true;
}

bool FEdModeActorPicker::LostFocus(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport)
{
	if (ViewportClient == GCurrentLevelEditingViewportClient)
	{
		// Make sure actor picking mode is disabled once the active viewport loses focus
		EndMode();
		return true;
	}

	return false;
}

bool FEdModeActorPicker::InputKey(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (ViewportClient == GCurrentLevelEditingViewportClient)
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
				if(IsActorValid(ActorHit->Actor))
				{
					OnActorSelected.ExecuteIfBound(ActorHit->Actor);
					EndMode();
				}
			}
			return true;
		}
		else if(Key == EKeys::Escape && Event == IE_Pressed)
		{
			EndMode();
			return true;
		}
	}
	else
	{
		EndMode();
	}

	return false;
}

bool FEdModeActorPicker::GetCursor(EMouseCursor::Type& OutCursor) const
{
	if(HoveredActor.IsValid() && PickState == EPickState::OverActor)
	{
		OutCursor = EMouseCursor::EyeDropper;
	}
	else
	{
		OutCursor = EMouseCursor::SlashedCircle;
	}
	
	return true;
}

bool FEdModeActorPicker::UsesToolkits() const
{
	return false;
}


void FEdModeActorPicker::BeginMode(FOnGetAllowedClasses InOnGetAllowedClasses, FOnShouldFilterActor InOnShouldFilterActor, FOnActorSelected InOnActorSelected )
{
	OnActorSelected = InOnActorSelected;
	OnGetAllowedClasses = InOnGetAllowedClasses;
	OnShouldFilterActor = InOnShouldFilterActor;
	GEditorModeTools().ActivateMode(GetID());
	CursorDecoratorWindow = SWindow::MakeCursorDecorator();
	FSlateApplication::Get().AddWindow(CursorDecoratorWindow.ToSharedRef(), true);
	CursorDecoratorWindow->SetContent(
		SNew(SToolTip)
		.Text(this, &FEdModeActorPicker::GetCursorDecoratorText)
		);

	HoveredActor.Reset();
	PickState = EPickState::NotOverViewport;
}

void FEdModeActorPicker::EndMode()
{
	OnActorSelected = FOnActorSelected();
	OnGetAllowedClasses = FOnGetAllowedClasses();
	OnShouldFilterActor = FOnShouldFilterActor();
	GEditorModeTools().DeactivateMode(GetID());

	if (CursorDecoratorWindow.IsValid())
	{
		CursorDecoratorWindow->RequestDestroyWindow();
		CursorDecoratorWindow.Reset();
	}

	HoveredActor.Reset();
	PickState = EPickState::NotOverViewport;
}

FString FEdModeActorPicker::GetCursorDecoratorText() const
{
	switch(PickState)
	{
	default:
	case EPickState::NotOverViewport:
		return LOCTEXT("PickActor_NotOverViewport", "Pick an actor by clicking on it in the active level viewport").ToString();
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

bool FEdModeActorPicker::IsActorValid(const AActor *const Actor) const
{
	bool bIsValid = false;

	if(Actor)
	{
		bool bHasValidClass = true;
		if(OnGetAllowedClasses.IsBound())
		{
			bHasValidClass = false;

			TArray<const UClass*> AllowedClasses;
			OnGetAllowedClasses.Execute(AllowedClasses);
			for(const UClass* AllowedClass : AllowedClasses)
			{
				if(Actor->IsA(AllowedClass))
				{
					bHasValidClass = true;
					break;
				}
			}
		}

		bool bHasValidActor = true;
		if(OnShouldFilterActor.IsBound())
		{
			bHasValidActor = OnShouldFilterActor.Execute(Actor);
		}

		bIsValid = bHasValidClass && bHasValidActor;
	}

	return bIsValid;
}

#undef LOCTEXT_NAMESPACE
