// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Factories.h"
#include "BusyCursor.h"
#include "Dialogs/DlgPickPath.h"

#include "FbxImporter.h"
#include "AssetSelection.h"

#include "FbxErrors.h"
#include "AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Animation/SkeletalMeshActor.h"
#include "PackageTools.h"

#include "SFbxSceneOptionWindow.h"
#include "MainFrame.h"

#include "Editor/KismetCompiler/Public/KismetCompilerModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Toolkits/AssetEditorManager.h"
#include "ContentBrowserModule.h"

#include "ObjectTools.h"
#include "FileHelpers.h"

#include "AI/Navigation/NavCollision.h"

#define LOCTEXT_NAMESPACE "FBXSceneReImportFactory"

UFbxSceneImportData *GetFbxSceneImportData(UObject *Obj)
{
	UFbxSceneImportData* SceneImportData = nullptr;
	if (Obj->IsA(UFbxSceneImportData::StaticClass()))
	{
		//Reimport from the scene data
		SceneImportData = Cast<UFbxSceneImportData>(Obj);
	}
	else
	{
		UFbxAssetImportData *ImportData = nullptr;
		if (Obj->IsA(UStaticMesh::StaticClass()))
		{
			//Reimport from one of the static mesh
			UStaticMesh* Mesh = Cast<UStaticMesh>(Obj);
			if (Mesh != nullptr && Mesh->AssetImportData != nullptr)
			{
				ImportData = Cast<UFbxAssetImportData>(Mesh->AssetImportData);
				SceneImportData = ImportData->bImportAsScene ? ImportData->FbxSceneImportDataReference : nullptr;
			}
		}
		//TODO: add all type the fbx scene import can create: material, texture, skeletal mesh, animation, ... 
	}
	return SceneImportData;
}

UReimportFbxSceneFactory::UReimportFbxSceneFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	SupportedClass = UFbxSceneImportData::StaticClass();
	Formats.Add(TEXT("fbx;FBX scene"));

	bCreateNew = false;
	bText = false;

	ImportPriority = DefaultImportPriority - 1;
}

bool UReimportFbxSceneFactory::FactoryCanImport(const FString& Filename)
{
	// Return false, we are a reimport only factory
	return false;
}

bool UReimportFbxSceneFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UFbxSceneImportData* SceneImportData = GetFbxSceneImportData(Obj);
	if (SceneImportData != nullptr)
	{
		OutFilenames.Add(SceneImportData->SourceFbxFile);
		return true;
	}
	return false;
}

void UReimportFbxSceneFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UFbxSceneImportData* SceneImportData = Cast<UFbxSceneImportData>(Obj);
	if (SceneImportData && ensure(NewReimportPaths.Num() == 1))
	{
		SceneImportData->SourceFbxFile = NewReimportPaths[0];
	}
}

void RecursivelyCreateOriginalPath(UnFbx::FFbxImporter* FbxImporter, TSharedPtr<FFbxNodeInfo> NodeInfo, FString AssetPath, TSet<uint64> &AssetPathDone)
{
	if (NodeInfo->AttributeInfo.IsValid() && !AssetPathDone.Contains(NodeInfo->AttributeInfo->UniqueId))
	{
		FString AssetName = AssetPath + TEXT("/") + NodeInfo->AttributeInfo->Name;
		NodeInfo->AttributeInfo->SetOriginalImportPath(AssetName);
		FString OriginalFullImportName = PackageTools::SanitizePackageName(AssetName);
		OriginalFullImportName = OriginalFullImportName + TEXT(".") + PackageTools::SanitizePackageName(NodeInfo->AttributeInfo->Name);
		NodeInfo->AttributeInfo->SetOriginalFullImportName(OriginalFullImportName);
		AssetPathDone.Add(NodeInfo->AttributeInfo->UniqueId);
	}
	if (NodeInfo->NodeName.Compare("RootNode") != 0)
	{
		AssetPath += TEXT("/") + NodeInfo->NodeName;
	}
	for (TSharedPtr<FFbxNodeInfo> Child : NodeInfo->Childrens)
	{
		RecursivelyCreateOriginalPath(FbxImporter, Child, AssetPath, AssetPathDone);
	}
}

bool GetFbxSceneReImportOptions(UnFbx::FFbxImporter* FbxImporter
	, TSharedPtr<FFbxSceneInfo> SceneInfoPtr
	, TSharedPtr<FFbxSceneInfo> SceneInfoOriginalPtr
	, UnFbx::FBXImportOptions *GlobalImportSettings
	, UFbxSceneImportOptions *SceneImportOptions
	, UFbxSceneImportOptionsStaticMesh *StaticMeshImportData
	, ImportOptionsNameMap &NameOptionsMap
	, FbxSceneReimportStatusMap &MeshStatusMap
	, FbxSceneReimportStatusMap &NodeStatusMap
	, bool bCanReimportHierarchy
	, FString Path)
{
	//Make sure we don't put the global transform into the vertex position of the mesh
	GlobalImportSettings->bTransformVertexToAbsolute = false;
	//Avoid combining meshes
	GlobalImportSettings->bCombineToSingle = false;
	//Use the full name (avoid creating one) to let us retrieve node transform and attachment later
	GlobalImportSettings->bUsedAsFullName = true;
	//Make sure we import the textures
	GlobalImportSettings->bImportTextures = true;
	//Make sure Material get imported
	GlobalImportSettings->bImportMaterials = true;

	GlobalImportSettings->bConvertScene = true;

	//TODO this options will be set by the fbxscene UI in the material options tab, it also should be save/load from config file
	//Prefix materials package name to put all material under Material folder (this avoid name clash with meshes)
	GlobalImportSettings->MaterialPrefixName = FName("/Materials/");

	TSharedPtr<SWindow> ParentWindow;
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}
	TSharedRef<SWindow> Window = SNew(SWindow)
		.ClientSize(FVector2D(800.f, 650.f))
		.Title(NSLOCTEXT("UnrealEd", "FBXSceneReimportOpionsTitle", "FBX Scene Reimport Options"));
	TSharedPtr<SFbxSceneOptionWindow> FbxSceneOptionWindow;

	//Make sure the display option show the save default options
	SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(GlobalImportSettings, StaticMeshImportData);

	Window->SetContent
		(
			SAssignNew(FbxSceneOptionWindow, SFbxSceneOptionWindow)
			.SceneInfo(SceneInfoPtr)
			.SceneInfoOriginal(SceneInfoOriginalPtr)
			.SceneImportOptionsDisplay(SceneImportOptions)
			.SceneImportOptionsStaticMeshDisplay(StaticMeshImportData)
			.OverrideNameOptionsMap(&NameOptionsMap)
			.MeshStatusMap(&MeshStatusMap)
			.CanReimportHierarchy(bCanReimportHierarchy)
			.NodeStatusMap(&NodeStatusMap)
			.GlobalImportSettings(GlobalImportSettings)
			.OwnerWindow(Window)
			.FullPath(Path)
			);

	FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

	if (!FbxSceneOptionWindow->ShouldImport())
	{
		return false;
	}

	//setup all options
	GlobalImportSettings->bImportStaticMeshLODs = SceneImportOptions->bImportStaticMeshLODs;
	GlobalImportSettings->bImportSkeletalMeshLODs = SceneImportOptions->bImportSkeletalMeshLODs;

	//Set the override material into the options
	for (TSharedPtr<FFbxNodeInfo> NodeInfo : SceneInfoPtr->HierarchyInfo)
	{
		for (TSharedPtr<FFbxMaterialInfo> Material : NodeInfo->Materials)
		{
			if (!GlobalImportSettings->OverrideMaterials.Contains(Material->UniqueId))
			{
				//If user don't want to import a material we have to replace it by the default material
				if (!Material->bImportAttribute)
				{
					UMaterial* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
					if (DefaultMaterial != nullptr)
					{
						GlobalImportSettings->OverrideMaterials.Add(Material->UniqueId, DefaultMaterial);
					}
				}
				else if (Material->bOverridePath)
				{
					UMaterialInterface *UnrealMaterial = static_cast<UMaterialInterface*>(Material->GetContentObject());
					if (UnrealMaterial != nullptr)
					{
						GlobalImportSettings->OverrideMaterials.Add(Material->UniqueId, UnrealMaterial);
					}
				}
			}
		}
	}

	SceneImportOptions->SaveConfig();
	return true;
}

EReimportResult::Type UReimportFbxSceneFactory::Reimport(UObject* Obj)
{
	ReimportData = GetFbxSceneImportData(Obj);
	if (!ReimportData)
	{
		return EReimportResult::Failed;
	}

	//We will call other factory store the filename value since UFactory::CurrentFilename is static
	FbxImportFileName = ReimportData->SourceFbxFile;

	UnFbx::FFbxImporter* FbxImporter = UnFbx::FFbxImporter::GetInstance();
	UnFbx::FFbxLoggerSetter Logger(FbxImporter);
	GWarn->BeginSlowTask(NSLOCTEXT("FbxSceneReImportFactory", "BeginReImportingFbxSceneTask", "ReImporting FBX scene"), true);

	GlobalImportSettings = FbxImporter->GetImportOptions();

	//Fill the original options
	for (auto kvp : ReimportData->NameOptionsMap)
	{
		if (kvp.Key.Compare(DefaultOptionName) == 0)
		{
			SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(kvp.Value, GlobalImportSettings);
			NameOptionsMap.Add(kvp.Key, GlobalImportSettings);
		}
		else
		{
			NameOptionsMap.Add(kvp.Key, kvp.Value);
		}
	}

	//Always convert the scene
	GlobalImportSettings->bConvertScene = true;
	GlobalImportSettings->bImportScene = ReimportData->bImportScene;

	//Read the fbx and store the hierarchy's information so we can reuse it after importing all the model in the fbx file
	if (!FbxImporter->ImportFromFile(*FbxImportFileName, FPaths::GetExtension(FbxImportFileName)))
	{
		// Log the error message and fail the import.
		GWarn->Log(ELogVerbosity::Error, FbxImporter->GetErrorMessage());
		FbxImporter->ReleaseScene();
		FbxImporter = nullptr;
		GWarn->EndSlowTask();
		return EReimportResult::Failed;
	}

	FString PackageName = "";
	Obj->GetOutermost()->GetName(PackageName);
	Path = FPaths::GetPath(PackageName);

	UnFbx::FbxSceneInfo SceneInfo;
	//Read the scene and found all instance with their scene information.
	FbxImporter->GetSceneInfo(FbxImportFileName, SceneInfo);

	//Convert old structure to the new scene export structure
	TSharedPtr<FFbxSceneInfo> SceneInfoPtr = ConvertSceneInfo(&SceneInfo);
	//Get import material info
	ExtractMaterialInfo(FbxImporter, SceneInfoPtr);

	if (!ReimportData->bCreateFolderHierarchy)
	{
		for (TSharedPtr<FFbxMeshInfo> MeshInfo : SceneInfoPtr->MeshInfo)
		{
			FString AssetName = Path + TEXT("/") + MeshInfo->Name;
			MeshInfo->SetOriginalImportPath(AssetName);
			FString OriginalFullImportName = PackageTools::SanitizePackageName(AssetName);
			OriginalFullImportName = OriginalFullImportName + TEXT(".") + PackageTools::SanitizePackageName(MeshInfo->Name);
			MeshInfo->SetOriginalFullImportName(OriginalFullImportName);
		}
	}
	else
	{
		TSet<uint64> AssetPathDone;
		FString AssetPath = Path;
		for (TSharedPtr<FFbxNodeInfo> NodeInfo : SceneInfoPtr->HierarchyInfo)
		{
			//Iterate the hierarchy and build the original path
			RecursivelyCreateOriginalPath(FbxImporter, NodeInfo, AssetPath, AssetPathDone);
		}
	}

	FillSceneHierarchyPath(SceneInfoPtr);

	FbxSceneReimportStatusMap MeshStatusMap;
	FbxSceneReimportStatusMap NodeStatusMap;
	bool bCanReimportHierarchy = ReimportData->HierarchyType == (int32)EFBXSceneOptionsCreateHierarchyType::FBXSOCHT_CreateBlueprint && !ReimportData->BluePrintFullName.IsEmpty();

	if (!GetFbxSceneReImportOptions(FbxImporter
		, SceneInfoPtr
		, ReimportData->SceneInfoSourceData
		, GlobalImportSettings
		, SceneImportOptions
		, SceneImportOptionsStaticMesh
		, NameOptionsMap
		, MeshStatusMap
		, NodeStatusMap
		, bCanReimportHierarchy
		, Path))
	{
		//User cancel the scene import
		FbxImporter->ReleaseScene();
		FbxImporter = nullptr;
		GlobalImportSettings = nullptr;
		GWarn->EndSlowTask();
		return EReimportResult::Cancelled;
	}
	
	GlobalImportSettingsReference = new UnFbx::FBXImportOptions();
	SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(GlobalImportSettings, GlobalImportSettingsReference);

	//Overwrite the reimport asset data with the new data
	ReimportData->SceneInfoSourceData = SceneInfoPtr;
	ReimportData->SourceFbxFile = FPaths::ConvertRelativePathToFull(FbxImportFileName);
	ReimportData->bImportScene = GlobalImportSettingsReference->bImportScene;
	//Copy the options map
	ReimportData->NameOptionsMap.Reset();
	for (auto kvp : NameOptionsMap)
	{
		ReimportData->NameOptionsMap.Add(kvp.Key, kvp.Value);
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataToDelete;
	for (TSharedPtr<FFbxMeshInfo> MeshInfo : SceneInfoPtr->MeshInfo)
	{
		//Delete all the delete asset
		if (!MeshStatusMap.Contains(MeshInfo->OriginalImportPath))
		{
			continue;
		}
		EFbxSceneReimportStatusFlags MeshStatus = *(MeshStatusMap.Find(MeshInfo->OriginalImportPath));
		if ((MeshStatus & EFbxSceneReimportStatusFlags::Removed) == EFbxSceneReimportStatusFlags::None || (MeshStatus & EFbxSceneReimportStatusFlags::ReimportAsset) == EFbxSceneReimportStatusFlags::None)
		{
			continue;
		}
		//Make sure we load all package that will be deleted
		UPackage* PkgExist = LoadPackage(nullptr, *(MeshInfo->GetImportPath()), LOAD_Verify | LOAD_NoWarn);
		if (PkgExist == nullptr)
		{
			continue;
		}
		PkgExist->FullyLoad();
		//Find the asset
		AssetDataToDelete.Add(AssetRegistryModule.Get().GetAssetByObjectPath(FName(*(MeshInfo->GetFullImportName()))));
	}

	AllNewAssets.Empty();
	AssetToSyncContentBrowser.Empty();
	EReimportResult::Type ReimportResult = EReimportResult::Succeeded;
	//Reimport and add asset
	for (TSharedPtr<FFbxMeshInfo> MeshInfo : SceneInfoPtr->MeshInfo)
	{
		if (!MeshStatusMap.Contains(MeshInfo->OriginalImportPath))
		{
			continue;
		}
		EFbxSceneReimportStatusFlags MeshStatus = *(MeshStatusMap.Find(MeshInfo->OriginalImportPath));
		
		//Set the import status for the next reimport
		MeshInfo->bImportAttribute = (MeshStatus & EFbxSceneReimportStatusFlags::ReimportAsset) != EFbxSceneReimportStatusFlags::None;
		
		if ((MeshStatus & EFbxSceneReimportStatusFlags::Removed) != EFbxSceneReimportStatusFlags::None ||
			(MeshStatus & EFbxSceneReimportStatusFlags::ReimportAsset) == EFbxSceneReimportStatusFlags::None)
		{
			continue;
		}

		if (((MeshStatus & EFbxSceneReimportStatusFlags::Same) != EFbxSceneReimportStatusFlags::None || (MeshStatus & EFbxSceneReimportStatusFlags::Added) != EFbxSceneReimportStatusFlags::None) &&
			(MeshStatus & EFbxSceneReimportStatusFlags::FoundContentBrowserAsset) != EFbxSceneReimportStatusFlags::None)
		{
			//Reimport over the old asset
			if (!MeshInfo->bIsSkelMesh)
			{
				ReimportResult = ReimportStaticMesh(FbxImporter, MeshInfo);
			}
			else
			{
				//TODO reimport skeletal mesh
			}
		}
		else if ((MeshStatus & EFbxSceneReimportStatusFlags::Added) != EFbxSceneReimportStatusFlags::None || (MeshStatus & EFbxSceneReimportStatusFlags::Same) != EFbxSceneReimportStatusFlags::None)
		{
			//Create a package for this node
			//Get Parent hierarchy name to create new package path
			ReimportResult = ImportStaticMesh(FbxImporter, MeshInfo, SceneInfoPtr);
		}
	}

	//Put back the default option in the static mesh import data, so next import will have those last import option
	SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(GlobalImportSettingsReference, GlobalImportSettings);
	SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(GlobalImportSettingsReference, SceneImportOptionsStaticMesh);
	SceneImportOptionsStaticMesh->FillStaticMeshInmportData(StaticMeshImportData, SceneImportOptions);
	StaticMeshImportData->SaveConfig();

	//Update the blueprint
	UBlueprint *ReimportBlueprint = nullptr;
	if (bCanReimportHierarchy && GlobalImportSettingsReference->bImportScene)
	{
		ReimportBlueprint = UpdateOriginalBluePrint(ReimportData->BluePrintFullName, &NodeStatusMap, SceneInfoPtr, ReimportData->SceneInfoSourceData, AssetDataToDelete);
	}

	//Remove the deleted meshinfo node from the reimport data
	TArray<TSharedPtr<FFbxMeshInfo>> ToRemoveHierarchyNode;
	for (TSharedPtr<FFbxMeshInfo> MeshInfo : ReimportData->SceneInfoSourceData->MeshInfo)
	{
		EFbxSceneReimportStatusFlags MeshStatus = *(MeshStatusMap.Find(MeshInfo->OriginalImportPath));
		if ((MeshStatus & EFbxSceneReimportStatusFlags::Removed) != EFbxSceneReimportStatusFlags::None)
		{
			ToRemoveHierarchyNode.Add(MeshInfo);
		}
	}
	for (TSharedPtr<FFbxMeshInfo> MeshInfo : ToRemoveHierarchyNode)
	{
		ReimportData->SceneInfoSourceData->MeshInfo.Remove(MeshInfo);
	}
	ReimportData->Modify();
	ReimportData->PostEditChange();
	
	//Make sure the content browser is in sync before we delete
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(AssetToSyncContentBrowser);

	if (AssetDataToDelete.Num() > 0)
	{
		bool AbortDelete = false;
		if (ReimportBlueprint != nullptr)
		{
			//Save the blueprint to avoid reference from the old blueprint
			FAssetData ReimportBlueprintAsset(ReimportBlueprint);
			TArray<UPackage*> Packages;
			Packages.Add(ReimportBlueprintAsset.GetPackage());
			FEditorFileUtils::PromptForCheckoutAndSave(Packages, false, false);

			//Make sure the Asset registry is up to date after the save
			TArray<FString> Paths;
			Paths.Add(ReimportBlueprintAsset.PackagePath.ToString());
			AssetRegistryModule.Get().ScanPathsSynchronous(Paths, true);
		}

		if (!AbortDelete)
		{
			//Delete the asset and use the normal dialog to make sure the user understand he will remove some content
			//The user can decide to cancel the delete or not. This will not interrupt the reimport process
			//The delete is done at the end because we want to remove the blueprint reference before deleting object
			ObjectTools::DeleteAssets(AssetDataToDelete, true);
		}
	}
	//Make sure the content browser is in sync
	ContentBrowserModule.Get().SyncBrowserToAssets(AssetToSyncContentBrowser);
	
	AllNewAssets.Empty();
	
	GlobalImportSettings = nullptr;
	GlobalImportSettingsReference = nullptr;

	FbxImporter->ReleaseScene();
	FbxImporter = nullptr;
	GWarn->EndSlowTask();
	return EReimportResult::Succeeded;
}

void RemoveChildNodeRecursively(USimpleConstructionScript* SimpleConstructionScript, USCS_Node* ScsNode)
{
	TArray<USCS_Node*> ChildNodes = ScsNode->GetChildNodes();
	for (USCS_Node* ChildNode : ChildNodes)
	{
		RemoveChildNodeRecursively(SimpleConstructionScript, ChildNode);
	}
	SimpleConstructionScript->RemoveNode(ScsNode);
}

UBlueprint *UReimportFbxSceneFactory::UpdateOriginalBluePrint(FString &BluePrintFullName, void* VoidNodeStatusMapPtr, TSharedPtr<FFbxSceneInfo> SceneInfoPtr, TSharedPtr<FFbxSceneInfo> SceneInfoOriginalPtr, TArray<FAssetData> &AssetDataToDelete)
{
	if (!SceneInfoPtr.IsValid() || VoidNodeStatusMapPtr == nullptr || !SceneInfoOriginalPtr.IsValid() || BluePrintFullName.IsEmpty())
	{
		return nullptr;
	}

	FbxSceneReimportStatusMapPtr NodeStatusMapPtr = (FbxSceneReimportStatusMapPtr)VoidNodeStatusMapPtr;
	//Find the BluePrint
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData BlueprintAssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FName(*(BluePrintFullName)));
	UPackage* PkgExist = LoadPackage(nullptr, *BlueprintAssetData.PackageName.ToString(), LOAD_Verify | LOAD_NoWarn);
	if (PkgExist == nullptr)
	{
		return nullptr;
	}
	//Load the package before searching the asset
	PkgExist->FullyLoad();
	UBlueprint* BluePrint = FindObjectSafe<UBlueprint>(ANY_PACKAGE, *BluePrintFullName);
	if (BluePrint == nullptr)
	{
		return nullptr;
	}
	//Close all editor that edit this blueprint
	FAssetEditorManager::Get().CloseAllEditorsForAsset(BluePrint);
	//Set the import status for the next reimport
	for (TSharedPtr<FFbxNodeInfo> NodeInfo : SceneInfoPtr->HierarchyInfo)
	{
		if (!NodeStatusMapPtr->Contains(NodeInfo->NodeHierarchyPath))
			continue;
		EFbxSceneReimportStatusFlags NodeStatus = *(NodeStatusMapPtr->Find(NodeInfo->NodeHierarchyPath));
		NodeInfo->bImportNode = (NodeStatus & EFbxSceneReimportStatusFlags::ReimportAsset) != EFbxSceneReimportStatusFlags::None;
	}
	//Add back the component that was in delete state but no flag for reimport
	for (TSharedPtr<FFbxNodeInfo> OriginalNodeInfo : SceneInfoOriginalPtr->HierarchyInfo)
	{
		if (!NodeStatusMapPtr->Contains(OriginalNodeInfo->NodeHierarchyPath))
		{
			continue;
		}

		EFbxSceneReimportStatusFlags NodeStatus = *(NodeStatusMapPtr->Find(OriginalNodeInfo->NodeHierarchyPath));
		if (OriginalNodeInfo->bImportNode != true || (NodeStatus & EFbxSceneReimportStatusFlags::ReimportAsset) != EFbxSceneReimportStatusFlags::None)
		{
			continue;
		}

		//Clear the child
		OriginalNodeInfo->Childrens.Empty();

		//hook the node to the new hierarchy parent
		bool bFoundParent = false;
		if (OriginalNodeInfo->ParentNodeInfo.IsValid())
		{
			int32 InsertIndex = 0;
			for (TSharedPtr<FFbxNodeInfo> NodeInfo : SceneInfoPtr->HierarchyInfo)
			{
				InsertIndex++;
				if (NodeInfo->bImportNode && NodeInfo->NodeHierarchyPath.Compare(OriginalNodeInfo->ParentNodeInfo->NodeHierarchyPath) == 0)
				{
					OriginalNodeInfo->ParentNodeInfo = NodeInfo;
					NodeInfo->Childrens.Add(OriginalNodeInfo);
					SceneInfoPtr->HierarchyInfo.Insert(OriginalNodeInfo, InsertIndex);
					bFoundParent = true;
					break;
				}
			}
		}

		if (!bFoundParent)
		{
			//Insert after the root node
			OriginalNodeInfo->ParentNodeInfo = nullptr;
			SceneInfoPtr->HierarchyInfo.Insert(OriginalNodeInfo, 1);
		}
	}
	//Create a brand new actor with the correct component hierarchy then replace the existing blueprint
	//This function is using the bImportNode flag not the EFbxSceneReimportStatusFlags
	AActor *HierarchyActor = CreateActorComponentsHierarchy(SceneInfoPtr);
	if (HierarchyActor != nullptr)
	{
		//Modify the current blueprint to reflect the new actor
		//Clear all nodes by removing all root node
		TArray<USCS_Node*> BluePrintRootNodes = BluePrint->SimpleConstructionScript->GetRootNodes();
		for(USCS_Node* RootNode : BluePrintRootNodes)
		{
			RemoveChildNodeRecursively(BluePrint->SimpleConstructionScript, RootNode);
		}
		//Create the new nodes from the hierarchy actor
		FKismetEditorUtilities::AddComponentsToBlueprint(BluePrint, HierarchyActor->GetInstanceComponents());
		//Cleanup the temporary actor
		HierarchyActor->Destroy();

		BluePrint->Modify();
		BluePrint->PostEditChange();
		AssetToSyncContentBrowser.Add(BluePrint);
		return BluePrint;
	}
	return nullptr;
}

EReimportResult::Type UReimportFbxSceneFactory::ImportStaticMesh(void* VoidFbxImporter, TSharedPtr<FFbxMeshInfo> MeshInfo, TSharedPtr<FFbxSceneInfo> SceneInfoPtr)
{
	UnFbx::FFbxImporter* FbxImporter = (UnFbx::FFbxImporter*)VoidFbxImporter;
	//FEditorDelegates::OnAssetPreImport.Broadcast(this, UStaticMesh::StaticClass(), GWorld, FName(""), TEXT("fbx"));
	FbxNode *GeometryParentNode = nullptr;
	//Get the first parent geometry node
	for (int idx = 0; idx < FbxImporter->Scene->GetGeometryCount(); ++idx)
	{
		FbxGeometry *Geometry = FbxImporter->Scene->GetGeometry(idx);
		if (Geometry->GetUniqueID() == MeshInfo->UniqueId)
		{
			GeometryParentNode = Geometry->GetNode();
			break;
		}
	}
	if (GeometryParentNode == nullptr)
	{
		FbxImporter->AddTokenizedErrorMessage(FTokenizedMessage::Create(EMessageSeverity::Error, FText::Format(FText::FromString("Reimport Mesh {0} fail, the mesh dont have any parent node inside the fbx."), FText::FromString(MeshInfo->GetImportPath()))), FName(TEXT("Reimport Fbx Scene")));
		return EReimportResult::Failed;
	}

	FString PackageName = MeshInfo->GetImportPath();
	FString StaticMeshName;
	UPackage* Pkg = CreatePackageForNode(PackageName, StaticMeshName);
	if (Pkg == nullptr)
	{
		return EReimportResult::Failed;
	}

	//Copy default options to StaticMeshImportData
	SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(GlobalImportSettingsReference, SceneImportOptionsStaticMesh);
	SceneImportOptionsStaticMesh->FillStaticMeshInmportData(StaticMeshImportData, SceneImportOptions);

	UnFbx::FBXImportOptions* OverrideImportSettings = GetOptionsFromName(MeshInfo->OptionName);
	if (OverrideImportSettings != nullptr)
	{
		SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(OverrideImportSettings, GlobalImportSettings);
		SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(OverrideImportSettings, SceneImportOptionsStaticMesh);
	}
	else
	{
		SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(GlobalImportSettingsReference, GlobalImportSettings);
		SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(GlobalImportSettingsReference, SceneImportOptionsStaticMesh);
	}
	SceneImportOptionsStaticMesh->FillStaticMeshInmportData(StaticMeshImportData, SceneImportOptions);

	FName StaticMeshFName = FName(*(MeshInfo->Name));
	UStaticMesh *NewObject = FbxImporter->ImportStaticMesh(Pkg, GeometryParentNode, StaticMeshFName, EObjectFlags::RF_Standalone, StaticMeshImportData );
	if (NewObject == nullptr)
	{
		return EReimportResult::Failed;
	}
	AllNewAssets.Add(MeshInfo, NewObject);
	AssetToSyncContentBrowser.Add(NewObject);
	return EReimportResult::Succeeded;
}

EReimportResult::Type UReimportFbxSceneFactory::ReimportStaticMesh(void* VoidFbxImporter, TSharedPtr<FFbxMeshInfo> MeshInfo)
{
	UnFbx::FFbxImporter* FbxImporter = (UnFbx::FFbxImporter*)VoidFbxImporter;
	//Find the UObject associate with this MeshInfo
	UPackage* PkgExist = LoadPackage(nullptr, *(MeshInfo->GetImportPath()), LOAD_Verify | LOAD_NoWarn);
	if (PkgExist != nullptr)
	{
		PkgExist->FullyLoad();
	}

	FString AssetName = MeshInfo->GetFullImportName();
	UStaticMesh* Mesh = FindObjectSafe<UStaticMesh>(ANY_PACKAGE, *AssetName);
	if (Mesh == nullptr)
	{
		//We reimport only static mesh here
		FbxImporter->AddTokenizedErrorMessage(FTokenizedMessage::Create(EMessageSeverity::Error, FText::Format(FText::FromString("Reimport Mesh {0} fail, the original staicmesh in the content browser cannot be load."), FText::FromString(MeshInfo->GetImportPath()))), FName(TEXT("Reimport Fbx Scene")));
		return EReimportResult::Failed;
	}
	
	//Copy default options to StaticMeshImportData
	SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(GlobalImportSettingsReference, SceneImportOptionsStaticMesh);
	SceneImportOptionsStaticMesh->FillStaticMeshInmportData(StaticMeshImportData, SceneImportOptions);

	UnFbx::FBXImportOptions* OverrideImportSettings = GetOptionsFromName(MeshInfo->OptionName);
	if (OverrideImportSettings != nullptr)
	{
		SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(OverrideImportSettings, GlobalImportSettings);
		SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(OverrideImportSettings, SceneImportOptionsStaticMesh);
	}
	else
	{
		SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(GlobalImportSettingsReference, GlobalImportSettings);
		SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(GlobalImportSettingsReference, SceneImportOptionsStaticMesh);
	}
	SceneImportOptionsStaticMesh->FillStaticMeshInmportData(StaticMeshImportData, SceneImportOptions);

	FbxImporter->ApplyTransformSettingsToFbxNode(FbxImporter->Scene->GetRootNode(), StaticMeshImportData);
	const TArray<UAssetUserData*>* UserData = Mesh->GetAssetUserDataArray();
	TArray<UAssetUserData*> UserDataCopy;
	if (UserData)
	{
		for (int32 Idx = 0; Idx < UserData->Num(); Idx++)
		{
			UserDataCopy.Add((UAssetUserData*)StaticDuplicateObject((*UserData)[Idx], GetTransientPackage()));
		}
	}

	// preserve settings in navcollision subobject
	UNavCollision* NavCollision = Mesh->NavCollision ?
		(UNavCollision*)StaticDuplicateObject(Mesh->NavCollision, GetTransientPackage()) :
		nullptr;

	// preserve extended bound settings
	const FVector PositiveBoundsExtension = Mesh->PositiveBoundsExtension;
	const FVector NegativeBoundsExtension = Mesh->NegativeBoundsExtension;
	Mesh = FbxImporter->ReimportSceneStaticMesh(MeshInfo->UniqueId, Mesh, StaticMeshImportData);
	if (Mesh != nullptr)
	{
		//Put back the new mesh data since the reimport is putting back the original import data
		SceneImportOptionsStaticMesh->FillStaticMeshInmportData(StaticMeshImportData, SceneImportOptions);
		Mesh->AssetImportData = StaticMeshImportData;

		// Copy user data to newly created mesh
		for (int32 Idx = 0; Idx < UserDataCopy.Num(); Idx++)
		{
			UserDataCopy[Idx]->Rename(nullptr, Mesh, REN_DontCreateRedirectors | REN_DoNotDirty);
			Mesh->AddAssetUserData(UserDataCopy[Idx]);
		}

		if (NavCollision)
		{
			Mesh->NavCollision = NavCollision;
			NavCollision->Rename(nullptr, Mesh, REN_DontCreateRedirectors | REN_DoNotDirty);
		}

		// Restore bounds extension settings
		Mesh->PositiveBoundsExtension = PositiveBoundsExtension;
		Mesh->NegativeBoundsExtension = NegativeBoundsExtension;

		Mesh->AssetImportData->Update(FbxImportFileName);

		// Try to find the outer package so we can dirty it up
		if (Mesh->GetOutermost())
		{
			Mesh->GetOutermost()->MarkPackageDirty();
		}
		else
		{
			Mesh->MarkPackageDirty();
		}
		AllNewAssets.Add(MeshInfo, Mesh);
		AssetToSyncContentBrowser.Add(Mesh);
	}
	else
	{
		return EReimportResult::Failed;
	}
	return EReimportResult::Succeeded;
}

int32 UReimportFbxSceneFactory::GetPriority() const
{
	return ImportPriority;
}

#undef LOCTEXT_NAMESPACE
