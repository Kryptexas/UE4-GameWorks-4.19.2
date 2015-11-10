// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SequencerContextMenus.h"
#include "SequencerHotspots.h"
#include "ScopedTransaction.h"
#include "MovieSceneToolHelpers.h"
#include "MovieSceneSection.h"

void FKeyContextMenu::BuildMenu(FMenuBuilder& MenuBuilder, FSequencer& InSequencer)
{
	TSharedRef<FKeyContextMenu> Menu = MakeShareable(new FKeyContextMenu(InSequencer));
	Menu->PopulateMenu(MenuBuilder);
}

void FKeyContextMenu::PopulateMenu(FMenuBuilder& MenuBuilder)
{
	FSequencer* SequencerPtr = &Sequencer.Get();
	MenuBuilder.BeginSection("SequencerInterpolation", NSLOCTEXT("Sequencer", "KeyInterpolationMenu", "Key Interpolation"));
	{
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "SetKeyInterpolationAuto", "Auto"),
			NSLOCTEXT("Sequencer", "SetKeyInterpolationAutoTooltip", "Set key interpolation to auto"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(SequencerPtr, &FSequencer::SetInterpTangentMode, RCIM_Cubic, RCTM_Auto),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(SequencerPtr, &FSequencer::IsInterpTangentModeSelected, RCIM_Cubic, RCTM_Auto) ),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "SetKeyInterpolationUser", "User"),
			NSLOCTEXT("Sequencer", "SetKeyInterpolationUserTooltip", "Set key interpolation to user"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(SequencerPtr, &FSequencer::SetInterpTangentMode, RCIM_Cubic, RCTM_User),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(SequencerPtr, &FSequencer::IsInterpTangentModeSelected, RCIM_Cubic, RCTM_User) ),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "SetKeyInterpolationBreak", "Break"),
			NSLOCTEXT("Sequencer", "SetKeyInterpolationBreakTooltip", "Set key interpolation to break"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(SequencerPtr, &FSequencer::SetInterpTangentMode, RCIM_Cubic, RCTM_Break),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(SequencerPtr, &FSequencer::IsInterpTangentModeSelected, RCIM_Cubic, RCTM_Break) ),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "SetKeyInterpolationLinear", "Linear"),
			NSLOCTEXT("Sequencer", "SetKeyInterpolationLinearTooltip", "Set key interpolation to linear"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(SequencerPtr, &FSequencer::SetInterpTangentMode, RCIM_Linear, RCTM_Auto),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(SequencerPtr, &FSequencer::IsInterpTangentModeSelected, RCIM_Linear, RCTM_Auto) ),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "SetKeyInterpolationConstant", "Constant"),
			NSLOCTEXT("Sequencer", "SetKeyInterpolationConstantTooltip", "Set key interpolation to constant"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(SequencerPtr, &FSequencer::SetInterpTangentMode, RCIM_Constant, RCTM_Auto),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(SequencerPtr, &FSequencer::IsInterpTangentModeSelected, RCIM_Constant, RCTM_Auto) ),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
	}
	MenuBuilder.EndSection(); // SequencerInterpolation

	MenuBuilder.BeginSection("SequencerKeys", NSLOCTEXT("Sequencer", "KeysMenu", "Keys"));
	{
		const bool bUseFrames = SequencerSnapValues::IsTimeSnapIntervalFrameRate(Sequencer->GetSettings()->GetTimeSnapInterval());

		MenuBuilder.AddMenuEntry(
			bUseFrames ? NSLOCTEXT("Sequencer", "SetKeyFrame", "Set Key Frame") : NSLOCTEXT("Sequencer", "SetKeyTime", "Set Key Time"),
			bUseFrames ? NSLOCTEXT("Sequencer", "SetKeyFrameTooltip", "Set key frame") : NSLOCTEXT("Sequencer", "SetKeyTimeTooltip", "Set key time"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(SequencerPtr, &FSequencer::SetKeyTime, bUseFrames),
				FCanExecuteAction::CreateSP(SequencerPtr, &FSequencer::CanSetKeyTime))
		);

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "SnapToFrame", "Snap to Frame"),
			NSLOCTEXT("Sequencer", "SnapToFrameToolTip", "Snap selected keys to frame"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(SequencerPtr, &FSequencer::SnapToFrame),
				FCanExecuteAction::CreateSP(SequencerPtr, &FSequencer::CanSnapToFrame))
		);

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "DeleteKey", "Delete"),
			NSLOCTEXT("Sequencer", "DeleteKeyToolTip", "Deletes the selected keys"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(SequencerPtr, &FSequencer::DeleteSelectedKeys))
		);
	}
	MenuBuilder.EndSection(); // SequencerKeys
}

void FSectionContextMenu::BuildMenu(FMenuBuilder& MenuBuilder, FSequencer& InSequencer)
{
	TSharedRef<FSectionContextMenu> Menu = MakeShareable(new FSectionContextMenu(InSequencer));
	Menu->PopulateMenu(MenuBuilder);
}

void FSectionContextMenu::PopulateMenu(FMenuBuilder& MenuBuilder)
{
	// Copy a reference to the context menu by value into each lambda handler to ensure the type stays alive until the menu is closed
	TSharedRef<FSectionContextMenu> Shared = AsShared();

	MenuBuilder.BeginSection("SequencerSections", NSLOCTEXT("Sequencer", "SectionsMenu", "Sections"));
	{
		bool bCanSelectAll = CanSelectAllKeys();
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "SelectAllKeys", "Select All Keys"),
			NSLOCTEXT("Sequencer", "SelectAllKeysTooltip", "Select all keys in section"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([=]{ return Shared->SelectAllKeys(); }),
				FCanExecuteAction::CreateLambda([=]{ return bCanSelectAll; }))
		);

		MenuBuilder.AddSubMenu(
			NSLOCTEXT("Sequencer", "EditSection", "Edit"),
			NSLOCTEXT("Sequencer", "EditSectionTooltip", "Edit section"),
			FNewMenuDelegate::CreateLambda([=](FMenuBuilder& InMenuBuilder){ Shared->AddEditMenu(InMenuBuilder); }));

		MenuBuilder.AddSubMenu(
			NSLOCTEXT("Sequencer", "SetPreInfinityExtrap", "Pre-Infinity"), 
			NSLOCTEXT("Sequencer", "SetPreInfinityExtrapTooltip", "Set pre-infinity extrapolation"),
			FNewMenuDelegate::CreateLambda([=](FMenuBuilder& InMenuBuilder){ Shared->AddExtrapolationMenu(InMenuBuilder, true); }));

		MenuBuilder.AddSubMenu(
			NSLOCTEXT("Sequencer", "SetPostInfinityExtrap", "Post-Infinity"), 
			NSLOCTEXT("Sequencer", "SetPostInfinityExtrapTooltip", "Set post-infinity extrapolation"),
			FNewMenuDelegate::CreateLambda([=](FMenuBuilder& InMenuBuilder){ Shared->AddExtrapolationMenu(InMenuBuilder, false); }));

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "ToggleSectionActive", "Active"),
			NSLOCTEXT("Sequencer", "ToggleSectionActiveTooltip", "Toggle section active/inactive"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([=]{ Shared->ToggleSectionActive(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([=]{ return Shared->IsToggleSectionActive(); })),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		// @todo Sequencer this should delete all selected sections
		// delete/selection needs to be rethought in general
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "DeleteSection", "Delete"),
			NSLOCTEXT("Sequencer", "DeleteSectionToolTip", "Deletes this section"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([=]{ return Shared->DeleteSection(); }))
		);
	}
	MenuBuilder.EndSection(); // SequencerSections
}

void FSectionContextMenu::AddEditMenu(FMenuBuilder& MenuBuilder)
{
	// Copy a reference to the context menu by value into each lambda handler to ensure the type stays alive until the menu is closed
	TSharedRef<FSectionContextMenu> Shared = AsShared();

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("Sequencer", "TrimSectionLeft", "Trim Left"),
		NSLOCTEXT("Sequencer", "TrimSectionLeftTooltip", "Trim section at current time to the left"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([=]{ Shared->TrimSection(true); }),
			FCanExecuteAction::CreateLambda([=]{ return Shared->IsTrimmable(); }))
	);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("Sequencer", "TrimSectionRight", "Trim Right"),
		NSLOCTEXT("Sequencer", "TrimSectionRightTooltip", "Trim section at current time to the right"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([=]{ Shared->TrimSection(false); }),
			FCanExecuteAction::CreateLambda([=]{ return Shared->IsTrimmable(); }))
	);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("Sequencer", "SplitSection", "Split"),
		NSLOCTEXT("Sequencer", "SplitSectionTooltip", "Split section at current time"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([=]{ Shared->SplitSection(); }),
			FCanExecuteAction::CreateLambda([=]{ return Shared->IsTrimmable(); }))
	);
}

void FSectionContextMenu::AddExtrapolationMenu(FMenuBuilder& MenuBuilder, bool bPreInfinity)
{
	// Copy a reference to the context menu by value into each lambda handler to ensure the type stays alive until the menu is closed
	TSharedRef<FSectionContextMenu> Shared = AsShared();

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("Sequencer", "SetExtrapCycle", "Cycle"),
		NSLOCTEXT("Sequencer", "SetExtrapCycleTooltip", "Set extrapolation cycle"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([=]{ Shared->SetExtrapolationMode(RCCE_Cycle, bPreInfinity); }),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([=]{ return Shared->IsExtrapolationModeSelected(RCCE_Cycle, bPreInfinity); })
		),
		NAME_None,
		EUserInterfaceActionType::RadioButton
	);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("Sequencer", "SetExtrapCycleWithOffset", "Cycle with Offset"),
		NSLOCTEXT("Sequencer", "SetExtrapCycleWithOffsetTooltip", "Set extrapolation cycle with offset"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([=]{ Shared->SetExtrapolationMode(RCCE_CycleWithOffset, bPreInfinity); }),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([=]{ return Shared->IsExtrapolationModeSelected(RCCE_CycleWithOffset, bPreInfinity); })
		),
		NAME_None,
		EUserInterfaceActionType::RadioButton
	);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("Sequencer", "SetExtrapOscillate", "Oscillate"),
		NSLOCTEXT("Sequencer", "SetExtrapOscillateTooltip", "Set extrapolation oscillate"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([=]{ Shared->SetExtrapolationMode(RCCE_Oscillate, bPreInfinity); }),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([=]{ return Shared->IsExtrapolationModeSelected(RCCE_Oscillate, bPreInfinity); })
		),
		NAME_None,
		EUserInterfaceActionType::RadioButton
	);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("Sequencer", "SetExtrapLinear", "Linear"),
		NSLOCTEXT("Sequencer", "SetExtrapLinearTooltip", "Set extrapolation linear"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([=]{ Shared->SetExtrapolationMode(RCCE_Linear, bPreInfinity); }),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([=]{ return Shared->IsExtrapolationModeSelected(RCCE_Linear, bPreInfinity); })
		),
		NAME_None,
		EUserInterfaceActionType::RadioButton
	);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("Sequencer", "SetExtrapConstant", "Constant"),
		NSLOCTEXT("Sequencer", "SetExtrapConstantTooltip", "Set extrapolation constant"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([=]{ Shared->SetExtrapolationMode(RCCE_Constant, bPreInfinity); }),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([=]{ return Shared->IsExtrapolationModeSelected(RCCE_Constant, bPreInfinity); })
		),
		NAME_None,
		EUserInterfaceActionType::RadioButton
	);
}

void FSectionContextMenu::SelectAllKeys()
{
	TArray<FSectionHandle> SelectedSections = StaticCastSharedRef<SSequencer>(Sequencer->GetSequencerWidget())->GetSectionHandles(Sequencer->GetSelection().GetSelectedSections());
	for (const FSectionHandle& Handle : SelectedSections)
	{
		UMovieSceneSection* Section = Handle.GetSectionObject();
		if (!Section)
		{
			continue;
		}

		FKeyAreaLayout Layout(*Handle.TrackNode, Handle.SectionIndex);
		for (const FKeyAreaLayoutElement& Element : Layout.GetElements())
		{
			TSharedPtr<IKeyArea> KeyArea = Element.GetKeyArea();
			for (const FKeyHandle& KeyHandle : KeyArea->GetUnsortedKeyHandles())
			{
				FSequencerSelectedKey SelectKey(*Section, KeyArea, KeyHandle);
				Sequencer->GetSelection().AddToSelection(SelectKey);
			}
		}
	}
}

bool FSectionContextMenu::CanSelectAllKeys() const
{
	TArray<FSectionHandle> SelectedSections = StaticCastSharedRef<SSequencer>(Sequencer->GetSequencerWidget())->GetSectionHandles(Sequencer->GetSelection().GetSelectedSections());
	for (const FSectionHandle& Handle : SelectedSections)
	{
		UMovieSceneSection* Section = Handle.GetSectionObject();
		if (!Section)
		{
			continue;
		}

		FKeyAreaLayout Layout(*Handle.TrackNode, Handle.SectionIndex);
		for (const FKeyAreaLayoutElement& Element : Layout.GetElements())
		{
			TArray<FKeyHandle> KeyHandles = Element.GetKeyArea()->GetUnsortedKeyHandles();
			if (KeyHandles.Num() > 0)
			{
				return true;
			}
		}
	}

	return false;
}

void FSectionContextMenu::TrimSection(bool bTrimLeft)
{
	FScopedTransaction TrimSectionTransaction(NSLOCTEXT("Sequencer", "TrimSection_Transaction", "Trim Section"));

	MovieSceneToolHelpers::TrimSection(Sequencer->GetSelection().GetSelectedSections(), Sequencer->GetGlobalTime(), bTrimLeft);
	Sequencer->NotifyMovieSceneDataChanged();
}

void FSectionContextMenu::SplitSection()
{
	FScopedTransaction SplitSectionTransaction(NSLOCTEXT("Sequencer", "SplitSection_Transaction", "Split Section"));

	MovieSceneToolHelpers::SplitSection(Sequencer->GetSelection().GetSelectedSections(), Sequencer->GetGlobalTime());
	Sequencer->NotifyMovieSceneDataChanged();
}

bool FSectionContextMenu::IsTrimmable() const
{
	for (auto Section : Sequencer->GetSelection().GetSelectedSections())
	{
		if (Section.IsValid() && Section->IsTimeWithinSection(Sequencer->GetGlobalTime()))
		{
			return true;
		}
	}
	return false;
}

void FSectionContextMenu::SetExtrapolationMode(ERichCurveExtrapolation ExtrapMode, bool bPreInfinity)
{
	FScopedTransaction SetExtrapolationModeTransaction(NSLOCTEXT("Sequencer", "SetExtrapolationMode_Transaction", "Set Extrapolation Mode"));

	bool bAnythingChanged = false;

	TArray<FSectionHandle> SelectedSections = StaticCastSharedRef<SSequencer>(Sequencer->GetSequencerWidget())->GetSectionHandles(Sequencer->GetSelection().GetSelectedSections());
	for (const FSectionHandle& Handle : SelectedSections)
	{
		UMovieSceneSection* Section = Handle.GetSectionObject();
		if (!Section)
		{
			continue;
		}

		Section->Modify();

		FKeyAreaLayout Layout(*Handle.TrackNode, Handle.SectionIndex);
		for (const FKeyAreaLayoutElement& Element : Layout.GetElements())
		{
			TSharedPtr<IKeyArea> KeyArea = Element.GetKeyArea();
			bAnythingChanged = true;
			KeyArea->SetExtrapolationMode(ExtrapMode, bPreInfinity);
		}
	}

	if (bAnythingChanged)
	{
		Sequencer->UpdateRuntimeInstances();
	}
	else
	{
		SetExtrapolationModeTransaction.Cancel();
	}
}

bool FSectionContextMenu::IsExtrapolationModeSelected(ERichCurveExtrapolation ExtrapMode, bool bPreInfinity) const
{
	// @todo Sequencer should operate on selected sections
	bool bAllSelected = false;

	TArray<FSectionHandle> SelectedSections = StaticCastSharedRef<SSequencer>(Sequencer->GetSequencerWidget())->GetSectionHandles(Sequencer->GetSelection().GetSelectedSections());
	for (const FSectionHandle& Handle : SelectedSections)
	{
		FKeyAreaLayout Layout(*Handle.TrackNode, Handle.SectionIndex);
		for (const FKeyAreaLayoutElement& Element : Layout.GetElements())
		{
			TSharedPtr<IKeyArea> KeyArea = Element.GetKeyArea();

			bAllSelected = true;
			if (KeyArea->GetExtrapolationMode(bPreInfinity) != ExtrapMode)
			{
				bAllSelected = false;
				break;
			}
		}
	}

	return bAllSelected;
}

void FSectionContextMenu::ToggleSectionActive()
{
	FScopedTransaction ToggleSectionActiveTransaction( NSLOCTEXT("Sequencer", "ToggleSectionActive_Transaction", "Toggle Section Active") );
	bool bIsActive = !IsToggleSectionActive();
	bool bAnythingChanged = false;

	for (auto Section : Sequencer->GetSelection().GetSelectedSections())
	{
		bAnythingChanged = true;
		Section->Modify();
		Section->SetIsActive(bIsActive);
	}

	if (bAnythingChanged)
	{
		Sequencer->UpdateRuntimeInstances();
	}
	else
	{
		ToggleSectionActiveTransaction.Cancel();
	}
}

bool FSectionContextMenu::IsToggleSectionActive() const
{
	// Active only if all are active
	for (auto Section : Sequencer->GetSelection().GetSelectedSections())
	{
		if (!Section->IsActive())
		{
			return false;
		}
	}

	return true;
}

void FSectionContextMenu::DeleteSection()
{
	Sequencer->DeleteSections(Sequencer->GetSelection().GetSelectedSections());
}