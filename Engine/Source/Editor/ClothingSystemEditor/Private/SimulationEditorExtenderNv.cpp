// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SimulationEditorExtenderNv.h"

#if WITH_NVCLOTH

#include "ClothingSimulation.h"
#include "ClothingSimulationNv.h"
#include "ClothingSimulationFactory.h"
#include "IPersonaPreviewScene.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "UIAction.h"
#include "MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "NvSimEditorExtender"

FNvVisualizationOptions::FNvVisualizationOptions()
{
	FMemory::Memset(Flags, 0, (int32)EVisualizationOption::Max);

	OptionData[(int32)EVisualizationOption::PhysMesh].DisplayName = LOCTEXT("VisName_PhysMesh", "Physical Mesh");
	OptionData[(int32)EVisualizationOption::PhysMesh].ToolTip = LOCTEXT("VisName_PhysMesh_ToolTip", "Draws the current physical mesh result");

	OptionData[(int32)EVisualizationOption::Normals].DisplayName = LOCTEXT("VisName_Normals", "Simulation Normals");
	OptionData[(int32)EVisualizationOption::Normals].ToolTip = LOCTEXT("VisName_Normals_ToolTip", "Draws the current normals for the simulation mesh");

	OptionData[(int32)EVisualizationOption::Collision].DisplayName = LOCTEXT("VisName_Collision", "Collisions");
	OptionData[(int32)EVisualizationOption::Collision].ToolTip= LOCTEXT("VisName_Collision_ToolTip", "Draws the collision bodies the simulation is currently using");

	OptionData[(int32)EVisualizationOption::Backstop].DisplayName = LOCTEXT("VisName_Backstop", "Backstops");
	OptionData[(int32)EVisualizationOption::Backstop].ToolTip = LOCTEXT("VisName_Backstop_ToolTip", "Draws the backstop offset for each simulation particle");

	OptionData[(int32)EVisualizationOption::MaxDistances].DisplayName = LOCTEXT("VisName_MaxDistance", "Max Distances");
	OptionData[(int32)EVisualizationOption::MaxDistances].ToolTip = LOCTEXT("VisName_MaxDistance_ToolTip", "Draws the current max distances for the sim particles as a line along its normal");

	OptionData[(int32)EVisualizationOption::SelfCollision].DisplayName = LOCTEXT("VisName_SelfCollision", "Self Collision Radii");
	OptionData[(int32)EVisualizationOption::SelfCollision].ToolTip = LOCTEXT("VisName_SelfCollision_ToolTip", "Draws the self collision radius for each particle if self collision is enabled");

	OptionData[(int32)EVisualizationOption::AnimDrive].DisplayName = LOCTEXT("VisName_AnimDrive", "Anim Drive");
	OptionData[(int32)EVisualizationOption::AnimDrive].ToolTip = LOCTEXT("VisName_AnimDrive_Tooltip", "Draws the current skinned reference mesh for the simulation which anim drive will attempt to reach if enabled");

	OptionData[(int32)EVisualizationOption::Backstop].bDisablesSimulation = true;
	OptionData[(int32)EVisualizationOption::MaxDistances].bDisablesSimulation = true;
}

bool FNvVisualizationOptions::ShouldDisableSimulation() const
{
	for(int32 OptionIndex = 0; OptionIndex < (int32)FNvVisualizationOptions::EVisualizationOption::Max; ++OptionIndex)
	{
		if(Flags[OptionIndex])
		{
			const FOptionData& Data = OptionData[OptionIndex];

			if(Data.bDisablesSimulation)
			{
				return true;
			}
		}
	}

	return false;
}

UClass* FSimulationEditorExtenderNv::GetSupportedSimulationFactoryClass()
{
	return UClothingSimulationFactoryNv::StaticClass();
}

void FSimulationEditorExtenderNv::ExtendViewportShowMenu(FMenuBuilder& InMenuBuilder, TSharedRef<IPersonaPreviewScene> InPreviewScene)
{
	InMenuBuilder.BeginSection(TEXT("NvSim_Visualizations"), LOCTEXT("VisSection", "Visualizations"));
	{
		for(int32 OptionIndex = 0; OptionIndex < (int32)FNvVisualizationOptions::EVisualizationOption::Max; ++OptionIndex)
		{
			FNvVisualizationOptions::EVisualizationOption Option = (FNvVisualizationOptions::EVisualizationOption)OptionIndex;

			FExecuteAction ExecuteAction = FExecuteAction::CreateRaw(this, &FSimulationEditorExtenderNv::OnEntryClicked, Option, InPreviewScene);
			FIsActionChecked IsActionChecked = FIsActionChecked::CreateRaw(this, &FSimulationEditorExtenderNv::IsEntryChecked, Option);

			FUIAction Action(ExecuteAction, FCanExecuteAction(), IsActionChecked);

			InMenuBuilder.AddMenuEntry(VisulizationFlags.OptionData[OptionIndex].DisplayName, VisulizationFlags.OptionData[OptionIndex].ToolTip, FSlateIcon(), Action, NAME_None, EUserInterfaceActionType::ToggleButton);
		}
	}
	InMenuBuilder.EndSection();
}

void FSimulationEditorExtenderNv::DebugDrawSimulation(const IClothingSimulation* InSimulation, USkeletalMeshComponent* InOwnerComponent, FPrimitiveDrawInterface* PDI)
{
	if(!InSimulation)
	{
		return;
	}

	const FClothingSimulationNv* NvSimulation = static_cast<const FClothingSimulationNv*>(InSimulation);

	for(int32 OptionIndex = 0; OptionIndex < (int32)FNvVisualizationOptions::EVisualizationOption::Max; ++OptionIndex)
	{
		FNvVisualizationOptions::EVisualizationOption Option = (FNvVisualizationOptions::EVisualizationOption)OptionIndex;

		if(VisulizationFlags.IsSet(Option))
		{
			switch(Option)
			{
				default:
					break;
				case FNvVisualizationOptions::EVisualizationOption::PhysMesh:
					NvSimulation->DebugDraw_PhysMesh(InOwnerComponent, PDI);
					break;
				case FNvVisualizationOptions::EVisualizationOption::Normals:
					NvSimulation->DebugDraw_Normals(InOwnerComponent, PDI);
					break;
				case FNvVisualizationOptions::EVisualizationOption::Collision:
					NvSimulation->DebugDraw_Collision(InOwnerComponent, PDI);
					break;
				case FNvVisualizationOptions::EVisualizationOption::Backstop:
					NvSimulation->DebugDraw_Backstops(InOwnerComponent, PDI);
					break;
				case FNvVisualizationOptions::EVisualizationOption::MaxDistances:
					NvSimulation->DebugDraw_MaxDistances(InOwnerComponent, PDI);
					break;
				case FNvVisualizationOptions::EVisualizationOption::SelfCollision:
					NvSimulation->DebugDraw_SelfCollision(InOwnerComponent, PDI);
					break;
				case FNvVisualizationOptions::EVisualizationOption::AnimDrive:
					NvSimulation->DebugDraw_AnimDrive(InOwnerComponent, PDI);
					break;
			}
		}
	}
}

void FSimulationEditorExtenderNv::OnEntryClicked(FNvVisualizationOptions::EVisualizationOption InOption, TSharedRef<IPersonaPreviewScene> InPreviewScene)
{
	VisulizationFlags.Toggle(InOption);

	if(UDebugSkelMeshComponent* MeshComponent = InPreviewScene->GetPreviewMeshComponent())
	{
		bool bShouldDisableSim = VisulizationFlags.ShouldDisableSimulation();

		// If we need to toggle the disabled state, handle it
		if(bShouldDisableSim && MeshComponent->bDisableClothSimulation!= bShouldDisableSim)
		{
			MeshComponent->bDisableClothSimulation = !MeshComponent->bDisableClothSimulation;
		}
	}
}

bool FSimulationEditorExtenderNv::IsEntryChecked(FNvVisualizationOptions::EVisualizationOption InOption) const
{
	return VisulizationFlags.IsSet(InOption);
}

#undef LOCTEXT_NAMESPACE

#endif
