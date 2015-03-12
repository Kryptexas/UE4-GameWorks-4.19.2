// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SequencerCommands.h"

#define LOCTEXT_NAMESPACE ""

void FSequencerCommands::RegisterCommands()
{
	UI_COMMAND( SetKey, "Set Key", "Sets a key at the current time for the selected actor.", EUserInterfaceActionType::Button, FInputGesture(EKeys::K) );

	UI_COMMAND( ToggleAutoKeyEnabled, "Auto Key", "Enables and disables auto keying.", EUserInterfaceActionType::ToggleButton, FInputGesture() );

	UI_COMMAND( ToggleIsSnapEnabled, "Enable Snapping", "Enables and disables snapping.", EUserInterfaceActionType::ToggleButton, FInputGesture() );

	UI_COMMAND( ToggleSnapKeyTimesToInterval, "Snap to the interval", "Snap keys to the interval.", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ToggleSnapKeyTimesToKeys, "Snap to other keys", "Snap keys to other keys in the section.", EUserInterfaceActionType::ToggleButton, FInputGesture() );

	UI_COMMAND( ToggleSnapSectionTimesToInterval, "Snap to the interval", "Snap sections to the interval.", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ToggleSnapSectionTimesToSections, "Snap to other sections", "Snap sections to other sections.", EUserInterfaceActionType::ToggleButton, FInputGesture() );

	UI_COMMAND( ToggleSnapPlayTimeToInterval, "Snap to the interval while scrubbing", "Snap the play time to the interval while scrubbing.", EUserInterfaceActionType::ToggleButton, FInputGesture() );

	UI_COMMAND(ToggleSnapCurveValueToInterval, "Snap curve key values", "Snap curve keys to the value snapping interval.", EUserInterfaceActionType::ToggleButton, FInputGesture() );

	UI_COMMAND( ToggleCleanView, "Clean View", "Enable 'Clean View' mode which displays only global tracks when no filter is applied.", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	
	UI_COMMAND( ToggleShowCurveEditor, "Curve Editor", "Show the animation keys in a curve editor.", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ToggleShowCurveEditorCurveToolTips, "Curve Tool Tips", "Show a tool tip with name and values when hoving over a curve.", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( SetAllCurveVisibility, "All Curves", "Show all curves in the curve editor.", EUserInterfaceActionType::RadioButton, FInputGesture() );
	UI_COMMAND( SetSelectedCurveVisibility, "Selected Curves", "Show only selected curves in the curve editor.", EUserInterfaceActionType::RadioButton, FInputGesture() );
	UI_COMMAND( SetAnimatedCurveVisibility, "Animated Curves", "Show only curves with keyframes in the curve editor.", EUserInterfaceActionType::RadioButton, FInputGesture() );
}

#undef LOCTEXT_NAMESPACE