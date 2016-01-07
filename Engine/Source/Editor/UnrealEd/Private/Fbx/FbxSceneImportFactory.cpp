// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Factories.h"
#include "BusyCursor.h"
#include "SSkeletonWidget.h"
#include "Dialogs/DlgPickPath.h"

#include "AssetSelection.h"

#include "FbxImporter.h"
#include "FbxErrors.h"
#include "AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Animation/SkeletalMeshActor.h"
#include "PackageTools.h"

#include "SFbxSceneOptionWindow.h"
#include "MainFrame.h"
#include "PackageTools.h"

#include "Editor/KismetCompiler/Public/KismetCompilerModule.h"
#include "Kismet2/KismetEditorUtilities.h"

#include "OutputDevice.h"

#include "FbxImporter.h"

#define LOCTEXT_NAMESPACE "FBXSceneImportFactory"

using namespace UnFbx;

//////////////////////////////////////////////////////////////////////////
// TODO list
// -. Set the combineMesh to true when importing a group of staticmesh
// -. Export correctly skeletal mesh, in fbxreview the skeletal mesh is not
//    playing is animation.
// -. Create some tests
// -. Support for camera
// -. Support for light

//Initialize static default option name
FString UFbxSceneImportFactory::DefaultOptionName = FString(TEXT("Default"));

bool GetFbxSceneImportOptions(UnFbx::FFbxImporter* FbxImporter
	, TSharedPtr<FFbxSceneInfo> SceneInfoPtr
	, UnFbx::FBXImportOptions *GlobalImportSettings
	, UFbxSceneImportOptions *SceneImportOptions
	, UFbxSceneImportOptionsStaticMesh *StaticMeshImportData
	, ImportOptionsNameMap &NameOptionsMap
	, UFbxSceneImportOptionsSkeletalMesh *SkeletalMeshImportData
	, UFbxSceneImportOptionsAnimation *AnimSequenceImportData
	, UFbxSceneImportOptionsMaterial *TextureImportData
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
		.Title(NSLOCTEXT("UnrealEd", "FBXSceneImportOpionsTitle", "FBX Scene Import Options"));
	TSharedPtr<SFbxSceneOptionWindow> FbxSceneOptionWindow;

	Window->SetContent
		(
			SAssignNew(FbxSceneOptionWindow, SFbxSceneOptionWindow)
			.SceneInfo(SceneInfoPtr)
			.GlobalImportSettings(GlobalImportSettings)
			.SceneImportOptionsDisplay(SceneImportOptions)
			.SceneImportOptionsStaticMeshDisplay(StaticMeshImportData)
			.OverrideNameOptionsMap(&NameOptionsMap)
			.SceneImportOptionsSkeletalMeshDisplay(SkeletalMeshImportData)
			.SceneImportOptionsAnimationDisplay(AnimSequenceImportData)
			.SceneImportOptionsMaterialDisplay(TextureImportData)
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
				//If user dont want to import a material we have to replace it by the default material
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

	//Save the import options in ini config file
	SceneImportOptions->SaveConfig();

	//Save the Default setting copy them in the UObject and save them
	SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(GlobalImportSettings, StaticMeshImportData);
	StaticMeshImportData->SaveConfig();

	//TODO save all other options

	return true;
}

bool IsEmptyAttribute(FString AttributeType)
{
	return (AttributeType.Compare("eNull") == 0 || AttributeType.Compare("eUnknown") == 0);
}

static void ExtractPropertyTextures(FbxSurfaceMaterial *FbxMaterial, TSharedPtr<FFbxMaterialInfo> MaterialInfo, const char *MaterialProperty, TMap<uint64, TSharedPtr<FFbxTextureInfo>> ExtractedTextures)
{
	FbxProperty FbxProperty = FbxMaterial->FindProperty(MaterialProperty);
	if (FbxProperty.IsValid())
	{
		int32 LayeredTextureCount = FbxProperty.GetSrcObjectCount<FbxLayeredTexture>();
		if (LayeredTextureCount == 0)
		{
			int32 TextureCount = FbxProperty.GetSrcObjectCount<FbxTexture>();
			if (TextureCount > 0)
			{
				for (int32 TextureIndex = 0; TextureIndex < TextureCount; ++TextureIndex)
				{
					FbxFileTexture* FbxTexture = FbxProperty.GetSrcObject<FbxFileTexture>(TextureIndex);
					TSharedPtr<FFbxTextureInfo> TextureInfo = nullptr;
					if (ExtractedTextures.Contains(FbxTexture->GetUniqueID()))
					{
						TextureInfo = *(ExtractedTextures.Find(FbxTexture->GetUniqueID()));
					}
					else
					{
						TextureInfo = MakeShareable(new FFbxTextureInfo());
						TextureInfo->Name = UTF8_TO_TCHAR(FbxTexture->GetName());
						TextureInfo->UniqueId = FbxTexture->GetUniqueID();
						TextureInfo->TexturePath = UTF8_TO_TCHAR(FbxTexture->GetFileName());
						ExtractedTextures.Add(TextureInfo->UniqueId, TextureInfo);
					}
					//Add the texture
					MaterialInfo->Textures.Add(TextureInfo);
				}
			}
		}
	}
}

static void ExtractMaterialInfoFromNode(UnFbx::FFbxImporter* FbxImporter, FbxNode *Node, TSharedPtr<FFbxSceneInfo> SceneInfoPtr, TMap<uint64, TSharedPtr<FFbxMaterialInfo>> ExtractedMaterials, TMap<uint64, TSharedPtr<FFbxTextureInfo>> ExtractedTextures, FString CurrentHierarchyPath)
{
	TSharedPtr<FFbxNodeInfo> FoundNode = nullptr;
	for (TSharedPtr<FFbxNodeInfo> Nodeinfo : SceneInfoPtr->HierarchyInfo)
	{
		if (Nodeinfo->UniqueId == Node->GetUniqueID())
		{
			FoundNode = Nodeinfo;
		}
	}
	if (FoundNode.IsValid())
	{
		if (!CurrentHierarchyPath.IsEmpty())
		{
			CurrentHierarchyPath += TEXT("/");
		}
		CurrentHierarchyPath += FoundNode->NodeName;

		for (int MaterialIndex = 0; MaterialIndex < Node->GetMaterialCount(); ++MaterialIndex)
		{
			FbxSurfaceMaterial *FbxMaterial = Node->GetMaterial(MaterialIndex);
			TSharedPtr<FFbxMaterialInfo> MaterialInfo = nullptr;
			if (ExtractedMaterials.Contains(FbxMaterial->GetUniqueID()))
			{
				MaterialInfo = *(ExtractedMaterials.Find(FbxMaterial->GetUniqueID()));
			}
			else
			{
				MaterialInfo = MakeShareable(new FFbxMaterialInfo());
				MaterialInfo->HierarchyPath = CurrentHierarchyPath;
				MaterialInfo->UniqueId = FbxMaterial->GetUniqueID();
				MaterialInfo->Name = UTF8_TO_TCHAR(FbxMaterial->GetName());
				ExtractPropertyTextures(FbxMaterial, MaterialInfo, FbxSurfaceMaterial::sDiffuse, ExtractedTextures);
				ExtractPropertyTextures(FbxMaterial, MaterialInfo, FbxSurfaceMaterial::sEmissive, ExtractedTextures);
				ExtractPropertyTextures(FbxMaterial, MaterialInfo, FbxSurfaceMaterial::sSpecular, ExtractedTextures);
				//ExtractPropertyTextures(FbxMaterial, MaterialInfo, FbxSurfaceMaterial::sSpecularFactor, ExtractedTextures);
				//ExtractPropertyTextures(FbxMaterial, MaterialInfo, FbxSurfaceMaterial::sShininess, ExtractedTextures);
				ExtractPropertyTextures(FbxMaterial, MaterialInfo, FbxSurfaceMaterial::sNormalMap, ExtractedTextures);
				ExtractPropertyTextures(FbxMaterial, MaterialInfo, FbxSurfaceMaterial::sBump, ExtractedTextures);
				ExtractPropertyTextures(FbxMaterial, MaterialInfo, FbxSurfaceMaterial::sTransparentColor, ExtractedTextures);
				ExtractPropertyTextures(FbxMaterial, MaterialInfo, FbxSurfaceMaterial::sTransparencyFactor, ExtractedTextures);
				ExtractedMaterials.Add(MaterialInfo->UniqueId, MaterialInfo);
			}
			//Add the Material to the node
			FoundNode->Materials.AddUnique(MaterialInfo);
		}
	}
	for (int ChildIndex = 0; ChildIndex < Node->GetChildCount(); ++ChildIndex)
	{
		FbxNode *ChildNode = Node->GetChild(ChildIndex);
		ExtractMaterialInfoFromNode(FbxImporter, ChildNode, SceneInfoPtr, ExtractedMaterials, ExtractedTextures, CurrentHierarchyPath);
	}
}

void UFbxSceneImportFactory::ExtractMaterialInfo(void* FbxImporterVoid, TSharedPtr<FFbxSceneInfo> SceneInfoPtr)
{
	UnFbx::FFbxImporter* FbxImporter = (UnFbx::FFbxImporter*)FbxImporterVoid;
	TMap<uint64, TSharedPtr<FFbxMaterialInfo>> ExtractedMaterials;
	TMap<uint64, TSharedPtr<FFbxTextureInfo>> ExtractedTextures;
	FbxNode* RootNode = FbxImporter->Scene->GetRootNode();
	FString CurrentHierarchyPath = TEXT("");
	ExtractMaterialInfoFromNode(FbxImporter, RootNode, SceneInfoPtr, ExtractedMaterials, ExtractedTextures, CurrentHierarchyPath);
}

// TODO we should replace the old UnFbx:: data by the new data that use shared pointer.
// For now we convert the old structure to the new one
TSharedPtr<FFbxSceneInfo> UFbxSceneImportFactory::ConvertSceneInfo(void* VoidFbxSceneInfo)
{
	UnFbx::FbxSceneInfo &SceneInfo = *((UnFbx::FbxSceneInfo*)VoidFbxSceneInfo);
	TSharedPtr<FFbxSceneInfo> SceneInfoPtr = MakeShareable(new FFbxSceneInfo());
	SceneInfoPtr->NonSkinnedMeshNum = SceneInfo.NonSkinnedMeshNum;
	SceneInfoPtr->SkinnedMeshNum = SceneInfo.SkinnedMeshNum;
	SceneInfoPtr->TotalGeometryNum = SceneInfo.TotalGeometryNum;
	SceneInfoPtr->TotalMaterialNum = SceneInfo.TotalMaterialNum;
	SceneInfoPtr->TotalTextureNum = SceneInfo.TotalTextureNum;
	SceneInfoPtr->bHasAnimation = SceneInfo.bHasAnimation;
	SceneInfoPtr->FrameRate = SceneInfo.FrameRate;
	SceneInfoPtr->TotalTime = SceneInfo.TotalTime;
	for (const UnFbx::FbxMeshInfo MeshInfo : SceneInfo.MeshInfo)
	{
		TSharedPtr<FFbxMeshInfo> MeshInfoPtr = MakeShareable(new FFbxMeshInfo());
		MeshInfoPtr->FaceNum = MeshInfo.FaceNum;
		MeshInfoPtr->VertexNum = MeshInfo.VertexNum;
		MeshInfoPtr->bTriangulated = MeshInfo.bTriangulated;
		MeshInfoPtr->MaterialNum = MeshInfo.MaterialNum;
		MeshInfoPtr->bIsSkelMesh = MeshInfo.bIsSkelMesh;
		MeshInfoPtr->SkeletonRoot = MeshInfo.SkeletonRoot;
		MeshInfoPtr->SkeletonElemNum = MeshInfo.SkeletonElemNum;
		MeshInfoPtr->LODGroup = MeshInfo.LODGroup;
		MeshInfoPtr->LODLevel = MeshInfo.LODLevel;
		MeshInfoPtr->MorphNum = MeshInfo.MorphNum;
		MeshInfoPtr->Name = MeshInfo.Name;
		MeshInfoPtr->UniqueId = MeshInfo.UniqueId;
		SceneInfoPtr->MeshInfo.Add(MeshInfoPtr);
	}
	for (const UnFbx::FbxNodeInfo& NodeInfo : SceneInfo.HierarchyInfo)
	{
		TSharedPtr<FFbxNodeInfo> NodeInfoPtr = MakeShareable(new FFbxNodeInfo());
		NodeInfoPtr->NodeName = NodeInfo.ObjectName;
		NodeInfoPtr->UniqueId = NodeInfo.UniqueId;
		NodeInfoPtr->AttributeType = NodeInfo.AttributeType;
		
		//Find the parent
		NodeInfoPtr->ParentNodeInfo = nullptr;
		for (TSharedPtr<FFbxNodeInfo> ParentPtr : SceneInfoPtr->HierarchyInfo)
		{
			if (ParentPtr->UniqueId == NodeInfo.ParentUniqueId)
			{
				NodeInfoPtr->ParentNodeInfo = ParentPtr;
				ParentPtr->Childrens.Add(NodeInfoPtr);
				break;
			}
		}

		//Find the attribute info
		NodeInfoPtr->AttributeInfo = nullptr;
		for (TSharedPtr<FFbxMeshInfo> AttributePtr : SceneInfoPtr->MeshInfo)
		{
			if (AttributePtr->UniqueId == NodeInfo.AttributeUniqueId)
			{
				NodeInfoPtr->AttributeInfo = AttributePtr;
				break;
			}
		}

		//Set the transform
		NodeInfoPtr->Transform = FTransform::Identity;
		FbxVector4 NewLocalT = NodeInfo.Transform.GetT();
		FbxVector4 NewLocalS = NodeInfo.Transform.GetS();
		FbxQuaternion NewLocalQ = NodeInfo.Transform.GetQ();
		NodeInfoPtr->Transform.SetTranslation(UnFbx::FFbxDataConverter::ConvertPos(NewLocalT));
		NodeInfoPtr->Transform.SetScale3D(UnFbx::FFbxDataConverter::ConvertScale(NewLocalS));
		NodeInfoPtr->Transform.SetRotation(UnFbx::FFbxDataConverter::ConvertRotToQuat(NewLocalQ));

		//by default we import all node
		NodeInfoPtr->bImportNode = true;

		//Add the node to the hierarchy
		SceneInfoPtr->HierarchyInfo.Add(NodeInfoPtr);
	}
	return SceneInfoPtr;
}

UClass *FFbxMeshInfo::GetType()
{
	if (bIsSkelMesh)
		return USkeletalMesh::StaticClass();
	return UStaticMesh::StaticClass();
}

UClass *FFbxTextureInfo::GetType()
{
	return UTexture::StaticClass();
}

UClass *FFbxMaterialInfo::GetType()
{
	return UMaterial::StaticClass();
}

UPackage *FFbxAttributeInfo::GetContentPackage()
{
	if (!IsContentObjectUpToDate)
	{
		//Update the Object, this will update the ContentPackage and set the IsContentUpToDate state
		GetContentObject();
	}
	return ContentPackage;
}

UObject *FFbxAttributeInfo::GetContentObject()
{
	if (IsContentObjectUpToDate)
		return ContentObject;
	ContentPackage = nullptr;
	ContentObject = nullptr;
	FString ImportPath = PackageTools::SanitizePackageName(GetImportPath());
	FString AssetName = GetFullImportName();
	ContentPackage = LoadPackage(nullptr, *ImportPath, LOAD_Verify | LOAD_NoWarn);
	if (ContentPackage != nullptr)
	{
		ContentPackage->FullyLoad();
	}
	ContentObject = FindObjectSafe<UObject>(ANY_PACKAGE, *AssetName);
	if (ContentObject != nullptr)
	{
		if (ContentObject->HasAnyFlags(RF_Transient) || ContentObject->IsPendingKill())
		{
			ContentObject = nullptr;
		}
		else if (ContentPackage == nullptr)
		{
			//If we are able to find the object but not to load the package, this mean that the package is a new created package that is not save yet
			ContentPackage = ContentObject->GetOutermost();
		}
	}

	IsContentObjectUpToDate = true;
	return ContentObject;
}

UFbxSceneImportFactory::UFbxSceneImportFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UWorld::StaticClass();
	Formats.Add(TEXT("fbx;Fbx Scene"));

	bCreateNew = false;
	bText = false;
	bEditorImport = true;
	Path = "";
	ImportWasCancel = false;

	SceneImportOptions = CreateDefaultSubobject<UFbxSceneImportOptions>(TEXT("SceneImportOptions"), true);
	SceneImportOptionsStaticMesh = CreateDefaultSubobject<UFbxSceneImportOptionsStaticMesh>(TEXT("SceneImportOptionsStaticMesh"), true);
	SceneImportOptionsSkeletalMesh = CreateDefaultSubobject<UFbxSceneImportOptionsSkeletalMesh>(TEXT("SceneImportOptionsSkeletalMesh"), true);
	SceneImportOptionsAnimation = CreateDefaultSubobject<UFbxSceneImportOptionsAnimation>(TEXT("SceneImportOptionsAnimation"), true);
	SceneImportOptionsMaterial = CreateDefaultSubobject<UFbxSceneImportOptionsMaterial>(TEXT("SceneImportOptionsMaterial"), true);

	StaticMeshImportData = CreateDefaultSubobject<UFbxStaticMeshImportData>(TEXT("StaticMeshImportData"), true);
	SkeletalMeshImportData = CreateDefaultSubobject<UFbxSkeletalMeshImportData>(TEXT("SkeletalMeshImportData"), true);
	AnimSequenceImportData = CreateDefaultSubobject<UFbxAnimSequenceImportData>(TEXT("AnimSequenceImportData"), true);
	TextureImportData = CreateDefaultSubobject<UFbxTextureImportData>(TEXT("TextureImportData"), true);

	ReimportData = nullptr;
}

void UFbxSceneImportFactory::FillSceneHierarchyPath(TSharedPtr<FFbxSceneInfo> SceneInfo)
{
	//Set the hierarchy path for every node this data will be use by the reimport
	for (FbxNodeInfoPtr NodeInfo : SceneInfo->HierarchyInfo)
	{
		FString NodeTreePath = NodeInfo->NodeName;
		FbxNodeInfoPtr CurrentNode = NodeInfo->ParentNodeInfo;
		while (CurrentNode.IsValid())
		{
			NodeTreePath += TEXT(".");
			NodeTreePath += CurrentNode->NodeName;
			CurrentNode = CurrentNode->ParentNodeInfo;
		}
		NodeInfo->NodeHierarchyPath = NodeTreePath;
	}
}

UFbxSceneImportData* CreateReImportAsset(const FString &PackagePath, const FString &FbxImportFileName, UFbxSceneImportOptions* SceneImportOptions, TSharedPtr<FFbxSceneInfo> SceneInfo, ImportOptionsNameMap &NameOptionsMap)
{
	//Create a package
	FString FilenameBase = FPaths::GetBaseFilename(FbxImportFileName);
	FString FbxReImportPkgName = PackagePath + TEXT("/FbxScene_") + FilenameBase;
	FbxReImportPkgName = PackageTools::SanitizePackageName(FbxReImportPkgName);
	FString AssetName = TEXT("FbxScene_") + FilenameBase;
	AssetName = PackageTools::SanitizePackageName(AssetName);
	UPackage* PkgExist = nullptr;
	if (!FEditorFileUtils::IsMapPackageAsset(FbxReImportPkgName))
	{
		PkgExist = FindPackage(nullptr, *FbxReImportPkgName);
	}
	int32 tryCount = 1;
	while (PkgExist != nullptr)
	{
		FbxReImportPkgName = PackagePath + TEXT("/");
		AssetName = TEXT("FbxScene_") + FilenameBase;
		AssetName += TEXT("_");
		AssetName += FString::FromInt(tryCount++);
		FbxReImportPkgName += AssetName;
		FbxReImportPkgName = PackageTools::SanitizePackageName(FbxReImportPkgName);
		if (!FEditorFileUtils::IsMapPackageAsset(FbxReImportPkgName))
		{
			PkgExist = FindPackage(nullptr, *FbxReImportPkgName);
		}
	}
	UPackage* Pkg = CreatePackage(nullptr, *FbxReImportPkgName);
	if (!ensure(Pkg))
	{
		//TODO log an import warning stipulate that there is no re-import asset created
		return nullptr;
	}
	Pkg->FullyLoad();

	FbxReImportPkgName = FPackageName::GetLongPackageAssetName(Pkg->GetOutermost()->GetName());
	//Save the re-import data asset
	UFbxSceneImportData *ReImportAsset = NewObject<UFbxSceneImportData>(Pkg, NAME_None, RF_Public | RF_Standalone);
	FString NewUniqueName = AssetName;
	if (!ReImportAsset->Rename(*NewUniqueName, nullptr, REN_Test))
	{
		NewUniqueName = MakeUniqueObjectName(ReImportAsset, UFbxSceneImportData::StaticClass(), FName(*AssetName)).ToString();
	}
	ReImportAsset->Rename(*NewUniqueName);
	ReImportAsset->SceneInfoSourceData = SceneInfo;
	//Copy the options map
	for (auto kvp : NameOptionsMap)
	{
		ReImportAsset->NameOptionsMap.Add(kvp.Key, kvp.Value);
	}
	
	ReImportAsset->SourceFbxFile = FPaths::ConvertRelativePathToFull(FbxImportFileName);
	ReImportAsset->bCreateFolderHierarchy = SceneImportOptions->bCreateContentFolderHierarchy;
	ReImportAsset->HierarchyType = SceneImportOptions->HierarchyType.GetValue();
	return ReImportAsset;
}


UObject* UFbxSceneImportFactory::FactoryCreateBinary
(
	UClass*				Class,
	UObject*			InParent,
	FName				Name,
	EObjectFlags		Flags,
	UObject*			Context,
	const TCHAR*		Type,
	const uint8*&		Buffer,
	const uint8*		BufferEnd,
	FFeedbackContext*	Warn,
	bool& bOutOperationCanceled
	)
{
	UObject* ReturnObject = FactoryCreateBinary(Class, InParent, Name, Flags, Context, Type, Buffer, BufferEnd, Warn);
	bOutOperationCanceled = ImportWasCancel;
	ImportWasCancel = false;
	return ReturnObject;
}

UObject* UFbxSceneImportFactory::FactoryCreateBinary
(
UClass*				Class,
UObject*			InParent,
FName				Name,
EObjectFlags		Flags,
UObject*			Context,
const TCHAR*		Type,
const uint8*&		Buffer,
const uint8*		BufferEnd,
FFeedbackContext*	Warn
)
{
	if (InParent == nullptr)
	{
		return nullptr;
	}
	NameOptionsMap.Reset();
	UWorld* World = GWorld;
	ULevel* CurrentLevel = World->GetCurrentLevel();
	//We will call other factory store the filename value since UFactory::CurrentFilename is static
	FString FbxImportFileName = UFactory::CurrentFilename;
	// Unselect all actors.
	GEditor->SelectNone(false, false);

	FEditorDelegates::OnAssetPreImport.Broadcast(this, Class, InParent, Name, Type);
	
	//TODO verify if we really need this when instancing actor in a level from an import
	//In that case we should change the variable name.
	GEditor->IsImportingT3D = 1;
	GIsImportingT3D = GEditor->IsImportingT3D;

	// logger for all error/warnings
	// this one prints all messages that are stored in FFbxImporter
	UnFbx::FFbxImporter* FbxImporter = UnFbx::FFbxImporter::GetInstance();

	UnFbx::FFbxLoggerSetter Logger(FbxImporter);

	Warn->BeginSlowTask(NSLOCTEXT("FbxSceneFactory", "BeginImportingFbxSceneTask", "Importing FBX scene"), true);
	
	GlobalImportSettings = FbxImporter->GetImportOptions();
	
	//Always convert the scene
	GlobalImportSettings->bConvertScene = true;

	//Read the fbx and store the hierarchy's information so we can reuse it after importing all the model in the fbx file
	if (!FbxImporter->ImportFromFile(*FbxImportFileName, Type))
	{
		// Log the error message and fail the import.
		Warn->Log(ELogVerbosity::Error, FbxImporter->GetErrorMessage());
		FbxImporter->ReleaseScene();
		FbxImporter = nullptr;
		// Mark us as no longer importing a T3D.
		GEditor->IsImportingT3D = 0;
		GIsImportingT3D = false;
		Warn->EndSlowTask();
		FEditorDelegates::OnAssetPostImport.Broadcast(this, World);
		return nullptr;
	}

	FString PackageName = "";
	InParent->GetName(PackageName);
	Path = FPaths::GetPath(PackageName);

	UnFbx::FbxSceneInfo SceneInfo;
	//Read the scene and found all instance with their scene information.
	FbxImporter->GetSceneInfo(FbxImportFileName, SceneInfo);
	
	GlobalImportSettingsReference = new UnFbx::FBXImportOptions();
	SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(GlobalImportSettings, GlobalImportSettingsReference);

	//Convert old structure to the new scene export structure
	TSharedPtr<FFbxSceneInfo> SceneInfoPtr = ConvertSceneInfo(&SceneInfo);
	
	//Get import material info
	ExtractMaterialInfo(FbxImporter, SceneInfoPtr);

	if(!GetFbxSceneImportOptions(FbxImporter
		, SceneInfoPtr
		, GlobalImportSettingsReference
		, SceneImportOptions
		, SceneImportOptionsStaticMesh
		, NameOptionsMap
		, SceneImportOptionsSkeletalMesh
		, SceneImportOptionsAnimation
		, SceneImportOptionsMaterial
		, Path))
	{
		//User cancel the scene import
		ImportWasCancel = true;
		FbxImporter->ReleaseScene();
		FbxImporter = nullptr;
		GlobalImportSettings = nullptr;
		GlobalImportSettingsReference = nullptr;
		// Mark us as no longer importing a T3D.
		GEditor->IsImportingT3D = 0;
		GIsImportingT3D = false;
		Warn->EndSlowTask();
		FEditorDelegates::OnAssetPostImport.Broadcast(this, World);
		return nullptr;
	}

	SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(GlobalImportSettingsReference, GlobalImportSettings);

	FillSceneHierarchyPath(SceneInfoPtr);

	ReimportData = CreateReImportAsset(Path, FbxImportFileName, SceneImportOptions, SceneInfoPtr, NameOptionsMap);
	if (ReimportData == nullptr)
	{
		//Cannot save the reimport data
		const FText CreateReimportDataFailed = LOCTEXT("CreateReimportDataFailed", "Failed to create the re import data asset, which will make impossible the re import of this scene.\nLook in the logs to see the reason.\nPress Ok to continue or Cancel to abort the import process");
		if (FMessageDialog::Open(EAppMsgType::OkCancel, CreateReimportDataFailed) == EAppReturnType::Cancel)
		{
			//User cancel the scene import
			ImportWasCancel = true;
			FbxImporter->ReleaseScene();
			FbxImporter = nullptr;
			GlobalImportSettings = nullptr;
			// Mark us as no longer importing a T3D.
			GEditor->IsImportingT3D = 0;
			GIsImportingT3D = false;
			Warn->EndSlowTask();
			FEditorDelegates::OnAssetPostImport.Broadcast(this, World);
			return nullptr;
		}
	}

	//We are a scene import set the flag for the reimport factory
	StaticMeshImportData->bImportAsScene = true;
	StaticMeshImportData->FbxSceneImportDataReference = ReimportData;

	//Get the scene root node
	FbxNode* RootNodeToImport = nullptr;
	RootNodeToImport = FbxImporter->Scene->GetRootNode();
	
	//Apply the options scene transform to the root node
	ApplySceneTransformOptionsToRootNode(SceneInfoPtr);

	// For animation and static mesh we assume there is at lease one interesting node by default
	int32 InterestingNodeCount = 1;

	AllNewAssets.Empty();

	int32 NodeIndex = 0;

	//////////////////////////////////////////////////////////////////////////
	// IMPORT ALL STATIC MESH
	ImportAllStaticMesh(RootNodeToImport, FbxImporter, Flags, NodeIndex, InterestingNodeCount, SceneInfoPtr);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	//IMPORT ALL SKELETAL MESH
	ImportAllSkeletalMesh(RootNodeToImport, FbxImporter, Flags, NodeIndex, InterestingNodeCount, SceneInfoPtr);
	//////////////////////////////////////////////////////////////////////////

	UObject *ReturnObject = nullptr;
	for (auto ItAsset = AllNewAssets.CreateIterator(); ItAsset; ++ItAsset)
	{
		UObject *AssetObject = ItAsset.Value();
		if (AssetObject && ReturnObject == nullptr)
		{
			//Set the first import object as the return object to prevent false error from the caller of this factory
			ReturnObject = AssetObject;
		}
		if (AssetObject->IsA(UStaticMesh::StaticClass()) || AssetObject->IsA(USkeletalMesh::StaticClass()))
		{
			//Mark the mesh as modified so the render will draw the mesh correctly
			AssetObject->Modify();
			AssetObject->PostEditChange();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// CREATE AND PLACE ACTOR
	// Instantiate all the scene hierarchy in the current level with link to previous created objects
	// go through the hierarchy and instantiate actor in the current level	
	switch (SceneImportOptions->HierarchyType)
	{
		case EFBXSceneOptionsCreateHierarchyType::FBXSOCHT_CreateLevelActors:
		{
			CreateLevelActorHierarchy(SceneInfoPtr);
		}
		break;
		case EFBXSceneOptionsCreateHierarchyType::FBXSOCHT_CreateActorComponents:
		case EFBXSceneOptionsCreateHierarchyType::FBXSOCHT_CreateBlueprint:
		{
			AActor *HierarchyActor = CreateActorComponentsHierarchy(SceneInfoPtr);
			//If the user want to export to a BP replace the container actor with a BP link
			if (SceneImportOptions->HierarchyType == EFBXSceneOptionsCreateHierarchyType::FBXSOCHT_CreateBlueprint && HierarchyActor != nullptr)
			{
				//The location+name of the BP is the user select content path + fbx base filename
				FString FullnameBP = Path + TEXT("/") + FPaths::GetBaseFilename(UFactory::CurrentFilename);
				FullnameBP = PackageTools::SanitizePackageName(FullnameBP);
				//Create the blueprint from the actor and replace the actor with a blueprintactor that point on the blueprint
				UBlueprint* SceneBlueprint = FKismetEditorUtilities::CreateBlueprintFromActor(FullnameBP, HierarchyActor, true);
				if (SceneBlueprint != nullptr && ReimportData != nullptr)
				{
					//Set the blueprint path name in the re import scene data asset, this will allow re import to find the original import blueprint
					ReimportData->BluePrintFullName = SceneBlueprint->GetPathName();
				}
			}
		}
		break;
	}

	//Release the FbxImporter 
	FbxImporter->ReleaseScene();
	FbxImporter = nullptr;
	GlobalImportSettings = nullptr;
	GlobalImportSettingsReference = nullptr;

	// Mark us as no longer importing a T3D.
	GEditor->IsImportingT3D = 0;
	GIsImportingT3D = false;
	ReimportData = nullptr;

	Warn->EndSlowTask();
	FEditorDelegates::OnAssetPostImport.Broadcast(this, World);

	return ReturnObject;
}

void UFbxSceneImportFactory::ApplySceneTransformOptionsToRootNode(TSharedPtr<FFbxSceneInfo> SceneInfoPtr)
{
	for (TSharedPtr<FFbxNodeInfo> NodeInfo : SceneInfoPtr->HierarchyInfo)
	{
		if (NodeInfo->NodeName.Compare("RootNode") == 0)
		{
			FTransform SceneTransformAddition(SceneImportOptions->ImportRotation, SceneImportOptions->ImportTranslation, FVector(SceneImportOptions->ImportUniformScale));
			FTransform OutTransform = FTransform::Identity;
			//We apply the scene pre transform before the existing root node transform and store the result in the rootnode transform
			FTransform::Multiply(&OutTransform, &NodeInfo->Transform, &SceneTransformAddition);
			NodeInfo->Transform = OutTransform;
			return;
		}
	}
}

bool UFbxSceneImportFactory::SetStaticMeshComponentOverrideMaterial(UStaticMeshComponent* StaticMeshComponent, TSharedPtr<FFbxNodeInfo> NodeInfo)
{
	bool bOverrideMaterial = false;
	UStaticMesh *StaticMesh = StaticMeshComponent->StaticMesh;
	if (StaticMesh->Materials.Num() == NodeInfo->Materials.Num())
	{
		for (int32 MaterialIndex = 0; MaterialIndex < NodeInfo->Materials.Num(); ++MaterialIndex)
		{
			TSharedPtr<FFbxMaterialInfo> MaterialInfo = NodeInfo->Materials[MaterialIndex];
			UMaterialInterface *MaterialInterface = Cast<UMaterialInterface>(MaterialInfo->GetContentObject());
			if (MaterialInterface != nullptr && StaticMesh->GetMaterial(MaterialIndex) != MaterialInterface)
			{
				bOverrideMaterial = true;
				break;
			}
		}
		if (bOverrideMaterial)
		{
			for (int32 MaterialIndex = 0; MaterialIndex < NodeInfo->Materials.Num(); ++MaterialIndex)
			{
				TSharedPtr<FFbxMaterialInfo> MaterialInfo = NodeInfo->Materials[MaterialIndex];
				UMaterialInterface *MaterialInterface = Cast<UMaterialInterface>(MaterialInfo->GetContentObject());
				if (MaterialInterface != nullptr && StaticMesh->GetMaterial(MaterialIndex) != MaterialInterface)
				{
					StaticMeshComponent->SetMaterial(MaterialIndex, MaterialInterface);
				}
			}
		}
	}
	return bOverrideMaterial;
}

void UFbxSceneImportFactory::CreateLevelActorHierarchy(TSharedPtr<FFbxSceneInfo> SceneInfoPtr)
{
	EComponentMobility::Type MobilityType = SceneImportOptions->bImportAsDynamic ? EComponentMobility::Movable : EComponentMobility::Static;
	TMap<uint64, AActor *> NewActorNameMap;
	FTransform RootTransform = FTransform::Identity;
	bool bSelectActor = true;
	//////////////////////////////////////////////////////////////////////////
	// iterate the whole hierarchy and create all actors
	for (TSharedPtr<FFbxNodeInfo>  NodeInfo : SceneInfoPtr->HierarchyInfo)
	{
		if (NodeInfo->NodeName.Compare("RootNode") == 0)
		{
			RootTransform = NodeInfo->Transform;
			continue;
		}

		//Export only the node that are mark for export
		if (!NodeInfo->bImportNode)
		{
			continue;
		}
		//Find the asset that link with this node attribute
		UObject *AssetToPlace = (NodeInfo->AttributeInfo.IsValid() && AllNewAssets.Contains(NodeInfo->AttributeInfo)) ? AllNewAssets[NodeInfo->AttributeInfo] : nullptr;

		//create actor
		AActor *PlacedActor = nullptr;
		if (AssetToPlace != nullptr)
		{
			//Create an actor from the asset
			//default flag is RF_Transactional;
			PlacedActor = FActorFactoryAssetProxy::AddActorForAsset(AssetToPlace, bSelectActor);
			
			//Set the actor override material
			if (PlacedActor->IsA(AStaticMeshActor::StaticClass()))
			{
				UStaticMeshComponent *StaticMeshComponent = Cast<UStaticMeshComponent>(PlacedActor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
				SetStaticMeshComponentOverrideMaterial(StaticMeshComponent, NodeInfo);
			}
		}
		else if (IsEmptyAttribute(NodeInfo->AttributeType) || NodeInfo->AttributeType.Compare("eMesh") == 0)
		{
			//Create an empty actor if the node is an empty attribute or the attribute is a mesh(static mesh or skeletal mesh) that was not export
			UActorFactory* Factory = GEditor->FindActorFactoryByClass(UActorFactoryEmptyActor::StaticClass());
			FAssetData EmptyActorAssetData = FAssetData(Factory->GetDefaultActorClass(FAssetData()));
			//This is a group create an empty actor that just have a transform
			UObject* EmptyActorAsset = EmptyActorAssetData.GetAsset();
			//Place an empty actor
			PlacedActor = FActorFactoryAssetProxy::AddActorForAsset(EmptyActorAsset, bSelectActor);
			USceneComponent* RootComponent = NewObject<USceneComponent>(PlacedActor, USceneComponent::GetDefaultSceneRootVariableName());
			RootComponent->Mobility = MobilityType;
			RootComponent->bVisualizeComponent = true;
			PlacedActor->SetRootComponent(RootComponent);
			PlacedActor->AddInstanceComponent(RootComponent);
			RootComponent->RegisterComponent();
		}
		else
		{
			//TODO log which fbx attribute we cannot create an actor from
		}

		if (PlacedActor != nullptr)
		{
			//Rename the actor correctly
			//When importing a scene we don't want to change the actor name even if there is similar label already existing
			PlacedActor->SetActorLabel(NodeInfo->NodeName);

			USceneComponent* RootComponent = Cast<USceneComponent>(PlacedActor->GetRootComponent());
			if (RootComponent)
			{
				//Set the mobility
				RootComponent->Mobility = MobilityType;
				//Map the new actor name with the old name in case the name is changing
				NewActorNameMap.Add(NodeInfo->UniqueId, PlacedActor);
				uint64 ParentUniqueId = NodeInfo->ParentNodeInfo.IsValid() ? NodeInfo->ParentNodeInfo->UniqueId : 0;
				AActor *ParentActor = nullptr;
				//If there is a parent we must set the parent actor
				if (NewActorNameMap.Contains(ParentUniqueId))
				{
					ParentActor = *NewActorNameMap.Find(ParentUniqueId);
					if (ParentActor != nullptr)
					{
						USceneComponent* ParentRootComponent = Cast<USceneComponent>(ParentActor->GetRootComponent());
						if (ParentRootComponent)
						{
							if (GEditor->CanParentActors(ParentActor, PlacedActor))
							{
								GEditor->ParentActors(ParentActor, PlacedActor, NAME_None);
							}
						}
					}
				}
				//Apply the hierarchy local transform to the root component
				ApplyTransformToComponent(RootComponent, &(NodeInfo->Transform), ParentActor == nullptr ? &RootTransform : nullptr);
			}
		}
		//We select only the first actor
		bSelectActor = false;
	}
	// End of iteration of the hierarchy
	//////////////////////////////////////////////////////////////////////////
}

AActor *UFbxSceneImportFactory::CreateActorComponentsHierarchy(TSharedPtr<FFbxSceneInfo> SceneInfoPtr)
{
	FString FbxImportFileName = UFactory::CurrentFilename;
	UBlueprint *NewBluePrintActor = nullptr;
	AActor *RootActorContainer = nullptr;
	FString FilenameBase = FbxImportFileName.IsEmpty() ? TEXT("TransientToBlueprintActor") : FPaths::GetBaseFilename(FbxImportFileName);
	USceneComponent *ActorRootComponent = nullptr;
	TMap<uint64, USceneComponent *> NewSceneComponentNameMap;
	EComponentMobility::Type MobilityType = SceneImportOptions->bImportAsDynamic ? EComponentMobility::Movable : EComponentMobility::Static;
	
	//////////////////////////////////////////////////////////////////////////
	//Create the Actor where to put components in
	UActorFactory* Factory = GEditor->FindActorFactoryByClass(UActorFactoryEmptyActor::StaticClass());
	FAssetData EmptyActorAssetData = FAssetData(Factory->GetDefaultActorClass(FAssetData()));
	//This is a group create an empty actor that just have a transform
	UObject* EmptyActorAsset = EmptyActorAssetData.GetAsset();
	//Place an empty actor
	RootActorContainer = FActorFactoryAssetProxy::AddActorForAsset(EmptyActorAsset, true);
	check(RootActorContainer != nullptr);
	ActorRootComponent = NewObject<USceneComponent>(RootActorContainer, USceneComponent::GetDefaultSceneRootVariableName());
	check(ActorRootComponent != nullptr);
	ActorRootComponent->Mobility = MobilityType;
	ActorRootComponent->bVisualizeComponent = true;
	RootActorContainer->SetRootComponent(ActorRootComponent);
	RootActorContainer->AddInstanceComponent(ActorRootComponent);
	ActorRootComponent->RegisterComponent();
	RootActorContainer->SetActorLabel(FilenameBase);

	//////////////////////////////////////////////////////////////////////////
	// iterate the whole hierarchy and create all component
	FTransform RootTransform = FTransform::Identity;
	for (TSharedPtr<FFbxNodeInfo> NodeInfo : SceneInfoPtr->HierarchyInfo)
	{
		//Set the root transform if its the root node and skip the node
		//The root transform will be use for every node under the root node
		if (NodeInfo->NodeName.Compare("RootNode") == 0)
		{
			RootTransform = NodeInfo->Transform;
			continue;
		}
		//Export only the node that are mark for export
		if (!NodeInfo->bImportNode)
		{
			continue;
		}
		//Find the asset that link with this node attribute
		UObject *AssetToPlace = (NodeInfo->AttributeInfo.IsValid() && AllNewAssets.Contains(NodeInfo->AttributeInfo)) ? AllNewAssets[NodeInfo->AttributeInfo] : nullptr;

		//Create the component where the type depend on the asset point by the component
		//In case there is no asset we create a SceneComponent
		USceneComponent* SceneComponent = nullptr;
		if (AssetToPlace != nullptr)
		{
			if (AssetToPlace->GetClass() == UStaticMesh::StaticClass())
			{
				//Component will be rename later
				UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(RootActorContainer, NAME_None);
				StaticMeshComponent->SetStaticMesh(Cast<UStaticMesh>(AssetToPlace));
				StaticMeshComponent->DepthPriorityGroup = SDPG_World;
				SetStaticMeshComponentOverrideMaterial(StaticMeshComponent, NodeInfo);
				SceneComponent = StaticMeshComponent;
				SceneComponent->Mobility = MobilityType;
			}
			else if (AssetToPlace->GetClass() == USkeletalMesh::StaticClass())
			{
				//Component will be rename later
				USkeletalMeshComponent* SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(RootActorContainer, NAME_None);
				SkeletalMeshComponent->SetSkeletalMesh(Cast<USkeletalMesh>(AssetToPlace));
				SkeletalMeshComponent->DepthPriorityGroup = SDPG_World;
				SceneComponent = SkeletalMeshComponent;
				SceneComponent->Mobility = MobilityType;
			}
		}
		else if (IsEmptyAttribute(NodeInfo->AttributeType) || NodeInfo->AttributeType.Compare("eMesh") == 0)
		{
			//Component will be rename later
			SceneComponent = NewObject<USceneComponent>(RootActorContainer, NAME_None);
			SceneComponent->Mobility = MobilityType;
		}
		else
		{
			//TODO log which fbx attribute we cannot create an actor from
			continue;
		}

		//////////////////////////////////////////////////////////////////////////
		//Make sure scenecomponent name are unique in the hierarchy of the outer
		FString NewUniqueName = NodeInfo->NodeName;
		if (!SceneComponent->Rename(*NewUniqueName, nullptr, REN_Test))
		{
			NewUniqueName = MakeUniqueObjectName(RootActorContainer, USceneComponent::StaticClass(), FName(*NodeInfo->NodeName)).ToString();
		}
		SceneComponent->Rename(*NewUniqueName);

		//Add the component to the owner actor and register it
		RootActorContainer->AddInstanceComponent(SceneComponent);
		SceneComponent->RegisterComponent();

		//Add the component to the temporary map so we can retrieve it later when we search for parent
		NewSceneComponentNameMap.Add(NodeInfo->UniqueId, SceneComponent);

		//Find the parent component by unique ID and attach(as child) the newly created scenecomponent
		//Attach the component to the rootcomponent if we dont find any parent component
		uint64 ParentUniqueId = NodeInfo->ParentNodeInfo.IsValid() ? NodeInfo->ParentNodeInfo->UniqueId : 0;
		USceneComponent *ParentRootComponent = nullptr;
		if (NewSceneComponentNameMap.Contains(ParentUniqueId))
		{
			ParentRootComponent = *NewSceneComponentNameMap.Find(ParentUniqueId);
			if (ParentRootComponent != nullptr)
			{
				SceneComponent->AttachTo(ParentRootComponent, NAME_None, EAttachLocation::KeepWorldPosition);
			}
		}
		else
		{
			SceneComponent->AttachTo(ActorRootComponent, NAME_None, EAttachLocation::KeepWorldPosition);
		}

		//Apply the local transform to the scene component
		ApplyTransformToComponent(SceneComponent, &(NodeInfo->Transform), ParentRootComponent != nullptr ? nullptr : &RootTransform);
	}
	// End of iteration of the hierarchy
	//////////////////////////////////////////////////////////////////////////

	return RootActorContainer;
}

void UFbxSceneImportFactory::ApplyTransformToComponent(USceneComponent *SceneComponent, FTransform *LocalTransform, FTransform *PreMultiplyTransform)
{
	//In case there is no parent we must multiply the root transform
	if (PreMultiplyTransform != nullptr)
	{
		FTransform OutTransform = FTransform::Identity;
		FTransform::Multiply(&OutTransform, LocalTransform, PreMultiplyTransform);
		SceneComponent->SetRelativeTransform(OutTransform);
	}
	else
	{
		SceneComponent->SetRelativeTransform(*LocalTransform);
	}
}
void UFbxSceneImportFactory::ImportAllSkeletalMesh(void* VoidRootNodeToImport, void* VoidFbxImporter, EObjectFlags Flags, int32& NodeIndex, int32& InterestingNodeCount, TSharedPtr<FFbxSceneInfo> SceneInfo)
{
	UnFbx::FFbxImporter* FbxImporter = (UnFbx::FFbxImporter*)VoidFbxImporter;
	FbxNode *RootNodeToImport = (FbxNode *)VoidRootNodeToImport;
	InterestingNodeCount = 1;
	TArray< TArray<FbxNode*>* > SkelMeshArray;
	FbxImporter->FillFbxSkelMeshArrayInScene(RootNodeToImport, SkelMeshArray, false);
	InterestingNodeCount = SkelMeshArray.Num();

	int32 TotalNumNodes = 0;

	for (int32 i = 0; i < SkelMeshArray.Num(); i++)
	{
		UObject* NewObject = nullptr;
		TArray<FbxNode*> NodeArray = *SkelMeshArray[i];
		UPackage* Pkg = nullptr;
		TotalNumNodes += NodeArray.Num();
		TSharedPtr<FFbxNodeInfo> RootNodeInfo;
		if (TotalNumNodes > 0)
		{
			FbxNode* RootNodeArrayNode = NodeArray[0];
			if (!FindSceneNodeInfo(SceneInfo, RootNodeArrayNode->GetUniqueID(), RootNodeInfo))
			{
				continue;
			}
			if (!RootNodeInfo->AttributeInfo.IsValid() || RootNodeInfo->AttributeInfo->GetType() != USkeletalMesh::StaticClass())
			{
				continue;
			}
		}
		// check if there is LODGroup for this skeletal mesh
		int32 MaxLODLevel = 1;
		for (int32 j = 0; j < NodeArray.Num(); j++)
		{
			FbxNode* Node = NodeArray[j];
			if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eLODGroup)
			{
				// get max LODgroup level
				if (MaxLODLevel < Node->GetChildCount())
				{
					MaxLODLevel = Node->GetChildCount();
				}
			}
		}
		int32 LODIndex;
		for (LODIndex = 0; LODIndex < MaxLODLevel; LODIndex++)
		{
			TArray<FbxNode*> SkelMeshNodeArray;
			for (int32 j = 0; j < NodeArray.Num(); j++)
			{
				FbxNode* Node = NodeArray[j];
				if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eLODGroup)
				{
					if (Node->GetChildCount() > LODIndex)
					{
						SkelMeshNodeArray.Add(Node->GetChild(LODIndex));
					}
					else // in less some LODGroups have less level, use the last level
					{
						SkelMeshNodeArray.Add(Node->GetChild(Node->GetChildCount() - 1));
					}
				}
				else
				{
					SkelMeshNodeArray.Add(Node);
				}
			}

			if (LODIndex == 0 && SkelMeshNodeArray.Num() != 0)
			{
				FName OutputName = FbxImporter->MakeNameForMesh(SkelMeshNodeArray[0]->GetName(), SkelMeshNodeArray[0]);
				FString PackageName = Path + TEXT("/") + OutputName.ToString();
				FString SkeletalMeshName;
				Pkg = CreatePackageForNode(PackageName, SkeletalMeshName);
				if (Pkg == nullptr)
					break;
				RootNodeInfo->AttributeInfo->SetOriginalImportPath(PackageName);
				FName SkeletalMeshFName = FName(*SkeletalMeshName);
				USkeletalMesh* NewMesh = FbxImporter->ImportSkeletalMesh(Pkg, SkelMeshNodeArray, SkeletalMeshFName, Flags, SkeletalMeshImportData);
				NewObject = NewMesh;
				if (NewMesh)
				{
					TSharedPtr<FFbxNodeInfo> SkelMeshNodeInfo;
					if (FindSceneNodeInfo(SceneInfo, SkelMeshNodeArray[0]->GetUniqueID(), SkelMeshNodeInfo) && SkelMeshNodeInfo.IsValid() && SkelMeshNodeInfo->AttributeInfo.IsValid())
					{
						AllNewAssets.Add(SkelMeshNodeInfo->AttributeInfo, NewObject);
					}
					// We need to remove all scaling from the root node before we set up animation data.
					// Othewise some of the global transform calculations will be incorrect.
					FbxImporter->RemoveTransformSettingsFromFbxNode(RootNodeToImport, SkeletalMeshImportData);
					FbxImporter->SetupAnimationDataFromMesh(NewMesh, Pkg, SkelMeshNodeArray, AnimSequenceImportData, OutputName.ToString());

					// Reapply the transforms for the rest of the import
					FbxImporter->ApplyTransformSettingsToFbxNode(RootNodeToImport, SkeletalMeshImportData);
				}
			}
			else if (NewObject) // the base skeletal mesh is imported successfully
			{
				USkeletalMesh* BaseSkeletalMesh = Cast<USkeletalMesh>(NewObject);
				FName LODObjectName = NAME_None;
				USkeletalMesh *LODObject = FbxImporter->ImportSkeletalMesh(GetTransientPackage(), SkelMeshNodeArray, LODObjectName, RF_NoFlags, SkeletalMeshImportData);
				bool bImportSucceeded = FbxImporter->ImportSkeletalMeshLOD(LODObject, BaseSkeletalMesh, LODIndex, false);

				if (bImportSucceeded)
				{
					BaseSkeletalMesh->LODInfo[LODIndex].ScreenSize = 1.0f / (MaxLODLevel * LODIndex);
				}
				else
				{
					FbxImporter->AddTokenizedErrorMessage(FTokenizedMessage::Create(EMessageSeverity::Error, LOCTEXT("FailedToImport_SkeletalMeshLOD", "Failed to import Skeletal mesh LOD.")), FFbxErrors::SkeletalMesh_LOD_FailedToImport);
				}
			}

			// import morph target
			if (NewObject && SkeletalMeshImportData->bImportMorphTargets)
			{
				if (Pkg == nullptr)
					continue;
				// TODO: Disable material importing when importing morph targets
				FbxImporter->ImportFbxMorphTarget(SkelMeshNodeArray, Cast<USkeletalMesh>(NewObject), Pkg, LODIndex);
			}
		}

		if (NewObject)
		{
			NodeIndex++;
		}
	}

	for (int32 i = 0; i < SkelMeshArray.Num(); i++)
	{
		delete SkelMeshArray[i];
	}

	// if total nodes we found is 0, we didn't find anything. 
	if (TotalNumNodes == 0)
	{
		FbxImporter->AddTokenizedErrorMessage(FTokenizedMessage::Create(EMessageSeverity::Error, LOCTEXT("FailedToImport_NoMeshFoundOnRoot", "Could not find any valid mesh on the root hierarchy. If you have mesh in the sub hierarchy, please enable option of [Import Meshes In Bone Hierarchy] when import.")),
			FFbxErrors::SkeletalMesh_NoMeshFoundOnRoot);
	}
}

void UFbxSceneImportFactory::ImportAllStaticMesh(void* VoidRootNodeToImport, void* VoidFbxImporter, EObjectFlags Flags, int32& NodeIndex, int32& InterestingNodeCount, TSharedPtr<FFbxSceneInfo> SceneInfo)
{
	UnFbx::FFbxImporter* FbxImporter = (UnFbx::FFbxImporter*)VoidFbxImporter;
	FbxNode *RootNodeToImport = (FbxNode *)VoidRootNodeToImport;
	
	//Copy default options to StaticMeshImportData
	SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(GlobalImportSettingsReference, SceneImportOptionsStaticMesh);
	SceneImportOptionsStaticMesh->FillStaticMeshInmportData(StaticMeshImportData, SceneImportOptions);

	FbxImporter->ApplyTransformSettingsToFbxNode(RootNodeToImport, StaticMeshImportData);

	// count meshes in lod groups if we don't care about importing LODs
	int32 NumLODGroups = 0;
	bool bCountLODGroupMeshes = !GlobalImportSettingsReference->bImportStaticMeshLODs;
	InterestingNodeCount = FbxImporter->GetFbxMeshCount(RootNodeToImport, bCountLODGroupMeshes, NumLODGroups);

	int32 ImportedMeshCount = 0;
	UStaticMesh* NewStaticMesh = nullptr;
	UObject* Object = RecursiveImportNode(FbxImporter, RootNodeToImport, Flags, NodeIndex, InterestingNodeCount, SceneInfo, Path);

	NewStaticMesh = Cast<UStaticMesh>(Object);

	// Make sure to notify the asset registry of all assets created other than the one returned, which will notify the asset registry automatically.
	for (auto AssetItKvp = AllNewAssets.CreateIterator(); AssetItKvp; ++AssetItKvp)
	{
		UObject* Asset = AssetItKvp.Value();
		if (Asset != NewStaticMesh)
		{
			FAssetRegistryModule::AssetCreated(Asset);
			Asset->MarkPackageDirty();
		}
	}
	ImportedMeshCount = AllNewAssets.Num();
	if (ImportedMeshCount == 1 && NewStaticMesh)
	{
		FbxImporter->ImportStaticMeshSockets(NewStaticMesh);
	}
}

// @todo document
UObject* UFbxSceneImportFactory::RecursiveImportNode(void* VoidFbxImporter, void* VoidNode, EObjectFlags Flags, int32& NodeIndex, int32 Total, TSharedPtr<FFbxSceneInfo>  SceneInfo, FString PackagePath)
{
	UObject* NewObject = nullptr;
	TSharedPtr<FFbxNodeInfo> OutNodeInfo;
	UnFbx::FFbxImporter* FFbxImporter = (UnFbx::FFbxImporter*)VoidFbxImporter;

	FbxNode* Node = (FbxNode*)VoidNode;

	if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eLODGroup && Node->GetChildCount() > 0)
	{
		// import base mesh
		NewObject = ImportANode(VoidFbxImporter, Node->GetChild(0), Flags, NodeIndex, SceneInfo, OutNodeInfo, PackagePath, Total);

		if (NewObject)
		{
			//We should always have a valid attribute if we just create a new asset
			check(OutNodeInfo.IsValid() && OutNodeInfo->AttributeInfo.IsValid());

			AllNewAssets.Add(OutNodeInfo->AttributeInfo, NewObject);
			if (GlobalImportSettingsReference->bImportStaticMeshLODs)
			{
				// import LOD meshes
				for (int32 LODIndex = 1; LODIndex < Node->GetChildCount(); LODIndex++)
				{
					FbxNode* ChildNode = Node->GetChild(LODIndex);
					ImportANode(VoidFbxImporter, ChildNode, Flags, NodeIndex, SceneInfo, OutNodeInfo, PackagePath, Total, NewObject, LODIndex);
				}
			}
		}
	}
	else
	{
		if (Node->GetMesh() && Node->GetMesh()->GetPolygonVertexCount() > 0)
		{
			NewObject = ImportANode(VoidFbxImporter, Node, Flags, NodeIndex, SceneInfo, OutNodeInfo, PackagePath, Total);

			if (NewObject)
			{
				//We should always have a valid attribute if we just create a new asset
				check(OutNodeInfo.IsValid() && OutNodeInfo->AttributeInfo.IsValid());

				AllNewAssets.Add(OutNodeInfo->AttributeInfo, NewObject);
			}
		}
		
		if (SceneImportOptions->bCreateContentFolderHierarchy)
		{
			FString NodeName = FString(FFbxImporter->MakeName(Node->GetName()));
			if (NodeName.Compare("RootNode") != 0)
			{
				PackagePath += TEXT("/") + NodeName;
			}
		}

		for (int32 ChildIndex = 0; ChildIndex < Node->GetChildCount(); ++ChildIndex)
		{
			UObject* SubObject = RecursiveImportNode(VoidFbxImporter, Node->GetChild(ChildIndex), Flags, NodeIndex, Total, SceneInfo, PackagePath);
			if (NewObject == nullptr)
			{
				NewObject = SubObject;
			}
		}
	}

	return NewObject;
}

// @todo document
UObject* UFbxSceneImportFactory::ImportANode(void* VoidFbxImporter, void* VoidNode, EObjectFlags Flags, int32& NodeIndex, TSharedPtr<FFbxSceneInfo> SceneInfo, TSharedPtr<FFbxNodeInfo> &OutNodeInfo, FString PackagePath, int32 Total, UObject* InMesh, int LODIndex)
{
	UnFbx::FFbxImporter* FFbxImporter = (UnFbx::FFbxImporter*)VoidFbxImporter;
	FbxNode* Node = (FbxNode*)VoidNode;
	FString ParentName;
	if (Node->GetParent() != nullptr)
	{
		ParentName = FFbxImporter->MakeName(Node->GetParent()->GetName());
	}
	else
	{
		ParentName.Empty();
	}
	
	FbxString NodeName(FFbxImporter->MakeName(Node->GetName()));
	//Find the scene node info in the hierarchy
	if (!FindSceneNodeInfo(SceneInfo, Node->GetUniqueID(), OutNodeInfo) || !OutNodeInfo->AttributeInfo.IsValid())
	{
		//We cannot instantiate this asset if its not part of the hierarchy
		return nullptr;
	}

	if(OutNodeInfo->AttributeInfo->GetType() != UStaticMesh::StaticClass() || !OutNodeInfo->AttributeInfo->bImportAttribute)
	{
		//export only static mesh or the user specify to not import this mesh
		return nullptr;
	}

	//Check if the Mesh was already import
	if (AllNewAssets.Contains(OutNodeInfo->AttributeInfo))
	{
		return AllNewAssets[OutNodeInfo->AttributeInfo];
	}

	UObject* NewObject = nullptr;
	// skip collision models
	if (NodeName.Find("UCX") != -1 || NodeName.Find("MCDCX") != -1 ||
		NodeName.Find("UBX") != -1 || NodeName.Find("USP") != -1)
	{
		return nullptr;
	}

	//Create a package for this node
	FString PackageName = PackagePath + TEXT("/") + OutNodeInfo->AttributeInfo->Name;
	FString StaticMeshName;
	UPackage* Pkg = CreatePackageForNode(PackageName, StaticMeshName);
	if (Pkg == nullptr)
		return nullptr;

	//Apply the correct fbx options
	TSharedPtr<FFbxMeshInfo> MeshInfo = StaticCastSharedPtr<FFbxMeshInfo>(OutNodeInfo->AttributeInfo);
	
	UnFbx::FBXImportOptions* OverrideImportSettings = GetOptionsFromName(MeshInfo->OptionName);
	if (OverrideImportSettings != nullptr)
	{
		SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(OverrideImportSettings, GlobalImportSettings);
		SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(OverrideImportSettings, SceneImportOptionsStaticMesh);
		SceneImportOptionsStaticMesh->FillStaticMeshInmportData(StaticMeshImportData, SceneImportOptions);
	}
	else
	{
		//Use the default options if we dont found options
		SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(GlobalImportSettingsReference, GlobalImportSettings);
		SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(GlobalImportSettingsReference, SceneImportOptionsStaticMesh);
		SceneImportOptionsStaticMesh->FillStaticMeshInmportData(StaticMeshImportData, SceneImportOptions);
	}
	FName StaticMeshFName = FName(*(OutNodeInfo->AttributeInfo->Name));
	NewObject = FFbxImporter->ImportStaticMesh(Pkg, Node, StaticMeshFName, Flags, StaticMeshImportData, Cast<UStaticMesh>(InMesh), LODIndex);
	OutNodeInfo->AttributeInfo->SetOriginalImportPath(PackageName);
	OutNodeInfo->AttributeInfo->SetOriginalFullImportName(NewObject->GetPathName());

	if (NewObject)
	{
		NodeIndex++;
		FFormatNamedArguments Args;
		Args.Add(TEXT("NodeIndex"), NodeIndex);
		Args.Add(TEXT("ArrayLength"), Total);
		GWarn->StatusUpdate(NodeIndex, Total, FText::Format(NSLOCTEXT("UnrealEd", "Importingf", "Importing ({NodeIndex} of {ArrayLength})"), Args));
	}

	return NewObject;
}

UnFbx::FBXImportOptions *UFbxSceneImportFactory::GetOptionsFromName(FString OptionsName)
{
	for (auto kvp : NameOptionsMap)
	{
		if (kvp.Key.Compare(OptionsName) == 0)
		{
			return kvp.Value;
		}
	}
	return nullptr;
}

bool UFbxSceneImportFactory::FindSceneNodeInfo(TSharedPtr<FFbxSceneInfo> SceneInfo, uint64 NodeInfoUniqueId, TSharedPtr<FFbxNodeInfo> &OutNodeInfo)
{
	for (auto NodeIt = SceneInfo->HierarchyInfo.CreateIterator(); NodeIt; ++NodeIt)
	{
		if (NodeInfoUniqueId == (*NodeIt)->UniqueId)
		{
			OutNodeInfo = (*NodeIt);
			return true;
		}
	}
	return false;
}

UPackage *UFbxSceneImportFactory::CreatePackageForNode(FString PackageName, FString &StaticMeshName)
{
	FString PackageNameOfficial = PackageTools::SanitizePackageName(PackageName);
	// We can not create assets that share the name of a map file in the same location
	if (FEditorFileUtils::IsMapPackageAsset(PackageNameOfficial))
	{
		return nullptr;
	}
	bool IsPkgExist = FPackageName::DoesPackageExist(PackageNameOfficial);
	int32 tryCount = 1;
	while (IsPkgExist)
	{
		PackageNameOfficial = PackageName;
		PackageNameOfficial += TEXT("_");
		PackageNameOfficial += FString::FromInt(tryCount++);
		PackageNameOfficial = PackageTools::SanitizePackageName(PackageNameOfficial);
		IsPkgExist = FPackageName::DoesPackageExist(PackageNameOfficial);
	}
	UPackage* Pkg = CreatePackage(nullptr, *PackageNameOfficial);
	if (!ensure(Pkg))
	{
		return nullptr;
	}
	Pkg->FullyLoad();

	StaticMeshName = FPackageName::GetLongPackageAssetName(Pkg->GetOutermost()->GetName());
	return Pkg;
}

#undef LOCTEXT_NAMESPACE
