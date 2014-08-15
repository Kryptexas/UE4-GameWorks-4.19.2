// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Tests/AutomationTestSettings.h"
#include "AutomationEditorCommon.h"
#include "AutomationTest.h"
#include "AssertionMacros.h"
#include "AutomationCommon.h"
#include "AssetEditorManager.h"

//Includes needed for opening certain assets
#include "Materials/MaterialFunction.h"
#include "Slate/SlateBrushAsset.h"



bool FOpenActualAssetEditors(const FString& Parameters)
{
	//start with all editors closed
	FAssetEditorManager::Get().CloseAllAssetEditors();

	// below is all latent action, so before sending there, verify the asset exists
	UObject* Object = StaticLoadObject(UObject::StaticClass(), NULL, *Parameters);
	if (!Object)
	{
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to find object: %s."), *Parameters);
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FOpenEditorForAssetCommand(*Parameters));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FCloseAllAssetEditorsCommand());

	return true;
}
//////////////////////////////////////////////////////////////////////////
/**
* Test to open the sub editor windows for a specified list of assets.
* This list can be setup in the Editor Preferences window within the editor or the DefaultEngine.ini file for that particular project.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenAssetEditors, "QA.Open Asset Editors", EAutomationTestFlags::ATF_Editor);

void FOpenAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	UAutomationTestSettings const* AutomationTestSettings = GetDefault<UAutomationTestSettings>();
	check(AutomationTestSettings);

	bool bUnAttended = FApp::IsUnattended();

	TArray<FString> AssetNames;
	for (auto Iter = AutomationTestSettings->TestAssetsToOpen.CreateConstIterator(); Iter; ++Iter)
	{
		if (Iter->bSkipTestWhenUnAttended && bUnAttended)
		{
			continue;
		}

		if (Iter->AssetToOpen.FilePath.Len() > 0)
		{
			AssetNames.AddUnique(Iter->AssetToOpen.FilePath);
		}
	}

	//Location of the engine folder
	FString EngineContentFolderLocation = FPaths::ConvertRelativePathToFull(*FPaths::EngineContentDir());
	//Put the Engine Content Folder Location into an array.  
	TArray<FString> EngineContentFolderLocationArray;
	EngineContentFolderLocation.ParseIntoArray(&EngineContentFolderLocationArray, TEXT("/"), true);

	for (int32 i = 0; i < AssetNames.Num(); ++i)
	{
		//true means that the life is located in the Engine/Content folder.
		bool bFileIsLocatedInTheEngineContentFolder = true;

		//Get the location of the asset that is being opened.
		FString Filename = FPaths::ConvertRelativePathToFull(AssetNames[i]);

		//Put File location into an array.  
		TArray<FString> FilenameArray;
		Filename.ParseIntoArray(&FilenameArray, TEXT("/"), true);

		//Loop through the location array's and compare each element.  
		//The loop runs while the index is less than the number of elements in the EngineContentFolderLocation array.
		//If the elements are the same when the counter is up then it is assumed that the asset file is in the engine content folder. 
		//Otherwise we'll assume it's in the game content folder.
		for (int32 index = 0; index < EngineContentFolderLocationArray.Num(); index++)
		{
			if ((FilenameArray[index] != EngineContentFolderLocationArray[index]))
			{
				//If true it will proceed to add the asset to the Open Asset Editor test list.
				//This will be false if the asset is on a different drive.
				if (FPaths::MakePathRelativeTo(Filename, *FPaths::GameContentDir()))
				{
					FString ShortName = FPaths::GetBaseFilename(Filename);
					FString PathName = FPaths::GetPath(Filename);
					OutBeautifiedNames.Add(ShortName);
					FString AssetName = FString::Printf(TEXT("/Game/%s/%s.%s"), *PathName, *ShortName, *ShortName);
					OutTestCommands.Add(AssetName);
					bFileIsLocatedInTheEngineContentFolder = false;
					break;
				}
				else
				{
					UE_LOG(LogEditorAutomationTests, Error, TEXT("Invalid asset path: %s."), *Filename);
					bFileIsLocatedInTheEngineContentFolder = false;
					break;
				}
			}
		}
		//If true then the asset is in the Engine/Content folder and not in the Game/Content folder.
		if (bFileIsLocatedInTheEngineContentFolder)
		{
			//If true it will proceed to add the asset to the Open Asset Editor test list.
			//This will be false if the asset is on a different drive.
			if (FPaths::MakePathRelativeTo(Filename, *FPaths::EngineContentDir()))
			{
				FString ShortName = FPaths::GetBaseFilename(Filename);
				FString PathName = FPaths::GetPath(Filename);
				OutBeautifiedNames.Add(ShortName);
				FString AssetName = FString::Printf(TEXT("/Engine/%s/%s.%s"), *PathName, *ShortName, *ShortName);
				OutTestCommands.Add(AssetName);
			}
			else
			{
				UE_LOG(LogEditorAutomationTests, Error, TEXT("Invalid asset path: %s."), *Filename);
			}
		}
	}
}

bool FOpenAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}


//////////////////////////////////////////////////////////////////////////
/**
* This test opens each AimOffsetBlendSpace into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenAimOffsetBlendSpaceAssetEditors, "Editor.Content.Skeletal Mesh.Animation.BlendSpaces.Open Aim Offset BlendSpace Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenAimOffsetBlendSpaceAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each AimOffsetBlendSpace asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UAimOffsetBlendSpace::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenAimOffsetBlendSpaceAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each AimOffsetBlendSpace1D into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenAimOffsetBlendSpace1DAssetEditors, "Editor.Content.Skeletal Mesh.Animation.BlendSpaces.Open Aim Offset BlendSpace1D Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenAimOffsetBlendSpace1DAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each AimOffsetBlendSpace1D asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UAimOffsetBlendSpace1D::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenAimOffsetBlendSpace1DAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each AnimMontage into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenAnimMontageAssetEditors, "Editor.Content.Skeletal Mesh.Animation.Open AnimMontage Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenAnimMontageAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each AnimMontage asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UAnimMontage::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenAnimMontageAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each AnimSequence into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenAnimSequenceAssetEditors, "Editor.Content.Skeletal Mesh.Animation.Open AnimSequence Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenAnimSequenceAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each AnimSequence asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UAnimSequence::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenAnimSequenceAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each BlendSpace into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenBlendSpaceAssetEditors, "Editor.Content.Skeletal Mesh.Animation.BlendSpaces.Open BlendSpace Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenBlendSpaceAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each BlendSpace asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UBlendSpace::StaticClass(), false, OutBeautifiedNames, OutTestCommands);
}

bool FOpenBlendSpaceAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each BlendSpace1D into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenBlendSpace1DAssetEditors, "Editor.Content.Skeletal Mesh.Animation.BlendSpaces.Open BlendSpace1D Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenBlendSpace1DAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each BlendSpace1D asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UBlendSpace1D::StaticClass(), false, OutBeautifiedNames, OutTestCommands);
}

bool FOpenBlendSpace1DAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}


//////////////////////////////////////////////////////////////////////////
/**
* This test opens each DESTRUCTIBLE MESH into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenDestructibleMeshAssetEditors, "Editor.Content.Skeletal Mesh.Open Destructible Mesh Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenDestructibleMeshAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each DestructibleMesh asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UDestructibleMesh::StaticClass(), false, OutBeautifiedNames, OutTestCommands);
}

bool FOpenDestructibleMeshAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each DialogueVoice into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenDialogueVoiceAssetEditors, "Editor.Content.Sounds.Open Dialogue Voice Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenDialogueVoiceAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each DialogueVoice asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UDialogueVoice::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenDialogueVoiceAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each DialogueWave into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenDialogueWaveAssetEditors, "Editor.Content.Sounds.Open Dialogue Wave Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenDialogueWaveAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each DialogueWave asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UDialogueWave::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenDialogueWaveAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each Material into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenMaterialAssetEditors, "Editor.Content.Materials.Open Material Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenMaterialAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each Material asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UMaterial::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenMaterialAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

////////////////////////////////////////////////////////////////////////////
///**
//* This test opens each MaterialFunction into its sub-editor.
//*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenMaterialFunctionAssetEditors, "Editor.Content.Materials.Open Material Function Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenMaterialFunctionAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each MaterialFunction asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UMaterialFunction::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenMaterialFunctionAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each MaterialInstanceConstant into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenMaterialInstanceConstantAssetEditors, "Editor.Content.Materials.Open Material Instance Constant Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenMaterialInstanceConstantAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each MaterialInstanceConstant asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UMaterialInstanceConstant::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenMaterialInstanceConstantAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each MaterialParameterCollection into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenMaterialParameterCollectionAssetEditors, "Editor.Content.Materials.Open Material Parameter Collection Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenMaterialParameterCollectionAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each MaterialParameterCollection asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UMaterialParameterCollection::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenMaterialParameterCollectionAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each PARTICLE SYSTEM into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenParticleSystemAssetEditors, "Editor.Content.Particle System.Open Particle System Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenParticleSystemAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each ParticleSystem asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UParticleSystem::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenParticleSystemAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each PhysicalMaterial into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenPhysicalMaterialAssetEditors, "Editor.Content.Physics.Open Physical Material Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenPhysicalMaterialAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each PhysicalMaterial asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UPhysicalMaterial::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenPhysicalMaterialAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}


//////////////////////////////////////////////////////////////////////////
/**
* This test opens each PhysicsAsset into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenPhysicsAssetAssetEditors, "Editor.Content.Physics.Open Physics Asset Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenPhysicsAssetAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each PhysicsAsset asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UPhysicsAsset::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenPhysicsAssetAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}


//////////////////////////////////////////////////////////////////////////
/**
* This test opens each ReverbEffect into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenReverbEffectAssetEditors, "Editor.Content.Sounds.Open Reverb Effect Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenReverbEffectAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each ReverbEffect asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UReverbEffect::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenReverbEffectAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}



//////////////////////////////////////////////////////////////////////////
/**
* This test opens each SlateWidgetStyleAsset into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenSlateWidgetStyleAssetAssetEditors, "Editor.Content.Slate.Open Slate Widget Style Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenSlateWidgetStyleAssetAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each SlateWidgetStyleAsset asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(USlateWidgetStyleAsset::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenSlateWidgetStyleAssetAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each Slate Brush Asset into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenSlateBrushAssetAssetEditors, "Editor.Content.Slate.Open Slate Brush Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenSlateBrushAssetAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each SlateBrushAsset asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(USlateBrushAsset::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenSlateBrushAssetAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}
//////////////////////////////////////////////////////////////////////////
/**
* This test opens each SoundAttenuation into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenSoundAttenuationAssetEditors, "Editor.Content.Sounds.Open Sound Attenuation Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenSoundAttenuationAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each SoundAttenuation asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(USoundAttenuation::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenSoundAttenuationAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each Static Mesh into their sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenStaticMeshAssetEditors, "Editor.Content.Static Mesh.Open Static Mesh Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenStaticMeshAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each Static Mesh in the Game/Content
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UStaticMesh::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenStaticMeshAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each Skeletal Mesh into their sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenSkeletalMeshAssetEditors, "Editor.Content.Skeletal Mesh.Open Skeletal Mesh Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenSkeletalMeshAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each SKeletal Mesh in the Game/Content
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(USkeletalMesh::StaticClass(), false, OutBeautifiedNames, OutTestCommands);
}

bool FOpenSkeletalMeshAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each Skeleton into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenSkeletonAssetEditors, "Editor.Content.Skeletal Mesh.Open Skeleton Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenSkeletonAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each Skeleton asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(USkeleton::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenSkeletonAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each SoundClass into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenSoundClassAssetEditors, "Editor.Content.Sounds.Open SoundClass Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenSoundClassAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each SoundClass asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(USoundClass::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenSoundClassAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}


//////////////////////////////////////////////////////////////////////////
/**
* This test opens each SoundCue into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenSoundCueAssetEditors, "Editor.Content.Sounds.Open Sound Cue Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenSoundCueAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each SoundCue asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(USoundCue::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenSoundCueAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}


//////////////////////////////////////////////////////////////////////////
/**
* This test opens each SoundMix into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenSoundMixAssetEditors, "Editor.Content.Sounds.Open Sound Mix Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenSoundMixAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each SoundMix asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(USoundMix::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenSoundMixAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}


//////////////////////////////////////////////////////////////////////////
/**
* This test opens each SoundWave into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenSoundWaveAssetEditors, "Editor.Content.Sounds.Open Sound Wave Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenSoundWaveAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each SoundWave asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(USoundWave::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenSoundWaveAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}
//////////////////////////////////////////////////////////////////////////
/**
* This test opens each SubsurfaceProfile into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenSubsurfaceProfileAssetEditors, "Editor.Content.Materials.Open Subsurface Profile Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenSubsurfaceProfileAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each SubsurfaceProfile asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(USubsurfaceProfile::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenSubsurfaceProfileAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}



//////////////////////////////////////////////////////////////////////////
/**
* This test opens each TEXTURE asset into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenTextureAssetEditors, "Editor.Content.Textures.Open Texture Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenTextureAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each texture file in the Game/Content
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UTexture::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenTextureAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}


//////////////////////////////////////////////////////////////////////////
/**
* This test opens each Font into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenFontAssetEditors, "Editor.Content.Textures.Open Font Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenFontAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each Font asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UFont::StaticClass(), true, OutBeautifiedNames, OutTestCommands);
}

bool FOpenFontAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}

//////////////////////////////////////////////////////////////////////////
/**
* This test opens each misc asset into its sub-editor.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FOpenMiscAssetEditors, "Editor.Content.Misc.Open Misc Assets", (EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser));

void FOpenMiscAssetEditors::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	//This grabs each TextureRenderTarget2D asset in the Game/Content directory
	FEditorAutomationTestUtilities::CollectMiscGameContentTestsByClass(OutBeautifiedNames, OutTestCommands);
}

bool FOpenMiscAssetEditors::RunTest(const FString& Parameters)
{
	bool bDidTheTestPass = FOpenActualAssetEditors(Parameters);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	return bDidTheTestPass;
}