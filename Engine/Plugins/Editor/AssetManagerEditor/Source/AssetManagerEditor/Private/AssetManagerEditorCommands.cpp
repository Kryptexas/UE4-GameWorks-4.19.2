// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AssetManagerEditorCommands.h"

//////////////////////////////////////////////////////////////////////////
// FAssetManagerEditorCommands

#define LOCTEXT_NAMESPACE "AssetManagerEditorCommands"

FAssetManagerEditorCommands::FAssetManagerEditorCommands() : TCommands<FAssetManagerEditorCommands>(
	"AssetManagerEditorCommands",
	NSLOCTEXT("Contexts", "AssetManagerEditorCommands", "Asset Management"),
	NAME_None, 
	FEditorStyle::GetStyleSetName())
{
}

void FAssetManagerEditorCommands::RegisterCommands()
{
	UI_COMMAND(ViewReferences, "Reference Viewer...", "Launches the reference viewer showing the selected assets' references", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::R));
	UI_COMMAND(ViewSizeMap, "Size Map...", "Displays an interactive map showing the approximate size of this asset and everything it references", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::M));
	UI_COMMAND(ViewAssetAudit, "Audit Assets...", "Opens the Asset Audit UI and displays information about the selected assets", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::A));

	UI_COMMAND(OpenSelectedInAssetEditor, "Edit...", "Opens the selected asset in the relevent editor.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ReCenterGraph, "Re-Center Graph", "Re-centers the graph on this node, showing all referencers and references for this asset instead", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(CopyReferencedObjects, "Copy Referenced Objects List", "Copies the list of objects that the selected asset references to the clipboard.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(CopyReferencingObjects, "Copy Referencing Objects List", "Copies the list of objects that reference the selected asset to the clipboard.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ShowReferencedObjects, "Show Referenced Objects List", "Shows a list of objects that the selected asset references.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ShowReferencingObjects, "Show Referencing Objects List", "Shows a list of objects that reference the selected asset.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ShowReferenceTree, "Show Reference Tree", "Shows a reference tree for the selected asset.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AuditReferencedObjects, "Audit References", "Opens the Asset Audit UI and displays information about all assets referenced by selected asset", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(MakeLocalCollectionWithReferencers, "Local", "Local. This collection is only visible to you and is not in source control.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(MakePrivateCollectionWithReferencers, "Private", "Private. This collection is only visible to you.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(MakeSharedCollectionWithReferencers, "Shared", "Shared. This collection is visible to everyone.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(MakeLocalCollectionWithDependencies, "Local", "Local. This collection is only visible to you and is not in source control.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(MakePrivateCollectionWithDependencies, "Private", "Private. This collection is only visible to you.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(MakeSharedCollectionWithDependencies, "Shared", "Shared. This collection is visible to everyone.", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
