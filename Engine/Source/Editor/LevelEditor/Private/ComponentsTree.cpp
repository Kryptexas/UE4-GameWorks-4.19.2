// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LevelEditor.h"
#include "ComponentsTree.h"
#include "ScopedTransaction.h"
#include "Editor/UnrealEd/Public/SComponentClassCombo.h"
#include "Editor/UnrealEd/Public/ClassIconFinder.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"

//////////////////////////////////////////////////////////////////////////

FComponentTreeNode::FComponentTreeNode(UActorComponent* InComponent)
{
	Component = InComponent;

	USceneComponent* SceneComp = Cast<USceneComponent>(InComponent);
	if(SceneComp != NULL)
	{
		for(int32 ChildIdx=0; ChildIdx<SceneComp->AttachChildren.Num(); ChildIdx++)
		{
			USceneComponent* ChildComp = SceneComp->AttachChildren[ChildIdx];
			if(ChildComp && !ChildComp->IsPendingKill())
			{
				FComponentTreeNodePtrType ChildNode = MakeShareable(new FComponentTreeNode(ChildComp));
				ChildNodes.Add(ChildNode);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////

void SComponentRowWidget::Construct( const FArguments& InArgs, FComponentTreeNodePtrType InNodePtr )
{
	NodePtr = InNodePtr;

	FString ComponentName;
	const FSlateBrush* ComponentIcon = FEditorStyle::GetBrush("SCS.NativeComponent");
	FSlateColor ComponentColor = FLinearColor::White;

	TSharedPtr<FComponentTreeNode> Node = NodePtr.Pin();
	if(Node.IsValid() && Node->Component.IsValid())
	{
		ComponentName = Node->Component->GetName();
		ComponentIcon = FClassIconFinder::FindIconForClass( Node->Component->GetClass(), TEXT("SCS.Component") );

		// Color native and BP-made components in different colors
		if(Node->Component->IsDefaultSubobject())
		{
			ComponentColor = FLinearColor(0.08f,0.15f,0.6f);
		}
		else if(Node->Component->bCreatedByConstructionScript)
		{
			ComponentColor = FLinearColor(0.08f,0.35f,0.6f);
		}
	}

	this->ChildSlot
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(ComponentIcon)
			.ColorAndOpacity(ComponentColor)
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4,0,4,0)
		[
			SNew(STextBlock)
			.Text(ComponentName)
		]
	];
}

//////////////////////////////////////////////////////////////////////////

// Don't show any 'disable on instance' properties, these are instances of components we are seeing
static bool IsPropertyVisible( const FPropertyAndParent& PropertyAndParent )
{
	if ( PropertyAndParent.Property.HasAllPropertyFlags(CPF_DisableEditOnInstance) )
	{
		return false;
	}

	return true;
}

void SComponentsTree::Construct(const FArguments& InArgs, TSharedPtr<class IDetailsView> InPropertyView )
{
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs( /*bUpdateFromSelection=*/ false, /*bLockable=*/ false, /*bAllowSearch=*/ true, /*bObjectsUseNameArea=*/ true, /*bHideSelectionTip=*/ true );
	DetailsViewArgs.bHideActorNameArea = true;

	PropertyView = InPropertyView;

	this->ChildSlot
	[
		SNew(SBorder)
		.Padding( 2 )
		.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
		.Content()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 2.f, 0.f, 0.f)
			[
				SNew(SComponentClassCombo)
				.OnComponentClassSelected(this, &SComponentsTree::OnSelectedCompClass)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(TreeWidget, SComponentTreeType)
				.TreeItemsSource( &RootNodes )
				.OnGenerateRow( this, &SComponentsTree::MakeTableRowWidget )
				.OnGetChildren( this, &SComponentsTree::OnGetChildrenForTree )
				.OnSelectionChanged( this, &SComponentsTree::OnTreeSelectionChanged )
				.ItemHeight( 24 )
			]
		]
	];

	UpdateTree();
}

void SComponentsTree::SetObjects(const TArray<UObject*>& InObjects)
{
	if (InObjects.Num() == 1)
	{
		Actor = Cast<AActor>(InObjects[0]);
	}
	else
	{
		Actor = NULL;
	}
	Visibility = (Actor.IsValid() ? EVisibility::Visible : EVisibility::Hidden);
	UpdateTree();
}

TSharedRef<ITableRow> SComponentsTree::MakeTableRowWidget( FComponentTreeNodePtrType InNodePtr, const TSharedRef<STableViewBase>& OwnerTable )
{
	return 
		SNew(STableRow<FComponentTreeNodePtrType>, OwnerTable)
		[
			SNew(SComponentRowWidget, InNodePtr)
		];
}

void SComponentsTree::OnGetChildrenForTree( FComponentTreeNodePtrType InNodePtr, TArray<FComponentTreeNodePtrType>& OutChildren )
{
	if(InNodePtr.IsValid())
	{
		OutChildren = InNodePtr->ChildNodes;
	}
}

void SComponentsTree::OnTreeSelectionChanged(FComponentTreeNodePtrType InSelectedNodePtr, ESelectInfo::Type SelectInfo)
{
	TArray<UObject*> Objects;

	USelection* SelectedComponents = GEditor->GetSelectedComponents();
	SelectedComponents->BeginBatchSelectOperation();
	SelectedComponents->DeselectAll();

	for (FComponentTreeNodePtrType SelectedNode : TreeWidget->GetSelectedItems())
	{
		if (SelectedNode.IsValid() && SelectedNode->Component.IsValid())
		{
			UActorComponent* Component = SelectedNode->Component.Get();
			Objects.Add(Component);
			SelectedComponents->Select(Component);
		}
	}
	
	SelectedComponents->EndBatchSelectOperation();
	PropertyView->SetObjects(Objects, false);

	GEditor->RedrawLevelEditingViewports();
}

void SComponentsTree::OnSelectedCompClass(TSubclassOf<UActorComponent> CompClass)
{
	if(Actor.IsValid())
	{
		const FScopedTransaction Transaction(NSLOCTEXT("Editor", "UndoAction_AddComponent", "Add Component"));

		Actor->Modify();

		// Create new component
		UActorComponent* NewComp = ConstructObject<UActorComponent>(CompClass, Actor.Get(), NAME_None, RF_Transactional);
		check(NewComp);

		// Add to SerializedComponents array so it gets saved
		Actor->SerializedComponents.Add(NewComp);

		// If a scene component, and actor has a root component, attach it to root
		USceneComponent* NewSceneComp = Cast<USceneComponent>(NewComp);
		USceneComponent* RootComp = Actor->GetRootComponent();
		if(NewSceneComp && RootComp)
		{
			NewSceneComp->AttachTo(RootComp);
		}

		// Register component
		NewComp->RegisterComponent();

		// Update tree to see result
		UpdateTree();
	}
}


void SComponentsTree::UpdateTree()
{
	check(TreeWidget.IsValid());

	RootNodes.Empty();

	if(Actor.IsValid())
	{
		// Add root component (which will add its children)
		USceneComponent* RootComp = Actor->GetRootComponent();
		if(RootComp && !RootComp->IsPendingKill())
		{
			FComponentTreeNodePtrType RootNode = MakeShareable(new FComponentTreeNode(RootComp));
			RootNodes.Add(RootNode);
		}

		// Add non-scene components in Components array
		TInlineComponentArray<UActorComponent*> Components;
		Actor->GetComponents(Components);

		for(int32 CompIdx=0; CompIdx<Components.Num(); CompIdx++)
		{
			UActorComponent* Comp = Components[CompIdx];
			if(!Comp->IsA(USceneComponent::StaticClass()) && !Comp->IsPendingKill())
			{
				FComponentTreeNodePtrType Node = MakeShareable(new FComponentTreeNode(Comp));
				RootNodes.Add(Node);
			}
		}
	}

	// refresh widget
	for (const FComponentTreeNodePtrType& RootNode : RootNodes)
	{
		TreeWidget->SetItemExpansion(RootNode, true);
	}
	TreeWidget->RequestTreeRefresh();
}
