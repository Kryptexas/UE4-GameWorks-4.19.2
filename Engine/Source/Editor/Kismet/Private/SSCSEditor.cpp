// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintEditorPrivatePCH.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "BlueprintUtilities.h"
#include "ComponentAssetBroker.h"

#include "SSCSEditor.h"
#include "SKismetInspector.h"
#include "SSCSEditorViewport.h"
#include "SComponentClassCombo.h"
#include "STutorialWrapper.h"

#include "AssetSelection.h"
#include "Editor/SceneOutliner/Private/SSocketChooser.h"
#include "ScopedTransaction.h"

#include "DragAndDrop/AssetDragDropOp.h"
#include "ClassIconFinder.h"

#include "ObjectTools.h"

#include "IDocumentation.h"
#include "Kismet2NameValidators.h"

#define LOCTEXT_NAMESPACE "SSCSEditor"

static const FName SCS_ColumnName_ComponentClass( "ComponentClass" );
static const FName SCS_ColumnName_Asset( "Asset" );
static const FName SCS_ColumnName_Mobility( "Mobility" );

//////////////////////////////////////////////////////////////////////////
// SSCSEditorDragDropTree
void SSCSEditorDragDropTree::Construct( const FArguments& InArgs )
{
	SCSEditor = InArgs._SCSEditor;

	STreeView<FSCSEditorTreeNodePtrType>::FArguments BaseArgs;
	BaseArgs.OnGenerateRow( InArgs._OnGenerateRow )
			.OnItemScrolledIntoView( InArgs._OnItemScrolledIntoView )
			.OnGetChildren( InArgs._OnGetChildren )
			.TreeItemsSource( InArgs._TreeItemsSource )
			.ItemHeight( InArgs._ItemHeight )
			.OnContextMenuOpening( InArgs._OnContextMenuOpening )
			.OnMouseButtonDoubleClick( InArgs._OnMouseButtonDoubleClick )
			.OnSelectionChanged( InArgs._OnSelectionChanged )
			.OnExpansionChanged( InArgs._OnExpansionChanged )
			.SelectionMode( InArgs._SelectionMode )
			.HeaderRow( InArgs._HeaderRow )
			.ClearSelectionOnClick( InArgs._ClearSelectionOnClick )
			.ExternalScrollbar( InArgs._ExternalScrollbar );

	STreeView<FSCSEditorTreeNodePtrType>::Construct( BaseArgs );
}

FReply SSCSEditorDragDropTree::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	FReply Handled = FReply::Unhandled();

	if ( SCSEditor != NULL )
	{
		if(DragDrop::IsTypeMatch<FExternalDragOperation>( DragDropEvent.GetOperation() ) )
		{
			Handled = AssetUtil::CanHandleAssetDrag(DragDropEvent);
		}
	}

	return Handled;
}

FReply SSCSEditorDragDropTree::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) 
{
	FReply Handled = FReply::Unhandled();

	if ( SCSEditor != NULL )
	{
		if(DragDrop::IsTypeMatch<FExternalDragOperation>( DragDropEvent.GetOperation() ) || DragDrop::IsTypeMatch<FAssetDragDropOp>(DragDropEvent.GetOperation()))
		{
			TArray< FAssetData > DroppedAssetData = AssetUtil::ExtractAssetDataFromDrag( DragDropEvent );
			const int32 NumAssets = DroppedAssetData.Num();

			if ( NumAssets > 0 )
			{
				GWarn->BeginSlowTask( LOCTEXT("LoadingComponents", "Loading Component(s)"), true );

				for (int32 DroppedAssetIdx = 0; DroppedAssetIdx < NumAssets; ++DroppedAssetIdx)
				{
					const FAssetData& AssetData = DroppedAssetData[DroppedAssetIdx];
				
					TSubclassOf<UActorComponent>  ComponentClasses = FComponentAssetBrokerage::GetPrimaryComponentForAsset( AssetData.GetClass() );
					if ( NULL != ComponentClasses )
					{
						GWarn->StatusUpdate( DroppedAssetIdx, NumAssets, FText::Format( LOCTEXT("LoadingComponent", "Loading Component {0}"), FText::FromName( AssetData.AssetName ) ) );
						SCSEditor->AddNewComponent(ComponentClasses, AssetData.GetAsset() );
					}
				}

				GWarn->EndSlowTask();
			}

			Handled = FReply::Handled();
		}
	}

	return Handled;
}



//////////////////////////////////////////////////////////////////////////
//

class FSCSRowDragDropOp : public FDragDropOperation, public TSharedFromThis<FSCSRowDragDropOp>
{
public:
	static FString GetTypeId() {static FString Type = TEXT("FSCSRowDragDropOp"); return Type;}

	/** Available drop actions */
	enum EDropActionType
	{
		DropAction_None,
		DropAction_AttachTo,
		DropAction_DetachFrom,
		DropAction_MakeNewRoot,
		DropAction_AttachToOrMakeNewRoot
	};

	/** Row that we started the drag from */
	TWeakPtr<SSCS_RowWidget> SourceRow;

	/** String to show as hover text */
	FText CurrentHoverText;

	/** The type of drop action that's pending while dragging */
	EDropActionType PendingDropAction;
	
	/** The widget decorator to use */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE
	{
		return SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Graph.ConnectorFeedback.Border"))
			[				
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0,0,3,0)
				[
					SNew(SImage)
					.Image(this, &FSCSRowDragDropOp::GetIcon)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock) 
					.Text(this, &FSCSRowDragDropOp::GetHoverText)
				]
			];
	}

	FText GetHoverText() const
	{
		return !CurrentHoverText.IsEmpty()
			? CurrentHoverText
			: LOCTEXT("DropActionToolTip_InvalidDropTarget", "Cannot drop here.");
	}

	const FSlateBrush* GetIcon() const
	{
		return PendingDropAction != DropAction_None
			? FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"))
			: FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
	}

	static TSharedRef<FSCSRowDragDropOp> New()
	{
		TSharedPtr<FSCSRowDragDropOp> Operation = MakeShareable(new FSCSRowDragDropOp);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FSCSRowDragDropOp>(Operation);
		Operation->Construct();
		return Operation.ToSharedRef();
	}
};

//////////////////////////////////////////////////////////////////////////
// FSCSEditorTreeNode

FSCSEditorTreeNode::FSCSEditorTreeNode()
	:bIsInherited(false)
{
}

FSCSEditorTreeNode::FSCSEditorTreeNode(USCS_Node* InSCSNode, bool bInIsInherited)
	:bIsInherited(bInIsInherited)
	,SCSNodePtr(InSCSNode)
	,ComponentTemplatePtr(InSCSNode != NULL ? InSCSNode->ComponentTemplate : NULL)
{
}

FSCSEditorTreeNode::FSCSEditorTreeNode(UActorComponent* InComponentTemplate)
	:bIsInherited(false)
	,SCSNodePtr(NULL)
	,ComponentTemplatePtr(InComponentTemplate)
{
}

FName FSCSEditorTreeNode::GetVariableName() const
{
	FName VariableName = NAME_None;

	USCS_Node* SCS_Node = GetSCSNode();
	UActorComponent* ComponentTemplate = GetComponentTemplate();

	if(SCS_Node != NULL)
	{
		// Use the same variable name as is obtained by the compiler
		VariableName = SCS_Node->GetVariableName();
	}
	else if(ComponentTemplate != NULL)
	{
		// If the owner class is a Blueprint class, see if there's a corresponding object property that contains the component template
		check(ComponentTemplate->GetOwner());
		UClass* OwnerClass = ComponentTemplate->GetOwner()->GetActorClass();
		if(OwnerClass != NULL)
		{
			UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(OwnerClass);
			if(Blueprint != NULL && Blueprint->ParentClass != NULL)
			{
				for ( TFieldIterator<UProperty> It(Blueprint->ParentClass); It; ++It )
				{
					UProperty* Property  = *It;
					if(UObjectProperty* ObjectProp = Cast<UObjectProperty>(Property))
					{
						UObject* CDO = Blueprint->ParentClass->GetDefaultObject();
						UObject* Object = ObjectProp->GetObjectPropertyValue(ObjectProp->ContainerPtrToValuePtr<void>(CDO));

						if(Object)
						{
							if(Object->GetClass() != ComponentTemplate->GetClass())
							{
								continue;
							}

							if(Object->GetFName() == ComponentTemplate->GetFName())
							{
								VariableName = ObjectProp->GetFName();
								break;
							}
						}
					}
				}
			}
		}
	}

	return VariableName;
}

FString FSCSEditorTreeNode::GetDisplayString() const
{
	FName VariableName = GetVariableName();
	UActorComponent* ComponentTemplate = GetComponentTemplate();

	// Only display SCS node variable names in the tree if they have not been autogenerated
	if (VariableName != NAME_None)
	{
		return VariableName.ToString();
	}
	else if(IsNative() && ComponentTemplate != NULL)
	{
		return ComponentTemplate->GetFName().ToString();
	}
	else
	{
		FString UnnamedString = LOCTEXT("UnnamedToolTip", "Unnamed").ToString();
		FString NativeString = IsNative() ? LOCTEXT("NativeToolTip", "Native ").ToString() : TEXT("");

		if(ComponentTemplate != NULL)
		{
			return FString::Printf(TEXT("[%s %s%s]"), *UnnamedString, *NativeString, *ComponentTemplate->GetClass()->GetName());
		}
		else
		{
			return FString::Printf(TEXT("[%s %s]"), *UnnamedString, *NativeString);
		}
	}
}

UActorComponent* FSCSEditorTreeNode::FindComponentInstanceInActor(const AActor* InActor, bool bIsPreviewActor) const
{
	USCS_Node* SCS_Node = GetSCSNode();
	UActorComponent* ComponentTemplate = GetComponentTemplate();

	UActorComponent* ComponentInstance = NULL;
	if(InActor != NULL)
	{
		if(SCS_Node != NULL)
		{
			FName VariableName = SCS_Node->GetVariableName();
			if(VariableName != NAME_None)
			{
				UObjectPropertyBase* Property = FindField<UObjectPropertyBase>(InActor->GetClass(), VariableName);
				if(Property != NULL)
				{
					// Return the component instance that's stored in the property with the given variable name
					ComponentInstance = Cast<UActorComponent>(Property->GetObjectPropertyValue_InContainer(InActor));
				}
				else if(bIsPreviewActor)
				{
					// If this is the preview actor, return the cached component instance that's being used for the preview actor prior to recompiling the Blueprint
					ComponentInstance = SCS_Node->EditorComponentInstance;
				}
			}
		}
		else if(ComponentTemplate != NULL)
		{
			// Look for a native component instance with a name that matches the template name
			TArray<UActorComponent*> Components;
			InActor->GetComponents(Components);

			for(auto It = Components.CreateConstIterator(); It; ++It)
			{
				UActorComponent* Component = *It;
				if(Component->GetFName() == ComponentTemplate->GetFName())
				{
					ComponentInstance = Component;
					break;
				}
			}
		}
	}

	return ComponentInstance;
}

bool FSCSEditorTreeNode::IsRoot() const
{
	bool bIsRoot = true;
	USCS_Node* SCS_Node = GetSCSNode();
	UActorComponent* ComponentTemplate = GetComponentTemplate();

	if(SCS_Node != NULL)
	{
		USimpleConstructionScript* SCS = SCS_Node->GetSCS();
		if(SCS != NULL)
		{
			// Evaluate to TRUE if we have an SCS node reference, it is contained in the SCS root set and does not have an external parent
			bIsRoot = SCS->GetRootNodes().Contains(SCS_Node) && SCS_Node->ParentComponentOrVariableName == NAME_None;
		}
	}
	else if(ComponentTemplate != NULL)
	{
		AActor* CDO = ComponentTemplate->GetOwner();
		if(CDO != NULL)
		{
			// Evaluate to TRUE if we have a valid component reference that matches the native root component
			bIsRoot = (ComponentTemplate == CDO->GetRootComponent());
		}
	}

	return bIsRoot;
}

bool FSCSEditorTreeNode::IsAttachedTo(FSCSEditorTreeNodePtrType InNodePtr) const
{ 
	FSCSEditorTreeNodePtrType TestParentPtr = ParentNodePtr;
	while(TestParentPtr.IsValid())
	{
		if(TestParentPtr == InNodePtr)
		{
			return true;
		}

		TestParentPtr = TestParentPtr->ParentNodePtr;
	}

	return false; 
}

bool FSCSEditorTreeNode::IsDefaultSceneRoot() const
{
	USCS_Node* SCS_Node = GetSCSNode();
	if(SCS_Node != NULL)
	{
		USimpleConstructionScript* SCS = SCS_Node->GetSCS();
		if(SCS != NULL)
		{
			return SCS_Node == SCS->GetDefaultSceneRootNode();
		}
	}

	return false;
}

bool FSCSEditorTreeNode::CanEditDefaults() const
{
	bool bCanEdit = false;
	USCS_Node* SCS_Node = GetSCSNode();
	UActorComponent* ComponentTemplate = GetComponentTemplate();

	if(!IsNative())
	{
		// Evaluate to TRUE for non-native nodes if it represents a valid SCS node and it is not inherited from a parent Blueprint
		bCanEdit = SCS_Node != NULL && !IsInherited();
	}
	else if(ComponentTemplate != NULL)
	{
		// Evaluate to TRUE for native nodes if it is bound to a member variable and that variable has either EditDefaultsOnly or EditAnywhere flags set
		check(ComponentTemplate->GetOwner());
		UClass* OwnerClass = ComponentTemplate->GetOwner()->GetActorClass();
		if(OwnerClass != NULL)
		{
			UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(OwnerClass);
			if(Blueprint != NULL && Blueprint->ParentClass != NULL)
			{
				for(TFieldIterator<UProperty> It(Blueprint->ParentClass); It; ++It)
				{
					UProperty* Property = *It;
					if(UObjectProperty* ObjectProp = Cast<UObjectProperty>(Property))
					{
						//must be editable
						if((Property->PropertyFlags & (CPF_Edit)) == 0)
						{
							continue;
						}

						UObject* ParentCDO = Blueprint->ParentClass->GetDefaultObject();

						if(!ComponentTemplate->GetClass()->IsChildOf(ObjectProp->PropertyClass))
						{
							continue;
						}

						UObject* Object = ObjectProp->GetObjectPropertyValue(ObjectProp->ContainerPtrToValuePtr<void>(ParentCDO));
						bCanEdit = Object != NULL && Object->GetFName() == ComponentTemplate->GetFName();

						if (bCanEdit)
						{
							break;
						}
					}
				}
			}
		}
	}

	return bCanEdit;
}

void FSCSEditorTreeNode::AddChild(FSCSEditorTreeNodePtrType InChildNodePtr)
{
	USCS_Node* SCS_Node = GetSCSNode();
	UActorComponent* ComponentTemplate = GetComponentTemplate();

	// Ensure the node is not already parented elsewhere
	if(InChildNodePtr->GetParent().IsValid())
	{
		InChildNodePtr->GetParent()->RemoveChild(InChildNodePtr);
	}

	// Add the given node as a child and link its parent
	Children.AddUnique(InChildNodePtr);
	InChildNodePtr->ParentNodePtr = AsShared();

	// Add a child node to the SCS tree node if not already present
	USCS_Node* SCS_ChildNode = InChildNodePtr->GetSCSNode();
	if(SCS_ChildNode != NULL)
	{
		// Get the SCS instance that owns the child node
		USimpleConstructionScript* SCS = SCS_ChildNode->GetSCS();
		if(SCS != NULL)
		{
			// If the parent is also a valid SCS node
			if(SCS_Node != NULL)
			{
				// If the parent and child are both owned by the same SCS instance
				if(SCS_Node->GetSCS() == SCS)
				{
					// Add the child into the parent's list of children
					if(!SCS_Node->ChildNodes.Contains(SCS_ChildNode))
					{
						SCS_Node->AddChildNode(SCS_ChildNode);
					}
				}
				else
				{
					// Adds the child to the SCS root set if not already present
					SCS->AddNode(SCS_ChildNode);

					// Set parameters to parent this node to the "inherited" SCS node
					SCS_ChildNode->SetParent(SCS_Node);
				}
			}
			else if(ComponentTemplate != NULL)
			{
				// Adds the child to the SCS root set if not already present
				SCS->AddNode(SCS_ChildNode);

				// Set parameters to parent this node to the native component template
				SCS_ChildNode->SetParent(Cast<USceneComponent>(ComponentTemplate));
			}
			else
			{
				// Adds the child to the SCS root set if not already present
				SCS->AddNode(SCS_ChildNode);
			}
		}
	}
}

FSCSEditorTreeNodePtrType FSCSEditorTreeNode::AddChild(USCS_Node* InSCSNode, bool bInIsInherited)
{
	// Ensure that the given SCS node is valid
	check(InSCSNode != NULL);

	// If it doesn't already exist as a child node
	FSCSEditorTreeNodePtrType ChildNodePtr = FindChild(InSCSNode);
	if(!ChildNodePtr.IsValid())
	{
		// Add a child node to the SCS editor tree
		AddChild(ChildNodePtr = MakeShareable(new FSCSEditorTreeNode(InSCSNode, bInIsInherited)));
	}

	return ChildNodePtr;
}

FSCSEditorTreeNodePtrType FSCSEditorTreeNode::AddChild(UActorComponent* InComponentTemplate)
{
	// Ensure that the given component template is valid
	check(InComponentTemplate != NULL);

	// If it doesn't already exist in the SCS editor tree
	FSCSEditorTreeNodePtrType ChildNodePtr = FindChild(InComponentTemplate);
	if(!ChildNodePtr.IsValid())
	{
		// Add a child node to the SCS editor tree
		AddChild(ChildNodePtr = MakeShareable(new FSCSEditorTreeNode(InComponentTemplate)));
	}

	return ChildNodePtr;
}

FSCSEditorTreeNodePtrType FSCSEditorTreeNode::FindChild(const USCS_Node* InSCSNode) const
{
	// Ensure that the given SCS node is valid
	if(InSCSNode != NULL)
	{
		// Look for a match in our set of child nodes
		for(int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
		{
			if(InSCSNode == Children[ChildIndex]->GetSCSNode())
			{
				return Children[ChildIndex];
			}
		}
	}

	// Return an empty node reference if no match is found
	return FSCSEditorTreeNodePtrType();
}

FSCSEditorTreeNodePtrType FSCSEditorTreeNode::FindChild(const UActorComponent* InComponentTemplate) const
{
	// Ensure that the given component template is valid
	if(InComponentTemplate != NULL)
	{
		// Look for a match in our set of child nodes
		for(int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
		{
			if(InComponentTemplate == Children[ChildIndex]->GetComponentTemplate())
			{
				return Children[ChildIndex];
			}
		}
	}

	// Return an empty node reference if no match is found
	return FSCSEditorTreeNodePtrType();
}

void FSCSEditorTreeNode::RemoveChild(FSCSEditorTreeNodePtrType InChildNodePtr)
{
	// Remove the given node as a child and reset its parent link
	Children.Remove(InChildNodePtr);
	InChildNodePtr->ParentNodePtr.Reset();

	// Remove the SCS node from the SCS tree, if present
	USCS_Node* SCS_ChildNode = InChildNodePtr->GetSCSNode();
	if(SCS_ChildNode != NULL)
	{
		USimpleConstructionScript* SCS = SCS_ChildNode->GetSCS();
		if(SCS != NULL)
		{
			SCS->RemoveNode(SCS_ChildNode);
		}
	}
}

void FSCSEditorTreeNode::OnRequestRename()
{
	RenameRequestedDelegate.ExecuteIfBound();
}

//////////////////////////////////////////////////////////////////////////
// SSCS_RowWidget

void SSCS_RowWidget::Construct( const FArguments& InArgs, TSharedPtr<SSCSEditor> InSCSEditor, FSCSEditorTreeNodePtrType InNodePtr, TSharedPtr<STableViewBase> InOwnerTableView  )
{
	check(InNodePtr.IsValid());

	SCSEditor = InSCSEditor;
	NodePtr = InNodePtr;

	SMultiColumnTableRow<FSCSEditorTreeNodePtrType>::Construct( FSuperRowType::FArguments().Padding(FMargin(0.f, 0.f, 0.f, 4.f)), InOwnerTableView.ToSharedRef() );
}

SSCS_RowWidget::~SSCS_RowWidget()
{
	// Clear delegate when widget goes away
	//Ask SCSEditor if Node is still active, if it isn't it might have been collected so we can't do anything to it
	TSharedPtr<SSCSEditor> Editor = SCSEditor.Pin();
	if(Editor.IsValid())
	{
		USCS_Node* SCS_Node = NodePtr->GetSCSNode();
		if(SCS_Node != NULL && Editor->IsNodeInSimpleConstructionScript(SCS_Node))
		{
			SCS_Node->SetOnNameChanged(FSCSNodeNameChanged());
		}
	}
}


TSharedRef<SWidget> SSCS_RowWidget::GenerateWidgetForColumn( const FName& ColumnName )
{
	if(ColumnName == SCS_ColumnName_ComponentClass)
	{
		// Setup a default icon brush.
		const FSlateBrush* ComponentIcon = FEditorStyle::GetBrush("SCS.NativeComponent");
		if(NodePtr->GetComponentTemplate() != NULL)
		{
			ComponentIcon = FClassIconFinder::FindIconForClass( NodePtr->GetComponentTemplate()->GetClass(), TEXT("SCS.Component") );
		}

		TSharedPtr<SInlineEditableTextBlock> InlineWidget = 
			SNew(SInlineEditableTextBlock)
				.Text(this, &SSCS_RowWidget::GetNameLabel)
				.OnVerifyTextChanged( this, &SSCS_RowWidget::OnNameTextVerifyChanged )
				.OnTextCommitted( this, &SSCS_RowWidget::OnNameTextCommit )
				.IsSelected( this, &SSCS_RowWidget::IsSelectedExclusively )
				.IsReadOnly( !NodePtr->CanRename() || !SCSEditor.Pin()->InEditingMode() );

		NodePtr->SetRenameRequestedDelegate(FSCSEditorTreeNode::FOnRenameRequested::CreateSP(InlineWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode));

		return	SNew(SHorizontalBox)
				.ToolTip(IDocumentation::Get()->CreateToolTip(
					TAttribute<FText>(this, &SSCS_RowWidget::GetTooltipText),
					NULL,
					GetDocumentationLink(),
					GetDocumentationExcerptName()))
				+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SExpanderArrow, SharedThis(this))
					]
				+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image(ComponentIcon)
						.ColorAndOpacity(this, &SSCS_RowWidget::GetColorTint)
					]
				+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4,0,4,0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("RootLabel", "[ROOT]"))
						.Visibility(this, &SSCS_RowWidget::GetRootLabelVisibility)
					]
				+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						InlineWidget.ToSharedRef()
					];
	}
	else if(ColumnName == SCS_ColumnName_Asset)
	{
		return
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(2, 0, 0, 0)
			[
				SNew(STextBlock)
				.Visibility(this, &SSCS_RowWidget::GetAssetVisibility)
				.Text(this, &SSCS_RowWidget::GetAssetName)
				.ToolTipText(this, &SSCS_RowWidget::GetAssetNameTooltip)
			];
	}
	else if (ColumnName == SCS_ColumnName_Mobility)
	{
		TWeakObjectPtr<USCS_Node> SCSNode(NodePtr->GetSCSNode());

		TSharedPtr<SToolTip> MobilityTooltip;
		SAssignNew(MobilityTooltip, SToolTip)
			.Text(this, &SSCS_RowWidget::GetMobilityToolTipText, SCSNode);

		return	SNew(SHorizontalBox)
					.ToolTip(MobilityTooltip)
					.Visibility(EVisibility::Visible) // so we still get tooltip text for an empty SHorizontalBox
				+SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SImage)
							.Image(this, &SSCS_RowWidget::GetMobilityIconImage, SCSNode)
							.ToolTip(MobilityTooltip)
					];
	}
	else
	{
		return	SNew(STextBlock)
				.Text( LOCTEXT("UnknownColumn", "Unknown Column") );
	}
}

FSlateBrush const* SSCS_RowWidget::GetMobilityIconImage(TWeakObjectPtr<USCS_Node> SCSNode) const
{
	if (!SCSNode.IsValid())
	{
		return NULL;
	}

	USceneComponent* SceneComponentTemplate = Cast<USceneComponent>(SCSNode->ComponentTemplate);
	if (SceneComponentTemplate == NULL)
	{
		return NULL;
	}

	if (SceneComponentTemplate->Mobility == EComponentMobility::Movable)
	{
		return FEditorStyle::GetBrush(TEXT("ClassIcon.MovableMobilityIcon"));
	}
	else if (SceneComponentTemplate->Mobility == EComponentMobility::Stationary)
	{
		return FEditorStyle::GetBrush(TEXT("ClassIcon.StationaryMobilityIcon"));
	}

	// static components don't get an icon (because static is the most common
	// mobility type, and we'd like to keep the icon clutter to a minimum)
	return NULL;
}

FText SSCS_RowWidget::GetMobilityToolTipText(TWeakObjectPtr<USCS_Node> SCSNode) const
{
	FText MobilityToolTip = LOCTEXT("NoMobilityTooltip", "This component does not have 'Mobility' associated with it");
	if (SCSNode.IsValid())
	{
		USceneComponent* SceneComponentTemplate = Cast<USceneComponent>(SCSNode->ComponentTemplate);
		if (SceneComponentTemplate == NULL)
		{
			MobilityToolTip = LOCTEXT("NoMobilityTooltip", "This component does not have 'Mobility' associated with it");
		}
		else if (SceneComponentTemplate->Mobility == EComponentMobility::Movable)
		{
			MobilityToolTip = LOCTEXT("MovableMobilityTooltip", "Movable component");
		}
		else if (SceneComponentTemplate->Mobility == EComponentMobility::Stationary)
		{
			MobilityToolTip = LOCTEXT("StationaryMobilityTooltip", "Stationary component");
		}
		else 
		{
			// make sure we're the mobility type we're expecting (we've handled Movable & Stationary)
			ensureMsgf(SceneComponentTemplate->Mobility == EComponentMobility::Static, TEXT("Unhandled mobility type [%d], is this a new type that we don't handle here?"), SceneComponentTemplate->Mobility.GetValue());

			MobilityToolTip = LOCTEXT("StaticMobilityTooltip", "Static component");
		}
	}

	return MobilityToolTip;
}

FString SSCS_RowWidget::GetAssetName() const
{
	FString AssetName(TEXT("None"));
	if(NodePtr.IsValid() && NodePtr->GetComponentTemplate())
	{
		UObject* Asset = FComponentAssetBrokerage::GetAssetFromComponent(NodePtr->GetComponentTemplate());
		if(Asset != NULL)
		{
			AssetName = Asset->GetName();
		}
	}

	return AssetName;
}

FString SSCS_RowWidget::GetAssetNameTooltip() const
{
	FString AssetName(TEXT("None"));
	if(NodePtr.IsValid() && NodePtr->GetComponentTemplate())
	{
		UObject* Asset = FComponentAssetBrokerage::GetAssetFromComponent(NodePtr->GetComponentTemplate());
		if(Asset != NULL)
		{
			AssetName = Asset->GetPathName();
		}
	}

	return AssetName;
}


EVisibility SSCS_RowWidget::GetAssetVisibility() const
{
	if(NodePtr.IsValid() && NodePtr->GetComponentTemplate() && FComponentAssetBrokerage::SupportsAssets(NodePtr->GetComponentTemplate()))
	{
		return EVisibility::Visible;
	}
	else
	{
		return EVisibility::Hidden;
	}
}

FSlateColor SSCS_RowWidget::GetColorTint() const
{
	if(NodePtr->IsNative())
	{
		return FLinearColor(0.08f,0.15f,0.6f);
	}
	else if(NodePtr->IsInherited())
	{
		return FLinearColor(0.08f,0.35f,0.6f);
	}

	return FSlateColor::UseForeground();
}

EVisibility SSCS_RowWidget::GetRootLabelVisibility() const
{
	if(NodePtr.IsValid() && SCSEditor.IsValid() && NodePtr == SCSEditor.Pin()->SceneRootNodePtr)
	{
		return EVisibility::Visible;
	}
	else
	{
		return EVisibility::Collapsed;
	}
}

TSharedPtr<SWidget> SSCS_RowWidget::BuildSceneRootDropActionMenu(FSCSEditorTreeNodePtrType DroppedNodePtr)
{
	FMenuBuilder MenuBuilder(true, SCSEditor.Pin()->CommandList);

	MenuBuilder.BeginSection("SceneRootNodeDropActions", LOCTEXT("SceneRootNodeDropActionContextMenu", "Drop Actions"));
	{
		const FText DroppedVariableNameText = FText::FromName( DroppedNodePtr->GetVariableName() );
		const FText NodeVariableNameText = FText::FromName( NodePtr->GetVariableName() );

		MenuBuilder.AddMenuEntry(
			LOCTEXT("DropActionLabel_AttachToRootNode", "Attach"),
			FText::Format( LOCTEXT("DropActionToolTip_AttachToRootNode", "Attach {0} to {1}."), DroppedVariableNameText, NodeVariableNameText ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SSCS_RowWidget::OnAttachToDropAction, DroppedNodePtr),
				FCanExecuteAction()));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("DropActionLabel_MakeNewRootNode", "Make New Root"),
			FText::Format( LOCTEXT("DropActionToolTip_MakeNewRootNode", "Make {0} the new root."), DroppedVariableNameText ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SSCS_RowWidget::OnMakeNewRootDropAction, DroppedNodePtr),
				FCanExecuteAction()));
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

FReply SSCS_RowWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		FReply Reply = SMultiColumnTableRow<FSCSEditorTreeNodePtrType>::OnMouseButtonDown( MyGeometry, MouseEvent );
		return Reply.DetectDrag( SharedThis(this) , EKeys::LeftMouseButton );
	}
	else
	{
		return FReply::Unhandled();
	}
}

FReply SSCS_RowWidget::OnDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ( MouseEvent.IsMouseButtonDown( EKeys::LeftMouseButton )
			&& SCSEditor.Pin()->InEditingMode() ) //can only drag when editing
	{
		TSharedRef<FSCSRowDragDropOp> Operation = FSCSRowDragDropOp::New();
		Operation->SourceRow = StaticCastSharedRef<SSCS_RowWidget>(AsShared());
		Operation->PendingDropAction = FSCSRowDragDropOp::DropAction_None;
		if(!NodePtr->CanReparent())
		{
			// We set the tooltip text here because it won't change across entry/leave events
			Operation->CurrentHoverText = LOCTEXT("DropActionToolTip_Error_CannotReparent", "The selected component cannot be moved.");
		}
		else
		{
			Operation->CurrentHoverText = FText::GetEmpty();
		}
		return FReply::Handled().BeginDragDrop(Operation);
	}
	else
	{
		return FReply::Unhandled();
	}
}

void SSCS_RowWidget::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FSCSRowDragDropOp>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FSCSRowDragDropOp> DragRowOp = StaticCastSharedPtr<FSCSRowDragDropOp>( DragDropEvent.GetOperation() );
		if(DragRowOp->SourceRow.IsValid())
		{
			FSCSEditorTreeNodePtrType DraggedNodePtr = DragRowOp->SourceRow.Pin()->NodePtr;
			check(DraggedNodePtr.IsValid());
			FSCSEditorTreeNodePtrType SceneRootNodePtr = SCSEditor.Pin()->SceneRootNodePtr;
			check(SceneRootNodePtr.IsValid());

			USceneComponent* HoveredTemplate = Cast<USceneComponent>(NodePtr->GetComponentTemplate());
			USceneComponent* DraggedTemplate = Cast<USceneComponent>(DraggedNodePtr->GetComponentTemplate());

			if(!DraggedNodePtr->CanReparent())
			{
				// Cannot reparent the selected node; we already set the ToolTip text
				return;
			}
			else if(DraggedNodePtr == NodePtr)
			{
				// Attempted to drag and drop onto self
				DragRowOp->CurrentHoverText = LOCTEXT("DropActionToolTip_Error_CannotAttachToSelf", "Can't attach to self.");
			}
			else if(NodePtr->IsAttachedTo(DraggedNodePtr))
			{
				// Attempted to drop a parent onto a child
				DragRowOp->CurrentHoverText = LOCTEXT("DropActionToolTip_Error_CannotToChild", "Can't attach to a child.");
			}
			else if(HoveredTemplate == NULL || DraggedTemplate == NULL)
			{
				// Can't attach non-USceneComponent types
				DragRowOp->CurrentHoverText = LOCTEXT("DropActionToolTip_Error_NotAttachable", "Cannot attach to this component.");
			}
			else if(DraggedNodePtr->IsAttachedTo(NodePtr))
			{
				if(NodePtr == SceneRootNodePtr)
				{
					if(!NodePtr->CanReparent())
					{
						// Cannot make the dropped node the new root if we cannot reparent the current root
						DragRowOp->CurrentHoverText = LOCTEXT("DropActionToolTip_Error_CannotReparentRootNode", "The root component in this Blueprint cannot be replaced.");						
					}
					else if (DraggedTemplate->IsEditorOnly() && !HoveredTemplate->IsEditorOnly()) 
					{
						// can't have a new root that's editor-only (when children would be around in-game)
						DragRowOp->CurrentHoverText = LOCTEXT("DropActionToolTip_Error_CannotReparentEditorOnly", "Cannot re-parent game components under editor-only ones.");
					}
					else
					{
						// Make the dropped node the new root
						DragRowOp->CurrentHoverText = FText::Format( LOCTEXT("DropActionToolTip_DropMakeNewRootNode", "Drop here to make {0} the new root."), FText::FromName( DraggedNodePtr->GetVariableName() ) );
						DragRowOp->PendingDropAction = FSCSRowDragDropOp::DropAction_MakeNewRoot;
					}
				}
				else if(DraggedNodePtr->IsDirectlyAttachedTo(NodePtr)) // if dropped onto parent
				{
					// Detach the dropped node from the current node and reattach it to the root node
					DragRowOp->CurrentHoverText = FText::Format( LOCTEXT("DropActionToolTip_DetachFromThisNode", "Drop here to detach {0} from {1}."), FText::FromName( DraggedNodePtr->GetVariableName() ),  FText::FromName( NodePtr->GetVariableName() ) );
					DragRowOp->PendingDropAction = FSCSRowDragDropOp::DropAction_DetachFrom;
				}
			}
			else if (!DraggedTemplate->IsEditorOnly() && HoveredTemplate->IsEditorOnly()) 
			{
				// can't have a game component child nested under an editor-only one
				DragRowOp->CurrentHoverText = LOCTEXT("DropActionToolTip_Error_CannotAttachToEditorOnly", "Cannot attach game components to editor-only ones.");
			}
			else if ((DraggedTemplate->Mobility == EComponentMobility::Static) && ((HoveredTemplate->Mobility == EComponentMobility::Movable) || (HoveredTemplate->Mobility == EComponentMobility::Stationary)))
			{
				// Can't attach Static components to mobile ones
				DragRowOp->CurrentHoverText = LOCTEXT("DropActionToolTip_Error_CannotAttachStatic", "Cannot attach Static components to movable ones.");
			}
			else if ((DraggedTemplate->Mobility == EComponentMobility::Stationary) && (HoveredTemplate->Mobility == EComponentMobility::Movable))
			{
				// Can't attach Static components to mobile ones
				DragRowOp->CurrentHoverText = LOCTEXT("DropActionToolTip_Error_CannotAttachStationary", "Cannot attach Stationary components to movable ones.");
			}
			else if(NodePtr == SceneRootNodePtr && NodePtr->CanReparent())
			{
				if(HoveredTemplate->CanAttachAsChild(DraggedTemplate, NAME_None))
				{
					// User can choose to either attach to the current root or make the dropped node the new root
					DragRowOp->CurrentHoverText = LOCTEXT("DropActionToolTip_AttachToOrMakeNewRoot", "Drop here to see available actions.");
					DragRowOp->PendingDropAction = FSCSRowDragDropOp::DropAction_AttachToOrMakeNewRoot;
				}
				else
				{
					// Only available action is to make the dropped node the new root
					DragRowOp->CurrentHoverText = FText::Format( LOCTEXT("DropActionToolTip_DropMakeNewRootNode", "Drop here to make {0} the new root."), FText::FromName( DraggedNodePtr->GetVariableName() ) );
					DragRowOp->PendingDropAction = FSCSRowDragDropOp::DropAction_MakeNewRoot;
				}
			}
			else if(HoveredTemplate->CanAttachAsChild(DraggedTemplate, NAME_None))
			{
				// Attach the dropped node to this node
				DragRowOp->CurrentHoverText = FText::Format( LOCTEXT("DropActionToolTip_AttachToThisNode", "Drop here to attach {0} to {1}."), FText::FromName( DraggedNodePtr->GetVariableName() ), FText::FromName( NodePtr->GetVariableName() ) );
				DragRowOp->PendingDropAction = FSCSRowDragDropOp::DropAction_AttachTo;
			}
			else
			{
				// The dropped node cannot be attached to the current node
				DragRowOp->CurrentHoverText = FText::Format( LOCTEXT("DropActionToolTip_Error_TooManyAttachments", "Unable to attach {0} to {1}."), FText::FromName( DraggedNodePtr->GetVariableName() ), FText::FromName( NodePtr->GetVariableName() ) );
			}
		}
	}
	else if ( DragDrop::IsTypeMatch<FExternalDragOperation>( DragDropEvent.GetOperation() ) || DragDrop::IsTypeMatch<FAssetDragDropOp>(DragDropEvent.GetOperation() ) )
	{
		// defer to the tree widget's handler for this type of operation
		TSharedPtr<SSCSEditor> PinnedEditor = SCSEditor.Pin();
		if ( PinnedEditor.IsValid() && PinnedEditor->SCSTreeWidget.IsValid() )
		{
			PinnedEditor->SCSTreeWidget->OnDragEnter( MyGeometry, DragDropEvent );
		}
	}
}

void SSCS_RowWidget::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FSCSRowDragDropOp>(DragDropEvent.GetOperation()) )
	{
		TSharedPtr<FSCSRowDragDropOp> DragRowOp = StaticCastSharedPtr<FSCSRowDragDropOp>( DragDropEvent.GetOperation() );
		if(DragRowOp->SourceRow.IsValid())
		{
			FSCSEditorTreeNodePtrType DraggedNodePtr = DragRowOp->SourceRow.Pin()->NodePtr;
			check(DraggedNodePtr.IsValid());

			// Only clear the tooltip text if the dragged node supports it
			if(DraggedNodePtr->CanReparent())
			{
				DragRowOp->CurrentHoverText = FText::GetEmpty();
				DragRowOp->PendingDropAction = FSCSRowDragDropOp::DropAction_None;
			}
		}
	}
}

FReply SSCS_RowWidget::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	if ( DragDrop::IsTypeMatch<FSCSRowDragDropOp>(DragDropEvent.GetOperation()) && NodePtr->GetComponentTemplate()->IsA<USceneComponent>())	
	{
		TSharedPtr<FSCSRowDragDropOp> DragRowOp = StaticCastSharedPtr<FSCSRowDragDropOp>( DragDropEvent.GetOperation() );		
		if(DragRowOp->SourceRow.IsValid() && SCSEditor.IsValid())
		{
			FSCSEditorTreeNodePtrType DraggedNodePtr = DragRowOp->SourceRow.Pin()->NodePtr;
			check(DraggedNodePtr.IsValid());
			FSCSEditorTreeNodePtrType SceneRootNodePtr = SCSEditor.Pin()->SceneRootNodePtr;
			check(SceneRootNodePtr.IsValid());

			switch(DragRowOp->PendingDropAction)
			{
			case FSCSRowDragDropOp::DropAction_AttachTo:
				OnAttachToDropAction(DraggedNodePtr);
				break;
			
			case FSCSRowDragDropOp::DropAction_DetachFrom:
				OnDetachFromDropAction(DraggedNodePtr);
				break;

			case FSCSRowDragDropOp::DropAction_MakeNewRoot:
				OnMakeNewRootDropAction(DraggedNodePtr);
				break;

			case FSCSRowDragDropOp::DropAction_AttachToOrMakeNewRoot:
				FSlateApplication::Get().PushMenu(
					SharedThis(this),
					BuildSceneRootDropActionMenu(DraggedNodePtr).ToSharedRef(),
					FSlateApplication::Get().GetCursorPos(),
					FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup)
				);
				break;

			case FSCSRowDragDropOp::DropAction_None:
			default:
				break;
			}
		}
	}
	else if ( DragDrop::IsTypeMatch<FExternalDragOperation>( DragDropEvent.GetOperation() ) || DragDrop::IsTypeMatch<FAssetDragDropOp>(DragDropEvent.GetOperation() ) )
	{
		// defer to the tree widget's handler for this type of operation
		TSharedPtr<SSCSEditor> PinnedEditor = SCSEditor.Pin();
		if ( PinnedEditor.IsValid() && PinnedEditor->SCSTreeWidget.IsValid() )
		{
			PinnedEditor->SCSTreeWidget->OnDrop( MyGeometry, DragDropEvent );
		}
	}

	return FReply::Handled();
}

void SSCS_RowWidget::OnAttachToDropAction(FSCSEditorTreeNodePtrType DroppedNodePtr)
{
	check(NodePtr.IsValid());
	check(DroppedNodePtr.IsValid());

	if(DroppedNodePtr->GetParent().IsValid())
	{
		DroppedNodePtr->GetParent()->RemoveChild(DroppedNodePtr);
	}
	
	NodePtr->AddChild(DroppedNodePtr);

	check(SCSEditor.IsValid());
	check(SCSEditor.Pin()->SCSTreeWidget.IsValid());
	SCSEditor.Pin()->SCSTreeWidget->SetItemExpansion(NodePtr, true);

	PostDragDropAction(false);
}

void SSCS_RowWidget::OnDetachFromDropAction(FSCSEditorTreeNodePtrType DroppedNodePtr)
{
	check(NodePtr.IsValid());
	check(DroppedNodePtr.IsValid());

	NodePtr->RemoveChild(DroppedNodePtr);

	check(SCSEditor.IsValid());

	check(SCSEditor.Pin()->SceneRootNodePtr.IsValid());
	SCSEditor.Pin()->SceneRootNodePtr->AddChild(DroppedNodePtr);

	PostDragDropAction(false);
}

void SSCS_RowWidget::OnMakeNewRootDropAction(FSCSEditorTreeNodePtrType DroppedNodePtr)
{
	check(SCSEditor.IsValid());

	// Get the current SCS context
	USimpleConstructionScript* SCS = SCSEditor.Pin()->SCS;
	check(SCS != NULL);

	// Get the current scene root node
	FSCSEditorTreeNodePtrType& SceneRootNodePtr = SCSEditor.Pin()->SceneRootNodePtr;

	check(NodePtr.IsValid() && NodePtr == SceneRootNodePtr);
	check(DroppedNodePtr.IsValid());

	// Remove the dropped node from its existing parent
	if(DroppedNodePtr->GetParent().IsValid())
	{
		DroppedNodePtr->GetParent()->RemoveChild(DroppedNodePtr);
	}

	check(SceneRootNodePtr->CanReparent());

	// Remove the current scene root node from the SCS context
	SCS->RemoveNode(SceneRootNodePtr->GetSCSNode());

	// Save old root node
	FSCSEditorTreeNodePtrType OldSceneRootNodePtr = SceneRootNodePtr;

	// Set node we are dropping as new root
	SceneRootNodePtr = DroppedNodePtr;

	// Add dropped node to the SCS context
	SCS->AddNode(SceneRootNodePtr->GetSCSNode());

	// Set old root as child of new root
	if(OldSceneRootNodePtr.IsValid())
	{
		SceneRootNodePtr->AddChild(OldSceneRootNodePtr);
	}

	PostDragDropAction(true);
}

void SSCS_RowWidget::PostDragDropAction(bool bRegenerateTreeNodes)
{
	TSharedPtr<SSCSEditor> PinnedEditor = SCSEditor.Pin();
	if(PinnedEditor.IsValid())
	{
		PinnedEditor->UpdateTree(bRegenerateTreeNodes);

		PinnedEditor->RefreshSelectionDetails();

		check(PinnedEditor->SCS != NULL);
		FBlueprintEditorUtils::PostEditChangeBlueprintActors(PinnedEditor->SCS->GetBlueprint());
	}
}

FText SSCS_RowWidget::GetNameLabel() const
{
	// NOTE: Whatever this returns also becomes the variable name
	return FText::FromString(NodePtr->GetDisplayString());
}

FText SSCS_RowWidget::GetTooltipText() const
{
	if(NodePtr->IsDefaultSceneRoot())
	{
		return LOCTEXT("DefaultSceneRootToolTip", "This is the default scene root component. It cannot be renamed or deleted. Adding a new scene component will automatically replace it as the new root.");
	}
	else
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("ClassName"), FText::FromString( (NodePtr->GetComponentTemplate() != NULL) ? NodePtr->GetComponentTemplate()->GetClass()->GetName() : TEXT("(null)") ) );
		Args.Add( TEXT("NodeName"), FText::FromString( NodePtr->GetDisplayString() ) );

		if ( NodePtr->IsNative() )
		{
			if(NodePtr->GetComponentTemplate() != NULL)
			{
				return FText::Format( LOCTEXT("NativeClassToolTip", "Native {ClassName}"), Args );
			}

			return FText::Format( LOCTEXT("MissingNativeComponentToolTip", "MISSING!! Native {NodeName}"), Args );
		}
		else
		{
			if ( NodePtr->IsInherited() )
			{
				if ( NodePtr->GetComponentTemplate() != NULL )
				{
					return FText::Format( LOCTEXT("InheritedToolTip", "Inherited {ClassName}"), Args );
				}

				return FText::Format( LOCTEXT("MissingInheritedComponentToolTip", "MISSING!! Inherited {NodeName}"), Args );
			}
			else
			{
				if ( NodePtr->GetComponentTemplate() != NULL )
				{
					return FText::Format( LOCTEXT("RegularToolTip", "{ClassName}"), Args );
				}

				return FText::Format( LOCTEXT("MissingRegularComponentToolTip", "MISSING!! {NodeName}"), Args );
			}
		}
	}
}

FString SSCS_RowWidget::GetDocumentationLink() const
{
	check(SCSEditor.IsValid());

	if(NodePtr == SCSEditor.Pin()->SceneRootNodePtr
		|| NodePtr->IsNative() || NodePtr->IsInherited())
	{
		return TEXT("Shared/Editors/BlueprintEditor/ComponentsMode");
	}

	return TEXT("");
}

FString SSCS_RowWidget::GetDocumentationExcerptName() const
{
	check(SCSEditor.IsValid());

	if(NodePtr == SCSEditor.Pin()->SceneRootNodePtr)
	{
		return TEXT("RootComponent");
	}
	else if(NodePtr->IsNative())
	{
		return TEXT("NativeComponents");
	}
	else if(NodePtr->IsInherited())
	{
		return TEXT("InheritedComponents");
	}

	return TEXT("");
}

UBlueprint* SSCS_RowWidget::GetBlueprint() const
{
	return SCSEditor.Pin()->Kismet2Ptr.Pin()->GetBlueprintObj();
}

bool SSCS_RowWidget::OnNameTextVerifyChanged(const FText& InNewText, FText& OutErrorMessage)
{
	USCS_Node* SCS_Node = NodePtr->GetSCSNode();
	if(SCS_Node != NULL && !InNewText.IsEmpty() && !SCS_Node->IsValidVariableNameString(InNewText.ToString()))
	{
		OutErrorMessage = LOCTEXT("RenameFailed_NotValid", "This name is reserved for engine use.");
		return false;
	}

	TSharedPtr<INameValidatorInterface> NameValidator = MakeShareable(new FKismetNameValidator(GetBlueprint(), NodePtr->GetVariableName()));

	EValidatorResult ValidatorResult = NameValidator->IsValid(InNewText.ToString());
	if(ValidatorResult == EValidatorResult::AlreadyInUse)
	{
		OutErrorMessage = FText::Format(LOCTEXT("RenameFailed_InUse", "{0} is in use by another variable or function!"), InNewText);
	}
	else if(ValidatorResult == EValidatorResult::EmptyName)
	{
		OutErrorMessage = LOCTEXT("RenameFailed_LeftBlank", "Names cannot be left blank!");
	}
	else if(ValidatorResult == EValidatorResult::TooLong)
	{
		OutErrorMessage = LOCTEXT("RenameFailed_NameTooLong", "Names must have fewer than 100 characters!");
	}

	if(OutErrorMessage.IsEmpty())
	{
		return true;
	}

	return false;
}

void SSCS_RowWidget::OnNameTextCommit(const FText& InNewName, ETextCommit::Type InTextCommit)
{
	const FScopedTransaction Transaction( LOCTEXT("RenameComponentVariable", "Rename Component Variable") );
	FBlueprintEditorUtils::RenameComponentMemberVariable(GetBlueprint(), NodePtr->GetSCSNode(), FName( *InNewName.ToString() ));
}

//////////////////////////////////////////////////////////////////////////
// SSCSEditor


void SSCSEditor::Construct( const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InKismet2, USimpleConstructionScript* InSCS )
{
	Kismet2Ptr = InKismet2;
	SCS = InSCS;
	DeferredRenameRequest = NULL;

	CommandList = MakeShareable( new FUICommandList );
	CommandList->MapAction( FGenericCommands::Get().Duplicate,
		FUIAction( FExecuteAction::CreateSP( this, &SSCSEditor::OnDuplicateComponent ), 
		FCanExecuteAction::CreateSP( this, &SSCSEditor::CanDuplicateComponent ) ) 
		);

	CommandList->MapAction( FGenericCommands::Get().Delete,
		FUIAction( FExecuteAction::CreateSP( this, &SSCSEditor::OnDeleteNodes ), 
		FCanExecuteAction::CreateSP( this, &SSCSEditor::CanDeleteNodes ) ) 
		);

	CommandList->MapAction( FGenericCommands::Get().Rename,
			FUIAction( FExecuteAction::CreateSP( this, &SSCSEditor::OnRenameComponent ), 
			FCanExecuteAction::CreateSP( this, &SSCSEditor::CanRenameComponent ) ) 
		);

	FSlateBrush const* MobilityHeaderBrush = FEditorStyle::GetBrush(TEXT("ClassIcon.ComponentMobilityHeaderIcon"));

	this->ChildSlot
	[
		SNew(STutorialWrapper)
		.Name(TEXT("ComponentsPanel"))
		.Content()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(ComponentClassCombo, SComponentClassCombo)
				.OnComponentClassSelected(this, &SSCSEditor::PerformComboAddClass)
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSpacer)
				.Size(FVector2D(0.0f,2.0f))
			]
			+SVerticalBox::Slot()
			.FillHeight(.8f)
			[
				SAssignNew(SCSTreeWidget, SSCSTreeType)
				.ToolTipText(LOCTEXT("DropAssetToAddComponent", "Drop asset here to add component.").ToString())
				.SCSEditor( this )
				.TreeItemsSource( &RootNodes )
				.SelectionMode(ESelectionMode::Multi)
				.OnGenerateRow( this, &SSCSEditor::MakeTableRowWidget )
				.OnGetChildren( this, &SSCSEditor::OnGetChildrenForTree )
				.OnSelectionChanged( this, &SSCSEditor::OnTreeSelectionChanged )
				.OnContextMenuOpening(this, &SSCSEditor::CreateContextMenu )
				.OnItemScrolledIntoView(this, &SSCSEditor::OnItemScrolledIntoView )
				.ItemHeight( 24 )
				.HeaderRow
				(
					SNew(SHeaderRow)
					+SHeaderRow::Column(SCS_ColumnName_Mobility)
						.DefaultLabel(LOCTEXT("MobilityColumnLabel", "Mobility").ToString())
						.FixedWidth(16.0f) // mobility icons are 16px (16 slate-units = 16px, when application scale == 1)
						.HeaderContent()
						[
							SNew(SHorizontalBox)
								.ToolTip(SNew(SToolTip).Text(LOCTEXT("MobilityColumnTooltip", "Mobility").ToString()))
							+SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								[
									SNew(SImage).Image(MobilityHeaderBrush)
								]
						]
					+SHeaderRow::Column(SCS_ColumnName_ComponentClass).DefaultLabel( LOCTEXT("Class", "Class").ToString() ).FillWidth(4)
					+SHeaderRow::Column(SCS_ColumnName_Asset).DefaultLabel( LOCTEXT("Asset", "Asset").ToString() ).FillWidth(3)
				
				)
			]
		]
	];

	// Refresh the tree widget
	UpdateTree();

	// Expand the scene root node so we show all children by default
	if(SceneRootNodePtr.IsValid())
	{
		SCSTreeWidget->SetItemExpansion(SceneRootNodePtr, true);
	}
}

FReply SSCSEditor::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	if ( CommandList->ProcessCommandBindings( InKeyboardEvent ) )
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

TSharedRef<ITableRow> SSCSEditor::MakeTableRowWidget( FSCSEditorTreeNodePtrType InNodePtr, const TSharedRef<STableViewBase>& OwnerTable )
{
	// Native components do not have a SCSNode, so ignore them
	if(InNodePtr->GetSCSNode() && DeferredRenameRequest == InNodePtr->GetSCSNode())
	{
		SCSTreeWidget->SetSelection(InNodePtr);
		OnRenameComponent();
	}

	return SNew(SSCS_RowWidget, SharedThis(this), InNodePtr, OwnerTable);
}

void SSCSEditor::GetSelectedItemsForContextMenu(TArray<FComponentEventConstructionData>& OutSelectedItems) const
{
	TArray<FSCSEditorTreeNodePtrType> SelectedTreeItems = SCSTreeWidget->GetSelectedItems();
	for ( auto NodeIter = SelectedTreeItems.CreateConstIterator(); NodeIter; ++NodeIter )
	{
		FComponentEventConstructionData NewItem;
		auto TreeNode = *NodeIter;
		NewItem.VariableName = TreeNode->GetVariableName();
		NewItem.Component = TreeNode->GetComponentTemplate();
		OutSelectedItems.Add(NewItem);
	}
}

TSharedPtr< SWidget > SSCSEditor::CreateContextMenu()
{
	TArray<FSCSEditorTreeNodePtrType> SelectedNodes = SCSTreeWidget->GetSelectedItems();

	if (SelectedNodes.Num() > 0)
	{
		const bool CloseAfterSelection = true;
		FMenuBuilder MenuBuilder( CloseAfterSelection, CommandList );

		MenuBuilder.BeginSection("ComponentActions", LOCTEXT("ComponentContextMenu", "Component Actions") );
		{
			MenuBuilder.AddMenuEntry( FGenericCommands::Get().Duplicate );
			MenuBuilder.AddMenuEntry( FGenericCommands::Get().Delete );
			MenuBuilder.AddMenuEntry( FGenericCommands::Get().Rename );

			// Collect the classes of all selected objects
			TArray<UClass*> SelectionClasses;
			for( auto NodeIter = SelectedNodes.CreateConstIterator(); NodeIter; ++NodeIter )
			{
				auto TreeNode = *NodeIter;
				if( TreeNode->GetComponentTemplate() )
				{
					SelectionClasses.Add( TreeNode->GetComponentTemplate()->GetClass() );
				}
			}

			TSharedPtr<FBlueprintEditor> PinnedBlueprintEditor = Kismet2Ptr.Pin();
			if ( PinnedBlueprintEditor.IsValid() && SelectionClasses.Num() )
			{
				// Find the common base class of all selected classes
				UClass* SelectedClass = UClass::FindCommonBase( SelectionClasses );
				// Build an event submenu if we can generate events
				if( FBlueprintEditor::CanClassGenerateEvents( SelectedClass ))
				{
					MenuBuilder.AddSubMenu(	LOCTEXT("AddEventSubMenu", "Add Event"), 
											LOCTEXT("ActtionsSubMenu_ToolTip", "Add Event"), 
											FNewMenuDelegate::CreateStatic( &SSCSEditor::BuildMenuEventsSection, PinnedBlueprintEditor, SelectedClass, 
											FGetSelectedObjectsDelegate::CreateSP(this, &SSCSEditor::GetSelectedItemsForContextMenu)));
				}
			}
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}
	return TSharedPtr<SWidget>();
}

void SSCSEditor::BuildMenuEventsSection(FMenuBuilder& Menu, TSharedPtr<FBlueprintEditor> BlueprintEditor, UClass* SelectedClass, FGetSelectedObjectsDelegate GetSelectedObjectsDelegate)
{
	// Get Selected Nodes
	TArray<FComponentEventConstructionData> SelectedNodes;
	GetSelectedObjectsDelegate.ExecuteIfBound( SelectedNodes );
	UBlueprint* Blueprint = BlueprintEditor->GetBlueprintObj();

	struct FMenuEntry
	{
		FText		Label;
		FText		ToolTip;
		FUIAction	UIAction;
	};

	TArray< FMenuEntry > Actions;
	TArray< FMenuEntry > NodeActions;
	// Build Events entries
	for (TFieldIterator<UMulticastDelegateProperty> PropertyIt(SelectedClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;

		// Check for multicast delegates that we can safely assign
		if (!Property->HasAnyPropertyFlags(CPF_Parm) && Property->HasAllPropertyFlags(CPF_BlueprintAssignable))
		{
			FName EventName = Property->GetFName();
			int32 ComponentEventViewEntries = 0;
			// Add View Event Per Component
			for (auto NodeIter = SelectedNodes.CreateConstIterator(); NodeIter; ++NodeIter )
			{
				if( NodeIter->Component.IsValid() )
				{
					FName VariableName = NodeIter->VariableName;
					UObjectProperty* VariableProperty = FindField<UObjectProperty>( Blueprint->SkeletonGeneratedClass, VariableName );

					if( VariableProperty && FKismetEditorUtilities::FindBoundEventForComponent( Blueprint, EventName, VariableProperty->GetFName() ))
					{
						FMenuEntry NewEntry;
						NewEntry.Label = ( SelectedNodes.Num() > 1 ) ?	FText::Format( LOCTEXT("ViewEvent_ToolTipFor", "{0} for {1}"), FText::FromName( EventName ), FText::FromName( VariableName )) : 
																		FText::Format( LOCTEXT("ViewEvent_ToolTip", "{0}"), FText::FromName( EventName ));
						NewEntry.UIAction =	FUIAction(FExecuteAction::CreateStatic( &SSCSEditor::ViewEvent, Blueprint, EventName, *NodeIter ),
													  FCanExecuteAction::CreateSP( BlueprintEditor.Get(), &FBlueprintEditor::InEditingMode ));
						NodeActions.Add( NewEntry );
						ComponentEventViewEntries++;
					}
				}
			}
			if( ComponentEventViewEntries < SelectedNodes.Num() )
			{
			// Create menu Add entry
				FMenuEntry NewEntry;
				NewEntry.Label = FText::Format( LOCTEXT("AddEvent_ToolTip", "Add {0}" ), FText::FromName( EventName ));
				NewEntry.UIAction =	FUIAction(FExecuteAction::CreateStatic( &SSCSEditor::CreateEventsForSelection, BlueprintEditor, EventName, GetSelectedObjectsDelegate),
											  FCanExecuteAction::CreateSP(BlueprintEditor.Get(), &FBlueprintEditor::InEditingMode));
				Actions.Add( NewEntry );
		}
	}
}
	// Build Menu Sections
	Menu.BeginSection("AddComponentActions", LOCTEXT("AddEventHeader", "Add Event"));
	for (auto ItemIter = Actions.CreateConstIterator(); ItemIter; ++ItemIter )
	{
		Menu.AddMenuEntry( ItemIter->Label, ItemIter->ToolTip, FSlateIcon(), ItemIter->UIAction );
	}
	Menu.EndSection();
	Menu.BeginSection("ViewComponentActions", LOCTEXT("ViewEventHeader", "View Existing Events"));
	for (auto ItemIter = NodeActions.CreateConstIterator(); ItemIter; ++ItemIter )
	{
		Menu.AddMenuEntry( ItemIter->Label, ItemIter->ToolTip, FSlateIcon(), ItemIter->UIAction );
	}
	Menu.EndSection();
}

void SSCSEditor::CreateEventsForSelection(TSharedPtr<FBlueprintEditor> BlueprintEditor, FName EventName, FGetSelectedObjectsDelegate GetSelectedObjectsDelegate)
{	
	if (EventName != NAME_None)
	{
		UBlueprint* Blueprint = BlueprintEditor->GetBlueprintObj();
		TArray<FComponentEventConstructionData> SelectedNodes;
		GetSelectedObjectsDelegate.ExecuteIfBound(SelectedNodes);

		for (auto SelectionIter = SelectedNodes.CreateConstIterator(); SelectionIter; ++SelectionIter)
		{
			ConstructEvent( Blueprint, EventName, *SelectionIter );
		}
	}
}

void SSCSEditor::ConstructEvent(UBlueprint* Blueprint, const FName EventName, const FComponentEventConstructionData EventData)
{
	// Find the corresponding variable property in the Blueprint
	UObjectProperty* VariableProperty = FindField<UObjectProperty>(Blueprint->SkeletonGeneratedClass, EventData.VariableName );

	if( VariableProperty )
	{
		if (!FKismetEditorUtilities::FindBoundEventForComponent(Blueprint, EventName, VariableProperty->GetFName()))
		{
			FKismetEditorUtilities::CreateNewBoundEventForComponent(EventData.Component.Get(), EventName, Blueprint, VariableProperty);
		}
	}
}

void SSCSEditor::ViewEvent(UBlueprint* Blueprint, const FName EventName, const FComponentEventConstructionData EventData)
{
	// Find the corresponding variable property in the Blueprint
	UObjectProperty* VariableProperty = FindField<UObjectProperty>(Blueprint->SkeletonGeneratedClass, EventData.VariableName );

	if( VariableProperty )
	{
		const UK2Node_ComponentBoundEvent* ExistingNode = FKismetEditorUtilities::FindBoundEventForComponent(Blueprint, EventName, VariableProperty->GetFName());
		if (ExistingNode)
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(ExistingNode);
		}
	}
}

bool SSCSEditor::CanDuplicateComponent() const
{
	if(!InEditingMode())
	{
		return false;
	}

	TArray<FSCSEditorTreeNodePtrType> SelectedNodes = SCSTreeWidget->GetSelectedItems();
	for (int32 i = 0; i < SelectedNodes.Num(); ++i)
	{
		if(SelectedNodes[i]->GetComponentTemplate() == NULL || SelectedNodes[i]->IsDefaultSceneRoot())
		{
			return false;
		}
	}
	return SelectedNodes.Num() > 0;
}

void SSCSEditor::OnDuplicateComponent()
{
	TArray<FSCSEditorTreeNodePtrType> SelectedNodes = SCSTreeWidget->GetSelectedItems();
	if(SelectedNodes.Num() > 0)
	{
		for (int32 i = 0; i < SelectedNodes.Num(); ++i)
		{
			UActorComponent* ComponentTemplate = SelectedNodes[i]->GetComponentTemplate();
			if(ComponentTemplate != NULL)
			{
				UActorComponent* CloneComponent = AddNewComponent(ComponentTemplate->GetClass(), NULL);
				UActorComponent* OriginalComponent = ComponentTemplate;

				//Serialize object properties using write/read operations.
				TArray<uint8> SavedProperties;
				FObjectWriter Writer(OriginalComponent, SavedProperties);
				FObjectReader(CloneComponent, SavedProperties);

				// If we've duplicated a scene component, attempt to reposition the duplicate in the hierarchy if the original
				// was attached to another scene component as a child. By default, the duplicate is attached to the scene root node.
				USceneComponent* NewSceneComponent = Cast<USceneComponent>(CloneComponent);
				if(NewSceneComponent != NULL)
				{
					// Ensure that any native attachment relationship inherited from the original copy is removed (to prevent a GLEO assertion)
					NewSceneComponent->DetachFromParent(true);

					// Attempt to locate the original node in the SCS tree
					FSCSEditorTreeNodePtrType OriginalNodePtr = FindTreeNode(OriginalComponent);
					if(OriginalNodePtr.IsValid())
					{
						// If the original node was parented, attempt to add the duplicate as a child of the same parent node
						FSCSEditorTreeNodePtrType ParentNodePtr = OriginalNodePtr->GetParent();
						if(ParentNodePtr.IsValid() && ParentNodePtr != SceneRootNodePtr)
						{
							// Locate the duplicate node (as a child of the current scene root node), and switch it to be a child of the original node's parent
							FSCSEditorTreeNodePtrType NewChildNodePtr = SceneRootNodePtr->FindChild(NewSceneComponent);
							if(NewChildNodePtr.IsValid())
							{
								// Note: This method will handle removal from the scene root node as well
								ParentNodePtr->AddChild(NewChildNodePtr);
							}
						}
					}
				}
			}
		}
	}
}

void SSCSEditor::OnGetChildrenForTree( FSCSEditorTreeNodePtrType InNodePtr, TArray<FSCSEditorTreeNodePtrType>& OutChildren )
{
	OutChildren.Empty();

	if(InNodePtr.IsValid())
	{
		OutChildren = InNodePtr->GetChildren();
	}
}


void SSCSEditor::PerformComboAddClass(TSubclassOf<UActorComponent> ComponentClass)
	{
	UClass* NewClass = ComponentClass;

	FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();
	USelection* Selection =  GEditor->GetSelectedObjects();

	bool bAddedComponent = false;

	// This adds components according to the type selected in the drop down. If the user
	// has the appropriate objects selected in the content browser then those are added,
	// else we go down the previous route of adding components by type.
	if ( Selection->Num() > 0 )
	{
		for(FSelectionIterator ObjectIter(*Selection);ObjectIter;++ObjectIter)
		{
			UObject* Object = *ObjectIter;
			UClass*  Class	= Object->GetClass();

			TArray< TSubclassOf<UActorComponent> > ComponentClasses = FComponentAssetBrokerage::GetComponentsForAsset(Object);

			// if the selected asset supports the selected component type then go ahead and add it
			for ( int32 ComponentIndex = 0; ComponentIndex < ComponentClasses.Num(); ComponentIndex++ )
			{
				if ( ComponentClasses[ComponentIndex]->IsChildOf( NewClass ) )
				{
					AddNewComponent( NewClass, Object );
					bAddedComponent = true;
					break;
				}
			}
		}
	}

	if ( !bAddedComponent )
	{
		// As the SCS splits up the scene and actor components, can now add directly
		AddNewComponent(ComponentClass, NULL);
	}
}






TArray<FSCSEditorTreeNodePtrType>  SSCSEditor::GetSelectedNodes() const
{
	return SCSTreeWidget->GetSelectedItems();
}

FSCSEditorTreeNodePtrType SSCSEditor::GetNodeFromActorComponent(const UActorComponent* ActorComponent, bool bIncludeAttachedComponents) const
{
	FSCSEditorTreeNodePtrType NodePtr;

	if(ActorComponent)
	{
		// If the given component instance is not already an archetype object
		if(!ActorComponent->IsTemplate())
		{
			// Get the component owner's class object
			check(ActorComponent->GetOwner() != NULL);
			UClass* OwnerClass = ActorComponent->GetOwner()->GetActorClass();

			// If the given component is one that's created during Blueprint construction
			if(ActorComponent->bCreatedByConstructionScript)
			{
				// Get the Blueprint object associated with the owner's class
				UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(OwnerClass);
				if(Blueprint && Blueprint->SimpleConstructionScript)
				{
					// Attempt to locate an SCS node with a variable name that matches the name of the given component
					TArray<USCS_Node*> AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
					for(int32 i = 0; i < AllNodes.Num(); ++i)
					{
						USCS_Node* SCS_Node = AllNodes[i];
						
						check(SCS_Node != NULL);
						if(SCS_Node->VariableName == ActorComponent->GetFName())
						{
							// We found a match; redirect to the component archetype instance that may be associated with a tree node
							ActorComponent = SCS_Node->ComponentTemplate;
							break;
						}
					}
				}
			}
			else
			{
				// Get the class default object
				const AActor* CDO = Cast<AActor>(OwnerClass->GetDefaultObject());
				if(CDO)
				{
					// Iterate over the Components array and attempt to find a component with a matching name
					TArray<UActorComponent*> Components;
					CDO->GetComponents(Components);

					for(auto It = Components.CreateConstIterator(); It; ++It)
					{
						UActorComponent* ComponentTemplate = *It;
						if(ComponentTemplate->GetFName() == ActorComponent->GetFName())
						{
							// We found a match; redirect to the component archetype instance that may be associated with a tree node
							ActorComponent = ComponentTemplate;
							break;
						}
					}
				}
			}
		}

		// If we have a valid component archetype instance, attempt to find a tree node that corresponds to it
		if(ActorComponent->IsTemplate())
		{
			for(int32 i = 0; i < RootNodes.Num() && !NodePtr.IsValid(); i++)
			{
				NodePtr = FindTreeNode(ActorComponent, RootNodes[i]);
			}
		}

		// If we didn't find it in the tree, step up the chain to the parent of the given component and recursively see if that is in the tree (unless the flag is false)
		if(!NodePtr.IsValid() && bIncludeAttachedComponents)
		{
			const USceneComponent* SceneComponent = Cast<const USceneComponent>(ActorComponent);
			if(SceneComponent && SceneComponent->AttachParent)
			{
				return GetNodeFromActorComponent(SceneComponent->AttachParent, bIncludeAttachedComponents);
			}
		}
	}

	return NodePtr;
}

void SSCSEditor::SelectNode(FSCSEditorTreeNodePtrType InNodeToSelect, bool IsCntrlDown) 
{
	if(SCSTreeWidget.IsValid() && InNodeToSelect.IsValid())
	{
		if(!IsCntrlDown)
		{
			SCSTreeWidget->SetSelection(InNodeToSelect);
		}
		else
		{
			SCSTreeWidget->SetItemSelection(InNodeToSelect, !SCSTreeWidget->IsItemSelected(InNodeToSelect));
		}
	}
}

void SSCSEditor::UpdateTree(bool bRegenerateTreeNodes)
{
	check(SCSTreeWidget.IsValid());

	if(bRegenerateTreeNodes)
	{
		// Obtain the set of expanded tree nodes
		TSet<FSCSEditorTreeNodePtrType> ExpandedTreeNodes;
		SCSTreeWidget->GetExpandedItems(ExpandedTreeNodes);

		// Obtain the list of selected items
		TArray<FSCSEditorTreeNodePtrType> SelectedTreeNodes = SCSTreeWidget->GetSelectedItems();

		// Clear the current tree
		RootNodes.Empty();

		// Reset the scene root node
		SceneRootNodePtr.Reset();

		// Get the Blueprint that's being edited
		UBlueprint* Blueprint = Kismet2Ptr.Pin()->GetBlueprintObj();

		// Get the Blueprint class default object as well as the inheritance stack
		AActor* CDO = NULL;
		TArray<UBlueprint*> ParentBPStack;
		if(Blueprint->GeneratedClass != NULL)
		{
			CDO = Blueprint->GeneratedClass->GetDefaultObject<AActor>();
			UBlueprint::GetBlueprintHierarchyFromClass(Blueprint->GeneratedClass, ParentBPStack);
		}

		if(CDO != NULL)
		{
			// Add native ActorComponent nodes to the root set first
			TArray<UActorComponent*> Components;
			CDO->GetComponents(Components);

			for(auto CompIter = Components.CreateIterator(); CompIter; ++CompIter)
			{
				UActorComponent* ActorComp = *CompIter;
				if (!ActorComp->IsA<USceneComponent>())
				{
					RootNodes.Add(MakeShareable(new FSCSEditorTreeNode(ActorComp)));
				}
			}

		// Add the native base class SceneComponent hierarchy
			for(auto CompIter = Components.CreateIterator(); CompIter; ++CompIter)
			{
				USceneComponent* SceneComp = Cast<USceneComponent>(*CompIter);
				if(SceneComp != NULL)
				{
					AddTreeNode(SceneComp);
				}
			}
		}

		// Add the full SCS tree node hierarchy (including SCS nodes inherited from parent blueprints)
		for(int32 StackIndex = ParentBPStack.Num() - 1; StackIndex >= 0; --StackIndex)
		{
			if(ParentBPStack[StackIndex]->SimpleConstructionScript != NULL)
			{
				const TArray<USCS_Node*>& SCS_RootNodes = ParentBPStack[StackIndex]->SimpleConstructionScript->GetRootNodes();
				for(int32 NodeIndex = 0; NodeIndex < SCS_RootNodes.Num(); ++NodeIndex)
				{
					USCS_Node* SCS_Node = SCS_RootNodes[NodeIndex];
					check(SCS_Node != NULL);

					if(SCS_Node->ParentComponentOrVariableName != NAME_None)
					{
						USceneComponent* ParentComponent = SCS_Node->GetParentComponentTemplate(Blueprint);
						if(ParentComponent != NULL)
						{
							FSCSEditorTreeNodePtrType ParentNodePtr = FindTreeNode(ParentComponent);
							if(ParentNodePtr.IsValid())
							{
								AddTreeNode(SCS_Node, ParentNodePtr);
							}
						}
					}
					else
					{
						AddTreeNode(SCS_Node, SceneRootNodePtr);
					}
				}
			}
		}

		// Restore the previous expansion state on the new tree nodes
		TArray<FSCSEditorTreeNodePtrType> ExpandedTreeNodeArray = ExpandedTreeNodes.Array();
		for(int i = 0; i < ExpandedTreeNodeArray.Num(); ++i)
		{
			// Look for a component match in the new hierarchy; if found, mark it as expanded to match the previous setting
			FSCSEditorTreeNodePtrType NodeToExpandPtr = FindTreeNode(ExpandedTreeNodeArray[i]->GetComponentTemplate());
			if(NodeToExpandPtr.IsValid())
			{
				SCSTreeWidget->SetItemExpansion(NodeToExpandPtr, true);
			}
		}

		// Restore the previous selection state on the new tree nodes
		for(int i = 0; i < SelectedTreeNodes.Num(); ++i)
		{
			FSCSEditorTreeNodePtrType NodeToSelectPtr = FindTreeNode(SelectedTreeNodes[i]->GetComponentTemplate());
			if(NodeToSelectPtr.IsValid())
			{
				SCSTreeWidget->SetItemSelection(NodeToSelectPtr, true);
			}
		}
	}

	// refresh widget
	SCSTreeWidget->RequestTreeRefresh();
}

void SSCSEditor::ClearSelection()
{
	check(SCSTreeWidget.IsValid());
	SCSTreeWidget->ClearSelection();
	
	Kismet2Ptr.Pin()->GetInspector()->ShowDetailsForObjects(TArray<UObject*>());
}



void SSCSEditor::SaveSCSCurrentState( USimpleConstructionScript* SCSObj )
{
	if( SCSObj )
	{
		SCSObj->Modify();

		const TArray<USCS_Node*>& SCS_RootNodes = SCSObj->GetRootNodes();
		for(int32 i = 0; i < SCS_RootNodes.Num(); ++i)
		{
			SaveSCSNode( SCS_RootNodes[i] );
		}
	}
}

void SSCSEditor::SaveSCSNode( USCS_Node* Node )
{
	if( Node )
	{
		Node->Modify();

		for( int32 i=0; i<Node->ChildNodes.Num(); i++ )
		{
			SaveSCSNode( Node->ChildNodes[i] );
		}
	}
}

bool SSCSEditor::InEditingMode() const
{
	return NULL == GEditor->PlayWorld;
}

UActorComponent* SSCSEditor::AddNewComponent( UClass* NewComponentClass, UObject* Asset  )
{
	const FScopedTransaction Transaction( LOCTEXT("AddComponent", "Add Component") );

	// Create a new template and add it to the blueprint
	UBlueprint* Blueprint = Kismet2Ptr.Pin()->GetBlueprintObj();
	Blueprint->Modify();

	SaveSCSCurrentState( Blueprint->SimpleConstructionScript );

	USCS_Node* NewNode = SCS->CreateNode(NewComponentClass);
	return AddNewNode(NewNode, Asset, true);
}

UActorComponent* SSCSEditor::AddNewNode(USCS_Node* NewNode,  UObject* Asset, bool bMarkBlueprintModified)
{
	UBlueprint* Blueprint = Kismet2Ptr.Pin()->GetBlueprintObj();
	if(Asset)
	{
		FComponentAssetBrokerage::AssignAssetToComponent(NewNode->ComponentTemplate, Asset);
	}

	FSCSEditorTreeNodePtrType NewNodePtr;

	// Reset the scene root node if it's set to the default one that's managed by the SCS
	if(SceneRootNodePtr.IsValid() && SceneRootNodePtr->GetSCSNode() == SCS->GetDefaultSceneRootNode())
	{
		SceneRootNodePtr.Reset();
	}

	// Add the new node to the editor tree
	NewNodePtr = AddTreeNode(NewNode, SceneRootNodePtr);

	// Will call UpdateTree as part of OnBlueprintChanged handling
	if(bMarkBlueprintModified)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}

	// Potentially adjust variable names for any child blueprints
	if(NewNode->VariableName != NAME_None)
	{
		FBlueprintEditorUtils::ValidateBlueprintChildVariables(Blueprint, NewNode->VariableName);
	}
	
	// Select and request a rename on the new component
	SCSTreeWidget->SetSelection(NewNodePtr);
	OnRenameComponent();

	return NewNode->ComponentTemplate;
}

bool SSCSEditor::IsComponentSelected(const UPrimitiveComponent* PrimComponent) const
{
	check(PrimComponent != NULL);

	FSCSEditorTreeNodePtrType NodePtr = GetNodeFromActorComponent(PrimComponent);
	if(NodePtr.IsValid() && SCSTreeWidget.IsValid())
	{
		return SCSTreeWidget->IsItemSelected(NodePtr);
	}

	return false;
}

void SSCSEditor::SetSelectionOverride(UPrimitiveComponent* PrimComponent) const
{
	PrimComponent->SelectionOverrideDelegate = UPrimitiveComponent::FSelectionOverride::CreateSP(this, &SSCSEditor::IsComponentSelected);
	PrimComponent->PushSelectionToProxy();
}

bool SSCSEditor::CanDeleteNodes() const
{
	if(!InEditingMode())
	{
		return false;
	}

	TArray<FSCSEditorTreeNodePtrType> SelectedNodes = SCSTreeWidget->GetSelectedItems();
	for (int32 i = 0; i < SelectedNodes.Num(); ++i)
	{
		if (!SelectedNodes[i]->CanDelete()) {return false;}
	}
	return SelectedNodes.Num() > 0;
}

void SSCSEditor::OnDeleteNodes()
{
	const FScopedTransaction Transaction( LOCTEXT("RemoveComponent", "Remove Component") );

	UBlueprint* Blueprint = Kismet2Ptr.Pin()->GetBlueprintObj();

	// Get the current render info for the blueprint. If this is NULL then the blueprint is not currently visualizable (no visible primitive components)
	FThumbnailRenderingInfo* RenderInfo = GUnrealEd->GetThumbnailManager()->GetRenderingInfo( Blueprint );

	// Saving objects for restoring purpose.
	Blueprint->Modify();
	SSCSEditor::SaveSCSCurrentState( Blueprint->SimpleConstructionScript );

	// Remove node from SCS
	TArray<FSCSEditorTreeNodePtrType> SelectedNodes = SCSTreeWidget->GetSelectedItems();
	for (int32 i = 0; i < SelectedNodes.Num(); ++i)
	{
		auto Node = SelectedNodes[i];

		RemoveComponentNode(Node);
	}

	// Will call UpdateTree as part of OnBlueprintChanged handling
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	// Do this AFTER marking the Blueprint as modified
	UpdateSelectionFromNodes(SCSTreeWidget->GetSelectedItems());

	// If we had a thumbnail before we deleted any components, check to see if we should clear it
	// If we deleted the final visualizable primitive from the blueprint, GetRenderingInfo should return NULL
	FThumbnailRenderingInfo* NewRenderInfo = GUnrealEd->GetThumbnailManager()->GetRenderingInfo( Blueprint );
	if ( RenderInfo && !NewRenderInfo )
	{
		// We removed the last visible primitive component, clear the thumbnail
		const FString BPFullName = FString::Printf(TEXT("%s %s"), *Blueprint->GetClass()->GetName(), *Blueprint->GetPathName());
		UPackage* BPPackage = Blueprint->GetOutermost();
		ThumbnailTools::CacheEmptyThumbnail( BPFullName, BPPackage );
	}
}

void SSCSEditor::RemoveComponentNode(FSCSEditorTreeNodePtrType InNodePtr)
{
	check(InNodePtr.IsValid());

	UBlueprint* Blueprint = Kismet2Ptr.Pin()->GetBlueprintObj();

	// Remove any instances of variable accessors from the blueprint graphs
	FBlueprintEditorUtils::RemoveVariableNodes(Blueprint, InNodePtr->GetVariableName());

	// Clear selection if current
	if(SCSTreeWidget->GetSelectedItems().Contains(InNodePtr))
	{
		SCSTreeWidget->ClearSelection();
	}

	USCS_Node* SCS_Node = InNodePtr->GetSCSNode();
	if(SCS_Node != NULL)
	{
		// Remove node from SCS tree
		check(SCS_Node->GetSCS() == SCS);
		SCS->RemoveNodeAndPromoteChildren(SCS_Node);

		// Clear the delegate
		SCS_Node->SetOnNameChanged(FSCSNodeNameChanged());
	}
}

void SSCSEditor::UpdateSelectionFromNodes(const TArray<FSCSEditorTreeNodePtrType> &SelectedNodes )
{
	// Convert the selection set to an array of UObject* pointers
	FString InspectorTitle;
	TArray<UObject*> InspectorObjects;
	InspectorObjects.Empty(SelectedNodes.Num());
	for (auto NodeIt = SelectedNodes.CreateConstIterator(); NodeIt; ++NodeIt)
	{
		auto NodePtr = *NodeIt;
		if(NodePtr.IsValid() && NodePtr->CanEditDefaults())
		{
			InspectorTitle = NodePtr->GetDisplayString();
			InspectorObjects.Add(NodePtr->GetComponentTemplate());
		}
	}

	// Update the details panel
	SKismetInspector::FShowDetailsOptions Options(InspectorTitle, true);
	Kismet2Ptr.Pin()->GetInspector()->ShowDetailsForObjects(InspectorObjects, Options);
}

void SSCSEditor::RefreshSelectionDetails()
{
	UpdateSelectionFromNodes(SCSTreeWidget->GetSelectedItems());
}

void SSCSEditor::OnTreeSelectionChanged(FSCSEditorTreeNodePtrType, ESelectInfo::Type /*SelectInfo*/)
{
	UpdateSelectionFromNodes(SCSTreeWidget->GetSelectedItems());

	TSharedPtr<class SSCSEditorViewport> ViewportPtr = Kismet2Ptr.Pin()->GetSCSViewport();
	if (ViewportPtr.IsValid())
	{
		ViewportPtr->OnComponentSelectionChanged();
	}

	// Update the selection visualization
	AActor* EditorActorInstance = SCS->GetComponentEditorActorInstance();
	if (EditorActorInstance != NULL)
	{
		TArray<UPrimitiveComponent*> PrimitiveComponents;
		EditorActorInstance->GetComponents(PrimitiveComponents);

		for (int32 Idx = 0; Idx < PrimitiveComponents.Num(); ++Idx)
		{
			PrimitiveComponents[Idx]->PushSelectionToProxy();
		}
	}
}

bool SSCSEditor::IsNodeInSimpleConstructionScript( USCS_Node* Node ) const
{
	check(Node);

	USimpleConstructionScript* NodeSCS = Node->GetSCS();
	if(NodeSCS != NULL)
	{
		return NodeSCS->GetAllNodes().Contains(Node);
	}
	
	return false;
}

FSCSEditorTreeNodePtrType SSCSEditor::AddTreeNode(USCS_Node* InSCSNode, FSCSEditorTreeNodePtrType InParentNodePtr)
{
	FSCSEditorTreeNodePtrType NewNodePtr;

	check(InSCSNode != NULL);
	check(InSCSNode->ComponentTemplate != NULL);
	checkf(InSCSNode->ParentComponentOrVariableName == NAME_None
		|| (!InSCSNode->bIsParentComponentNative && InParentNodePtr->GetSCSNode() != NULL && InParentNodePtr->GetSCSNode()->VariableName == InSCSNode->ParentComponentOrVariableName)
		|| (InSCSNode->bIsParentComponentNative && InParentNodePtr->GetComponentTemplate() != NULL && InParentNodePtr->GetComponentTemplate()->GetFName() == InSCSNode->ParentComponentOrVariableName),
			TEXT("Failed to add SCS node %s to tree:\n- bIsParentComponentNative=%d\n- Stored ParentComponentOrVariableName=%s\n- Actual ParentComponentOrVariableName=%s"),
				*InSCSNode->VariableName.ToString(),
				!!InSCSNode->bIsParentComponentNative,
				*InSCSNode->ParentComponentOrVariableName.ToString(),
				!InSCSNode->bIsParentComponentNative
					? (InParentNodePtr->GetSCSNode() != NULL ? *InParentNodePtr->GetSCSNode()->VariableName.ToString() : TEXT("NULL"))
					: (InParentNodePtr->GetComponentTemplate() != NULL ? *InParentNodePtr->GetComponentTemplate()->GetFName().ToString() : TEXT("NULL")));
	
	// Determine whether or not the given node is inherited from a parent Blueprint
	USimpleConstructionScript* NodeSCS = InSCSNode->GetSCS();
	const bool bIsInherited = NodeSCS != Kismet2Ptr.Pin()->GetBlueprintObj()->SimpleConstructionScript;

	if(InSCSNode->ComponentTemplate->IsA(USceneComponent::StaticClass()))
	{
		FSCSEditorTreeNodePtrType ParentPtr = InParentNodePtr.IsValid() ? InParentNodePtr : SceneRootNodePtr;
		if(ParentPtr.IsValid())
		{
			// do this first, because we need a FSCSEditorTreeNodePtrType for the new node
			NewNodePtr = ParentPtr->AddChild(InSCSNode, bIsInherited);

			bool bParentIsEditorOnly = ParentPtr->GetComponentTemplate()->IsEditorOnly();
			// if you can't nest this new node under the proposed parent (then swap the two)
			if (bParentIsEditorOnly && !InSCSNode->ComponentTemplate->IsEditorOnly() && ParentPtr->CanReparent())
			{
				FSCSEditorTreeNodePtrType OldParentPtr = ParentPtr;
				ParentPtr = OldParentPtr->GetParent();

				OldParentPtr->RemoveChild(NewNodePtr);
				NodeSCS->RemoveNode(OldParentPtr->GetSCSNode());

				// if the grandparent node is invalid (assuming this means that the parent node was the scene-root)
				if (!ParentPtr.IsValid())
				{
					check(OldParentPtr == SceneRootNodePtr);
					SceneRootNodePtr = NewNodePtr;
					NodeSCS->AddNode(SceneRootNodePtr->GetSCSNode());
				}
				else 
				{
					ParentPtr->AddChild(NewNodePtr);
				}

				// move the proposed parent in as a child to the new node
				NewNodePtr->AddChild(OldParentPtr);
			} // if bParentIsEditorOnly...
		}
		//else, if !SceneRootNodePtr.IsValid(), make it the scene root node if it has not been set yet
		else 
		{
			// Create a new root node
			NewNodePtr = MakeShareable(new FSCSEditorTreeNode(InSCSNode, bIsInherited));

			// Add it to the root set
			NodeSCS->AddNode(InSCSNode);
			RootNodes.Add(NewNodePtr);

			// Make it the scene root node
			SceneRootNodePtr = NewNodePtr;

			// Expand the scene root node by default
			SCSTreeWidget->SetItemExpansion(SceneRootNodePtr, true);
		}
	}
	else
	{
		// If the given SCS node does not contain a scene component template, we create a new root node
		NewNodePtr = MakeShareable(new FSCSEditorTreeNode(InSCSNode, bIsInherited));

		// Add it to the root set prior to the scene root node that represents the start of the scene hierarchy
		int32 SceneRootNodeIndex = RootNodes.Find(SceneRootNodePtr);
		if(SceneRootNodeIndex != INDEX_NONE)
		{
			RootNodes.Insert(NewNodePtr, SceneRootNodeIndex);
		}
		else
		{
			RootNodes.Add(NewNodePtr);
		}

		// If the SCS root node array does not already contain the given node, this will add it (this should only occur after node creation)
		if(NodeSCS != NULL)
		{
			NodeSCS->AddNode(InSCSNode);
		}
	}

	// Recursively add the given SCS node's child nodes
	for(int32 NodeIndex = 0; NodeIndex < InSCSNode->ChildNodes.Num(); ++NodeIndex)
	{
		AddTreeNode(InSCSNode->ChildNodes[NodeIndex], NewNodePtr);
	}

	return NewNodePtr;
}

FSCSEditorTreeNodePtrType SSCSEditor::AddTreeNode(USceneComponent* InSceneComponent)
{
	FSCSEditorTreeNodePtrType NewNodePtr;

	check(InSceneComponent != NULL);

	// If the given component has a parent
	if(InSceneComponent->AttachParent != NULL)
	{
		// Attempt to find the parent node in the current tree
		FSCSEditorTreeNodePtrType ParentNodePtr = FindTreeNode(InSceneComponent->AttachParent);
		if(!ParentNodePtr.IsValid())
		{
			// Recursively add the parent node to the tree if it does not exist yet
			ParentNodePtr = AddTreeNode(InSceneComponent->AttachParent);
		}

		// Add a new tree node for the given scene component
		check(ParentNodePtr.IsValid());
		NewNodePtr = ParentNodePtr->AddChild(InSceneComponent);
	}
	else
	{
		// Make it the scene root node if it has not been set yet
		if(!SceneRootNodePtr.IsValid())
		{
			// Create a new root node
			NewNodePtr = MakeShareable(new FSCSEditorTreeNode(InSceneComponent));

			// Add it to the root set
			RootNodes.Add(NewNodePtr);

			// Make it the scene root node
			SceneRootNodePtr = NewNodePtr;

			// Expand the scene root node by default
			SCSTreeWidget->SetItemExpansion(SceneRootNodePtr, true);
		}
		else
		{
			NewNodePtr = SceneRootNodePtr->AddChild(InSceneComponent);
		}
	}

	return NewNodePtr;
}

FSCSEditorTreeNodePtrType SSCSEditor::FindTreeNode(const USCS_Node* InSCSNode, FSCSEditorTreeNodePtrType InStartNodePtr) const
{
	FSCSEditorTreeNodePtrType NodePtr;
	if(InSCSNode != NULL)
	{
		// Start at the scene root node if none was given
		if(!InStartNodePtr.IsValid())
		{
			InStartNodePtr = SceneRootNodePtr;
		}

		if(InStartNodePtr.IsValid())
		{
			// Check to see if the given SCS node matches the given tree node
			if(InStartNodePtr->GetSCSNode() == InSCSNode)
			{
				NodePtr = InStartNodePtr;
			}
			else
			{
				// Recursively search for the node in our child set
				NodePtr = InStartNodePtr->FindChild(InSCSNode);
				if(!NodePtr.IsValid())
				{
					for(int32 i = 0; i < InStartNodePtr->GetChildren().Num() && !NodePtr.IsValid(); ++i)
					{
						NodePtr = FindTreeNode(InSCSNode, InStartNodePtr->GetChildren()[i]);
					}
				}
			}
		}
	}

	return NodePtr;
}

FSCSEditorTreeNodePtrType SSCSEditor::FindTreeNode(const UActorComponent* InComponent, FSCSEditorTreeNodePtrType InStartNodePtr) const
{
	FSCSEditorTreeNodePtrType NodePtr;
	if(InComponent != NULL)
	{
		// Start at the scene root node if none was given
		if(!InStartNodePtr.IsValid())
		{
			InStartNodePtr = SceneRootNodePtr;
		}

		if(InStartNodePtr.IsValid())
		{
			// Check to see if the given component template matches the given tree node
			if(InStartNodePtr->GetComponentTemplate() == InComponent)
			{
				NodePtr = InStartNodePtr;
			}
			else
			{
				// Recursively search for the node in our child set
				NodePtr = InStartNodePtr->FindChild(InComponent);
				if(!NodePtr.IsValid())
				{
					for(int32 i = 0; i < InStartNodePtr->GetChildren().Num() && !NodePtr.IsValid(); ++i)
					{
						NodePtr = FindTreeNode(InComponent, InStartNodePtr->GetChildren()[i]);
					}
				}
			}
		}
	}

	return NodePtr;
}

void SSCSEditor::OnItemScrolledIntoView( FSCSEditorTreeNodePtrType InItem, const TSharedPtr<ITableRow>& InWidget)
{
	if( DeferredRenameRequest == InItem->GetSCSNode() )
	{
		DeferredRenameRequest = NULL;
		InItem->OnRequestRename();
	}
}

void SSCSEditor::OnRenameComponent()
{
	TArray< FSCSEditorTreeNodePtrType > SelectedItems = SCSTreeWidget->GetSelectedItems();

	// Should already be prevented from making it here.
	check(SelectedItems.Num() == 1);

	SCSTreeWidget->RequestScrollIntoView(SelectedItems[0]);
	DeferredRenameRequest = SelectedItems[0]->GetSCSNode();
}

bool SSCSEditor::CanRenameComponent() const
{
	return SCSTreeWidget->GetSelectedItems().Num() == 1 && SCSTreeWidget->GetSelectedItems()[0]->CanRename();
}


#undef LOCTEXT_NAMESPACE

