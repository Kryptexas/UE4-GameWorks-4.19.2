// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintNodeTemplateCache.h"
#include "BlueprintEditorUtils.h"	// for FindFirstNativeClass(), FindBlueprintForGraph()
#include "EdGraph/EdGraph.h"
#include "KismetEditorUtilities.h"	// for CreateBlueprint()
#include "BlueprintNodeSpawner.h"   // for NodeClass/Invoke()
#include "BlueprintEditorSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintNodeCache, Log, All);

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

	/**
	 * 
	 * 
	 * @param  Object	
	 * @return 
	 */
	static int32 ApproximateMemFootprint(UObject* Object);

	/**
	 * 
	 * 
	 * @return 
	 */
	static int32 GetCacheCapSize();
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
static int32 BlueprintNodeTemplateCacheImpl::ApproximateMemFootprint(UObject* Object)
{
	TArray<UObject*> ChildObjs;
	GetObjectsWithOuter(Object, ChildObjs);

	int32 ApproimateDataSize = sizeof(*Object);
	for (UObject* ChildObj : ChildObjs)
	{
		// @TODO: doesn't account for any internal allocated memory (for member TArrays, etc.)
		ApproimateDataSize += sizeof(*ChildObj);
	}
	return ApproimateDataSize;
}

//------------------------------------------------------------------------------
static int32 BlueprintNodeTemplateCacheImpl::GetCacheCapSize()
{
	UBlueprintEditorSettings const* BpSettings = GetDefault<UBlueprintEditorSettings>();
	// have to convert from MB to bytes
	return (BpSettings->NodeTemplateCacheCapMB * 1024.f * 1024.f);
}

//------------------------------------------------------------------------------
FBlueprintNodeTemplateCache::FBlueprintNodeTemplateCache()
	: ApproximateAllocationTotal(0)
{
	using namespace BlueprintNodeTemplateCacheImpl; // for MakeCompatibleBlueprint()

	UBlueprint* StandardBlueprint = MakeCompatibleBlueprint(UBlueprint::StaticClass(), AActor::StaticClass(), UBlueprintGeneratedClass::StaticClass());
	AddBlueprintOuter(StandardBlueprint);

	UBlueprint* AnimBlueprint = MakeCompatibleBlueprint(UAnimBlueprint::StaticClass(), UAnimInstance::StaticClass(), UAnimBlueprintGeneratedClass::StaticClass());
	AddBlueprintOuter(AnimBlueprint);
}

//------------------------------------------------------------------------------
UEdGraphNode* FBlueprintNodeTemplateCache::GetNodeTemplate(UBlueprintNodeSpawner const* NodeSpawner, UEdGraph* TargetGraph)
{
	using namespace BlueprintNodeTemplateCacheImpl;
	static bool LoggedOnce = false;

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
			//check(IsCompatible(NodeCDO, TargetGraph));

			TargetBlueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
			check(TargetBlueprint != nullptr);
			BlueprintClass = TargetBlueprint->GetClass();

			TargetGraph = FindCompatibleGraph(TargetBlueprint, NodeCDO);
			check(TargetGraph != nullptr);
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

				// @TODO: if we're out of space, we shouldn't be 
				CompatibleBlueprint = MakeCompatibleBlueprint(BlueprintClass, TargetBlueprint->ParentClass, GeneratedClassType);
				if (!AddBlueprintOuter(CompatibleBlueprint) && !LoggedOnce)
				{
					UE_LOG(LogBlueprintNodeCache, Display, TEXT("The blueprint template-node cache is full. As a result, you may experience slower than usual menu interactions. To avoid this, increase the cache's cap in the blueprint editor prefences."));
					LoggedOnce = true;
				}

				// this graph may come default with a compatible graph
				CompatibleOuter = FindCompatibleGraph(CompatibleBlueprint, NodeCDO);
			}

			if (CompatibleOuter == nullptr)
			{
				CompatibleOuter = AddGraph(CompatibleBlueprint, TargetGraph->Schema);
				ApproximateAllocationTotal += ApproximateMemFootprint(CompatibleOuter);
			}
		}

		if (CompatibleOuter != nullptr)
		{
			TemplateNode = NodeSpawner->Invoke(CompatibleOuter, IBlueprintNodeBinder::FBindingSet(), FVector2D::ZeroVector);
			if (!CacheTemplateNode(NodeSpawner, TemplateNode) && !LoggedOnce)
			{
				UE_LOG(LogBlueprintNodeCache, Display, TEXT("The blueprint template-node cache is full. As a result, you may experience slower than usual menu interactions. To avoid this, increase the cache's cap in the blueprint editor prefences."));
				LoggedOnce = true;
			}
		}
	}

	return TemplateNode;
}

//------------------------------------------------------------------------------
UEdGraphNode* FBlueprintNodeTemplateCache::GetNodeTemplate(UBlueprintNodeSpawner const* NodeSpawner, ENoInit) const
{
	UEdGraphNode* TemplateNode = nullptr;
	if (UEdGraphNode* const* FoundNode = NodeTemplateCache.Find(NodeSpawner))
	{
		return *FoundNode;
	}
	return nullptr;
}

//------------------------------------------------------------------------------
void FBlueprintNodeTemplateCache::ClearCachedTemplate(UBlueprintNodeSpawner const* NodeSpawner)
{
	NodeTemplateCache.Remove(NodeSpawner); // GC should take care of the rest
}

//------------------------------------------------------------------------------
int32 FBlueprintNodeTemplateCache::EstimateAllocatedSize() const
{
	int32 TotalEstimatedSize = NodeTemplateCache.GetAllocatedSize();
	TotalEstimatedSize += TemplateOuters.GetAllocatedSize();
	TotalEstimatedSize += ApproximateAllocationTotal;

	return TotalEstimatedSize;
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

//------------------------------------------------------------------------------
bool FBlueprintNodeTemplateCache::AddBlueprintOuter(UBlueprint* Blueprint)
{
	int32 const ApproxBlueprintSize = BlueprintNodeTemplateCacheImpl::ApproximateMemFootprint(Blueprint);
	ApproximateAllocationTotal += ApproxBlueprintSize;
	int32 const NewEstimatedSize = EstimateAllocatedSize();

	if (NewEstimatedSize > BlueprintNodeTemplateCacheImpl::GetCacheCapSize())
	{
		ApproximateAllocationTotal -= ApproxBlueprintSize;
		return false;
	}
	else
	{
		TemplateOuters.Add(Blueprint);
		return true;
	}
}

//------------------------------------------------------------------------------
bool FBlueprintNodeTemplateCache::CacheTemplateNode(UBlueprintNodeSpawner const* NodeSpawner, UEdGraphNode* NewNode)
{
	if (NewNode == nullptr)
	{
		return true;
	}

	int32 const ApproxNodeSize = BlueprintNodeTemplateCacheImpl::ApproximateMemFootprint(NewNode);
	ApproximateAllocationTotal += ApproxNodeSize;
	int32 const NewEstimatedSize = EstimateAllocatedSize();

	if (NewEstimatedSize > BlueprintNodeTemplateCacheImpl::GetCacheCapSize())
	{
		ApproximateAllocationTotal -= ApproxNodeSize;
		return false;
	}
	else
	{
		NodeTemplateCache.Add(NodeSpawner, NewNode);
		return true;
	}
}
