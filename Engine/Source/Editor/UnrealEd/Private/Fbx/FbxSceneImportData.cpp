// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "FbxImporter.h"


UFbxSceneImportData::UFbxSceneImportData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	SourceFbxFile.Empty();
	BluePrintFullName.Empty();
	SceneInfoSourceData = nullptr;
	bCreateFolderHierarchy = false;
	bImportScene = true;
	HierarchyType = FBXSOCHT_CreateLevelActors;
#endif
}

#if WITH_EDITORONLY_DATA
UnFbx::FBXImportOptions *JSONToFbxOption(TSharedPtr<FJsonValue> OptionJsonValue, FString &OptionName)
{
	const TSharedPtr<FJsonObject>& OptionObj = OptionJsonValue->AsObject();
	if (!OptionObj.IsValid())
	{
		return nullptr;
	}
	UnFbx::FBXImportOptions *Option = new UnFbx::FBXImportOptions();

	if (!OptionObj->TryGetStringField("OptionName", OptionName))
	{
		return nullptr;
	}

	OptionObj->TryGetBoolField("bImportScene", Option->bImportScene);
	OptionObj->TryGetBoolField("bImportMaterials", Option->bImportMaterials);
	OptionObj->TryGetBoolField("bInvertNormalMap", Option->bInvertNormalMap);
	OptionObj->TryGetBoolField("bImportTextures", Option->bImportTextures);
	OptionObj->TryGetBoolField("bImportLOD", Option->bImportLOD);
	OptionObj->TryGetBoolField("bUsedAsFullName", Option->bUsedAsFullName);
	OptionObj->TryGetBoolField("bConvertScene", Option->bConvertScene);
	OptionObj->TryGetBoolField("bRemoveNameSpace", Option->bRemoveNameSpace);
	double X, Y, Z, Pitch, Yaw, Roll;
	const TSharedPtr<FJsonObject> *DataObj = nullptr;
	if (OptionObj->TryGetObjectField("ImportTranslation", DataObj))
	{
		(*DataObj)->TryGetNumberField("X", X);
		(*DataObj)->TryGetNumberField("Y", Y);
		(*DataObj)->TryGetNumberField("Z", Z);
		Option->ImportTranslation.Set((float)X, (float)Y, (float)Z);
	}
	if (OptionObj->TryGetObjectField("ImportRotation", DataObj))
	{
		(*DataObj)->TryGetNumberField("P", Pitch);
		(*DataObj)->TryGetNumberField("Y", Yaw);
		(*DataObj)->TryGetNumberField("R", Roll);
		Option->ImportRotation.Pitch = (float)Pitch;
		Option->ImportRotation.Yaw = (float)Yaw;
		Option->ImportRotation.Roll = (float)Roll;
	}
	double ImportUniformScale;
	if (OptionObj->TryGetNumberField("ImportUniformScale", ImportUniformScale))
	{
		Option->ImportUniformScale = (float)ImportUniformScale;
	}
	int32 NormalImportMethod;
	if (OptionObj->TryGetNumberField("NormalImportMethod", NormalImportMethod))
	{
		Option->NormalImportMethod = (EFBXNormalImportMethod)NormalImportMethod;
	}
	int32 NormalGenerationMethod;
	if (OptionObj->TryGetNumberField("NormalGenerationMethod", NormalGenerationMethod))
	{
		Option->NormalGenerationMethod = (EFBXNormalGenerationMethod::Type)NormalGenerationMethod;
	}
	OptionObj->TryGetBoolField("bTransformVertexToAbsolute", Option->bTransformVertexToAbsolute);
	OptionObj->TryGetBoolField("bCombineToSingle", Option->bCombineToSingle);
	int32 VertexColorImportOption;
	if (OptionObj->TryGetNumberField("VertexColorImportOption", VertexColorImportOption))
	{
		Option->VertexColorImportOption = (EVertexColorImportOption::Type)VertexColorImportOption;
	}
	if (OptionObj->TryGetObjectField("VertexOverrideColor", DataObj))
	{
		double R, G, B, A;
		(*DataObj)->TryGetNumberField("R", R);
		(*DataObj)->TryGetNumberField("G", G);
		(*DataObj)->TryGetNumberField("B", B);
		(*DataObj)->TryGetNumberField("A", A);
		Option->VertexOverrideColor.R = (float)R;
		Option->VertexOverrideColor.G = (float)G;
		Option->VertexOverrideColor.B = (float)B;
		Option->VertexOverrideColor.A = (float)A;
	}
	OptionObj->TryGetBoolField("bRemoveDegenerates", Option->bRemoveDegenerates);
	OptionObj->TryGetBoolField("bBuildAdjacencyBuffer", Option->bBuildAdjacencyBuffer);
	OptionObj->TryGetBoolField("bBuildReversedIndexBuffer", Option->bBuildReversedIndexBuffer);
	OptionObj->TryGetBoolField("bGenerateLightmapUVs", Option->bGenerateLightmapUVs);
	OptionObj->TryGetBoolField("bOneConvexHullPerUCX", Option->bOneConvexHullPerUCX);
	OptionObj->TryGetBoolField("bAutoGenerateCollision", Option->bAutoGenerateCollision);
	FString LODGroup;
	if (OptionObj->TryGetStringField("StaticMeshLODGroup", LODGroup))
	{
		Option->StaticMeshLODGroup = FName(*LODGroup);
	}
	OptionObj->TryGetBoolField("bImportStaticMeshLODs", Option->bImportStaticMeshLODs);
	FString MaterialPrefixName;
	if (OptionObj->TryGetStringField("MaterialPrefixName", MaterialPrefixName))
	{
		Option->MaterialPrefixName = FName(*MaterialPrefixName);
	}
	return Option;
}

FString FbxOptionToJSON(FString OptionName, UnFbx::FBXImportOptions *Option)
{
	check(Option != nullptr);
	check(!OptionName.IsEmpty());

	FString JsonString = FString::Printf(TEXT("{ \"OptionName\" : \"%s\", \"bImportScene\" : \"%d\", \"bImportMaterials\" : \"%d\", \"bInvertNormalMap\" : \"%d\", \"bImportTextures\" : \"%d\", \"bImportLOD\" : \"%d\", \"bUsedAsFullName\" : \"%d\", \"bConvertScene\" : \"%d\", \"bRemoveNameSpace\" : \"%d\", "),
		*OptionName,
		Option->bImportScene ? 1 : 0,
		Option->bImportMaterials ? 1 : 0,
		Option->bInvertNormalMap ? 1 : 0,
		Option->bImportTextures ? 1 : 0,
		Option->bImportLOD ? 1 : 0,
		Option->bUsedAsFullName ? 1 : 0,
		Option->bConvertScene ? 1 : 0,
		Option->bRemoveNameSpace ? 1 : 0
		);

	JsonString += FString::Printf(TEXT("\"ImportTranslation\" : {\"X\" : \"%f\", \"Y\" : \"%f\", \"Z\" : \"%f\"}, \"ImportRotation\" : {\"P\" : \"%f\", \"Y\" : \"%f\", \"R\" : \"%f\"}, \"ImportUniformScale\" : \"%f\", "),
		Option->ImportTranslation.X,
		Option->ImportTranslation.Y,
		Option->ImportTranslation.Z,
		Option->ImportRotation.Pitch,
		Option->ImportRotation.Yaw,
		Option->ImportRotation.Roll,
		Option->ImportUniformScale
		);

	JsonString += FString::Printf(TEXT("\"NormalImportMethod\" : \"%d\", \"NormalGenerationMethod\" : \"%d\", \"bTransformVertexToAbsolute\" : \"%d\", \"bCombineToSingle\" : \"%d\", \"VertexColorImportOption\" : \"%d\", \"VertexOverrideColor\" : {\"R\" : \"%d\", \"G\" : \"%d\", \"B\" : \"%d\", \"A\" : \"%d\" }, "),
		(int32)Option->NormalImportMethod,
		(int32)Option->NormalGenerationMethod,
		Option->bTransformVertexToAbsolute ? 1 : 0,
		Option->bCombineToSingle ? 1 : 0,
		(int32)Option->VertexColorImportOption,
		Option->VertexOverrideColor.R,
		Option->VertexOverrideColor.G,
		Option->VertexOverrideColor.B,
		Option->VertexOverrideColor.A
		);

	JsonString += FString::Printf(TEXT("\"bRemoveDegenerates\" : \"%d\", \"bBuildAdjacencyBuffer\" : \"%d\", \"bBuildReversedIndexBuffer\" : \"%d\", \"bGenerateLightmapUVs\" : \"%d\", \"bOneConvexHullPerUCX\" : \"%d\", \"bAutoGenerateCollision\" : \"%d\", \"StaticMeshLODGroup\" : \"%s\", \"bImportStaticMeshLODs\" : \"%d\", "),
		Option->bRemoveDegenerates ? 1 : 0,
		Option->bBuildAdjacencyBuffer ? 1 : 0,
		Option->bBuildReversedIndexBuffer ? 1 : 0,
		Option->bGenerateLightmapUVs ? 1 : 0,
		Option->bOneConvexHullPerUCX ? 1 : 0,
		Option->bAutoGenerateCollision ? 1 : 0,
		*(Option->StaticMeshLODGroup.ToString()),
		Option->bImportStaticMeshLODs ? 1 : 0
		);

	JsonString += FString::Printf(TEXT("\"MaterialPrefixName\" : \"%s\"}"), *(Option->MaterialPrefixName.ToString()));
	//TODO add skeletalmesh and animation
	return JsonString;
}

FString FbxNodeInfoToJSON(const TSharedPtr<FFbxNodeInfo> NodeInfo)
{
	check(NodeInfo.IsValid());

	FString JsonString = FString::Printf(TEXT("{ \"NodeName\" : \"%s\", \"UniqueId\" : \"%llu\", \"NodeHierarchyPath\" : \"%s\", \"bImportNode\" : \"%d\", \"ParentUniqueId\" : \"%llu\", \"AttributeType\" : \"%s\", \"AttributeInfo\" : \"%llu\", \"Materials\" : ["),
		*(NodeInfo->NodeName),
		NodeInfo->UniqueId,
		*(NodeInfo->NodeHierarchyPath),
		NodeInfo->bImportNode ? 1 : 0,
		NodeInfo->ParentNodeInfo.IsValid() ? NodeInfo->ParentNodeInfo->UniqueId : 0,
		*(NodeInfo->AttributeType),
		NodeInfo->AttributeInfo.IsValid() ? NodeInfo->AttributeInfo->UniqueId : 0
		);

	int32 MaterialCount = NodeInfo->Materials.Num();
	for (TSharedPtr<FFbxMaterialInfo> Material : NodeInfo->Materials)
	{
		JsonString += FString::Printf(TEXT("{ \"Name\" : \"%s\", \"HierarchyPath\" : \"%s\", \"UniqueId\" : \"%llu\", \"bImportAttribute\" : \"%d\", \"OriginalImportPath\" : \"%s\", \"OriginalFullImportName\" : \"%s\", \"bOverridePath\" : \"%d\", \"OverrideImportPath\" : \"%s\", \"OverrideFullImportName\" : \"%s\" }"),
			*(Material->Name),
			*(Material->HierarchyPath),
			Material->UniqueId,
			Material->bImportAttribute ? 1 : 0,
			*(Material->OriginalImportPath),
			*(Material->OriginalFullImportName),
			Material->bOverridePath ? 1 : 0,
			*(Material->OverrideImportPath),
			*(Material->OverrideFullImportName)
			);
		MaterialCount--;
		if (MaterialCount > 0)
		{
			JsonString += FString::Printf(TEXT(", "));
		}
	}
	JsonString += FString::Printf(TEXT("] }"));
	return JsonString;
}

FString FbxMeshInfoToJSON(const TSharedPtr<FFbxMeshInfo> MeshInfo)
{
	check(MeshInfo.IsValid());

	FString JsonString = FString::Printf(TEXT("{ \"Name\" : \"%s\", \"UniqueId\" : \"%llu\", \"bImportAttribute\" : \"%d\", \"OptionName\" : \"%s\", \"bIsSkelMesh\" : \"%d\", \"OriginalImportPath\" : \"%s\", \"OriginalFullImportName\" : \"%s\", \"bOverridePath\" : \"%d\", \"OverrideImportPath\" : \"%s\", \"OverrideFullImportName\" : \"%s\" }"),
		*(MeshInfo->Name),
		MeshInfo->UniqueId,
		MeshInfo->bImportAttribute ? 1 : 0,
		*(MeshInfo->OptionName),
		MeshInfo->bIsSkelMesh ? 1 : 0,
		*(MeshInfo->OriginalImportPath),
		*(MeshInfo->OriginalFullImportName),
		MeshInfo->bOverridePath ? 1 : 0,
		*(MeshInfo->OverrideImportPath),
		*(MeshInfo->OverrideFullImportName)
		);
	return JsonString;
}

FString UFbxSceneImportData::ToJson() const
{
	if (!SceneInfoSourceData.IsValid())
	{
		return FString();
	}
	FString Json;
	Json.Reserve(1024);
	//data array start
	Json += TEXT("[ { ");
	Json += FString::Printf(TEXT("\"bImportScene\" : \"%d\", \"bCreateFolderHierarchy\" : \"%d\", \"HierarchyType\" : \"%d\", \"BluePrintFullName\" : \"%s\" "),
		bImportScene ? 1 : 0,
		bCreateFolderHierarchy ? 1 : 0,
		HierarchyType,
		*BluePrintFullName
		);
		
	if (NameOptionsMap.Num() > 0)
	{
		Json += TEXT(", \"FbxOptions\" : [");
		bool bFirstOption = true;
		for (auto kvp : NameOptionsMap)
		{
			if (!bFirstOption)
			{
				Json += TEXT(", ");
			}
			bFirstOption = false;
			Json += FbxOptionToJSON(kvp.Key, kvp.Value);
		}
		Json += TEXT(" ]");
	}
	//the meshes
	if (SceneInfoSourceData->MeshInfo.Num() > 0)
	{
		Json += TEXT(", \"MeshInfo\" : [");
		bool bFirstNode = true;
		for (const TSharedPtr<FFbxMeshInfo> MeshInfo : SceneInfoSourceData->MeshInfo)
		{
			if (!MeshInfo.IsValid())
			{
				continue;
			}
			if (!bFirstNode)
			{
				Json += TEXT(", ");
			}
			bFirstNode = false;
			Json += FbxMeshInfoToJSON(MeshInfo);
		}
		//Close the MeshInfo array
		Json += TEXT(" ]");
	}

	//the hierarchy
	if (SceneInfoSourceData->HierarchyInfo.Num() > 0)
	{
		Json += TEXT(", \"Hierarchy\" : [");
		bool bFirstNode = true;
		for (TSharedPtr<FFbxNodeInfo> NodeInfo : SceneInfoSourceData->HierarchyInfo)
		{
			if (!NodeInfo.IsValid())
			{
				continue;
			}
			if (!bFirstNode)
			{
				Json += TEXT(", ");
			}
			bFirstNode = false;
			Json += FbxNodeInfoToJSON(NodeInfo);
		}
		//Close the hierarchy array
		Json += TEXT(" ]");
	}

	//Close the data array
	Json += TEXT(" } ]");
	return Json;
}

void UFbxSceneImportData::FromJson(FString InJsonString)
{
	//Allocate the SceneInfo result
	SceneInfoSourceData = MakeShareable(new FFbxSceneInfo());

	// Load json
	TSharedRef<TJsonReader<TCHAR>> Reader = FJsonStringReader::Create(MoveTemp(InJsonString));

	TArray<TSharedPtr<FJsonValue>> JSONSceneInfos;
	if (!FJsonSerializer::Deserialize(Reader, JSONSceneInfos))
	{
		//Cannot read the json file so return an empty array
		return;
	}

	//We should always have only one SceneInfo
	check(JSONSceneInfos.Num() == 1);

	for (const TSharedPtr<FJsonValue>& Value : JSONSceneInfos)
	{
		const TSharedPtr<FJsonObject>& SceneInfoObj = Value->AsObject();
		if (!SceneInfoObj.IsValid())
		{
			continue;
		}
		//Read General data
		if (!SceneInfoObj->TryGetBoolField("bImportScene", bImportScene)) continue;
		if (!SceneInfoObj->TryGetBoolField("bCreateFolderHierarchy", bCreateFolderHierarchy)) continue;
		if (!SceneInfoObj->TryGetNumberField("HierarchyType", HierarchyType)) continue;
		if (!SceneInfoObj->TryGetStringField("BluePrintFullName", BluePrintFullName)) continue;

		//Read Options
		const TArray<TSharedPtr<FJsonValue>>* JSONOptions;
		SceneInfoObj->TryGetArrayField("FbxOptions", JSONOptions);
		for (const auto& OptionJsonValue : *JSONOptions)
		{
			FString OptionName;
			UnFbx::FBXImportOptions *Option = JSONToFbxOption(OptionJsonValue, OptionName);
			if (Option != nullptr)
			{
				NameOptionsMap.Add(OptionName, Option);
			}
		}

		//Read Meshes
		const TArray<TSharedPtr<FJsonValue>>* JSONMeshes;
		SceneInfoObj->TryGetArrayField("MeshInfo", JSONMeshes);
		for (const auto& MeshJsonValue : *JSONMeshes)
		{
			const TSharedPtr<FJsonObject>& MeshInfoObj = MeshJsonValue->AsObject();
			if (!MeshInfoObj.IsValid())
			{
				continue;
			}
			TSharedPtr<FFbxMeshInfo> MeshInfo = MakeShareable(new FFbxMeshInfo());

			if (!MeshInfoObj->TryGetStringField("Name", MeshInfo->Name)) continue;

			FString UniqueIDStr;
			if (!MeshInfoObj->TryGetStringField("UniqueId", UniqueIDStr)) continue;
			MeshInfo->UniqueId = FCString::Atoi64(*UniqueIDStr);

			if (!MeshInfoObj->TryGetBoolField("bImportAttribute", MeshInfo->bImportAttribute)) continue;

			if (!MeshInfoObj->TryGetStringField("OptionName", MeshInfo->OptionName)) continue;

			if (!MeshInfoObj->TryGetBoolField("bIsSkelMesh", MeshInfo->bIsSkelMesh)) continue;

			if (!MeshInfoObj->TryGetStringField("OriginalImportPath", MeshInfo->OriginalImportPath)) continue;

			if (!MeshInfoObj->TryGetStringField("OriginalFullImportName", MeshInfo->OriginalFullImportName)) continue;

			if (!MeshInfoObj->TryGetBoolField("bOverridePath", MeshInfo->bOverridePath)) continue;

			if (!MeshInfoObj->TryGetStringField("OverrideImportPath", MeshInfo->OverrideImportPath)) continue;

			if (!MeshInfoObj->TryGetStringField("OverrideFullImportName", MeshInfo->OverrideFullImportName)) continue;
				
			SceneInfoSourceData->MeshInfo.Add(MeshInfo);
		}

		//Read Hierarchy
		const TArray<TSharedPtr<FJsonValue>>* JSONHierarchyNodes;
		SceneInfoObj->TryGetArrayField("Hierarchy", JSONHierarchyNodes);
		for (const auto& NodeJsonValue : *JSONHierarchyNodes)
		{
			const TSharedPtr<FJsonObject>& NodeInfoObj = NodeJsonValue->AsObject();
			if (!NodeInfoObj.IsValid())
			{
				continue;
			}
			TSharedPtr<FFbxNodeInfo> NodeInfo = MakeShareable(new FFbxNodeInfo());

			if (!NodeInfoObj->TryGetStringField("NodeName", NodeInfo->NodeName)) continue;

			if (!NodeInfoObj->TryGetStringField("NodeHierarchyPath", NodeInfo->NodeHierarchyPath)) continue;

			FString UniqueIDStr;
			if (!NodeInfoObj->TryGetStringField("UniqueId", UniqueIDStr)) continue;
			NodeInfo->UniqueId = FCString::Atoi64(*UniqueIDStr);

			if (!NodeInfoObj->TryGetBoolField("bImportNode", NodeInfo->bImportNode)) continue;

			//Find the parent
			if (!NodeInfoObj->TryGetStringField("ParentUniqueId", UniqueIDStr)) continue;
			uint64 ParentUniqueId = FCString::Atoi64(*UniqueIDStr);
			if (ParentUniqueId != 0)
			{
				//Find the parent
				for (TSharedPtr<FFbxNodeInfo> SearchNodeInfo : SceneInfoSourceData->HierarchyInfo)
				{
					if (SearchNodeInfo->UniqueId == ParentUniqueId)
					{
						NodeInfo->ParentNodeInfo = SearchNodeInfo;
						SearchNodeInfo->Childrens.Add(NodeInfo);
						break;
					}
				}
			}

			if (!NodeInfoObj->TryGetStringField("AttributeType", NodeInfo->AttributeType)) continue;

			if (!NodeInfoObj->TryGetStringField("AttributeInfo", UniqueIDStr)) continue;
			uint64 attributeUniqueID = FCString::Atoi64(*UniqueIDStr);
			if (attributeUniqueID != 0)
			{
				//Find the attribute info
				for (TSharedPtr<FFbxMeshInfo> MeshInfo : SceneInfoSourceData->MeshInfo)
				{
					if (MeshInfo->UniqueId == attributeUniqueID)
					{
						NodeInfo->AttributeInfo = MeshInfo;
						break;
					}
				}
			}

			//Materials
			const TArray<TSharedPtr<FJsonValue>>* JSONMaterials;
			NodeInfoObj->TryGetArrayField("Materials", JSONMaterials);
			for (const auto& MaterialJsonValue : *JSONMaterials)
			{
				const TSharedPtr<FJsonObject>& MaterialObj = MaterialJsonValue->AsObject();
				if (!MaterialObj.IsValid())
				{
					continue;
				}

				TSharedPtr<FFbxMaterialInfo> MaterialInfo = MakeShareable(new FFbxMaterialInfo());

				if (!MaterialObj->TryGetStringField("Name", MaterialInfo->Name)) continue;

				if (!MaterialObj->TryGetStringField("HierarchyPath", MaterialInfo->HierarchyPath)) continue;

				if (!MaterialObj->TryGetStringField("UniqueId", UniqueIDStr)) continue;
				MaterialInfo->UniqueId = FCString::Atoi64(*UniqueIDStr);

				if (!MaterialObj->TryGetBoolField("bImportAttribute", MaterialInfo->bImportAttribute)) continue;

				if (!MaterialObj->TryGetStringField("OriginalImportPath", MaterialInfo->OriginalImportPath)) continue;

				if (!MaterialObj->TryGetStringField("OriginalFullImportName", MaterialInfo->OriginalFullImportName)) continue;

				if (!MaterialObj->TryGetBoolField("bOverridePath", MaterialInfo->bOverridePath)) continue;

				if (!MaterialObj->TryGetStringField("OverrideImportPath", MaterialInfo->OverrideImportPath)) continue;

				if (!MaterialObj->TryGetStringField("OverrideFullImportName", MaterialInfo->OverrideFullImportName)) continue;

				NodeInfo->Materials.Add(MaterialInfo);
			}

			SceneInfoSourceData->HierarchyInfo.Add(NodeInfo);
		}
	}
}

void UFbxSceneImportData::Serialize(FArchive& Ar)
{
	if (!Ar.IsFilterEditorOnly())
	{
		FString Json;
		if (Ar.IsLoading())
		{
			Ar << Json;
			FromJson(MoveTemp(Json));
		}
		else if (Ar.IsSaving())
		{
			Json = ToJson();
			Ar << Json;
		}
	}

	Super::Serialize(Ar);
}
#endif