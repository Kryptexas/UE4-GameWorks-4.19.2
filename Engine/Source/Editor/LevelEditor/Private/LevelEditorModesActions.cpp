// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LevelEditor.h"
#include "LevelEditorModesActions.h"

DEFINE_LOG_CATEGORY_STATIC(LevelEditorModesActions, Log, All);

#define LOCTEXT_NAMESPACE "LevelEditorModesActions"

/** UI_COMMAND takes long for the compile to optimize */
PRAGMA_DISABLE_OPTIMIZATION
void FLevelEditorModesCommands::RegisterCommands()
{
	EditorModeCommands.Empty();

	TArray<FEdMode*> Modes;
	GEditorModeTools().GetModes(Modes);

	int editorMode = 0;

	struct FCompareEdModeByPriority
	{
		FORCEINLINE bool operator()(const FEdMode& A, const FEdMode& B) const
		{
			return A.GetPriorityOrder() < B.GetPriorityOrder();
		}
	};

	Modes.Sort(FCompareEdModeByPriority());

	FKey EdModeKeys[9] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };

	for (FEdMode* Mode : Modes)
	{
		// If the mode isn't visible don't create a menu option for it.
		if (!Mode->IsVisible())
		{
			continue;
		}

		FName EditorModeCommandName = FName(*(FString("EditorMode.") + Mode->GetID().ToString()));

		TSharedPtr<FUICommandInfo> EditorModeCommand = 
			FInputBindingManager::Get().FindCommandInContext(GetContextName(), EditorModeCommandName);

		// If a command isn't yet registered for this mode, we need to register one.
		if ( !EditorModeCommand.IsValid() )
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("Mode"), Mode->GetName() );
			const FText Tooltip = FText::Format( NSLOCTEXT("LevelEditor", "ModeTooltipF", "Activate {Mode} Editing Mode"), Args );

			FUICommandInfo::MakeCommandInfo(
				this->AsShared(),
				EditorModeCommand,
				EditorModeCommandName,
				Mode->GetName(),
				Tooltip,
				Mode->GetIcon(),
				EUserInterfaceActionType::ToggleButton,
				editorMode < 9 ? FInputGesture( EModifierKey::Shift, EdModeKeys[editorMode] ) : FInputGesture() );

			EditorModeCommands.Add(EditorModeCommand);
		}

		editorMode++;
	}
}

PRAGMA_ENABLE_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
