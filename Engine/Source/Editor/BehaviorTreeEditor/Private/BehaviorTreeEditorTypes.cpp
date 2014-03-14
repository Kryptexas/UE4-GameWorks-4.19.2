// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "Toolkits/AssetEditorManager.h"
#include "PackageTools.h"
#include "MessageLog.h"
#include "AssetRegistryModule.h"

const FString UBehaviorTreeEditorTypes::PinCategory_MultipleNodes("MultipleNodes");
const FString UBehaviorTreeEditorTypes::PinCategory_SingleComposite("SingleComposite");
const FString UBehaviorTreeEditorTypes::PinCategory_SingleTask("SingleTask");
const FString UBehaviorTreeEditorTypes::PinCategory_SingleNode("SingleNode");

#define LOCTEXT_NAMESPACE "SClassViewer"

FString FClassData::ToString() const
{
	UClass* MyClass = Class.Get();
	if (MyClass)
	{
		FString ClassDesc = MyClass->GetName();

		if (MyClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
		{
			return ClassDesc.LeftChop(2);
		}

		const int32 ShortNameIdx = ClassDesc.Find(TEXT("_"));
		if (ShortNameIdx != INDEX_NONE)
		{
			ClassDesc = ClassDesc.Mid(ShortNameIdx + 1);
		}

		return ClassDesc;
	}

	return AssetName;
}

FString FClassData::GetClassName() const
{
	return Class.IsValid() ? Class->GetName() : ClassName;
}

bool FClassData::IsAbstract() const
{
	return Class.IsValid() ? Class.Get()->HasAnyClassFlags(CLASS_Abstract) : false;
}

UClass* FClassData::GetClass()
{
	UClass* RetClass = Class.Get();
	if (RetClass == NULL && GeneratedClassPackage.Len())
	{
		GWarn->BeginSlowTask(LOCTEXT("LoadPackage", "Loading Package..."), true);

		UPackage* Package = LoadPackage(NULL, *GeneratedClassPackage, LOAD_NoRedirects);
		if (Package)
		{
			Package->FullyLoad();

			UObject* Object = FindObject<UObject>(Package, *AssetName);

			GWarn->EndSlowTask();

			UBlueprint* BlueprintOb = Cast<UBlueprint>(Object);
			RetClass = BlueprintOb ? *BlueprintOb->GeneratedClass : Object->GetClass();
			Class = RetClass;
		}
		else
		{
			GWarn->EndSlowTask();

			FMessageLog EditorErrors("EditorErrors");
			EditorErrors.Error(LOCTEXT("PackageLoadFail", "Package Load Failed"));
			EditorErrors.Info(FText::FromString(GeneratedClassPackage));
			EditorErrors.Notify(LOCTEXT("PackageLoadFail", "Package Load Failed"));
		}
	}

	return RetClass;
}

//////////////////////////////////////////////////////////////////////////

FClassBrowseHelper::FClassBrowseHelper()
{
	// Register with the Asset Registry to be informed when it is done loading up files.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().OnFilesLoaded().AddRaw( this, &FClassBrowseHelper::InvalidateCache );
	AssetRegistryModule.Get().OnAssetAdded().AddRaw( this, &FClassBrowseHelper::OnAssetAdded);
	AssetRegistryModule.Get().OnAssetRemoved().AddRaw( this, &FClassBrowseHelper::OnAssetRemoved );

	// Register to have Populate called when doing a Hot Reload or when a Blueprint is compiled.
	GEditor->OnHotReload().AddRaw( this, &FClassBrowseHelper::InvalidateCache );
	GEditor->OnBlueprintCompiled().AddRaw( this, &FClassBrowseHelper::InvalidateCache );
	GEditor->OnClassPackageLoadedOrUnloaded().AddRaw( this, &FClassBrowseHelper::InvalidateCache );
}

FClassBrowseHelper::~FClassBrowseHelper()
{
	// Unregister with the Asset Registry to be informed when it is done loading up files.
	if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().OnFilesLoaded().RemoveAll(this);
		AssetRegistryModule.Get().OnAssetAdded().RemoveAll(this);
		AssetRegistryModule.Get().OnAssetRemoved().RemoveAll(this);

		// Unregister to have Populate called when doing a Hot Reload or when a Blueprint is compiled.
		GEditor->OnHotReload().RemoveAll(this);
		GEditor->OnBlueprintCompiled().RemoveAll(this);
		GEditor->OnClassPackageLoadedOrUnloaded().RemoveAll(this);
	}
}

void FClassBrowseHelper::GatherClasses(const UClass* BaseClass, TArray<FClassData>& AvailableClasses)
{
	FBehaviorTreeEditorModule& BTEditorModule = FModuleManager::GetModuleChecked<FBehaviorTreeEditorModule>(TEXT("BehaviorTreeEditor"));
	FClassBrowseHelper* HelperInstance = BTEditorModule.GetClassCache().Get();
	if (HelperInstance)
	{
		const FString BaseClassName = BaseClass->GetName();
		if (!HelperInstance->RootNode.IsValid())
		{
			HelperInstance->BuildClassGraph();
		}

		TSharedPtr<FClassDataNode> BaseNode = HelperInstance->FindBaseClassNode(HelperInstance->RootNode, BaseClassName);
		HelperInstance->FindAllSubClasses(BaseNode, AvailableClasses);
	}
}

void FClassBrowseHelper::OnAssetAdded(const class FAssetData& AssetData)
{
	TSharedPtr<FClassDataNode> Node = CreateClassDataNode(AssetData);
	
	TSharedPtr<FClassDataNode> ParentNode;
	if (Node.IsValid())
	{
		ParentNode = FindBaseClassNode(RootNode, Node->ParentClassName);
	}

	if (ParentNode.IsValid())
	{
		ParentNode->SubNodes.Add(Node);
		Node->ParentNode = ParentNode;
	}
}

void FClassBrowseHelper::OnAssetRemoved(const class FAssetData& AssetData)
{
	const FString* GeneratedClassname = AssetData.TagsAndValues.Find("GeneratedClass");
	if (GeneratedClassname)
	{
		FString AssetClassName = *GeneratedClassname;
		UObject* Outer1(NULL);
		ResolveName(Outer1, AssetClassName, false, false);
		
		TSharedPtr<FClassDataNode> Node = FindBaseClassNode(RootNode, AssetClassName);
		if (Node.IsValid() && Node->ParentNode.IsValid())
		{
			Node->ParentNode->SubNodes.RemoveSingleSwap(Node);
		}
	}
}

void FClassBrowseHelper::InvalidateCache()
{
	RootNode.Reset();
}

TSharedPtr<FClassDataNode> FClassBrowseHelper::CreateClassDataNode(const class FAssetData& AssetData)
{
	TSharedPtr<FClassDataNode> Node;
	const FString* GeneratedClassname = AssetData.TagsAndValues.Find("GeneratedClass");
	const FString* ParentClassname = AssetData.TagsAndValues.Find("ParentClass");

	if (GeneratedClassname && ParentClassname)
	{
		FString AssetClassName = *GeneratedClassname;
		UObject* Outer1(NULL);
		ResolveName(Outer1, AssetClassName, false, false);

		FString AssetParentClassName = *ParentClassname;
		UObject* Outer2(NULL);
		ResolveName(Outer2, AssetParentClassName, false, false);

		Node = MakeShareable(new FClassDataNode);
		Node->ParentClassName = AssetParentClassName;

		FClassData NewData(AssetData.AssetName.ToString(), AssetData.PackageName.ToString(), AssetClassName);
		Node->Data = NewData;
	}

	return Node;
}

TSharedPtr<FClassDataNode> FClassBrowseHelper::FindBaseClassNode(TSharedPtr<FClassDataNode> Node, const FString& ClassName)
{
	TSharedPtr<FClassDataNode> RetNode;
	if (Node.IsValid())
	{
		if (Node->Data.GetClassName() == ClassName)
		{
			return Node;
		}

		for (int32 i = 0; i < Node->SubNodes.Num(); i++)
		{
			RetNode = FindBaseClassNode(Node->SubNodes[i], ClassName);
			if (RetNode.IsValid())
			{
				break;
			}
		}
	}

	return RetNode;
}

void FClassBrowseHelper::FindAllSubClasses(TSharedPtr<FClassDataNode> Node, TArray<FClassData>& AvailableClasses)
{
	if (Node.IsValid())
	{
		if (!Node->Data.IsAbstract())
		{
			AvailableClasses.Add(Node->Data);
		}

		for (int32 i = 0; i < Node->SubNodes.Num(); i++)
		{
			FindAllSubClasses(Node->SubNodes[i], AvailableClasses);
		}
	}
}

UClass* FClassBrowseHelper::FindAssetClass(const FString& GeneratedClassPackage, const FString& AssetName)
{
	UPackage* Package = FindPackage(NULL, *GeneratedClassPackage );
	if (Package)
	{
		UObject* Object = FindObject<UObject>(Package, *AssetName);
		if (Object)
		{
			UBlueprint* BlueprintOb = Cast<UBlueprint>(Object);
			return BlueprintOb ? *BlueprintOb->GeneratedClass : Object->GetClass();
		}
	}

	return NULL;
}

void FClassBrowseHelper::BuildClassGraph()
{
	UClass* RootNodeClass = UBTNode::StaticClass();

	TArray<TSharedPtr<FClassDataNode> > NodeList;
	RootNode.Reset();

	// gather all native classes
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* TestClass = *It;
		if (TestClass->HasAnyClassFlags(CLASS_Native) && TestClass->IsChildOf(RootNodeClass))
		{
			TSharedPtr<FClassDataNode> NewNode = MakeShareable(new FClassDataNode);
			NewNode->ParentClassName = TestClass->GetSuperClass()->GetName();
			
			FClassData NewData(TestClass);
			NewNode->Data = NewData;

			if (TestClass == RootNodeClass)
			{
				RootNode = NewNode;
			}

			NodeList.Add(NewNode);
		}
	}

	// gather all blueprints
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> BlueprintList;

	FARFilter Filter;
	Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
	AssetRegistryModule.Get().GetAssets(Filter, BlueprintList);

	for (int32 i = 0; i < BlueprintList.Num(); i++)
	{
		TSharedPtr<FClassDataNode> NewNode = CreateClassDataNode(BlueprintList[i]);
		NodeList.Add(NewNode);
	}

	// build class tree
	AddClassGraphChildren(RootNode, NodeList);
}

void FClassBrowseHelper::AddClassGraphChildren(TSharedPtr<FClassDataNode> Node, TArray<TSharedPtr<FClassDataNode> >& NodeList)
{
	if (!Node.IsValid())
	{
		return;
	}

	const FString NodeClassName = Node->Data.GetClassName();
	for (int32 i = NodeList.Num() - 1; i >= 0; i--)
	{
		if (NodeList[i]->ParentClassName == NodeClassName)
		{
			TSharedPtr<FClassDataNode> MatchingNode = NodeList[i];
			NodeList.RemoveAt(i);

			MatchingNode->ParentNode = Node;
			Node->SubNodes.Add(MatchingNode);

			AddClassGraphChildren(MatchingNode, NodeList);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

UBehaviorTreeEditorTypes::UBehaviorTreeEditorTypes(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

#undef LOCTEXT_NAMESPACE
