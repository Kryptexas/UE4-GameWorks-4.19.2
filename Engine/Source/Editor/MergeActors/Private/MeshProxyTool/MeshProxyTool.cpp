// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MeshProxyTool/MeshProxyTool.h"
#include "Misc/Paths.h"
#include "Misc/FeedbackContext.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/Selection.h"
#include "Editor.h"
#include "MeshProxyTool/SMeshProxyDialog.h"
#include "MeshUtilities.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "AssetRegistryModule.h"
#include "IMeshReductionInterfaces.h"
#include "IMeshMergeUtilities.h"
#include "MeshMergeModule.h"

#define LOCTEXT_NAMESPACE "MeshProxyTool"


FMeshProxyTool::FMeshProxyTool()
{
	SettingsObject = UMeshProxySettingsObject::Get();
}

TSharedRef<SWidget> FMeshProxyTool::GetWidget()
{
	SAssignNew(ProxyDialog, SMeshProxyDialog, this);
	return ProxyDialog.ToSharedRef();
}

FText  FMeshProxyTool::GetTooltipText() const
{
	return LOCTEXT("MeshProxyToolTooltip", "Harvest geometry from selected actors and merge them into single mesh.");
}


FString FMeshProxyTool::GetDefaultPackageName() const
{
	FString PackageName;

	USelection* SelectedActors = GEditor->GetSelectedActors();

	// Iterate through selected actors and find first static mesh asset
	// Use this static mesh path as destination package name for a merged mesh
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
		{
			TInlineComponentArray<UStaticMeshComponent*> SMComponets;
			Actor->GetComponents<UStaticMeshComponent>(SMComponets);
			for (UStaticMeshComponent* Component : SMComponets)
			{
				if (Component->GetStaticMesh())
				{
					PackageName = FPackageName::GetLongPackagePath(Component->GetStaticMesh()->GetOutermost()->GetName());
					PackageName += FString(TEXT("/PROXY_")) + Component->GetStaticMesh()->GetName();
					break;
				}
			}
		}

		if (!PackageName.IsEmpty())
		{
			break;
		}
	}

	if (PackageName.IsEmpty())
	{
		PackageName = FPackageName::FilenameToLongPackageName(FPaths::ProjectContentDir() + TEXT("PROXY"));
		PackageName = MakeUniqueObjectName(NULL, UPackage::StaticClass(), *PackageName).ToString();
	}

	return PackageName;
}


bool FMeshProxyTool::RunMerge(const FString& PackageName)
{
	TArray<AActor*> Actors;
	TArray<UObject*> AssetsToSync;

	// Get the module for the mesh merge utilities
	const IMeshMergeUtilities& MeshMergeUtilities = FModuleManager::Get().LoadModuleChecked<IMeshMergeModule>("MeshMergeUtilities").GetUtilities();

	USelection* SelectedActors = GEditor->GetSelectedActors();
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
		{
			Actors.Add(Actor);
		}
	}

	if (Actors.Num())
	{
		GWarn->BeginSlowTask(LOCTEXT("MeshProxy_CreatingProxy", "Creating Mesh Proxy"), true);
		GEditor->BeginTransaction(LOCTEXT("MeshProxy_Create", "Creating Mesh Proxy"));

		FVector ProxyLocation = FVector::ZeroVector;

		FCreateProxyDelegate ProxyDelegate;
		ProxyDelegate.BindLambda(
			[](const FGuid Guid, TArray<UObject*>& InAssetsToSync)
		{
			//Update the asset registry that a new static mash and material has been created
			if (InAssetsToSync.Num())
			{
				FAssetRegistryModule& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
				int32 AssetCount = InAssetsToSync.Num();
				for (int32 AssetIndex = 0; AssetIndex < AssetCount; AssetIndex++)
				{
					AssetRegistry.AssetCreated(InAssetsToSync[AssetIndex]);
					GEditor->BroadcastObjectReimported(InAssetsToSync[AssetIndex]);
				}

				//Also notify the content browser that the new assets exists
				FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				ContentBrowserModule.Get().SyncBrowserToAssets(InAssetsToSync, true);
			}
		});

		// Extracting static mesh components from the selected mesh components in the dialog
		const TArray<TSharedPtr<FMergeComponentData>>& SelectedComponents = ProxyDialog->GetSelectedComponents();
		TArray<UStaticMeshComponent*> StaticMeshComponentsToMerge;

		for (const TSharedPtr<FMergeComponentData>& SelectedComponent : SelectedComponents)
		{
			// Determine whether or not this component should be incorporated according the user settings
			if (SelectedComponent->bShouldIncorporate)
			{
				if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(SelectedComponent->PrimComponent.Get()))
					StaticMeshComponentsToMerge.Add(StaticMeshComponent);
			}
		}
		StaticMeshComponentsToMerge.RemoveAll([](UStaticMeshComponent* Val) { return !((Val->GetClass() == UStaticMeshComponent::StaticClass() && Val->GetStaticMesh()) || Val->IsA(USplineMeshComponent::StaticClass())); });
		
		FGuid JobGuid = FGuid::NewGuid();
		MeshMergeUtilities.CreateProxyMesh(StaticMeshComponentsToMerge, SettingsObject->Settings, nullptr, PackageName, JobGuid, ProxyDelegate);

		GEditor->EndTransaction();
		GWarn->EndSlowTask();
	}

	return true;
}

bool FMeshProxyTool::CanMerge() const
{
	return ProxyDialog->GetNumSelectedMeshComponents() >= 1;
}


FText FThirdPartyMeshProxyTool::GetTooltipText() const
{
	return LOCTEXT("MeshProxyToolTooltip", "Harvest geometry selected meshes and merge and and simplify them as a single mesh.");
}


FString FThirdPartyMeshProxyTool::GetDefaultPackageName() const
{
	FString PackageName;

	USelection* SelectedActors = GEditor->GetSelectedActors();

	// Iterate through selected actors and find first static mesh asset
	// Use this static mesh path as destination package name for a merged mesh
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
		{
			TInlineComponentArray<UStaticMeshComponent*> SMComponets;
			Actor->GetComponents<UStaticMeshComponent>(SMComponets);
			for (UStaticMeshComponent* Component : SMComponets)
			{
				if (Component->GetStaticMesh())
				{
					PackageName = FPackageName::GetLongPackagePath(Component->GetStaticMesh()->GetOutermost()->GetName());
					PackageName += FString(TEXT("/PROXY_")) + Component->GetStaticMesh()->GetName();
					break;
				}
			}
		}

		if (!PackageName.IsEmpty())
		{
			break;
		}
	}

	if (PackageName.IsEmpty())
	{
		PackageName = FPackageName::FilenameToLongPackageName(FPaths::ProjectContentDir() + TEXT("PROXY"));
		PackageName = MakeUniqueObjectName(NULL, UPackage::StaticClass(), *PackageName).ToString();
	}

	return PackageName;
}


bool FThirdPartyMeshProxyTool::RunMerge(const FString& PackageName)
{
	TArray<AActor*> Actors;
	TArray<UObject*> AssetsToSync;

	// Get the module for the mesh merge utilities
	const IMeshMergeUtilities& MeshMergeUtilities = FModuleManager::Get().LoadModuleChecked<IMeshMergeModule>("MeshMergeUtilities").GetUtilities();

	USelection* SelectedActors = GEditor->GetSelectedActors();
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
		{
			Actors.Add(Actor);
		}
	}

	if (Actors.Num())
	{
		GWarn->BeginSlowTask(LOCTEXT("MeshProxy_CreatingProxy", "Creating Mesh Proxy"), true);
		GEditor->BeginTransaction(LOCTEXT("MeshProxy_Create", "Creating Mesh Proxy"));

		FVector ProxyLocation = FVector::ZeroVector;
		
		FCreateProxyDelegate ProxyDelegate;
		ProxyDelegate.BindLambda(
			[](const FGuid Guid, TArray<UObject*>& InAssetsToSync)
		{
			//Update the asset registry that a new static mash and material has been created
			if (InAssetsToSync.Num())
			{
				FAssetRegistryModule& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
				int32 AssetCount = InAssetsToSync.Num();
				for (int32 AssetIndex = 0; AssetIndex < AssetCount; AssetIndex++)
				{
					AssetRegistry.AssetCreated(InAssetsToSync[AssetIndex]);
					GEditor->BroadcastObjectReimported(InAssetsToSync[AssetIndex]);
				}

				//Also notify the content browser that the new assets exists
				FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				ContentBrowserModule.Get().SyncBrowserToAssets(InAssetsToSync, true);
			}
		});		

		FGuid JobGuid = FGuid::NewGuid();
		MeshMergeUtilities.CreateProxyMesh(Actors, ProxySettings, nullptr, PackageName, JobGuid, ProxyDelegate);	

		GEditor->EndTransaction();
		GWarn->EndSlowTask();
	}

	return true;
}

bool FThirdPartyMeshProxyTool::CanMerge() const
{
	return true;
}

TSharedRef<SWidget> FThirdPartyMeshProxyTool::GetWidget()
{
	return SNew(SThirdPartyMeshProxyDialog, this);
}

#undef LOCTEXT_NAMESPACE
