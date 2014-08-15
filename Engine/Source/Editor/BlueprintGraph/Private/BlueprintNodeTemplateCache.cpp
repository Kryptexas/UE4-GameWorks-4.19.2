// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintNodeTemplateCache.h"
#include "BlueprintEditorUtils.h"	// for FindFirstNativeClass(), FindBlueprintForGraph()
#include "EdGraph/EdGraph.h"
#include "KismetEditorUtilities.h"	// for CreateBlueprint()
#include "BlueprintNodeSpawner.h"   // for NodeClass/Invoke()

/*******************************************************************************
 * Static FBlueprintNodeTemplateCache Helpers
 ******************************************************************************/

namespace BlueprintNodeTemplateCacheImpl
{
	/**
	 * 
	 * 
	 * @param  NodeObj	
	 * @param  Graph	
	 * @return 
	 */
	static bool IsCompatible(UEdGraphNode* NodeObj, UEdGraph* Graph);

	/**
	 * 
	 * 
	 * @param  BlueprintOuter	
	 * @param  NodeObj	
	 * @return 
	 */
	static UEdGraph* FindCompatibleGraph(UBlueprint* BlueprintOuter, UEdGraphNode* NodeObj);

	/**
	 * 
	 * 
	 * @param  BlueprintClass	
	 * @param  ParentClass	
	 * @param  GeneratedClassType	
	 * @return 
	 */
	static UBlueprint* MakeCompatibleBlueprint(TSubclassOf<UBlueprint> BlueprintClass, UClass* ParentClass, TSubclassOf<UBlueprintGeneratedClass> GeneratedClassType);

	/**
	 * 
	 * 
	 * @param  GraphClass	
	 * @param  SchemaClass	
	 * @param  BlueprintOuter	
	 * @param  GraphType	
	 * @return 
	 */
	static UEdGraph* AddGraph(UBlueprint* BlueprintOuter, TSubclassOf<UEdGraphSchema> SchemaClass);
}

//------------------------------------------------------------------------------
static bool BlueprintNodeTemplateCacheImpl::IsCompatible(UEdGraphNode* NodeObj, UEdGraph* Graph)
{
	return NodeObj->CanCreateUnderSpecifiedSchema(Graph->GetSchema());
}

//------------------------------------------------------------------------------
static UEdGraph* BlueprintNodeTemplateCacheImpl::FindCompatibleGraph(UBlueprint* BlueprintOuter, UEdGraphNode* NodeObj)
{
	UEdGraph* FoundGraph = nullptr;

	TArray<UObject*> BlueprintChildObjs;
	GetObjectsWithOuter(BlueprintOuter, BlueprintChildObjs, /*bIncludeNestedObjects =*/false, /*ExclusionFlags =*/RF_PendingKill);

	for (UObject* Child : BlueprintChildObjs)
	{
		UEdGraph* ChildGraph = Cast<UEdGraph>(Child);
		if ((ChildGraph != nullptr) && IsCompatible(NodeObj, ChildGraph))
		{
			FoundGraph = ChildGraph;
			break;
		}
	}

	return FoundGraph;
}

//------------------------------------------------------------------------------
static UBlueprint* BlueprintNodeTemplateCacheImpl::MakeCompatibleBlueprint(TSubclassOf<UBlueprint> BlueprintClass, UClass* ParentClass, TSubclassOf<UBlueprintGeneratedClass> GeneratedClassType)
{
	EBlueprintType BlueprintType = BPTYPE_Normal;
	// @TODO: BPTYPE_LevelScript requires a level outer, which we don't want to have here... can we get away without it?
// 	if (BlueprintClass->IsChildOf<ULevelScriptBlueprint>())
// 	{
// 		BlueprintType = BPTYPE_LevelScript;
// 	}

	if (GeneratedClassType == nullptr)
	{
		GeneratedClassType = UBlueprintGeneratedClass::StaticClass();
	}

	UPackage* BlueprintOuter = GetTransientPackage();
	FString const DesiredName = FString::Printf(TEXT("PROTO_BP_%s"), *BlueprintClass->GetName());
	FName   const BlueprintName = MakeUniqueObjectName(BlueprintOuter, BlueprintClass, FName(*DesiredName));

	BlueprintClass = FBlueprintEditorUtils::FindFirstNativeClass(BlueprintClass);
	UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(ParentClass, BlueprintOuter, BlueprintName, BlueprintType, BlueprintClass, GeneratedClassType);
	NewBlueprint->SetFlags(RF_Transient);

	return NewBlueprint;
}

//------------------------------------------------------------------------------
static UEdGraph* BlueprintNodeTemplateCacheImpl::AddGraph(UBlueprint* BlueprintOuter, TSubclassOf<UEdGraphSchema> SchemaClass)
{
	UClass* GraphClass = UEdGraph::StaticClass();
	FName const GraphName = MakeUniqueObjectName(BlueprintOuter, GraphClass, FName(TEXT("TEMPLATE_NODE_OUTER")));

	UEdGraph* NewGraph = ConstructObject<UEdGraph>(GraphClass, BlueprintOuter, GraphName, RF_Transient);
	NewGraph->Schema = SchemaClass;

	return NewGraph;
}

//------------------------------------------------------------------------------
FBlueprintNodeTemplateCache::FBlueprintNodeTemplateCache()
{
	using namespace BlueprintNodeTemplateCacheImpl; // for MakeCompatibleBlueprint()

	TemplateOuters.Add(MakeCompatibleBlueprint(UBlueprint::StaticClass(), AActor::StaticClass(), UBlueprintGeneratedClass::StaticClass()));
	TemplateOuters.Add(MakeCompatibleBlueprint(UAnimBlueprint::StaticClass(), UAnimInstance::StaticClass(), UAnimBlueprintGeneratedClass::StaticClass()));
}

//------------------------------------------------------------------------------
UEdGraphNode* FBlueprintNodeTemplateCache::GetNodeTemplate(UBlueprintNodeSpawner const* NodeSpawner, UEdGraph* TargetGraph)
{
	using namespace BlueprintNodeTemplateCacheImpl;

	UEdGraphNode* TemplateNode = nullptr;
	if (UEdGraphNode** FoundNode = NodeTemplateCache.Find(NodeSpawner))
	{
		TemplateNode = *FoundNode;
	}
	else if (NodeSpawner->NodeClass != nullptr)
	{
		UEdGraphNode* NodeCDO = NodeSpawner->NodeClass->GetDefaultObject<UEdGraphNode>();
		check(NodeCDO != nullptr);

		UBlueprint* TargetBlueprint = nullptr;
		TSubclassOf<UBlueprint> BlueprintClass;

		bool const bHasTargetGraph = (TargetGraph != nullptr);
		if (bHasTargetGraph)
		{
			// by the time we're asking for a prototype for this spawner, we should
			// be sure that it is compatible with the TargetGraph
			check(IsCompatible(NodeCDO, TargetGraph));

			UBlueprint* TargetBlueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
			check(TargetBlueprint != nullptr);
			TSubclassOf<UBlueprint> BlueprintClass = TargetBlueprint->GetClass();
		}		

		UBlueprint* CompatibleBlueprint = nullptr;
		UEdGraph*   CompatibleOuter     = nullptr;
		// find a compatible outer (don't want to have to create a new one if we don't have to)
		for (UBlueprint* Blueprint : TemplateOuters)
		{
			CompatibleOuter = FindCompatibleGraph(Blueprint, NodeCDO);
			if (CompatibleOuter != nullptr)
			{
				CompatibleBlueprint = Blueprint;
				break;
			}
			else if ((BlueprintClass != nullptr) && Blueprint->GetClass()->IsChildOf(BlueprintClass))
			{
				CompatibleBlueprint = Blueprint;
			}
		}

		// if a TargetGraph was supplied, and we couldn't find a suitable outer
		// for this template-node, then attempt to emulate that graph
		if (bHasTargetGraph)
		{
			if (CompatibleBlueprint == nullptr)
			{
				TSubclassOf<UBlueprintGeneratedClass> GeneratedClassType = UBlueprintGeneratedClass::StaticClass();
				if (TargetBlueprint->GeneratedClass != nullptr)
				{
					GeneratedClassType = TargetBlueprint->GeneratedClass->GetClass();
				}

				CompatibleBlueprint = MakeCompatibleBlueprint(BlueprintClass, TargetBlueprint->ParentClass, GeneratedClassType);
				TemplateOuters.Add(CompatibleBlueprint);

				// this graph may come default with a compatible graph
				CompatibleOuter = FindCompatibleGraph(CompatibleBlueprint, NodeCDO);
			}

			if (CompatibleOuter == nullptr)
			{
				CompatibleOuter = AddGraph(CompatibleBlueprint, TargetGraph->Schema);
			}
		}

		if (CompatibleOuter != nullptr)
		{
			TemplateNode = NodeSpawner->Invoke(CompatibleOuter, FVector2D::ZeroVector);
			NodeTemplateCache.Add(NodeSpawner, TemplateNode);
		}
	}

	return TemplateNode;
}

//------------------------------------------------------------------------------
void FBlueprintNodeTemplateCache::ClearCachedTemplate(UBlueprintNodeSpawner const* NodeSpawner)
{
	NodeTemplateCache.Remove(NodeSpawner); // GC should take care of the rest
}

//------------------------------------------------------------------------------
void FBlueprintNodeTemplateCache::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (auto& TemplateEntry : NodeTemplateCache)
	{
		Collector.AddReferencedObject(TemplateEntry.Value);
	}
	Collector.AddReferencedObjects(TemplateOuters);
}
