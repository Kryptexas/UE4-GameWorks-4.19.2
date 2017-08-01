// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "USDImportOptions.h"
#include "TokenizedMessage.h"

THIRD_PARTY_INCLUDES_START
/*
#if USING_CODE_ANALYSIS
MSVC_PRAGMA(warning(push))
MSVC_PRAGMA(warning(disable : ALL_CODE_ANALYSIS_WARNINGS))
#endif	// USING_CODE_ANALYSIS
*/

#include "UnrealUSDWrapper.h"

/*
#if USING_CODE_ANALYSIS
MSVC_PRAGMA(warning(pop))
#endif	// USING_CODE_ANALYSIS
*/
THIRD_PARTY_INCLUDES_END

#include "USDImporter.generated.h"

class UGeometryCache;
class UEditableStaticMesh;
class IUsdPrim;
class IUsdStage;
struct FUsdGeomData;

DECLARE_LOG_CATEGORY_EXTERN(LogUSDImport, Log, All);

struct FUsdPrimToImport
{
	IUsdPrim* Prim;
	int32 NumLODs;

	const FUsdGeomData* GetGeomData(int32 LODIndex, double Time) const;
};

USTRUCT()
struct FUsdImportContext
{
	GENERATED_BODY()
	
	TMap<IUsdPrim*, UObject*> PrimToAssetMap;

	/** Parent package to import a single mesh to */
	UPROPERTY()
	UObject* Parent;

	/** Name to use when importing a single mesh */
	UPROPERTY()
	FString ObjectName;

	UPROPERTY()
	FString ImportPathName;

	UPROPERTY()
	UUSDImportOptions* ImportOptions;

	IUsdStage* Stage;

	/** Root Prim of the USD file */
	IUsdPrim* RootPrim;

	/** Converts from a source coordinate system to Unreal */
	FTransform ConversionTransform;

	/** Object flags to apply to newly imported objects */
	EObjectFlags ObjectFlags;

	/** Whether or not to apply world transformations to the actual geometry */
	bool bApplyWorldTransformToGeometry;

	/** If true stop at any USD prim that has an unreal asset reference.  Geometry that is a child such prims will be ignored */
	bool bFindUnrealAssetReferences;

	virtual ~FUsdImportContext() { }

	virtual void Init(UObject* InParent, const FString& InName, EObjectFlags InFlags, IUsdStage* InStage);

	void AddErrorMessage(EMessageSeverity::Type MessageSeverity, FText ErrorMessage);
	void DisplayErrorMessages(bool bAutomated);
	void ClearErrorMessages();
private:
	/** Error messages **/
	TArray<TSharedRef<FTokenizedMessage>> TokenizedErrorMessages;
};


UCLASS(transient)
class UUSDImporter : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	bool ShowImportOptions(UObject& ImportOptions);

	IUsdStage* ReadUSDFile(FUsdImportContext& ImportContext, const FString& Filename);

	void FindPrimsToImport(FUsdImportContext& ImportContext, TArray<FUsdPrimToImport>& OutPrimsToImport);

	UObject* ImportMeshes(FUsdImportContext& ImportContext, const TArray<FUsdPrimToImport>& PrimsToImport);

	UObject* ImportSingleMesh(FUsdImportContext& ImportContext, EUsdMeshImportType ImportType, const FUsdPrimToImport& PrimToImport);
private:
	void FindPrimsToImport_Recursive(FUsdImportContext& ImportContext, IUsdPrim* Prim, TArray<FUsdPrimToImport>& OutTopLevelPrims);

};