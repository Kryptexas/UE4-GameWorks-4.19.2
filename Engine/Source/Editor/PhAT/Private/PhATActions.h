// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/*-----------------------------------------------------------------------------
   FPhATCommands
-----------------------------------------------------------------------------*/

class FPhATCommands : public TCommands<FPhATCommands>
{
public:
	/** Constructor */
	FPhATCommands() 
		: TCommands<FPhATCommands>("PhAT", NSLOCTEXT("Contexts", "PhAT", "PhAT"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}
	
	/** See tooltips in cpp for documentation */
	
	TSharedPtr<FUICommandInfo> ChangeDefaultMesh;
	TSharedPtr<FUICommandInfo> ResetEntireAsset;
	TSharedPtr<FUICommandInfo> RestetBoneCollision;
	TSharedPtr<FUICommandInfo> ApplyPhysicalMaterial;
	TSharedPtr<FUICommandInfo> EditingMode_Body;
	TSharedPtr<FUICommandInfo> EditingMode_Constraint;
	TSharedPtr<FUICommandInfo> MovementSpace_Local;
	TSharedPtr<FUICommandInfo> MovementSpace_World;
	TSharedPtr<FUICommandInfo> TranslationMode;
	TSharedPtr<FUICommandInfo> RotationMode;
	TSharedPtr<FUICommandInfo> ScaleMode;
	TSharedPtr<FUICommandInfo> Snap;
	TSharedPtr<FUICommandInfo> CopyProperties;
	TSharedPtr<FUICommandInfo> PasteProperties;
	TSharedPtr<FUICommandInfo> InstanceProperties;
	TSharedPtr<FUICommandInfo> RepeatLastSimulation;
	TSharedPtr<FUICommandInfo> SimulationNormal;
	TSharedPtr<FUICommandInfo> SimulationNoGravity;
	TSharedPtr<FUICommandInfo> ToggleSelectedSimulation;
	TSharedPtr<FUICommandInfo> MeshRenderingMode_Solid;
	TSharedPtr<FUICommandInfo> MeshRenderingMode_Wireframe;
	TSharedPtr<FUICommandInfo> MeshRenderingMode_None;
	TSharedPtr<FUICommandInfo> CollisionRenderingMode_Solid;
	TSharedPtr<FUICommandInfo> CollisionRenderingMode_Wireframe;
	TSharedPtr<FUICommandInfo> CollisionRenderingMode_None;
	TSharedPtr<FUICommandInfo> ConstraintRenderingMode_None;
	TSharedPtr<FUICommandInfo> ConstraintRenderingMode_AllPositions;
	TSharedPtr<FUICommandInfo> ConstraintRenderingMode_AllLimits;
	TSharedPtr<FUICommandInfo> ShowFixedBodies;
	TSharedPtr<FUICommandInfo> DrawGroundBox;
	TSharedPtr<FUICommandInfo> ToggleGraphicsHierarchy;
	TSharedPtr<FUICommandInfo> ToggleBoneInfuences;
	TSharedPtr<FUICommandInfo> ToggleMassProperties;
	TSharedPtr<FUICommandInfo> DisableCollision;
	TSharedPtr<FUICommandInfo> EnableCollision;
	TSharedPtr<FUICommandInfo> WeldToBody;
	TSharedPtr<FUICommandInfo> AddNewBody;
	TSharedPtr<FUICommandInfo> AddSphere;
	TSharedPtr<FUICommandInfo> AddSphyl;
	TSharedPtr<FUICommandInfo> AddBox;
	TSharedPtr<FUICommandInfo> DeletePrimitive;
	TSharedPtr<FUICommandInfo> DuplicatePrimitive;
	TSharedPtr<FUICommandInfo> ResetConstraint;
	TSharedPtr<FUICommandInfo> SnapConstraint;
	TSharedPtr<FUICommandInfo> ConvertToBallAndSocket;
	TSharedPtr<FUICommandInfo> ConvertToHinge;
	TSharedPtr<FUICommandInfo> ConvertToPrismatic;
	TSharedPtr<FUICommandInfo> ConvertToSkeletal;
	TSharedPtr<FUICommandInfo> DeleteConstraint;
	TSharedPtr<FUICommandInfo> PlayAnimation;
	TSharedPtr<FUICommandInfo> ShowSkeleton;
	TSharedPtr<FUICommandInfo> MakeBodyFixed;
	TSharedPtr<FUICommandInfo> MakeBodyUnfixed;
	TSharedPtr<FUICommandInfo> MakeBodyDefault;
	TSharedPtr<FUICommandInfo> FixAllBodiesBelow;
	TSharedPtr<FUICommandInfo> UnfixAllBodiesBelow;
	TSharedPtr<FUICommandInfo> MakeAllBodiesBelowDefault;
	TSharedPtr<FUICommandInfo> DeleteBody;
	TSharedPtr<FUICommandInfo> DeleteAllBodiesBelow;
	TSharedPtr<FUICommandInfo> ToggleMotor;
	TSharedPtr<FUICommandInfo> EnableMotorsBelow;
	TSharedPtr<FUICommandInfo> DisableMotorsBelow;
	TSharedPtr<FUICommandInfo> SelectAll;
	TSharedPtr<FUICommandInfo> HierarchyFilterAll;
	TSharedPtr<FUICommandInfo> HierarchyFilterBodies;

	/** Hotkey only commands */
	TSharedPtr<FUICommandInfo> SelectionLock;
	TSharedPtr<FUICommandInfo> DeleteSelection;
	TSharedPtr<FUICommandInfo> CycleConstraintOrientation;
	TSharedPtr<FUICommandInfo> CycleConstraintActive;
	TSharedPtr<FUICommandInfo> ToggleSwing1;
	TSharedPtr<FUICommandInfo> ToggleSwing2;
	TSharedPtr<FUICommandInfo> ToggleTwist;
	TSharedPtr<FUICommandInfo> FocusOnSelection;
	TSharedPtr<FUICommandInfo> CycleTransformMode;
	

	/** Initialize commands */
	virtual void RegisterCommands() OVERRIDE;
};
