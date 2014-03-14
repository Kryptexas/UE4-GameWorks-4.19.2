// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PersonaPrivatePCH.h"
#include "AnimViewportShowCommands.h"

void FAnimViewportShowCommands::RegisterCommands()
{
	UI_COMMAND(ShowGrid, "Show Grid", "Display Grid", EUserInterfaceActionType::ToggleButton, FInputGesture());
	UI_COMMAND(HighlightOrigin, "Highlight Origin", "Highlight the origin lines on the grid", EUserInterfaceActionType::ToggleButton, FInputGesture());

	UI_COMMAND(ShowFloor, "Show Floor", "Display Floor", EUserInterfaceActionType::ToggleButton, FInputGesture());
	UI_COMMAND(ShowSky, "Show Sky", "Display Sky", EUserInterfaceActionType::ToggleButton, FInputGesture());

	UI_COMMAND(MuteAudio, "Mute Audio", "Mutes audio from the preview", EUserInterfaceActionType::ToggleButton, FInputGesture());


	UI_COMMAND( ShowReferencePose, "Reference Pose", "Show reference pose on preview mesh", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowBound, "Bound", "Show bound on preview mesh", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowPreviewMesh, "Mesh", "Show the preview mesh", EUserInterfaceActionType::ToggleButton, FInputGesture() );

	UI_COMMAND( ShowBones, "Bones", "Display Skeleton in viewport", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowBoneNames, "Bone Names", "Display Bone Names in Viewport", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowDisplayInfo, "Display Info", "Display Mesh Info in Viewport", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowBoneWeight, "View Selected Bone Weight", "Display Selected Bone Weight in Viewport", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowRawAnimation, "Uncompressed Animation", "Display Skeleton With Uncompressed Animation Data", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowNonRetargetedAnimation, "NonRetargeted Animation", "Display Skeleton With non retargeted Animation Data", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowAdditiveBaseBones, "Additive Base", "Display Skeleton In Additive Base Pose", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowSockets, "Sockets", "Display socket hitpoints", EUserInterfaceActionType::ToggleButton, FInputGesture() );

	UI_COMMAND( ShowLocalAxesNone, "None", "Hides all local hierarchy axis", EUserInterfaceActionType::RadioButton, FInputGesture() );
	UI_COMMAND( ShowLocalAxesSelected, "Selected Hierarchy", "Shows only the local bone axis of the selected bones", EUserInterfaceActionType::RadioButton, FInputGesture() );
	UI_COMMAND( ShowLocalAxesAll, "All", "Shows all local hierarchy axes", EUserInterfaceActionType::RadioButton, FInputGesture() );

#if WITH_APEX_CLOTHING
	UI_COMMAND( DisableClothSimulation, "Disable Clothing", "Disable Cloth Simulation", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ApplyClothWind, "Apply Wind", "Apply Wind to Clothing", EUserInterfaceActionType::ToggleButton, FInputGesture() );

	UI_COMMAND( ShowClothSimulationNormals, "Simulation Normals", "Display Cloth Simulation Normals", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowClothGraphicalTangents, "Graphical Tangents", "Display Cloth Graphical Tangents", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowClothCollisionVolumes, "Collision Volumes", "Display Cloth Collision Volumes", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( EnableCollisionWithAttachedClothChildren, "Collide with Cloth Children", "Enables collision detection between collision primitives in the base mesh and clothing on any attachments in the preview scene.", EUserInterfaceActionType::ToggleButton, FInputGesture() );	

	UI_COMMAND( ShowOnlyClothSections, "Only Cloth Sections", "Display Only Cloth Mapped Sections", EUserInterfaceActionType::ToggleButton, FInputGesture() );
#endif// #if WITH_APEX_CLOTHING
}
