
#include "BlueprintEditorPrivatePCH.h"
#include "SCSDiff.h"
#include "SKismetInspector.h"
#include "SSCSEditor.h"

#include <vector>

FSCSDiff::FSCSDiff(const UBlueprint* InBlueprint)
{
	TSharedRef<SKismetInspector> Inspector = SNew(SKismetInspector)
		.HideNameArea(true)
		.ViewIdentifier(FName("BlueprintInspector"))
		.IsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateStatic([] { return false; }));

	ContainerWidget = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(SCSEditor, SSCSEditor, TSharedPtr<FBlueprintEditor>(), InBlueprint->SimpleConstructionScript, const_cast<UBlueprint*>(InBlueprint), Inspector)
		]
	+ SVerticalBox::Slot()
		[
			Inspector
		];
}

void FSCSDiff::HighlightProperty(FSCSDiffEntry Property)
{
	check( Property.TreeNodeName != FName() );
	if( const USimpleConstructionScript* SCS = SCSEditor->SCS )
	{
		TArray<USCS_Node*> Nodes = SCS->GetAllNodes();
		for( auto Node : Nodes )
		{
			if( Node->VariableName == Property.TreeNodeName )
			{
				SCSEditor->HighlightTreeNode( Node, Property.PropertyName );
				return;
			}
		}
	}

	SCSEditor->ClearSelection();
}

TSharedRef< SWidget > FSCSDiff::TreeWidget()
{
	return ContainerWidget.ToSharedRef();
}
