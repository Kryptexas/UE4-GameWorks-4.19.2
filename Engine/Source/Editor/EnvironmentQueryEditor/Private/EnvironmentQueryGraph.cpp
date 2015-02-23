// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "EnvironmentQueryEditorPrivatePCH.h"
#include "EnvironmentQueryEditorModule.h"

//////////////////////////////////////////////////////////////////////////
// EnvironmentQueryGraph

namespace EQSGraphVersion
{
	const int32 Initial = 0;
	const int32 NestedNodes = 1;
	const int32 CopyPasteOutersBug = 2;
	const int32 BlueprintClasses = 3;

	const int32 Latest = BlueprintClasses;
}

UEnvironmentQueryGraph::UEnvironmentQueryGraph(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Schema = UEdGraphSchema_EnvironmentQuery::StaticClass();
}

struct FCompareNodeXLocation
{
	FORCEINLINE bool operator()(const UEdGraphPin& A, const UEdGraphPin& B) const
	{
		return A.GetOwningNode()->NodePosX < B.GetOwningNode()->NodePosX;
	}
};

void UEnvironmentQueryGraph::UpdateAsset(int32 UpdateFlags)
{
	if (IsLocked())
	{
		return;
	}

	// let's find root node
	UEnvironmentQueryGraphNode_Root* RootNode = NULL;
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		RootNode = Cast<UEnvironmentQueryGraphNode_Root>(Nodes[Idx]);
		if (RootNode != NULL)
		{
			break;
		}
	}

	UEnvQuery* Query = Cast<UEnvQuery>(GetOuter());
	Query->Options.Reset();
	if (RootNode && RootNode->Pins.Num() > 0 && RootNode->Pins[0]->LinkedTo.Num() > 0)
	{
		UEdGraphPin* MyPin = RootNode->Pins[0];

		// sort connections so that they're organized the same as user can see in the editor
		MyPin->LinkedTo.Sort(FCompareNodeXLocation());

		for (int32 Idx = 0; Idx < MyPin->LinkedTo.Num(); Idx++)
		{
			UEnvironmentQueryGraphNode_Option* OptionNode = Cast<UEnvironmentQueryGraphNode_Option>(MyPin->LinkedTo[Idx]->GetOwningNode());
			if (OptionNode)
			{
				UEnvQueryOption* OptionInstance = Cast<UEnvQueryOption>(OptionNode->NodeInstance);
				if (OptionInstance != NULL)
				{
					OptionInstance->Tests.Reset();

					for (int32 TestIdx = 0; TestIdx < OptionNode->SubNodes.Num(); TestIdx++)
					{
						UEnvironmentQueryGraphNode_Test* TestNode = Cast<UEnvironmentQueryGraphNode_Test>(OptionNode->SubNodes[TestIdx]);
						if (TestNode && TestNode->bTestEnabled)
						{
							UEnvQueryTest* TestInstance = Cast<UEnvQueryTest>(TestNode->NodeInstance);
							if (TestInstance)
							{
								OptionInstance->Tests.Add(TestInstance);
							}
						}
					}

					Query->Options.Add(OptionInstance);
				}
			}
		}
	}

	RemoveOrphanedNodes();
#if USE_EQS_DEBUGGER
	UEnvQueryManager::NotifyAssetUpdate(Query);
#endif
}

void UEnvironmentQueryGraph::CalculateAllWeights()
{
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UEnvironmentQueryGraphNode_Option* OptionNode = Cast<UEnvironmentQueryGraphNode_Option>(Nodes[Idx]);
		if (OptionNode)
		{
			OptionNode->CalculateWeights();
		}
	}
}

void UEnvironmentQueryGraph::MarkVersion()
{
	GraphVersion = EQSGraphVersion::Latest;
}

void UEnvironmentQueryGraph::UpdateVersion()
{
	if (GraphVersion == EQSGraphVersion::Latest)
	{
		return;
	}

	// convert to nested nodes
	if (GraphVersion < EQSGraphVersion::NestedNodes)
	{
		UpdateVersion_NestedNodes();
	}

	if (GraphVersion < EQSGraphVersion::CopyPasteOutersBug)
	{
		UpdateVersion_FixupOuters();
	}

	if (GraphVersion < EQSGraphVersion::BlueprintClasses)
	{
		UpdateVersion_CollectClassData();
	}

	GraphVersion = EQSGraphVersion::Latest;
	Modify();
}

void UEnvironmentQueryGraph::UpdateVersion_NestedNodes()
{
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UEnvironmentQueryGraphNode_Option* OptionNode = Cast<UEnvironmentQueryGraphNode_Option>(Nodes[Idx]);
		if (OptionNode)
		{
			UEnvironmentQueryGraphNode* Node = OptionNode;
			while (Node)
			{
				UEnvironmentQueryGraphNode* NextNode = NULL;
				for (int32 iPin = 0; iPin < Node->Pins.Num(); iPin++)
				{
					UEdGraphPin* TestPin = Node->Pins[iPin];
					if (TestPin && TestPin->Direction == EGPD_Output)
					{
						for (int32 iLink = 0; iLink < TestPin->LinkedTo.Num(); iLink++)
						{
							UEdGraphPin* LinkedTo = TestPin->LinkedTo[iLink];
							UEnvironmentQueryGraphNode_Test* LinkedTest = LinkedTo ? Cast<UEnvironmentQueryGraphNode_Test>(LinkedTo->GetOwningNode()) : NULL;
							if (LinkedTest)
							{
								LinkedTest->ParentNode = OptionNode;
								OptionNode->SubNodes.Add(LinkedTest);

								NextNode = LinkedTest;
								break;
							}
						}

						break;
					}
				}

				Node = NextNode;
			}
		}
	}

	for (int32 Idx = Nodes.Num() - 1; Idx >= 0; Idx--)
	{
		UEnvironmentQueryGraphNode_Test* TestNode = Cast<UEnvironmentQueryGraphNode_Test>(Nodes[Idx]);
		if (TestNode)
		{
			TestNode->Pins.Empty();
			Nodes.RemoveAt(Idx);
			continue;
		}

		UEnvironmentQueryGraphNode_Option* OptionNode = Cast<UEnvironmentQueryGraphNode_Option>(Nodes[Idx]);
		if (OptionNode && OptionNode->Pins.IsValidIndex(1))
		{
			OptionNode->Pins.RemoveAt(1);
		}
	}
}

void UEnvironmentQueryGraph::UpdateVersion_FixupOuters()
{
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UEnvironmentQueryGraphNode* MyNode = Cast<UEnvironmentQueryGraphNode>(Nodes[Idx]);
		if (MyNode)
		{
			MyNode->PostEditImport();
		}
	}
}

void UEnvironmentQueryGraph::UpdateVersion_CollectClassData()
{
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UEnvironmentQueryGraphNode* MyNode = Cast<UEnvironmentQueryGraphNode>(Nodes[Idx]);
		if (MyNode)
		{
			UEnvQueryOption* OptionInstance = Cast<UEnvQueryOption>(MyNode->NodeInstance);
			if (OptionInstance && OptionInstance->Generator)
			{
				UpdateNodeClassData(MyNode, OptionInstance->Generator->GetClass());
			}

			for (int32 SubIdx = 0; SubIdx < MyNode->SubNodes.Num(); SubIdx++)
			{
				UAIGraphNode* SubNode = MyNode->SubNodes[SubIdx];
				if (SubNode && SubNode->NodeInstance)
				{
					UpdateNodeClassData(SubNode, SubNode->NodeInstance->GetClass());
				}
			}
		}
	}
}

void UEnvironmentQueryGraph::UpdateNodeClassData(UAIGraphNode* UpdateNode, UClass* InstanceClass)
{
	UBlueprint* BPOwner = Cast<UBlueprint>(InstanceClass->ClassGeneratedBy);
	if (BPOwner)
	{
		UpdateNode->ClassData = FGraphNodeClassData(BPOwner->GetName(), BPOwner->GetOutermost()->GetName(), InstanceClass->GetName(), InstanceClass);
	}
	else
	{
		UpdateNode->ClassData = FGraphNodeClassData(InstanceClass, "");
	}
}

void UEnvironmentQueryGraph::CollectAllNodeInstances(TSet<UObject*>& NodeInstances)
{
	Super::CollectAllNodeInstances(NodeInstances);

	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UEnvironmentQueryGraphNode* MyNode = Cast<UEnvironmentQueryGraphNode>(Nodes[Idx]);
		UEnvQueryOption* OptionInstance = MyNode ? Cast<UEnvQueryOption>(MyNode->NodeInstance) : nullptr;
		if (OptionInstance && OptionInstance->Generator)
		{
			NodeInstances.Add(OptionInstance->Generator);
		}
	}
}
