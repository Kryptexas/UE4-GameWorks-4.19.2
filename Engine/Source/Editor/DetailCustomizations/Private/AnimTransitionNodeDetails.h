// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PreviewScene.h"
#include "AnimGraphDefinitions.h"

class FAnimTransitionNodeDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;

protected:

	bool NameIsNone(TSharedPtr<IPropertyHandle> Property) const;
	bool ObjIsNULL(TSharedPtr<IPropertyHandle> Property) const;

	void CreateTransitionEventPropertyWidgets(IDetailCategoryBuilder& TransitionCategory, FString TransitionName);

	FReply OnClickEditBlendGraph();
	EVisibility GetBlendGraphButtonVisibility() const;

	TSharedRef<SWidget> GetWidgetForInlineShareMenu(FString TypeName, FString SharedName, bool bIsCurrentlyShared, FOnClicked PromoteClick, FOnClicked DemoteClick, FOnGetContent GetContentMenu);

	FReply OnPromoteToSharedClick(bool RuleShare);
	void PromoteToShared(const FText& NewTransitionName, ETextCommit::Type CommitInfo, bool bRuleShare);
	FReply OnUnshareClick(bool bUnshareRule);
	TSharedRef<SWidget> OnGetShareableNodesMenu(bool bShareRules);
	void BecomeSharedWith(UAnimStateTransitionNode* NewNode, bool bShareRules);
	void AssignUniqueColorsToAllSharedNodes(UEdGraph* CurrentGraph);


private:
	TWeakObjectPtr<UAnimStateTransitionNode> TransitionNode;
	TSharedPtr<STextEntryPopup> TextEntryWidget;
};

