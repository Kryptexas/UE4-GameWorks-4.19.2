// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once

typedef TSharedPtr<class FComponentTreeNode> FComponentTreeNodePtrType;
typedef STreeView<FComponentTreeNodePtrType> SComponentTreeType;

//////////////////////////////////////////////////////////////////////////

class FComponentTreeNode : public TSharedFromThis<FComponentTreeNode>
{
public:
	FComponentTreeNode(UActorComponent* InComponent);

	TWeakObjectPtr<UActorComponent>	Component;

	TArray<FComponentTreeNodePtrType> ChildNodes;
};

//////////////////////////////////////////////////////////////////////////

class SComponentRowWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SComponentRowWidget ){}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, FComponentTreeNodePtrType InNodePtr );

protected:
	/** Pointer to node we represent */
	TWeakPtr<FComponentTreeNode> NodePtr;
};

//////////////////////////////////////////////////////////////////////////

class SComponentsTree : public SCompoundWidget
{

public:

	SLATE_BEGIN_ARGS( SComponentsTree ){}

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TSharedPtr<class IDetailsView> InPropertyView );

	~SComponentsTree();

	void UpdateTree();

	void SetObjects(const TArray<UObject*>& InObjects);

	// Tree delegates
	TSharedRef<ITableRow> MakeTableRowWidget( FComponentTreeNodePtrType InNodePtr, const TSharedRef<STableViewBase>& OwnerTable );
	void OnGetChildrenForTree( FComponentTreeNodePtrType InNodePtr, TArray<FComponentTreeNodePtrType>& OutChildren );
	void OnTreeSelectionChanged(FComponentTreeNodePtrType InSelectedNodePtr, ESelectInfo::Type SelectInfo);

	void OnSelectedCompClass(TSubclassOf<UActorComponent> CompClass);

private:
	void OnEditorSelectionChanged(UObject* Object);
	FComponentTreeNodePtrType GetNodeFromActorComponent(UActorComponent* ActorComponent, bool bIncludeAttachedComponents = true);
	FComponentTreeNodePtrType FindTreeNodeRecursive(const UActorComponent* InComponent, const FComponentTreeNodePtrType& NodePtr) const;

	TWeakObjectPtr<AActor>				Actor;

	TArray<FComponentTreeNodePtrType>	RootNodes;

	TSharedPtr<SComponentTreeType>		TreeWidget;

	TSharedPtr<class IDetailsView>		PropertyView;

	bool bSelectionGuard;
};