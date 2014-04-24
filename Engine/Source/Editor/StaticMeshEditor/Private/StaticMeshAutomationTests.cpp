// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "StaticMeshEditorModule.h"

#include "AutomationCommon.h"
#include "SStaticMeshEditorViewport.h"
#include "StaticMeshEditorViewportClient.h"

namespace EditorViewButtonHelper
{
	/**
	 * The types of buttons that will be toggled on and off, if new buttons are made that want to be added, 
	 * all you need to do is add them to this list, and fill out the latent automation task below with how to toggle the button
	 */
	namespace EStaticMeshFlag
	{
		enum Type
		{
			Wireframe = 0,
			Vert,
			Grid,
			Bounds,
			Collision,
			Pivot,
			Normals,
			Tangents,
			Binormals,
			UV,
			Max //Do not go beyond this
		};
	}

	/**
	 * The struct we will be passing into the below Latent Automation Task
	 */
	struct PerformStaticMeshFlagParameters
	{
		FStaticMeshEditorViewportClient* ViewportClient;
		EStaticMeshFlag::Type CommandType;
	};

	/**
	 * Used for toggling a certain button on/off based on the passed in viewport client and button type.
	 */
	DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FPerformStaticMeshFlagToggle, PerformStaticMeshFlagParameters, AutomationParameters);

	bool EditorViewButtonHelper::FPerformStaticMeshFlagToggle::Update()
	{ 
		switch(AutomationParameters.CommandType)
		{
			case EStaticMeshFlag::Wireframe:
				if (AutomationParameters.ViewportClient->GetViewMode() != VMI_Wireframe)
				{
					AutomationParameters.ViewportClient->SetViewMode(VMI_Wireframe);
				}
				else
				{
					AutomationParameters.ViewportClient->SetViewMode(VMI_Lit);
				}
				break;
			case EStaticMeshFlag::Vert:
				if (!AutomationParameters.ViewportClient->EngineShowFlags.VertexColors)
				{
					AutomationParameters.ViewportClient->EngineShowFlags.VertexColors = true;
					AutomationParameters.ViewportClient->EngineShowFlags.Lighting = false;
				}
				else
				{
					AutomationParameters.ViewportClient->EngineShowFlags.VertexColors = false;
					AutomationParameters.ViewportClient->EngineShowFlags.Lighting = true;
				}
				break;
			case EStaticMeshFlag::Grid:
				AutomationParameters.ViewportClient->SetShowGrid();
				break;
			case EStaticMeshFlag::Bounds:
				if (AutomationParameters.ViewportClient->EngineShowFlags.Bounds)
				{
					AutomationParameters.ViewportClient->EngineShowFlags.Bounds = false;
				}
				else
				{
					AutomationParameters.ViewportClient->EngineShowFlags.Bounds = true;
				}
				break;
			case EStaticMeshFlag::Collision:
				AutomationParameters.ViewportClient->SetShowCollision();
				break;
			case EStaticMeshFlag::Pivot:
				AutomationParameters.ViewportClient->SetShowPivot();
				break;
			case EStaticMeshFlag::Normals:
				AutomationParameters.ViewportClient->SetShowNormals();
				break;
			case EStaticMeshFlag::Tangents:
				AutomationParameters.ViewportClient->SetShowTangents();
				break;
			case EStaticMeshFlag::Binormals:
				AutomationParameters.ViewportClient->SetShowBinormals();
				break;
			case EStaticMeshFlag::UV:
				AutomationParameters.ViewportClient->SetDrawUVOverlay();
				break;
			default:
				//Break out immediately.
				break;
		}
		return true;
	}

	/**
	* Close all asset editors
	*/
	DEFINE_LATENT_AUTOMATION_COMMAND(FCloseAllAssetEditorsCommand);

	bool FCloseAllAssetEditorsCommand::Update()
	{
		FAssetEditorManager::Get().CloseAllAssetEditors();

		return true;
	}
}

/**
 * Static mesh editor test
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST( FStaticMeshEditorTest, "Editor.Content.Static Mesh Editor Test", EAutomationTestFlags::ATF_Editor )

/**
 * Take screenshots of the SME window with each of the toolbar buttons toggled separately
 */
bool FStaticMeshEditorTest::RunTest(const FString& Parameters)
{
	TArray<FString> ButtonNamesToTestFor;
	FString LoadedObjectType = TEXT("EditorCylinder");
	FString MeshName;
	EditorViewButtonHelper::PerformStaticMeshFlagParameters AutomationParameters;
	WindowScreenshotParameters WindowParameters;

	//Pull from .ini the names of the buttons we want to test for
	GConfig->GetString( TEXT("AutomationTesting"), TEXT("EditorViewButtonsObject"), LoadedObjectType, GEngineIni);

	//Open static mesh in the editor and save out the window for the screenshots
	UObject* EditorMesh = (UStaticMesh*)StaticLoadObject(UStaticMesh::StaticClass(),
		NULL,
		*FString::Printf(TEXT("/Engine/EditorMeshes/%s.%s"), *LoadedObjectType, *LoadedObjectType),
		NULL,
		LOAD_None,
		NULL);
	FAssetEditorManager::Get().OpenEditorForAsset(EditorMesh);
	WindowParameters.CurrentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();

	//Grab the last opened Viewport (aka the asset manager)
	AutomationParameters.ViewportClient = static_cast<FStaticMeshEditorViewportClient*>(GEditor->AllViewportClients[GEditor->AllViewportClients.Num() - 1]);
	AutomationParameters.CommandType = EditorViewButtonHelper::EStaticMeshFlag::Wireframe; //Start it off at Wireframe because that is the first button we are attempting to press

	if (AutomationParameters.ViewportClient)
	{
		const FString BaseFileName = FPaths::ScreenShotDir() / TEXT("StaticMeshEditor");
		WindowParameters.ScreenshotName = FString::Printf(TEXT("%s00.bmp"), *BaseFileName);

		//Wait for the window to load and then take the initial screen shot		
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
		ADD_LATENT_AUTOMATION_COMMAND(FTakeEditorScreenshotCommand(WindowParameters));
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f));

		for(auto CurrentCommandType = 0; CurrentCommandType < EditorViewButtonHelper::EStaticMeshFlag::Max; ++CurrentCommandType)
		{
			WindowParameters.ScreenshotName = FString::Printf(TEXT("%s%02i.bmp"), *BaseFileName, CurrentCommandType + 1);

			//Toggle the command for the button, take a screenshot, and then re-toggle the command for the button
			ADD_LATENT_AUTOMATION_COMMAND(EditorViewButtonHelper::FPerformStaticMeshFlagToggle(AutomationParameters));
			ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f));
			ADD_LATENT_AUTOMATION_COMMAND(FTakeEditorScreenshotCommand(WindowParameters));
			ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f));
			ADD_LATENT_AUTOMATION_COMMAND(EditorViewButtonHelper::FPerformStaticMeshFlagToggle(AutomationParameters));
			
			//Increment to the next enum, so we can loop through all the types
			AutomationParameters.CommandType = (EditorViewButtonHelper::EStaticMeshFlag::Type)((int)AutomationParameters.CommandType + 1);
		}

		ADD_LATENT_AUTOMATION_COMMAND(EditorViewButtonHelper::FCloseAllAssetEditorsCommand());
	}

	return true;
}
