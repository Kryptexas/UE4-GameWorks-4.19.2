// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "HierarchicalRig.h"
#include "Engine/SkeletalMesh.h"
#include "AnimationRuntime.h"
#include "AnimationCoreLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

#define LOCTEXT_NAMESPACE "HierarchicalRig"

UHierarchicalRig::UHierarchicalRig()
{
}

#if WITH_EDITOR
void UHierarchicalRig::SetConstraints(const TArray<FTransformConstraint>& InConstraints)
{
	Constraints = InConstraints;
}

void UHierarchicalRig::BuildHierarchyFromSkeletalMesh(USkeletalMesh* SkeletalMesh)
{
	const TArray<FMeshBoneInfo>& MeshBoneInfos = SkeletalMesh->RefSkeleton.GetRawRefBoneInfo();
	const TArray<FTransform>& LocalTransforms = SkeletalMesh->RefSkeleton.GetRefBonePose();
	check(MeshBoneInfos.Num() == LocalTransforms.Num());
	
	// clear hierarchy - is this necessary? Maybe not, but it makes it simpler for not handling duplicated names and so on
	Hierarchy.Empty();

	TArray<FTransform> GlobalTransforms;
	FAnimationRuntime::FillUpComponentSpaceTransforms(SkeletalMesh->RefSkeleton, LocalTransforms, GlobalTransforms);

	const int32 BoneCount = MeshBoneInfos.Num();
	for (int32 BoneIndex = 0; BoneIndex < BoneCount; ++BoneIndex)
	{
		const FMeshBoneInfo& MeshBoneInfo = MeshBoneInfos[BoneIndex];
		const FTransform& LocalTransform = LocalTransforms[BoneIndex];
		const FTransform& GlobalTransform = GlobalTransforms[BoneIndex];
		FName ParentName;

		FConstraintNodeData NodeData;
		NodeData.BoneReference.BoneName = MeshBoneInfo.Name;
		if (MeshBoneInfo.ParentIndex != INDEX_NONE)
		{
			ParentName = MeshBoneInfos[MeshBoneInfo.ParentIndex].Name;
			const FTransform& GlobalParentTransform = GlobalTransforms[MeshBoneInfo.ParentIndex];
			NodeData.RelativeParent = GlobalTransform.GetRelativeTransform(GlobalParentTransform);
		}

		Hierarchy.Add(MeshBoneInfo.Name, ParentName, GlobalTransform, NodeData);
	}
}
#endif // WITH_EDITOR

FTransform UHierarchicalRig::GetLocalTransform(FName NodeName) const
{
	return Hierarchy.GetLocalTransformByName(NodeName);
}

FVector UHierarchicalRig::GetLocalLocation(FName NodeName) const
{
	return Hierarchy.GetLocalTransformByName(NodeName).GetLocation();
}

FRotator UHierarchicalRig::GetLocalRotation(FName NodeName) const
{
	return Hierarchy.GetLocalTransformByName(NodeName).GetRotation().Rotator();
}


FVector UHierarchicalRig::GetLocalScale(FName NodeName) const
{
	return Hierarchy.GetLocalTransformByName(NodeName).GetScale3D();
}

FTransform UHierarchicalRig::GetGlobalTransform(FName NodeName) const
{
	return Hierarchy.GetGlobalTransformByName(NodeName);
}

FTransform UHierarchicalRig::GetMappedGlobalTransform(FName NodeName) const
{
	FTransform GlobalTransform = GetGlobalTransform(NodeName);
	ApplyMappingTransform(NodeName, GlobalTransform);

	return GlobalTransform;
}

FTransform UHierarchicalRig::GetMappedLocalTransform(FName NodeName) const
{
	// should recalculate since mapping is happening in component space
	const FAnimationHierarchy& CacheHiearchy = GetHierarchy();
	int32 NodeIndex = CacheHiearchy.GetNodeIndex(NodeName);
	if (CacheHiearchy.IsValidIndex(NodeIndex))
	{
		FTransform GlobalTransform = CacheHiearchy.GetGlobalTransform(NodeIndex);
		ApplyMappingTransform(NodeName, GlobalTransform);

		int32 ParentIndex  = CacheHiearchy.GetParentIndex(NodeIndex);
		if (CacheHiearchy.IsValidIndex(ParentIndex))
		{
			FTransform ParentGlobalTransform = CacheHiearchy.GetGlobalTransform(ParentIndex);
			ApplyMappingTransform(CacheHiearchy.GetNodeName(ParentIndex), ParentGlobalTransform);
			GlobalTransform = GlobalTransform.GetRelativeTransform(ParentGlobalTransform);
		}

		return GlobalTransform;
	}

	return FTransform::Identity;
}

FVector UHierarchicalRig::GetGlobalLocation(FName NodeName) const
{
	return Hierarchy.GetGlobalTransformByName(NodeName).GetLocation();
}

FRotator UHierarchicalRig::GetGlobalRotation(FName NodeName) const
{
	return Hierarchy.GetGlobalTransformByName(NodeName).GetRotation().Rotator();
}

FVector UHierarchicalRig::GetGlobalScale(FName NodeName) const
{
	return Hierarchy.GetGlobalTransformByName(NodeName).GetScale3D();
}

void UHierarchicalRig::SetLocalTransform(FName NodeName, const FTransform& Transform)
{
	Hierarchy.SetLocalTransformByName(NodeName, Transform);
}

void UHierarchicalRig::SetGlobalTransform(FName NodeName, const FTransform& Transform)
{
	FTransform NewTransform = Transform;
	Hierarchy.SetGlobalTransformByName(NodeName, NewTransform);
	// we still have to call evaluate node to update all hierarchy and so on
	EvaluateNode(NodeName);
}

void UHierarchicalRig::SetMappedGlobalTransform(FName NodeName, const FTransform& Transform)
{
	FTransform NewTransform = Transform;
	ApplyInverseMappingTransform(NodeName, NewTransform);
	SetGlobalTransform(NodeName, NewTransform);
}

void UHierarchicalRig::SetMappedLocalTransform(FName NodeName, const FTransform& Transform)
{
	// should recalculate since mapping is happening in component space
	const FAnimationHierarchy& CacheHiearchy = GetHierarchy();
	int32 NodeIndex = CacheHiearchy.GetNodeIndex(NodeName);
	if (CacheHiearchy.IsValidIndex(NodeIndex))
	{
		FTransform GlobalTransform;
		int32 ParentIndex = CacheHiearchy.GetParentIndex(NodeIndex);
		if (CacheHiearchy.IsValidIndex(ParentIndex))
		{
			FTransform ParentGlobalTransform = CacheHiearchy.GetGlobalTransform(ParentIndex);
			ApplyMappingTransform(CacheHiearchy.GetNodeName(ParentIndex), ParentGlobalTransform);
			// have to apply to mapped transform
			GlobalTransform = Transform * ParentGlobalTransform;
		}
		else
		{
			GlobalTransform = Transform;
		}

		// inverse mapping transform
		ApplyInverseMappingTransform(NodeName, GlobalTransform);
		SetGlobalTransform(NodeName, GlobalTransform);
	}
}

void UHierarchicalRig::ApplyMappingTransform(FName NodeName, FTransform& InOutTransform) const
{
	if (NodeMappingContainer)
	{
		const FNodeMap* NodeMapping = NodeMappingContainer->GetNodeMapping(NodeName);
		if (NodeMapping)
		{
			InOutTransform = NodeMapping->SourceToTargetTransform * InOutTransform;
		}
		else
		{
			// get node data
			// @todo: do it here or create mapping for manually. Creating mapping will be more efficient
			const int32 Index = Hierarchy.GetNodeIndex(NodeName);
			if (Index != INDEX_NONE)
			{
				const FConstraintNodeData* UserData = static_cast<const FConstraintNodeData*> (Hierarchy.GetUserDataImpl(Index));
				if (UserData->LinkedNode != NAME_None)
				{
					NodeMapping = NodeMappingContainer->GetNodeMapping(UserData->LinkedNode);
					if (NodeMapping)
					{
						InOutTransform = NodeMapping->SourceToTargetTransform * InOutTransform;
					}
				}
			}
		}
	}
}

void UHierarchicalRig::ApplyInverseMappingTransform(FName NodeName, FTransform& InOutTransform) const
{
	if (NodeMappingContainer)
	{
		const FNodeMap* NodeMapping = NodeMappingContainer->GetNodeMapping(NodeName);
		if (NodeMapping)
		{
			InOutTransform = NodeMapping->SourceToTargetTransform.GetRelativeTransformReverse(InOutTransform);
		}
		else
		{
			// get node data
			// @todo: do it here or create mapping for manually. Creating mapping will be more efficient
			const int32 Index = Hierarchy.GetNodeIndex(NodeName);
			if (Index != INDEX_NONE)
			{
				const FConstraintNodeData* UserData = static_cast<const FConstraintNodeData*> (Hierarchy.GetUserDataImpl(Index));
				if (UserData->LinkedNode != NAME_None)
				{
					NodeMapping = NodeMappingContainer->GetNodeMapping(UserData->LinkedNode);
					if (NodeMapping)
					{
						InOutTransform = NodeMapping->SourceToTargetTransform.GetRelativeTransformReverse(InOutTransform);
					}
				}
			}
		}
	}
}

#if WITH_EDITOR
FText UHierarchicalRig::GetCategory() const
{
	return LOCTEXT("HierarchicalRigCategory", "Animation|ControlRigs");
}

FText UHierarchicalRig::GetTooltipText() const
{
	return LOCTEXT("HierarchicalRigTooltip", "Handles hierarchical (node based) data, constraints etc.");
}
#endif

void UHierarchicalRig::GetTickDependencies(TArray<FTickPrerequisite, TInlineAllocator<1>>& OutTickPrerequisites)
{
	if (SkeletalMeshComponent.IsValid())
	{
		OutTickPrerequisites.Add(FTickPrerequisite(SkeletalMeshComponent.Get(), SkeletalMeshComponent->PrimaryComponentTick));
	}
}

void UHierarchicalRig::Initialize()
{
	Super::Initialize();

	// Initialize any manipulators we have
	for (UControlManipulator* Manipulator : Manipulators)
	{
		if (Manipulator)
		{
			Manipulator->Initialize(this);
			if (Hierarchy.Contains(Manipulator->Name))
			{
				if (Manipulator->bInLocalSpace)
				{
					// do not add node in initialize, that is only for editor purpose and to serialize
					Manipulator->SetTransform(GetMappedLocalTransform(Manipulator->Name), this);
				}
				else
				{
					// do not add node in initialize, that is only for editor purpose and to serialize
					Manipulator->SetTransform(GetMappedGlobalTransform(Manipulator->Name), this);
				}
			}
		}
	}

	// @todo: I don't think this would work because we might not have all nodes we need here
	// @fixme: we need post initialize, but also we have to think about what serialize and what not
	// @fixme: constraints have to be fixed later
// 	if (Constraints.Num() > 0)
// 	{
// 		for (const FTransformConstraint& Constraint : Constraints)
// 		{
// 			AddConstraint(Constraint);
// 		}
// 
// //		CreateSortedNodes();
// 	}

	Sort();
}

AActor* UHierarchicalRig::GetHostingActor() const
{
	if (SkeletalMeshComponent.Get())
	{
		return SkeletalMeshComponent->GetOwner();
	}

	return Super::GetHostingActor();
}

void UHierarchicalRig::BindToObject(UObject* InObject)
{
	// check we aren't already bound
	check(SkeletalMeshComponent.Get() == nullptr);

	// If we are binding to an actor, find the first skeletal mesh component
	if (AActor* Actor = Cast<AActor>(InObject))
	{
		if (USkeletalMeshComponent* Component = Actor->FindComponentByClass<USkeletalMeshComponent>())
		{
			SkeletalMeshComponent = Component;
		}
	}
	else if (USkeletalMeshComponent* Component = Cast<USkeletalMeshComponent>(InObject))
	{
		SkeletalMeshComponent = Component;
	}
}

void UHierarchicalRig::UnbindFromObject()
{
	SkeletalMeshComponent = nullptr;
}

UObject* UHierarchicalRig::GetBoundObject() const
{
	return SkeletalMeshComponent.Get();
}

void UHierarchicalRig::PreEvaluate()
{
	Super::PreEvaluate();

	// @todo: sequencer will need this - 
	// Propagate manipulators to nodes
	for (UControlManipulator* Manipulator : Manipulators)
	{
		if (Manipulator)
		{
			if (Manipulator->bInLocalSpace)
			{
				SetMappedLocalTransform(Manipulator->Name, Manipulator->GetTransform(this));
			}
			else
			{
				SetMappedGlobalTransform(Manipulator->Name, Manipulator->GetTransform(this));
			}
		}
	}
}

void UHierarchicalRig::Evaluate()
{
	Super::Evaluate();

	// If we have no constraints then we dont need to do any of the heavy work here
	if (Constraints.Num() > 0)
	{
		// Calculate each node
		for (const FName& NodeName : SortedNodes)
		{
			const FTransform& GlobalTransfrom = GetGlobalTransform(NodeName);
			SetGlobalTransform(NodeName, GlobalTransfrom);
		}
	}
}

void UHierarchicalRig::PostEvaluate()
{
	Super::PostEvaluate();

	UpdateManipulatorToNode();
}

void UHierarchicalRig::UpdateManipulatorToNode()
{
	// Propagate back to manipulators after evaluation
	for (UControlManipulator* Manipulator : Manipulators)
	{
		if (Manipulator)
		{
			if (Manipulator->bInLocalSpace)
			{
				// do not add node in initialize, that is only for editor purpose and to serialize
				Manipulator->SetTransform(GetMappedLocalTransform(Manipulator->Name), this);
			}
			else
			{
				// do not add node in initialize, that is only for editor purpose and to serialize
				Manipulator->SetTransform(GetMappedGlobalTransform(Manipulator->Name), this);
			}
		}
	}
}

#if WITH_EDITOR
void UHierarchicalRig::AddNode(FName NodeName, FName ParentName, const FTransform& GlobalTransform, FName LinkedNode /*= NAME_None*/)
{
	FConstraintNodeData NewNodeData;
	NewNodeData.LinkedNode = LinkedNode;

	if (ParentName != NAME_None)
	{
		FTransform ParentTransform = Hierarchy.GetGlobalTransformByName(ParentName);
		NewNodeData.RelativeParent = GlobalTransform.GetRelativeTransform(ParentTransform);
	}

	Hierarchy.Add(NodeName, ParentName, GlobalTransform, NewNodeData);
}

void UHierarchicalRig::DeleteNode(FName NodeName)
{
	int32 NodeIndex = Hierarchy.GetNodeIndex(NodeName);
	if (Hierarchy.IsValidIndex(NodeIndex))
	{
		TArray<FName> Children = Hierarchy.GetChildren(NodeIndex);
		FName ParentName = Hierarchy.GetParentName(NodeIndex);

		// now reparent the children
		for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
		{
			int32 ChildNodeIndex = Hierarchy.GetNodeIndex(Children[ChildIndex]);
			Hierarchy.SetParentName(ChildNodeIndex, ParentName);
		}

		Hierarchy.Remove(NodeName);
	}
}

FNodeChain UHierarchicalRig::MakeNodeChain(FName RootNode, FName EndNode)
{
	FNodeChain NodeChain;

	// walk up hierarchy towards root from end to start
	FName BoneName = EndNode;
	while (BoneName != RootNode)
	{
		// we hit the root, so clear the bone chain - we have an invalid chain
		if (BoneName == NAME_None)
		{
			NodeChain.Nodes.Reset();
			break;
		}

		NodeChain.Nodes.EmplaceAt(0, BoneName);

		int32 NodeIndex = Hierarchy.GetNodeIndex(BoneName);
		if (NodeIndex == INDEX_NONE)
		{
			NodeChain.Nodes.Reset();
			break;
		}

		BoneName = Hierarchy.GetParentName(NodeIndex);
	}

	return NodeChain;
}

void UHierarchicalRig::AddConstraint(const FTransformConstraint& TransformConstraint)
{
	int32 NodeIndex = Hierarchy.GetNodeIndex(TransformConstraint.SourceNode);
	int32 ConstraintNodeIndex = Hierarchy.GetNodeIndex(TransformConstraint.TargetNode);

	if (NodeIndex != INDEX_NONE && ConstraintNodeIndex != INDEX_NONE)
	{
		FConstraintNodeData& NodeData = Hierarchy.GetNodeData<FConstraintNodeData>(NodeIndex);
		NodeData.Constraints.Add(TransformConstraint);

		// recalculate main offset for all constraint
		if (TransformConstraint.bMaintainOffset)
		{
			int32 ParentIndex = Hierarchy.GetParentIndex(NodeIndex);
			FTransform ParentTransform = Hierarchy.GetGlobalTransform(ParentIndex);
			FTransform LocalTransform = Hierarchy.GetLocalTransform(NodeIndex);
			FTransform TargetTransform = ResolveConstraints(LocalTransform, ParentTransform, NodeData);
			NodeData.ConstraintOffset.SaveInverseOffset(LocalTransform, TargetTransform, TransformConstraint.Operator);
		}
	}
}

void UHierarchicalRig::AddManipulator(FName NodeName, FName PropertyToManipulate)
{
	for (int32 ManipulatorIndex = 0; ManipulatorIndex < Manipulators.Num(); ++ManipulatorIndex)
	{
		if (UControlManipulator* Manipulator = Manipulators[ManipulatorIndex])
		{
			if (Manipulator->Name == NodeName)
			{
				// same name exists, failed, return
				return;
			}
		}
	}

	// make sure manipulator doesn't exists already
	if (Hierarchy.Contains(NodeName))
	{
		// @fixme: for now sphere by default
		USphereManipulator* NewManipulator = NewObject<USphereManipulator>(this, NAME_None, RF_Public | RF_Transactional | RF_ArchetypeObject);
		NewManipulator->Name = NodeName;
		NewManipulator->PropertyToManipulate = PropertyToManipulate;

		Manipulators.Add(NewManipulator);
	}
}

#endif // WITH_EDITOR
static int32& EnsureNodeExists(TMap<FName, int32>& InGraph, FName Name)
{
	int32* Value = InGraph.Find(Name);
	if (!Value)
	{
		Value = &InGraph.Add(Name);
		*Value = 0;
	}

	return *Value;
}

static void IncreaseEdgeCount(TMap<FName, int32>& InGraph, FName Name)
{
	int32& EdgeCount = EnsureNodeExists(InGraph, Name);
	++EdgeCount;
}

void UHierarchicalRig::CreateSortedNodes()
{
	SortedNodes.Reset();

	// name to incoming edge
	TMap<FName, int32> Graph;

	// calculate incoming edges
	for (int32 NodeIndex = 0; NodeIndex < Hierarchy.GetNum(); ++NodeIndex)
	{
		const FName& NodeName = Hierarchy.GetNodeName(NodeIndex);
		EnsureNodeExists(Graph, NodeName);

		TArray<FName> Neighbors;
		GetDependentArray(NodeName, Neighbors);
		// @todo: can include itself?
		for (int32 NeighborsIndex = 0; NeighborsIndex < Neighbors.Num(); ++NeighborsIndex)
		{
			IncreaseEdgeCount(Graph, Neighbors[NeighborsIndex]);
		}
	}

	// do run Kahn
	TArray<FName> SortingQueue;
	// first remove 0 in-degree vertices
	for (const TPair<FName, int32>& GraphPair : Graph)
	{
		if (GraphPair.Value == 0)
		{
			SortingQueue.Add(GraphPair.Key);
		}
	}

	// if sorting queue is same as node count, that means nothing is dependent
	if (SortingQueue.Num() == Hierarchy.GetNum())
	{
		SortedNodes = SortingQueue;
	}
	else
	{
		while (SortingQueue.Num() != 0)
		{
			FName Name = SortingQueue[0];

			// move the element
			SortingQueue.Remove(Name);
			SortedNodes.Add(Name);

			TArray<FName> Neighbors;

			GetDependentArray(Name, Neighbors);

			for (int32 NeighborsIndex = 0; NeighborsIndex < Neighbors.Num(); ++NeighborsIndex)
			{
				int32* EdgeCount = Graph.Find(Neighbors[NeighborsIndex]);
				--(*EdgeCount);
				if (*EdgeCount == 0)
				{
					SortingQueue.Add(Neighbors[NeighborsIndex]);
				}
			}
		}

		if (SortedNodes.Num() != Hierarchy.GetNum())
		{
			// if not same, we have cycle
			check(false);
		}
	}
}

void UHierarchicalRig::ApplyConstraint(const FName& NodeName)
{
	int32 NodeIndex = Hierarchy.GetNodeIndex(NodeName);
	if (NodeIndex != INDEX_NONE)
	{
		FConstraintNodeData& NodeData = Hierarchy.GetNodeData<FConstraintNodeData>(NodeIndex);
		if (NodeData.Constraints.Num() > 0)
		{
			FTransform LocalTransform = Hierarchy.GetLocalTransform(NodeIndex);
			int32 ParentIndex = Hierarchy.GetParentIndex(NodeIndex);
			FTransform ParentTransform = Hierarchy.GetGlobalTransform(ParentIndex);
			FTransform ConstraintTransform = ResolveConstraints(LocalTransform, ParentTransform, NodeData);
			NodeData.ConstraintOffset.ApplyInverseOffset(ConstraintTransform, LocalTransform);
			if (ParentIndex != INDEX_NONE)
			{
				Hierarchy.SetGlobalTransform(NodeIndex, LocalTransform * ParentTransform);
			}
		}
	}
}

void UHierarchicalRig::EvaluateNode(const FName& NodeName)
{
	// constraints have to update when current transform changes - I think that should happen before here
	ApplyConstraint(NodeName);

	int32 FoundIndex = SortedNodes.Find(NodeName);
	if (FoundIndex != INDEX_NONE)
	{
		int32 ChildIndex = FoundIndex - 1;
		while (ChildIndex >= 0)
		{
			FName ChildNodeName = SortedNodes[ChildIndex];
			int32 ChildNodeIndex = Hierarchy.GetNodeIndex(ChildNodeName);
			int32 ParentIndex = Hierarchy.GetParentIndex(ChildNodeIndex);
			if (ParentIndex != INDEX_NONE)
			{
				FTransform ParentTransform = Hierarchy.GetGlobalTransform(ParentIndex);
				Hierarchy.SetGlobalTransform(ChildNodeIndex, Hierarchy.GetLocalTransform(ChildNodeIndex) * ParentTransform);
			}
			ApplyConstraint(ChildNodeName);
			--ChildIndex;
		}
	}
}

void UHierarchicalRig::GetDependentArray(const FName& Name, TArray<FName>& OutList)
{
	int32 NodeIndex = Hierarchy.GetNodeIndex(Name);

	OutList.Reset();
	if (NodeIndex != INDEX_NONE)
	{
		FName ParentName = Hierarchy.GetParentName(NodeIndex);

		if (ParentName != NAME_None)
		{
			OutList.AddUnique(ParentName);
		}

		FConstraintNodeData& NodeData = Hierarchy.GetNodeData<FConstraintNodeData>(NodeIndex);
		for (int32 ConstraintsIndex = 0; ConstraintsIndex < NodeData.Constraints.Num(); ++ConstraintsIndex)
		{
			if (NodeData.Constraints[ConstraintsIndex].TargetNode != NAME_None)
			{
				OutList.AddUnique(NodeData.Constraints[ConstraintsIndex].TargetNode);
			}
		}
	}
}

FTransform UHierarchicalRig::ResolveConstraints(const FTransform& LocalTransform, const FTransform& ParentTransform, const FConstraintNodeData& NodeData)
{
	FTransform CurrentLocalTransform = LocalTransform;
	FGetGlobalTransform OnGetGlobalTransform;
	OnGetGlobalTransform.BindUObject(this, &UHierarchicalRig::GetGlobalTransform);
	return AnimationCore::SolveConstraints(CurrentLocalTransform, ParentTransform, NodeData.Constraints, OnGetGlobalTransform);
}

void UHierarchicalRig::Sort()
{
	CreateSortedNodes();
}

UControlManipulator* UHierarchicalRig::FindManipulator(const FName& Name)
{
	for (UControlManipulator* Manipulator : Manipulators)
	{
		if (Manipulator && Manipulator->Name == Name)
		{
			return Manipulator;
		}
	}

	return nullptr;
}

void UHierarchicalRig::GetMappableNodeData(TArray<FName>& OutNames, TArray<FTransform>& OutTransforms) const
{
	OutNames.Reset();
	OutTransforms.Reset();

	// now add all nodes
	TArray<FNodeObject> Nodes = Hierarchy.GetNodes();
	const TArray<FTransform>& Transforms = Hierarchy.GetTransforms();

	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		const FConstraintNodeData* UserData = static_cast<const FConstraintNodeData*> (Hierarchy.GetUserDataImpl(Index));
		// if no node is linked, we only allow them to map, so add them
		if (UserData->LinkedNode == NAME_None)
		{
			OutNames.Add(Nodes[Index].Name);
			OutTransforms.Add(Transforms[Index]);
		}
	}
}

void UHierarchicalRig::Setup()
{
	//Initialize();
}
#undef LOCTEXT_NAMESPACE