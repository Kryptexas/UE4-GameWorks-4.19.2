// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "AssetRegistryModule.h"
#include "DesktopPlatformModule.h"
#include "AssetToolsModule.h"
#include "MainFrame.h"
#include "ContentBrowserModule.h"
#include "ReferencedAssetsUtils.h"
#include "SPackageReportDialog.h"
#include "EngineAnalytics.h"
#include "IAnalyticsProvider.h"
#include "MessageLog.h"

#define LOCTEXT_NAMESPACE "AssetTools"

FAssetTools::FAssetTools()
	: AssetRenameManager( MakeShareable(new FAssetRenameManager) )
{
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_AnimationAsset) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_AnimBlueprint) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_AnimComposite) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_AnimMontage) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_AnimSequence) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_AimOffset) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_AimOffset1D) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_BlendSpace) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_BlendSpace1D) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_Blueprint) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_BlueprintGeneratedClass) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_CameraAnim) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_Curve) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_CurveFloat) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_CurveTable) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_CurveVector) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_CurveLinearColor) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_DataAsset) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_DataTable) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_DestructibleMesh) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_DialogueVoice) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_DialogueWave) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_Enum) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_Font) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_ForceFeedbackEffect) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_InstancedFoliageSettings) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_InterpData) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_LandscapeLayer) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_Material) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_MaterialFunction) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_MaterialInstanceConstant) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_MaterialInterface) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_MaterialParameterCollection) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_MorphTarget) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_ObjectLibrary) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_ParticleSystem) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_PhysicalMaterial) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_PhysicsAsset) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_Redirector) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_ReverbEffect) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_SkeletalMesh) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_Skeleton) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_SlateBrush) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_SlateWidgetStyle) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_SoundAttenuation) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_SoundBase) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_SoundClass) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_SoundCue) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_SoundMix) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_SoundWave) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_StaticMesh) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_Texture) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_Texture2D) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_TextureCube) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_TextureMovie) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_TextureRenderTarget) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_TextureRenderTarget2D) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_TextureRenderTargetCube) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_TextureLightProfile) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_TouchInterface) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_VectorField) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_VectorFieldAnimated) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_VectorFieldStatic) );
	RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_VertexAnimation) );

	if ( FParse::Param(FCommandLine::Get(), TEXT("WorldAssets")) )
	{
		RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_World) );
	}

	bool bEnableNiagara = false;
	GConfig->GetBool(TEXT("Niagara"), TEXT("EnableNiagara"), bEnableNiagara, GEngineIni);
	if ( bEnableNiagara )
	{
		RegisterAssetTypeActions( MakeShareable(new FAssetTypeActions_NiagaraScript) );
	}
}

FAssetTools::~FAssetTools()
{

}

void FAssetTools::RegisterAssetTypeActions(const TSharedRef<IAssetTypeActions>& NewActions)
{
	AssetTypeActionsList.Add(NewActions);
}

void FAssetTools::UnregisterAssetTypeActions(const TSharedRef<IAssetTypeActions>& ActionsToRemove)
{
	AssetTypeActionsList.Remove(ActionsToRemove);
}

void FAssetTools::GetAssetTypeActionsList( TArray<TWeakPtr<IAssetTypeActions>>& OutAssetTypeActionsList ) const
{
	for (auto ActionsIt = AssetTypeActionsList.CreateConstIterator(); ActionsIt; ++ActionsIt)
	{
		OutAssetTypeActionsList.Add(*ActionsIt);
	}
}

TWeakPtr<IAssetTypeActions> FAssetTools::GetAssetTypeActionsForClass( UClass* Class ) const
{
	TSharedPtr<IAssetTypeActions> MostDerivedAssetTypeActions;

	for (int32 TypeActionsIdx = 0; TypeActionsIdx < AssetTypeActionsList.Num(); ++TypeActionsIdx)
	{
		TSharedRef<IAssetTypeActions> TypeActions = AssetTypeActionsList[TypeActionsIdx];
		UClass* SupportedClass = TypeActions->GetSupportedClass();

		if ( Class->IsChildOf(SupportedClass) )
		{
			if ( !MostDerivedAssetTypeActions.IsValid() || SupportedClass->IsChildOf( MostDerivedAssetTypeActions->GetSupportedClass() ) )
			{
				MostDerivedAssetTypeActions = TypeActions;
			}
		}
	}

	return MostDerivedAssetTypeActions;
}

bool FAssetTools::GetAssetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder, bool bIncludeHeading )
{
	bool bAddedActions = false;

	if ( InObjects.Num() )
	{
		// Find the most derived common class for all passed in Objects
		UClass* CommonClass = InObjects[0]->GetClass();
		for (int32 ObjIdx = 1; ObjIdx < InObjects.Num(); ++ObjIdx)
		{
			while (!InObjects[ObjIdx]->IsA(CommonClass))
			{
				CommonClass = CommonClass->GetSuperClass();
			}
		}

		// Get the nearest common asset type for all the selected objects
		TSharedPtr<IAssetTypeActions> CommonAssetTypeActions = GetAssetTypeActionsForClass(CommonClass).Pin();

		// If we found a common type actions object, get actions from it
		if ( CommonAssetTypeActions.IsValid() && CommonAssetTypeActions->HasActions(InObjects) )
		{
			if ( bIncludeHeading )
			{
				MenuBuilder.BeginSection("GetAssetActions", FText::Format( LOCTEXT("AssetSpecificOptionsMenuHeading", "{0} Actions"), CommonAssetTypeActions->GetName() ) );
			}

			// Get the actions
			CommonAssetTypeActions->GetActions(InObjects, MenuBuilder);

			if ( bIncludeHeading )
			{
				MenuBuilder.EndSection();
			}

			bAddedActions = true;
		}
	}

	return bAddedActions;
}

struct FRootedOnScope
{
	UObject * Obj;

	FRootedOnScope(UObject * InObj) : Obj(NULL)
	{
		if (InObj && !InObj->IsRooted())
		{
			Obj = InObj;
			Obj->AddToRoot();
		}
	}

	~FRootedOnScope()
	{
		if (Obj)
		{
			Obj->RemoveFromRoot();
		}
	}
};

UObject* FAssetTools::CreateAsset(const FString& AssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory, FName CallingContext)
{
	FRootedOnScope DontGCFactory(Factory);

	// Verify the factory class
	if ( !ensure(AssetClass || Factory) )
	{
		FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("MustSupplyClassOrFactory", "The new asset wasn't created due to a problem finding the appropriate factory or class for the new asset.") );
		return NULL;
	}

	if ( AssetClass && Factory && !ensure(AssetClass->IsChildOf(Factory->GetSupportedClass())) )
	{
		FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("InvalidFactory", "The new asset wasn't created because the supplied factory does not support the supplied class.") );
		return NULL;
	}

	const FString PackageName = PackagePath + TEXT("/") + AssetName;

	// Make sure we can create the asset without conflicts
	if ( !CanCreateAsset(AssetName, PackageName, LOCTEXT("CreateANewObject", "Create a new object")) )
	{
		return NULL;
	}

	UClass* ClassToUse = AssetClass ? AssetClass : Factory->GetSupportedClass();

	UPackage* Pkg = CreatePackage(NULL,*PackageName);
	UObject* NewObj = NULL;
	EObjectFlags Flags = RF_Public|RF_Standalone;
	if ( Factory )
	{
		NewObj = Factory->FactoryCreateNew(ClassToUse, Pkg, FName( *AssetName ), Flags, NULL, GWarn, CallingContext);
	}
	else if ( AssetClass )
	{
		NewObj = StaticConstructObject(ClassToUse, Pkg, FName(*AssetName), Flags);
	}

	if( NewObj )
	{
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(NewObj);

		// Mark the package dirty...
		Pkg->MarkPackageDirty();
	}

	return NewObj;
}

UObject* FAssetTools::DuplicateAsset(const FString& AssetName, const FString& PackagePath, UObject* OriginalObject)
{
	// Verify the source object
	if ( !OriginalObject )
	{
		FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("InvalidSourceObject", "The new asset wasn't created due to a problem finding the object to duplicate.") );
		return NULL;
	}

	const FString PackageName = PackagePath + TEXT("/") + AssetName;

	// Make sure we can create the asset without conflicts
	if ( !CanCreateAsset(AssetName, PackageName, LOCTEXT("DuplicateAnObject", "Duplicate an object")) )
	{
		return NULL;
	}

	ObjectTools::FPackageGroupName PGN;
	PGN.PackageName = PackageName;
	PGN.GroupName = TEXT("");
	PGN.ObjectName = AssetName;

	TSet<UPackage*> ObjectsUserRefusedToFullyLoad;
	return ObjectTools::DuplicateSingleObject(OriginalObject, PGN, ObjectsUserRefusedToFullyLoad);
}

void FAssetTools::RenameAssets(const TArray<FAssetRenameData>& AssetsAndNames) const
{
	AssetRenameManager->RenameAssets(AssetsAndNames);
}

TArray<UObject*> FAssetTools::ImportAssets(const FString& DestinationPath)
{
	TArray<UObject*> ReturnObjects;
	FString FileTypes, AllExtensions;
	TArray<UFactory*> Factories;

	// Get the list of valid factories
	for( TObjectIterator<UClass> It ; It ; ++It )
	{
		UClass* CurrentClass = (*It);

		if( CurrentClass->IsChildOf(UFactory::StaticClass()) && !(CurrentClass->HasAnyClassFlags(CLASS_Abstract)) )
		{
			UFactory* Factory = Cast<UFactory>( CurrentClass->GetDefaultObject() );
			if( Factory->bEditorImport && Factory->ValidForCurrentGame() )
			{
				Factories.Add( Factory );
			}
		}
	}

	// Generate the file types and extensions represented by the selected factories
	ObjectTools::GenerateFactoryFileExtensions( Factories, FileTypes, AllExtensions );

	FileTypes = FString::Printf(TEXT("All Files (%s)|%s|%s"),*AllExtensions,*AllExtensions,*FileTypes);

	// Prompt the user for the filenames
	TArray<FString> OpenFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bOpened = false;
	if ( DesktopPlatform )
	{
		void* ParentWindowWindowHandle = NULL;

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		bOpened = DesktopPlatform->OpenFileDialog(
			ParentWindowWindowHandle,
			LOCTEXT("ImportDialogTitle", "Import").ToString(),
			FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT),
			TEXT(""),
			FileTypes,
			EFileDialogFlags::Multiple,
			OpenFilenames
			);
	}

	if ( bOpened )
	{
		if ( OpenFilenames.Num() > 0 )
		{
			FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_IMPORT, OpenFilenames[0]);
			ReturnObjects = ImportAssets(OpenFilenames, DestinationPath);
		}
	}

	return ReturnObjects;
}

TArray<UObject*> FAssetTools::ImportAssets(const TArray<FString>& Files, const FString& DestinationPath) const
{
	TArray<UObject*> ReturnObjects;
	TMap< FString, TArray<UFactory*> > ExtensionToFactoriesMap;

	GWarn->BeginSlowTask( LOCTEXT("ImportSlowTask", "Importing"), true );

	// Reset the 'Do you want to overwrite the existing object?' Yes to All / No to All prompt, to make sure the
	// user gets a chance to select something
	UFactory::ResetState();

	// First instantiate one factory for each file extension encountered that supports the extension
	for( TObjectIterator<UClass> ClassIt ; ClassIt ; ++ClassIt )
	{
		if( (*ClassIt)->IsChildOf(UFactory::StaticClass()) && !((*ClassIt)->HasAnyClassFlags(CLASS_Abstract)) )
		{
			UFactory* Factory = Cast<UFactory>( (*ClassIt)->GetDefaultObject() );
			if( Factory->bEditorImport && Factory->ValidForCurrentGame() )
			{
				TArray<FString> FactoryExtensions;
				Factory->GetSupportedFileExtensions(FactoryExtensions);
				for ( auto FileIt = Files.CreateConstIterator(); FileIt; ++FileIt )
				{
					const FString FileExtension = FPaths::GetExtension(*FileIt);

					// Case insensitive string compare with supported formats of this factory
					if ( FactoryExtensions.Contains(FileExtension) )
					{
						TArray<UFactory*>& ExistingFactories = ExtensionToFactoriesMap.FindOrAdd(FileExtension);
						
						// Do not remap extensions, just reuse the existing UFactory.
						// There may be multiple UFactories, so we will keep track of all of them
						bool bFactoryAlreadyInMap = false;
						for ( auto FoundFactoryIt = ExistingFactories.CreateConstIterator(); FoundFactoryIt; ++FoundFactoryIt )
						{
							if ( (*FoundFactoryIt)->GetClass() == Factory->GetClass() )
							{
								bFactoryAlreadyInMap = true;
								break;
							}
						}

						if ( !bFactoryAlreadyInMap )
						{
							// We found a factory for this file, it can be imported!
							// Create a new factory of the same class and make sure it doesnt get GCed.
							// The object will be removed from the root set at the end of this function.
							UFactory* NewFactory = ConstructObject<UFactory>( Factory->GetClass() );
							if ( NewFactory->ConfigureProperties() )
							{
								NewFactory->AddToRoot();
								ExistingFactories.Add(NewFactory);
							}
						}
					}
				}
			}
		}
	}

	// Some flags to keep track of what the user decided when asked about overwriting or replacing
	bool bOverwriteAll = false;
	bool bReplaceAll = false;
	bool bDontOverwriteAny = false;
	bool bDontReplaceAny = false;

	// Now iterate over the input files and use the same factory object for each file with the same extension
	for (int32 FileIdx = 0; FileIdx < Files.Num(); ++FileIdx)
	{
		const FString& Filename = Files[FileIdx];
		FString FileExtension = FPaths::GetExtension(Filename);

		const TArray<UFactory*>* FactoriesPtr = ExtensionToFactoriesMap.Find(FileExtension);
		UFactory* Factory = NULL;
		if ( FactoriesPtr )
		{
			const TArray<UFactory*>& Factories = *FactoriesPtr;

			// Handle the potential of multiple factories being found
			if( Factories.Num() > 0 )
			{
				Factory = Factories[0];

				for( auto FactoryIt = Factories.CreateConstIterator(); FactoryIt; ++FactoryIt )
				{
					UFactory* TestFactory = *FactoryIt;
					if( TestFactory->FactoryCanImport( Filename ) )
					{
						Factory = TestFactory;
						break;
					}
				}
			}
		}
		else
		{
			const FText Message = FText::Format( LOCTEXT("ImportFailed_UnknownExtension", "Failed to import '{0}'. Unknown extension '{1}'."), FText::FromString( Filename ), FText::FromString( FileExtension ) );
			FMessageDialog::Open( EAppMsgType::Ok, Message );
			UE_LOG(LogAssetTools, Warning, TEXT("%s"), *Message.ToString() );
		}

		if ( Factory != NULL )
		{
			UClass* ImportAssetType = Factory->SupportedClass;
			bool bImportSucceeded = false;
			bool bImportWasCancelled = false;
			FDateTime ImportStartTime = FDateTime::UtcNow();

			FString Name = ObjectTools::SanitizeObjectName(FPaths::GetBaseFilename(Filename));
			FString PackageName = DestinationPath + TEXT("/") + Name;

			// We can not create assets that share the name of a map file in the same location
			if ( FEditorFileUtils::IsMapPackageAsset(PackageName) )
			{
				const FText Message = FText::Format( LOCTEXT("AssetNameInUseByMap", "You can not create an asset named '{0}' because there is already a map file with this name in this folder."), FText::FromString( Name ) );
				FMessageDialog::Open( EAppMsgType::Ok, Message );
				UE_LOG(LogAssetTools, Warning, TEXT("%s"), *Message.ToString());
				OnNewImportRecord(ImportAssetType, FileExtension, bImportSucceeded, bImportWasCancelled, ImportStartTime);
				continue;
			}

			UPackage* Pkg = CreatePackage(NULL, *PackageName);
			if ( !ensure(Pkg) )
			{
				// Failed to create the package to hold this asset for some reason
				OnNewImportRecord(ImportAssetType, FileExtension, bImportSucceeded, bImportWasCancelled, ImportStartTime);
				continue;
			}

			// Make sure the destination package is loaded
			Pkg->FullyLoad();

			// Check for an existing object
			UObject* ExistingObject = StaticFindObject( UObject::StaticClass(), Pkg, *Name );
			if( ExistingObject != NULL )
			{
				// If the object is supported by the factory we are using, ask if we want to overwrite the asset
				// Otherwise, prompt to replace the object
				if ( Factory->DoesSupportClass(ExistingObject->GetClass()) )
				{
					// The factory can overwrite this object, ask if that is okay, unless "Yes To All" or "No To All" was already selected
					EAppReturnType::Type UserResponse;
					
					if ( bOverwriteAll || GIsAutomationTesting)
					{
						UserResponse = EAppReturnType::YesAll;
					}
					else if ( bDontOverwriteAny )
					{
						UserResponse = EAppReturnType::NoAll;
					}
					else
					{
						UserResponse = FMessageDialog::Open(
							EAppMsgType::YesNoYesAllNoAll,
							FText::Format( LOCTEXT("ImportObjectAlreadyExists_SameClass", "Do you want to overwrite the existing asset?\n\nAn asset already exists at the import location: {0}"), FText::FromString( PackageName ) ) );

						bOverwriteAll = UserResponse == EAppReturnType::YesAll;
						bDontOverwriteAny = UserResponse == EAppReturnType::NoAll;
					}

					const bool bWantOverwrite = UserResponse == EAppReturnType::Yes || UserResponse == EAppReturnType::YesAll;

					if( !bWantOverwrite )
					{
						// User chose not to replace the package
						bImportWasCancelled = true;
						OnNewImportRecord(ImportAssetType, FileExtension, bImportSucceeded, bImportWasCancelled, ImportStartTime);
						continue;
					}
				}
				else
				{
					// The factory can't overwrite this asset, ask if we should delete the object then import the new one. Only do this if "Yes To All" or "No To All" was not already selected.
					EAppReturnType::Type UserResponse;
					
					if ( bReplaceAll )
					{
						UserResponse = EAppReturnType::YesAll;
					}
					else if ( bDontReplaceAny )
					{
						UserResponse = EAppReturnType::NoAll;
					}
					else
					{
						UserResponse = FMessageDialog::Open(
								EAppMsgType::YesNoYesAllNoAll,
								FText::Format( LOCTEXT("ImportObjectAlreadyExists_DifferentClass", "Do you want to replace the existing asset?\n\nAn asset already exists at the import location: {0}"), FText::FromString( PackageName ) ) );

						bReplaceAll = UserResponse == EAppReturnType::YesAll;
						bDontReplaceAny = UserResponse == EAppReturnType::NoAll;
					}

					const bool bWantReplace = UserResponse == EAppReturnType::Yes || UserResponse == EAppReturnType::YesAll;

					if( bWantReplace )
					{
						// Delete the existing object
						int32 NumObjectsDeleted = 0;
						TArray< UObject* > ObjectsToDelete;
						ObjectsToDelete.Add(ExistingObject);

						// Dont let the package get garbage collected (just in case we are deleting the last asset in the package)
						Pkg->AddToRoot();
						NumObjectsDeleted = ObjectTools::DeleteObjects( ObjectsToDelete, /*bShowConfirmation=*/false );
						Pkg->RemoveFromRoot();

						const FString QualifiedName = PackageName + TEXT(".") + Name;
						FText Reason;
						if( NumObjectsDeleted == 0 || !IsUniqueObjectName( *QualifiedName, ANY_PACKAGE, Reason ) )
						{
							// Original object couldn't be deleted
							const FText Message = FText::Format( LOCTEXT("ImportDeleteFailed", "Failed to delete '{0}'. The asset is referenced by other content."), FText::FromString( PackageName ) );
							FMessageDialog::Open( EAppMsgType::Ok, Message );
							UE_LOG(LogAssetTools, Warning, TEXT("%s"), *Message.ToString());
							OnNewImportRecord(ImportAssetType, FileExtension, bImportSucceeded, bImportWasCancelled, ImportStartTime);
							continue;
						}
					}
					else
					{
						// User chose not to replace the package
						bImportWasCancelled = true;
						OnNewImportRecord(ImportAssetType, FileExtension, bImportSucceeded, bImportWasCancelled, ImportStartTime);
						continue;
					}
				}
			}

			// Check for a package that was marked for delete in source control
			if ( !CheckForDeletedPackage(Pkg) )
			{
				OnNewImportRecord(ImportAssetType, FileExtension, bImportSucceeded, bImportWasCancelled, ImportStartTime);
				continue;
			}

			ImportAssetType = Factory->ResolveSupportedClass();
			UObject* Result = UFactory::StaticImportObject( ImportAssetType, Pkg, FName( *Name ), RF_Public|RF_Standalone, bImportWasCancelled, *Filename, NULL, Factory );

			// Do not report any error if the operation was canceled.
			if(!bImportWasCancelled)
			{
				if ( Result )
				{
					ReturnObjects.Add( Result );

					// Notify the asset registry
					FAssetRegistryModule::AssetCreated(Result);
					GEditor->BroadcastObjectReimported(Result);

					bImportSucceeded = true;
				}
				else
				{
					const FText Message = FText::Format( LOCTEXT("ImportFailed_Generic", "Failed to import '{0}'. Failed to create asset '{1}'"), FText::FromString( Filename ), FText::FromString( PackageName ) );
					FMessageDialog::Open( EAppMsgType::Ok, Message );
					UE_LOG(LogAssetTools, Warning, TEXT("%s"), *Message.ToString());
				}
			}

			// Refresh the supported class.  Some factories (e.g. FBX) only resolve their type after reading the file
			ImportAssetType = Factory->ResolveSupportedClass();
			OnNewImportRecord(ImportAssetType, FileExtension, bImportSucceeded, bImportWasCancelled, ImportStartTime);
		}
		else
		{
			// A factory or extension was not found. The extension warning is above. If a factory was not found, the user likely canceled a factory configuration dialog.
		}
	}

	// Clean up and remove the factories we created from the root set
	for ( auto ExtensionIt = ExtensionToFactoriesMap.CreateConstIterator(); ExtensionIt; ++ExtensionIt)
	{
		for ( auto FactoryIt = ExtensionIt.Value().CreateConstIterator(); FactoryIt; ++FactoryIt )
		{
			(*FactoryIt)->CleanUp();
			(*FactoryIt)->RemoveFromRoot();
		}
	}

	GWarn->EndSlowTask();

	// Sync content browser to the newly created assets
	if ( ReturnObjects.Num() )
	{
		FAssetTools::Get().SyncBrowserToAssets(ReturnObjects);
	}

	return ReturnObjects;
}

void FAssetTools::CreateUniqueAssetName(const FString& InBasePackageName, const FString& InSuffix, FString& OutPackageName, FString& OutAssetName) const
{
	const FString PackagePath = FPackageName::GetLongPackagePath(InBasePackageName);
	const FString BaseAssetName = FPackageName::GetLongPackageAssetName(InBasePackageName);
	int32 IntSuffix = 1;
	UObject* ExistingObject = NULL;
	do
	{
		if ( IntSuffix <= 1 )
		{
			OutAssetName = FString::Printf( TEXT("%s%s"), *BaseAssetName, *InSuffix);
		}
		else
		{
			OutAssetName = FString::Printf( TEXT("%s%s%d"), *BaseAssetName, *InSuffix, IntSuffix );
		}
	
		OutPackageName = PackagePath + TEXT("/") + OutAssetName;
		FString ObjectPath = OutPackageName + TEXT(".") + OutAssetName;

		ExistingObject = LoadObject<UObject>(NULL, *ObjectPath, NULL, LOAD_NoWarn | LOAD_NoRedirects);
		IntSuffix++;
	}
	while (ExistingObject != NULL);
}

bool FAssetTools::AssetUsesGenericThumbnail( const FAssetData& AssetData ) const
{
	if ( !AssetData.IsValid() )
	{
		// Invalid asset, assume it does not use a shared thumbnail
		return false;
	}

	if( AssetData.IsAssetLoaded() )
	{
		// Loaded asset, see if there is a rendering info for it
		UObject* Asset = AssetData.GetAsset();
		FThumbnailRenderingInfo* RenderInfo = GUnrealEd->GetThumbnailManager()->GetRenderingInfo( Asset );
		return !RenderInfo || !RenderInfo->Renderer;
	}

	if ( AssetData.AssetClass == UBlueprint::StaticClass()->GetFName() )
	{
		// Unloaded blueprint asset
		// It would be more correct here to find the rendering info for the generated class,
		// but instead we are simply seeing if there is a thumbnail saved on disk for this asset
		FString PackageFilename;
		if ( FPackageName::DoesPackageExist(AssetData.PackageName.ToString(), NULL, &PackageFilename) )
		{
			TSet<FName> ObjectFullNames;
			FThumbnailMap ThumbnailMap;

			FName ObjectFullName = FName(*AssetData.GetFullName());
			ObjectFullNames.Add(ObjectFullName);

			ThumbnailTools::LoadThumbnailsFromPackage(PackageFilename, ObjectFullNames, ThumbnailMap);

			FObjectThumbnail* ThumbnailPtr = ThumbnailMap.Find(ObjectFullName);
			if (ThumbnailPtr)
			{
				return (*ThumbnailPtr).IsEmpty();
			}

			return true;
		}
	}
	else
	{
		// Unloaded non-blueprint asset. See if the class has a rendering info.
		UClass* Class = FindObject<UClass>(ANY_PACKAGE, *AssetData.AssetClass.ToString());

		UObject* ClassCDO = NULL;
		if (Class != NULL)
		{
			ClassCDO = Class->GetDefaultObject();
		}

		// Get the rendering info for this object
		FThumbnailRenderingInfo* RenderInfo = NULL;
		if (ClassCDO != NULL)
		{
			RenderInfo = GUnrealEd->GetThumbnailManager()->GetRenderingInfo( ClassCDO );
		}

		return !RenderInfo || !RenderInfo->Renderer;
	}

	return false;
}

void FAssetTools::DiffAgainstDepot( UObject* InObject, const FString& InPackagePath, const FString& InPackageName ) const
{
	check( InObject );

	// Make sure our history is up to date
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> UpdateStatusOperation = ISourceControlOperation::Create<FUpdateStatus>();
	UpdateStatusOperation->SetUpdateHistory(true);
	SourceControlProvider.Execute(UpdateStatusOperation, SourceControlHelpers::PackageFilename(InPackagePath));

	// Get the SCC state
	FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(SourceControlHelpers::PackageFilename(InPackagePath), EStateCacheUsage::Use);

	// If we have an asset and its in SCC..
	if( SourceControlState.IsValid() && InObject != NULL && SourceControlState->IsSourceControlled() )
	{
		// Get the file name of package
		FString RelativeFileName;
		if(FPackageName::DoesPackageExist(InPackagePath, NULL, &RelativeFileName))
		{
			if(SourceControlState->GetHistorySize() > 0)
			{
				TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(0);
				check(Revision.IsValid());

				// Get the head revision of this package from source control
				FString AbsoluteFileName = FPaths::ConvertRelativePathToFull(RelativeFileName);
				FString TempFileName;
				if(Revision->Get(TempFileName))
				{
					// Forcibly disable compile on load in case we are loading old blueprints that might try to update/compile
					TGuardValue<bool> DisableCompileOnLoad(GForceDisableBlueprintCompileOnLoad, true);

					// Try and load that package
					UPackage* TempPackage = LoadPackage(NULL, *TempFileName, LOAD_None);
					if(TempPackage != NULL)
					{
						// Grab the old asset from that old package
						UObject* OldObject = FindObject<UObject>(TempPackage, *InPackageName);
						if(OldObject != NULL)
						{
							/* Set the revision information*/
							auto Package = InObject->GetOutermost();
							auto PackageName = Package->GetName();

							FRevisionInfo OldRevision;
							OldRevision.Changelist = Revision->GetCheckInIdentifier();
							OldRevision.Date = Revision->GetDate();
							OldRevision.Revision = Revision->GetRevisionNumber();

							FRevisionInfo NewRevision; 
							NewRevision.Revision = -1;
							DiffAssets(OldObject, InObject, OldRevision, NewRevision);
						}
					}
				}
			}
		} 
	}
}

void FAssetTools::DiffAssets(UObject* OldAsset, UObject* NewAsset, const struct FRevisionInfo& OldRevision, const struct FRevisionInfo& NewRevision) const
{
	if(OldAsset == NULL || NewAsset == NULL)
	{
		UE_LOG(LogAssetTools, Warning, TEXT("DiffAssets: One of the supplied assets was NULL."));
		return;
	}

	// Get class of both assets 
	UClass* OldClass = OldAsset->GetClass();
	UClass* NewClass = NewAsset->GetClass();
	// If same class..
	if(OldClass == NewClass)
	{
		// Get class-specific actions
		TWeakPtr<IAssetTypeActions> Actions = GetAssetTypeActionsForClass( NewClass );
		if(Actions.IsValid())
		{
			// And use that to perform the Diff
			Actions.Pin()->PerformAssetDiff(OldAsset, NewAsset, OldRevision, NewRevision);
		}
	}
	else
	{
		UE_LOG(LogAssetTools, Warning, TEXT("DiffAssets: Classes were not the same."));
	}
}

FString FAssetTools::DumpAssetToTempFile(UObject* Asset) const
{
	check(Asset);

	// Clear the mark state for saving.
	UnMarkAllObjects(EObjectMark(OBJECTMARK_TagExp | OBJECTMARK_TagImp));

	FStringOutputDevice Archive;
	const FExportObjectInnerContext Context;

	// Export asset to archive
	UExporter::ExportToOutputDevice(&Context, Asset, NULL, Archive, TEXT("copy"), 0, PPF_ExportsNotFullyQualified|PPF_Copy|PPF_Delimited, false, Asset->GetOuter());

	// Used to generate unique file names during a run
	static int TempFileNum = 0;

	// Build name for temp text file
	FString RelTempFileName = FString::Printf(TEXT("%sText%s-%d.txt"), *FPaths::DiffDir(), *Asset->GetName(), TempFileNum++);
	FString AbsoluteTempFileName = FPaths::ConvertRelativePathToFull(RelTempFileName);

	// Save text into temp file
	if( !FFileHelper::SaveStringToFile( Archive, *AbsoluteTempFileName ) )
	{
		//UE_LOG(LogAssetTools, Warning, TEXT("DiffAssets: Could not write %s"), *AbsoluteTempFileName);
		return TEXT("");
	}
	else
	{
		return AbsoluteTempFileName;
	}
}

bool FAssetTools::CreateDiffProcess(const FString& DiffCommand, const FString& DiffArgs) const
{
	// Fire process
	if (!FPlatformProcess::CreateProc(*DiffCommand, *DiffArgs, true, false, false, NULL, 0, NULL, NULL).IsValid())
	{
		//failed to work, put up error msg
		FNotificationInfo Info( FText::Format( NSLOCTEXT("AssetTools", "DifFail", "Diff Failed: Could not launch external process '{0}'."), FText::FromString( DiffCommand ) ) );
		Info.ExpireDuration = 10.0f;
		Info.bUseLargeFont = false;
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if ( Notification.IsValid() )
		{
			Notification->SetCompletionState( SNotificationItem::CS_Fail );
		}

		return false;
	}
	return true;
}

void FAssetTools::MigratePackages(const TArray<FName>& PackageNamesToMigrate) const
{
	// Packages must be saved for the migration to work
	const bool bPromptUserToSave = true;
	const bool bSaveMapPackages = true;
	const bool bSaveContentPackages = true;
	if ( FEditorFileUtils::SaveDirtyPackages( bPromptUserToSave, bSaveMapPackages, bSaveContentPackages ) )
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		if ( AssetRegistryModule.Get().IsLoadingAssets() )
		{
			// Open a dialog asking the user to wait while assets are being discovered
			SDiscoveringAssetsDialog::OpenDiscoveringAssetsDialog(
				SDiscoveringAssetsDialog::FOnAssetsDiscovered::CreateRaw(this, &FAssetTools::PerformMigratePackages, PackageNamesToMigrate)
				);
		}
		else
		{
			// Assets are already discovered, perform the migration now
			PerformMigratePackages(PackageNamesToMigrate);
		}
	}
}


void FAssetTools::OnNewImportRecord(UClass* AssetType, const FString& FileExtension, bool bSucceeded, bool bWasCancelled, const FDateTime& StartTime)
{
	// Don't attempt to report usage stats if analytics isn't available
	if(AssetType != NULL && FEngineAnalytics::IsAvailable())
	{
		TArray<FAnalyticsEventAttribute> Attribs;
		Attribs.Add(FAnalyticsEventAttribute(TEXT("AssetType"), AssetType->GetName()));
		Attribs.Add(FAnalyticsEventAttribute(TEXT("FileExtension"), FileExtension));
		Attribs.Add(FAnalyticsEventAttribute(TEXT("Outcome"), bSucceeded ? TEXT("Success") : (bWasCancelled ? TEXT("Cancelled") : TEXT("Failed"))));
		FTimespan TimeTaken = FDateTime::UtcNow() - StartTime;
		Attribs.Add(FAnalyticsEventAttribute(TEXT("TimeTaken.Seconds"), (float)TimeTaken.GetTotalSeconds()));

		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.ImportAsset"), Attribs);
	}
}

FAssetTools& FAssetTools::Get()
{
	FAssetToolsModule& Module = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	return static_cast<FAssetTools&>(Module.Get());
}

void FAssetTools::SyncBrowserToAssets(const TArray<UObject*>& AssetsToSync)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets( AssetsToSync, /*bAllowLockedBrowsers=*/true );
}

void FAssetTools::SyncBrowserToAssets(const TArray<FAssetData>& AssetsToSync)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets( AssetsToSync, /*bAllowLockedBrowsers=*/true );
}

bool FAssetTools::CheckForDeletedPackage(const UPackage* Package) const
{
	if ( ISourceControlModule::Get().IsEnabled() )
	{
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		if ( SourceControlProvider.IsAvailable() )
		{
			FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(Package, EStateCacheUsage::ForceUpdate);
			if ( SourceControlState.IsValid() && SourceControlState->IsDeleted() )
			{
				// Creating an asset in a package that is marked for delete.
				// Prompt the user to check in the package.
				bool bWantCheckin =
				EAppReturnType::Yes == FMessageDialog::Open(
				EAppMsgType::YesNo,
				FText::Format(
					LOCTEXT("CheckInDeletedPackage", "'{0}' is marked for delete in source control. You must check in the delete request before creating an asset in its place. Do you want to check in the delete?"),
					FText::FromString( FPackageName::GetLongPackageAssetName(Package->GetName()) )
					) 
				);

				if( bWantCheckin )
				{
					TSharedRef<FCheckIn, ESPMode::ThreadSafe> CheckInOperation = ISourceControlOperation::Create<FCheckIn>();
					CheckInOperation->SetDescription( LOCTEXT("AutomaticDeleteDesc", "Automatic Delete").ToString() );
					if ( !SourceControlProvider.Execute(CheckInOperation, Package) )
					{
						// Failed to check in file
						FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("DeletedFileCheckinFailed", "Failed to check in deleted package."));
						return false;
					}
				}
				else
				{
					// User chose not to check in the package that is marked for delete
					return false;
				}
			}
		}
		else
		{
			FMessageLog EditorErrors("EditorErrors");
			EditorErrors.Warning(LOCTEXT( "DeletingNoSCCConnection", "Could not check for deleted file. No connection to source control available!"));
			EditorErrors.Notify();
		}
	}

	return true;
}

bool FAssetTools::CanCreateAsset(const FString& AssetName, const FString& PackageName, const FText& OperationText) const
{
	// @todo: These 'reason' messages are not localized strings!
	FText Reason;
	if (!FName(*AssetName).IsValidObjectName( Reason )
		|| !FPackageName::IsValidLongPackageName( PackageName, /*bIncludeReadOnlyRoots=*/false, &Reason ) )
	{
		FMessageDialog::Open( EAppMsgType::Ok, Reason );
		return false;
	}

	// We can not create assets that share the name of a map file in the same location
	if ( FEditorFileUtils::IsMapPackageAsset(PackageName) )
	{
		FMessageDialog::Open( EAppMsgType::Ok, FText::Format( LOCTEXT("AssetNameInUseByMap", "You can not create an asset named '{0}' because there is already a map file with this name in this folder."), FText::FromString( AssetName ) ) );
		return false;
	}

	// Find (or create!) the desired package for this object
	UPackage* Pkg = CreatePackage(NULL,*PackageName);

	// Handle fully loading packages before creating new objects.
	TArray<UPackage*> TopLevelPackages;
	TopLevelPackages.Add( Pkg );
	if( !PackageTools::HandleFullyLoadingPackages( TopLevelPackages, OperationText ) )
	{
		// User aborted.
		return false;
	}

	// We need to test again after fully loading.
	if (!FName(*AssetName).IsValidObjectName( Reason )
		||	!FPackageName::IsValidLongPackageName( PackageName, /*bIncludeReadOnlyRoots=*/false, &Reason ) )
	{
		FMessageDialog::Open( EAppMsgType::Ok, Reason );
		return false;
	}

	// Check for an existing object
	UObject* ExistingObject = StaticFindObject( UObject::StaticClass(), Pkg, *AssetName );
	if( ExistingObject != NULL )
	{
		// Object already exists in either the specified package or another package.  Check to see if the user wants
		// to replace the object.
		bool bWantReplace =
			EAppReturnType::Yes == FMessageDialog::Open(
			EAppMsgType::YesNo,
			FText::Format(
				NSLOCTEXT("UnrealEd", "ReplaceExistingObjectInPackage_F", "An object [{0}] of class [{1}] already exists in file [{2}].  Do you want to replace the existing object?  If you click 'Yes', the existing object will be deleted.  Otherwise, click 'No' and choose a unique name for your new object." ),
				FText::FromString(AssetName), FText::FromString(ExistingObject->GetClass()->GetName()), FText::FromString(PackageName) ) );

		if( bWantReplace )
		{
			// Replacing an object.  Here we go!
			// Delete the existing object
			bool bDeleteSucceeded = ObjectTools::DeleteSingleObject( ExistingObject );

			if (bDeleteSucceeded)
			{
				// Force GC so we can cleanly create a new asset (and not do an 'in place' replacement)
				CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );

				// Old package will be GC'ed... create a new one here
				Pkg = CreatePackage(NULL,*PackageName);
			}
			else
			{
				// Notify the user that the operation failed b/c the existing asset couldn't be deleted
				FMessageDialog::Open( EAppMsgType::Ok,	FText::Format( NSLOCTEXT("DlgNewGeneric", "ContentBrowser_CannotDeleteReferenced", "{0} wasn't created.\n\nThe asset is referenced by other content."), FText::FromString( AssetName ) ) );
			}

			if( !bDeleteSucceeded || !IsUniqueObjectName( *AssetName, Pkg, Reason ) )
			{
				// Original object couldn't be deleted
				return false;
			}
		}
		else
		{
			// User chose not to replace the object; they'll need to enter a new name
			return false;
		}
	}

	// Check for a package that was marked for delete in source control
	if ( !CheckForDeletedPackage(Pkg) )
	{
		return false;
	}

	return true;
}

void FAssetTools::PerformMigratePackages(TArray<FName> PackageNamesToMigrate) const
{
	// Form a full list of packages to move by including the dependencies of the supplied packages
	TSet<FName> AllPackageNamesToMove;
	{
		const bool bAllowNewSlowTask = true;
		FStatusMessageContext SlowTaskGatheringDependenciesMessage( LOCTEXT( "MigratePackages_GatheringDependencies", "Gathering Dependencies..." ) );

		for ( auto PackageIt = PackageNamesToMigrate.CreateConstIterator(); PackageIt; ++PackageIt )
		{
			if ( !AllPackageNamesToMove.Contains(*PackageIt) )
			{
				AllPackageNamesToMove.Add(*PackageIt);
				RecursiveGetDependencies(*PackageIt, AllPackageNamesToMove);
			}
		}
	}

	// Confirm that there is at least one package to move 
	if ( AllPackageNamesToMove.Num() == 0 )
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("MigratePackages_NoFilesToMove", "No files were found to move"));
		return;
	}

	// Prompt the user displaying all assets that are going to be migrated
	{
		const FText ReportMessage = LOCTEXT("MigratePackagesReportTitle", "The following assets will be migrated to another content folder.");
		TArray<FString> ReportPackageNames;
		for ( auto PackageIt = AllPackageNamesToMove.CreateConstIterator(); PackageIt; ++PackageIt )
		{
			ReportPackageNames.Add((*PackageIt).ToString());
		}
		SPackageReportDialog::FOnReportConfirmed OnReportConfirmed = SPackageReportDialog::FOnReportConfirmed::CreateRaw(this, &FAssetTools::MigratePackages_ReportConfirmed, ReportPackageNames);
		SPackageReportDialog::OpenPackageReportDialog(ReportMessage, ReportPackageNames, OnReportConfirmed);
	}
}

void FAssetTools::MigratePackages_ReportConfirmed(TArray<FString> ConfirmedPackageNamesToMigrate) const
{
	// Choose a destination folder
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	FString DestinationFolder;
	if ( ensure(DesktopPlatform) )
	{
		void* ParentWindowWindowHandle = NULL;

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		const FString Title = LOCTEXT("MigrateToFolderTitle", "Choose a destination Content folder").ToString();
		bool bFolderAccepted = false;
		while (!bFolderAccepted)
		{
			const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
				ParentWindowWindowHandle,
				Title,
				FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT),
				DestinationFolder
				);

			if ( !bFolderSelected )
			{
				// User canceled, return
				return;
			}

			FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, DestinationFolder);
			FPaths::NormalizeFilename(DestinationFolder);
			if ( !DestinationFolder.EndsWith(TEXT("/")) )
			{
				DestinationFolder += TEXT("/");
			}

			// Verify that it is a content folder
			if ( DestinationFolder.EndsWith(TEXT("/Content/")) )
			{
				bFolderAccepted = true;
			}
			else
			{
				// The user chose a non-content folder. Confirm that this was their intention.
				const FText Message = FText::Format(LOCTEXT("MigratePackages_NonContentFolder", "{0} does not appear to be a game Content folder. Migrated content will only work properly if placed in a Content folder. Would you like to place your content here anyway?"), FText::FromString(DestinationFolder));
				EAppReturnType::Type Response = FMessageDialog::Open(EAppMsgType::YesNo, Message);
				bFolderAccepted = (Response == EAppReturnType::Yes);
			}
		}
	}
	else
	{
		// Not on a platform that supports desktop functionality
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoDesktopPlatform", "Error: This platform does not support a file dialog."));
		return;
	}

	bool bUserCanceled = false;

	// Copy all specified assets and their dependencies to the destination folder
	const bool bAllowNewSlowTask = true;
	FStatusMessageContext SlowTaskMessage( LOCTEXT( "MigratePackages_CopyingFiles", "Copying Files..." ), bAllowNewSlowTask );

	EAppReturnType::Type LastResponse = EAppReturnType::Yes;
	TArray<FString> SuccessfullyCopiedFiles;
	TArray<FString> SuccessfullyCopiedPackages;
	FString CopyErrors;
	for ( auto PackageNameIt = ConfirmedPackageNamesToMigrate.CreateConstIterator(); PackageNameIt; ++PackageNameIt )
	{
		const FString& PackageName = *PackageNameIt;
		FString SrcFilename;
		if ( !FPackageName::DoesPackageExist(PackageName, NULL, &SrcFilename) )
		{
			const FString ErrorMessage = FText::Format(LOCTEXT("MigratePackages_PackageMissing", "{0} does not exist on disk."), FText::FromString(PackageName)).ToString();
			UE_LOG(LogAssetTools, Warning, TEXT("%s"), *ErrorMessage);
			CopyErrors += ErrorMessage + LINE_TERMINATOR;
		}
		else if (SrcFilename.Contains(FPaths::EngineContentDir()))
		{
			const FString LeafName = SrcFilename.Replace(*FPaths::EngineContentDir(), TEXT("Engine/"));
			CopyErrors += FText::Format(LOCTEXT("MigratePackages_EngineContent", "Unable to migrate Engine asset {0}. Engine assets cannot be migrated."), FText::FromString(LeafName)).ToString() + LINE_TERMINATOR;
		}
		else
		{
			const FString DestFilename = SrcFilename.Replace(*FPaths::GameContentDir(), *DestinationFolder);

			bool bFileOKToCopy = true;
			if ( IFileManager::Get().FileSize(*DestFilename) > 0 )
			{
				// The destination file already exists! Ask the user what to do.
				EAppReturnType::Type Response;
				if ( LastResponse == EAppReturnType::YesAll || LastResponse == EAppReturnType::NoAll )
				{
					Response = LastResponse;
				}
				else
				{
					const FText Message = FText::Format( LOCTEXT("MigratePackages_AlreadyExists", "An asset already exists at location {0} would you like to overwrite it?"), FText::FromString(DestFilename) );
					Response = FMessageDialog::Open( EAppMsgType::YesNoYesAllNoAllCancel, Message );
					if ( Response == EAppReturnType::Cancel )
					{
						// The user chose to cancel mid-operation. Break out.
						bUserCanceled = true;
						break;
					}
					LastResponse = Response;
				}

				const bool bWantOverwrite = Response == EAppReturnType::Yes || Response == EAppReturnType::YesAll;
				if( !bWantOverwrite )
				{
					// User chose not to replace the package
					bFileOKToCopy = false;
				}
			}

			if ( bFileOKToCopy )
			{
				if ( IFileManager::Get().Copy(*DestFilename, *SrcFilename) == COPY_OK )
				{
					SuccessfullyCopiedPackages.Add(PackageName);
					SuccessfullyCopiedFiles.Add(DestFilename);
				}
				else
				{
					UE_LOG(LogAssetTools, Warning, TEXT("Failed to copy %s to %s while migrating assets"), *SrcFilename, *DestFilename);
					CopyErrors += SrcFilename + LINE_TERMINATOR;
				}
			}
		}
	}

	FString SourceControlErrors;
	if ( !bUserCanceled && SuccessfullyCopiedFiles.Num() > 0 )
	{
		// attempt to add files to source control (this can quite easily fail, but if it works it is very useful)
		if(GetDefault<UEditorLoadingSavingSettings>()->bSCCAutoAddNewFiles)
		{
			if(ISourceControlModule::Get().IsEnabled())
			{
				ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
				if(!SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), SuccessfullyCopiedFiles))
				{
					for(auto FileIt(SuccessfullyCopiedFiles.CreateConstIterator()); FileIt; FileIt++)
					{
						if(!SourceControlProvider.GetState(*FileIt, EStateCacheUsage::Use)->IsAdded())
						{
							SourceControlErrors += FText::Format(LOCTEXT("MigratePackages_SourceControlError", "{0} could not be added to source control"), FText::FromString(*FileIt)).ToString();
							SourceControlErrors += LINE_TERMINATOR;
						}
					}
				}
			}
		}
	}

	FMessageLog MigrateLog("AssetRegistry");
	FText LogMessage = FText::FromString(TEXT("Content migration completed successfully!"));
	EMessageSeverity::Type Severity = EMessageSeverity::Info;
	if ( CopyErrors.Len() > 0 || SourceControlErrors.Len() > 0 )
	{
		FString ErrorMessage;
		Severity = EMessageSeverity::Error;
		if( CopyErrors.Len() > 0 )
		{
			MigrateLog.NewPage( LOCTEXT("MigratePackages_CopyErrorsPage", "Copy Errors") );
			MigrateLog.Error(FText::FromString(*CopyErrors));
			ErrorMessage += FText::Format(LOCTEXT( "MigratePackages_CopyErrors", "Copied {0} files. Some content could not be copied."), FText::AsNumber(SuccessfullyCopiedPackages.Num())).ToString();
		}
		if( SourceControlErrors.Len() > 0 )
		{
			MigrateLog.NewPage( LOCTEXT("MigratePackages_SourceControlErrorsListPage", "Source Control Errors") );
			MigrateLog.Error(FText::FromString(*SourceControlErrors));
			ErrorMessage += LINE_TERMINATOR;
			ErrorMessage += LOCTEXT( "MigratePackages_SourceControlErrorsList", "Some files reported source control errors.").ToString();
		}
		if ( SuccessfullyCopiedPackages.Num() > 0 )
		{
			MigrateLog.NewPage( LOCTEXT("MigratePackages_CopyErrorsSuccesslistPage", "Copied Successfully") );
			MigrateLog.Info(FText::FromString(*SourceControlErrors));
			ErrorMessage += LINE_TERMINATOR;
			ErrorMessage += LOCTEXT( "MigratePackages_CopyErrorsSuccesslist", "Some files were copied successfully.").ToString();
			for ( auto FileIt = SuccessfullyCopiedPackages.CreateConstIterator(); FileIt; ++FileIt )
			{
				if(FileIt->Len()>0)
				{
					MigrateLog.Info(FText::FromString(*FileIt));
				}
			}
		}
		LogMessage = FText::FromString(ErrorMessage);
	}
	else if ( !bUserCanceled )
	{
		MigrateLog.NewPage( LOCTEXT("MigratePackages_CompletePage", "Content migration completed successfully!") );
		for ( auto FileIt = SuccessfullyCopiedPackages.CreateConstIterator(); FileIt; ++FileIt )
		{
			if(FileIt->Len()>0)
			{
				MigrateLog.Info(FText::FromString(*FileIt));
			}
		}
	}
	MigrateLog.Notify(LogMessage, Severity, true);
}

void FAssetTools::RecursiveGetDependencies(const FName& PackageName, TSet<FName>& AllDependencies) const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FName> Dependencies;
	AssetRegistryModule.Get().GetDependencies(PackageName, Dependencies);
	
	for ( auto DependsIt = Dependencies.CreateConstIterator(); DependsIt; ++DependsIt )
	{
		if ( !AllDependencies.Contains(*DependsIt) )
		{
			// @todo Make this skip all packages whose root is different than the source package list root. For now we just skip engine content.
			const bool bIsEnginePackage = (*DependsIt).ToString().StartsWith(TEXT("/Engine"));
			const bool bIsScriptPackage = (*DependsIt).ToString().StartsWith(TEXT("/Script"));
			if ( !bIsEnginePackage && !bIsScriptPackage )
			{
				AllDependencies.Add(*DependsIt);
				RecursiveGetDependencies(*DependsIt, AllDependencies);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
