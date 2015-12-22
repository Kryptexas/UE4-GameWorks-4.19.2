// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "FbxSceneImportFactory.generated.h"

class UFbxSceneImportData;

#define INVALID_UNIQUE_ID 0xFFFFFFFFFFFFFFFF

class FFbxAttributeInfo : public TSharedFromThis<FFbxAttributeInfo>
{
public:
	FString Name;
	uint64 UniqueId;
	bool bImportAttribute;
	//Log the path where it was import so we can retrieve it when doing the re-import
	FString OriginalImportPath;
	FString OriginalFullImportName;

	bool bOverridePath;
	FString OverrideImportPath;
	FString OverrideFullImportName;

	//The name of the Options so reimport can show the options
	FString OptionName;

	FString GetImportPath()
	{
		if (bOverridePath)
		{
			return OverrideImportPath;
		}
		return OriginalImportPath;
	}

	FString GetFullImportName()
	{
		if (bOverridePath)
		{
			return OverrideFullImportName;
		}
		return OriginalFullImportName;
	}

	void SetOriginalImportPath(FString ImportPath)
	{
		OriginalImportPath = ImportPath;
		IsContentObjectUpToDate = false;
	}
	void SetOriginalFullImportName(FString FullImportName)
	{
		OriginalFullImportName = FullImportName;
		IsContentObjectUpToDate = false;
	}

	void SetOverridePath(bool OverridePath)
	{
		bOverridePath = OverridePath;
		IsContentObjectUpToDate = false;
	}

	FFbxAttributeInfo()
		: Name(TEXT(""))
		, UniqueId(INVALID_UNIQUE_ID)
		, bImportAttribute(true)
		, OriginalImportPath(TEXT(""))
		, OriginalFullImportName(TEXT(""))
		, bOverridePath(false)
		, OverrideImportPath(TEXT(""))
		, OverrideFullImportName(TEXT(""))
		, OptionName(TEXT(""))
		, IsContentObjectUpToDate(false)
		, ContentPackage(nullptr)
		, ContentObject(nullptr)
	{}

	virtual ~FFbxAttributeInfo() {}

	virtual UClass *GetType() = 0;

	UPackage *GetContentPackage();
	UObject *GetContentObject();
private:
	//Cache the existing object state
	bool IsContentObjectUpToDate;
	UPackage *ContentPackage;
	UObject *ContentObject;
};

class FFbxMeshInfo : public FFbxAttributeInfo, public TSharedFromThis<FFbxMeshInfo>
{
public:
	int32 FaceNum;
	int32 VertexNum;
	bool bTriangulated;
	int32 MaterialNum;
	bool bIsSkelMesh;
	FString SkeletonRoot;
	int32 SkeletonElemNum;
	FString LODGroup;
	int32 LODLevel;
	int32 MorphNum;

	FFbxMeshInfo()
		: FaceNum(0)
		, VertexNum(0)
		, bTriangulated(false)
		, MaterialNum(0)
		, bIsSkelMesh(false)
		, SkeletonRoot(TEXT(""))
		, SkeletonElemNum(0)
		, LODGroup(TEXT(""))
		, LODLevel(0)
		, MorphNum(0)
	{}

	virtual ~FFbxMeshInfo() {}

	virtual UClass *GetType();
};

class FFbxTextureInfo : public FFbxAttributeInfo, public TSharedFromThis<FFbxTextureInfo>
{
public:
	FString TexturePath;

	FFbxTextureInfo()
		: TexturePath(TEXT(""))
	{}

	virtual UClass *GetType();
};

class FFbxMaterialInfo : public FFbxAttributeInfo, public TSharedFromThis<FFbxMaterialInfo>
{
public:
	//This string is use to help match the material when doing a reimport
	FString HierarchyPath;

	//All the textures use by this material
	TArray<TSharedPtr<FFbxTextureInfo>> Textures;

	FFbxMaterialInfo()
		: HierarchyPath(TEXT(""))
	{}

	virtual UClass *GetType();
};

//Node use to store the scene hierarchy transform will be relative to the parent
class FFbxNodeInfo : public TSharedFromThis<FFbxNodeInfo>
{
public:
	FString NodeName;
	uint64 UniqueId;
	FString NodeHierarchyPath;

	TSharedPtr<FFbxNodeInfo> ParentNodeInfo;
	
	TSharedPtr<FFbxAttributeInfo> AttributeInfo;
	FString AttributeType;

	FTransform Transform;
	bool bImportNode;

	TArray<TSharedPtr<FFbxNodeInfo>> Childrens;
	TArray<TSharedPtr<FFbxMaterialInfo>> Materials;

	FFbxNodeInfo()
		: NodeName(TEXT(""))
		, UniqueId(INVALID_UNIQUE_ID)
		, NodeHierarchyPath(TEXT(""))
		, ParentNodeInfo(NULL)
		, AttributeInfo(NULL)
		, AttributeType(TEXT(""))
		, Transform(FTransform::Identity)
		, bImportNode(true)
	{}
};

class FFbxSceneInfo : public TSharedFromThis<FFbxSceneInfo>
{
public:
	// data for static mesh
	int32 NonSkinnedMeshNum;
	//data for skeletal mesh
	int32 SkinnedMeshNum;
	// common data
	int32 TotalGeometryNum;
	int32 TotalMaterialNum;
	int32 TotalTextureNum;
	TArray<TSharedPtr<FFbxMeshInfo>> MeshInfo;
	TArray<TSharedPtr<FFbxNodeInfo>> HierarchyInfo;
	/* true if it has animation */
	bool bHasAnimation;
	double FrameRate;
	double TotalTime;

	FFbxSceneInfo()
		: NonSkinnedMeshNum(0)
		, SkinnedMeshNum(0)
		, TotalGeometryNum(0)
		, TotalMaterialNum(0)
		, TotalTextureNum(0)
		, bHasAnimation(false)
		, FrameRate(0.0)
		, TotalTime(0.0)
	{}
};

namespace UnFbx
{
	struct FBXImportOptions;
}

typedef TMap<FString, UnFbx::FBXImportOptions*> ImportOptionsNameMap;
typedef ImportOptionsNameMap* ImportOptionsNameMapPtr;

UCLASS(hidecategories=Object)
class UNREALED_API UFbxSceneImportFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) override;
	virtual UObject* FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	//~ Begin UFactory Interface

	/** Import options UI detail when importing fbx scene */
	UPROPERTY(Transient)
	class UFbxSceneImportOptions* SceneImportOptions;
	
	/** Import options UI detail when importing fbx scene static mesh*/
	UPROPERTY(Transient)
	class UFbxSceneImportOptionsStaticMesh* SceneImportOptionsStaticMesh;
	
	/** Import options UI detail when importing fbx scene skeletal mesh*/
	UPROPERTY(Transient)
	class UFbxSceneImportOptionsSkeletalMesh* SceneImportOptionsSkeletalMesh;
	
	/** Import options UI detail when importing fbx scene animation*/
	UPROPERTY(Transient)
	class UFbxSceneImportOptionsAnimation* SceneImportOptionsAnimation;
	
	/** Import options UI detail when importing fbx scene material*/
	UPROPERTY(Transient)
	class UFbxSceneImportOptionsMaterial* SceneImportOptionsMaterial;


	/** Import data used when importing static meshes */
	UPROPERTY(Transient)
	class UFbxStaticMeshImportData* StaticMeshImportData;

	/** Import data used when importing skeletal meshes */
	UPROPERTY(Transient)
	class UFbxSkeletalMeshImportData* SkeletalMeshImportData;

	/** Import data used when importing animations */
	UPROPERTY(Transient)
	class UFbxAnimSequenceImportData* AnimSequenceImportData;

	/** Import data used when importing textures */
	UPROPERTY(Transient)
	class UFbxTextureImportData* TextureImportData;
	
	/* Default Options always have the same name "Default" */
	static FString DefaultOptionName;

protected:

	/* Compute the path of every node and fill the result in the node. This data will be use by the reimport
	*  as a unique key for for the reimport status of the node hierarchy.
	*/
	static void FillSceneHierarchyPath(TSharedPtr<FFbxSceneInfo> SceneInfo);

	/** Create a hierarchy of actor in the current level */
	void CreateLevelActorHierarchy(TSharedPtr<FFbxSceneInfo> SceneInfoPtr);

	/** Create a hierarchy of actor in the current level */
	AActor *CreateActorComponentsHierarchy(TSharedPtr<FFbxSceneInfo> SceneInfoPtr);

	/** Apply the LocalTransform to the SceneComponent and if PreMultiplyTransform is not null do a pre multiplication
	* SceneComponent: Must be a valid pointer
	* LocalTransform: Must be a valid pointer
	* PreMultiplyTransform: Can be nullptr
	*/
	void ApplyTransformToComponent(USceneComponent *SceneComponent, FTransform *LocalTransform, FTransform *PreMultiplyTransform);

	/** This will add the scene transform options to the root node */
	void ApplySceneTransformOptionsToRootNode(TSharedPtr<FFbxSceneInfo> SceneInfoPtr);

	/** Import all skeletal mesh from the fbx scene */
	void ImportAllSkeletalMesh(void* VoidRootNodeToImport, void* VoidFbxImporter, EObjectFlags Flags, int32& NodeIndex, int32& InterestingNodeCount , TSharedPtr<FFbxSceneInfo> SceneInfo);

	/** Import all static mesh from the fbx scene */
	void ImportAllStaticMesh(void* VoidRootNodeToImport, void* VoidFbxImporter, EObjectFlags Flags, int32& NodeIndex, int32& InterestingNodeCount, TSharedPtr<FFbxSceneInfo> SceneInfo);

	// @todo document
	UObject* RecursiveImportNode(void* FFbxImporter, void* VoidNode, EObjectFlags Flags, int32& Index, int32 Total, TSharedPtr<FFbxSceneInfo> SceneInfo, FString PackagePath);

	// @todo document
	UObject* ImportANode(void* VoidFbxImporter, void* VoidNode, EObjectFlags Flags, int32& NodeIndex, TSharedPtr<FFbxSceneInfo> SceneInfo, TSharedPtr<FFbxNodeInfo> &OutNodeInfo, FString PackagePath, int32 Total = 0, UObject* InMesh = NULL, int LODIndex = 0);

	/** Find the FFbxNodeInfo in the hierarchy. */
	bool FindSceneNodeInfo(TSharedPtr<FFbxSceneInfo> SceneInfo, uint64 NodeInfoUniqueId, TSharedPtr<FFbxNodeInfo> &OutNodeInfo);

	/** Create a package for the specified node. Package will be the concatenation of UFbxSceneImportFactory::Path and Node->GetName(). */
	UPackage *CreatePackageForNode(FString PackageName, FString &StaticMeshName);

	static TSharedPtr<FFbxSceneInfo> ConvertSceneInfo(void* VoidFbxSceneInfo);
	static void ExtractMaterialInfo(void* FbxImporterVoid, TSharedPtr<FFbxSceneInfo> SceneInfoPtr);
	bool SetStaticMeshComponentOverrideMaterial(class UStaticMeshComponent* StaticMeshComponent, TSharedPtr<FFbxNodeInfo> NodeInfo);

	/** The path of the asset to import */
	FString Path;

	/** Pointer on the fbx scene import data, we fill this object to be able to do re import of the scene */
	UFbxSceneImportData* ReimportData;
	
	/** Assets created by the factory*/
	TMap<TSharedPtr<FFbxAttributeInfo>, UObject*> AllNewAssets;

	/*Global Setting for non overriden Node*/
	UnFbx::FBXImportOptions* GlobalImportSettings;

	/*The Global Settings Reference*/
	UnFbx::FBXImportOptions* GlobalImportSettingsReference;
	
	/*The options dictionary*/
	ImportOptionsNameMap NameOptionsMap;

	/* Return the Options from the NameOptionMap Map. return nulptr if the options are not found*/
	UnFbx::FBXImportOptions *GetOptionsFromName(FString OptionName);

	/** Is the import was cancel*/
	bool ImportWasCancel;
};



