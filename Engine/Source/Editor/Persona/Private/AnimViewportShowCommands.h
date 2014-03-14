// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __AnimViewportShowCommands_h_
#define __AnimViewportShowCommands_h_

/**
 * Class containing commands for persona viewport show actions
 */
class FAnimViewportShowCommands : public TCommands<FAnimViewportShowCommands>
{
public:
	FAnimViewportShowCommands() 
		: TCommands<FAnimViewportShowCommands>
		(
			TEXT("AnimViewportShowCmd"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "AnimViewportShowCmd", "Animation Viewport Show Command"), // Localized context name for displaying
			NAME_None, // Parent context name. 
			FEditorStyle::GetStyleSetName() // Icon Style Set
		)
	{
	}

	/** Option to show grid */
	TSharedPtr< FUICommandInfo > ShowGrid;

	/** Option to highlight the origin */
	TSharedPtr< FUICommandInfo > HighlightOrigin;

	/** Option to show floor */
	TSharedPtr< FUICommandInfo > ShowFloor;

	/** Option to show floor */
	TSharedPtr< FUICommandInfo > ShowSky;

	/** Option to mute audio in the viewport */
	TSharedPtr< FUICommandInfo > MuteAudio;

	/** Show reference pose on preview mesh */
	TSharedPtr< FUICommandInfo > ShowReferencePose;
	
	/** Show Bound of preview mesh */
	TSharedPtr< FUICommandInfo > ShowBound;

	/** Show/hide the preview mesh */
	TSharedPtr< FUICommandInfo > ShowPreviewMesh;

	/** Show skeletal mesh bones */	
	TSharedPtr< FUICommandInfo > ShowBones;

	/** Show raw animation (vs compressed) */
	TSharedPtr< FUICommandInfo > ShowRawAnimation;

	/** Show non retargeted animation. */
	TSharedPtr< FUICommandInfo > ShowNonRetargetedAnimation;

	/** Show additive base pose */
	TSharedPtr< FUICommandInfo > ShowAdditiveBaseBones;

	/** Show skeletal mesh bone names */
	TSharedPtr< FUICommandInfo > ShowBoneNames;

	/** Show skeletal mesh info */
	TSharedPtr< FUICommandInfo > ShowDisplayInfo;

	/** Show selected bone weight */
	TSharedPtr< FUICommandInfo > ShowBoneWeight;

	/** Show socket hit point diamonds */
	TSharedPtr< FUICommandInfo > ShowSockets;

	/** Hide all local axes */
	TSharedPtr< FUICommandInfo > ShowLocalAxesNone;
	
	/** Show only selected axes */
	TSharedPtr< FUICommandInfo > ShowLocalAxesSelected;
	
	/** Show all local axes */
	TSharedPtr< FUICommandInfo > ShowLocalAxesAll;

#if WITH_APEX_CLOTHING
	/** Disable cloth simulation */
	TSharedPtr< FUICommandInfo > DisableClothSimulation;

	/** Apply wind for clothing */
	TSharedPtr< FUICommandInfo > ApplyClothWind;

	/** Show cloth simulation normals */
	TSharedPtr< FUICommandInfo > ShowClothSimulationNormals;

	/** Show cloth graphical tangents */
	TSharedPtr< FUICommandInfo > ShowClothGraphicalTangents;
	
	/** Show cloth collision volumes */
	TSharedPtr< FUICommandInfo > ShowClothCollisionVolumes;

	/** Enables collision detection between collision primitives in the base mesh 
	  * and clothing on any attachments in the preview scene. 
	*/
	TSharedPtr< FUICommandInfo > EnableCollisionWithAttachedClothChildren;

	/** Show only clothing mapped sections */
	TSharedPtr< FUICommandInfo > ShowOnlyClothSections;
#endif// #if WITH_APEX_CLOTHING

public:
	/** Registers our commands with the binding system */
	virtual void RegisterCommands() OVERRIDE;
};


#endif //__AnimViewportShowCommands_h_
