// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "EnvironmentQueryEditorPrivatePCH.h"
#include "EnvironmentQueryEditorModule.h"

//////////////////////////////////////////////////////////////////////////
// EnvironmentQueryGraph

namespace EQSGraphVersion
{
	const int32 Initial = 0;
	const int32 NestedNodes = 1;

	const int32 Latest = NestedNodes;
}

UEnvironmentQueryGraph::UEnvironmentQueryGraph(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
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

void UEnvironmentQueryGraph::UpdateAsset()
{
	//let's find root node
	UEnvironmentQueryGraphNode_Root* RootNode = NULL;
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		RootNode = Cast<UEnvironmentQueryGraphNode_Root>(Nodes[Index]);
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

		for (int32 Index=0; Index < MyPin->LinkedTo.Num(); ++Index)
		{
			UEnvironmentQueryGraphNode_Option* OptionNode = Cast<UEnvironmentQueryGraphNode_Option>(MyPin->LinkedTo[Index]->GetOwningNode());
			if (OptionNode)
			{
				UEnvQueryOption* OptionInstance = Cast<UEnvQueryOption>(OptionNode->NodeInstance);
				OptionInstance->Tests.Reset();

				for (int32 iTest = 0; iTest < OptionNode->Tests.Num(); iTest++)
				{
					UEnvironmentQueryGraphNode_Test* TestNode = OptionNode->Tests[iTest];
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

	UEnvQueryManager::NotifyAssetUpdate(Query);
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

	GraphVersion = EQSGraphVersion::Latest;
}

void UEnvironmentQueryGraph::UpdateVersion_NestedNodes()
{
	for (int32 i = 0; i < Nodes.Num(); i++)
	{
		UEnvironmentQueryGraphNode_Option* OptionNode = Cast<UEnvironmentQueryGraphNode_Option>(Nodes[i]);
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
								OptionNode->Tests.Add(LinkedTest);

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

	for (int32 i = Nodes.Num() - 1; i >= 0; i--)
	{
		UEnvironmentQueryGraphNode_Test* TestNode = Cast<UEnvironmentQueryGraphNode_Test>(Nodes[i]);
		if (TestNode)
		{
			TestNode->Pins.Empty();
			Nodes.RemoveAt(i);
			continue;
		}

		UEnvironmentQueryGraphNode_Option* OptionNode = Cast<UEnvironmentQueryGraphNode_Option>(Nodes[i]);
		if (OptionNode && OptionNode->Pins.IsValidIndex(1))
		{
			OptionNode->Pins.RemoveAt(1);
		}
	}
}
