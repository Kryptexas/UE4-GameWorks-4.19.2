// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "DetailTreeNode.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SDetailsViewBase.h"
#include "SDetailTableRowBase.h"

class IDetailKeyframeHandler;
struct FDetailLayoutCustomization;

class SConstrainedBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SConstrainedBox)
		: _MinWidth()
		, _MaxWidth()
	{}
	SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ATTRIBUTE(TOptional<float>, MinWidth)
		SLATE_ATTRIBUTE(TOptional<float>, MaxWidth)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
private:
	TAttribute< TOptional<float> > MinWidth;
	TAttribute< TOptional<float> > MaxWidth;
};

/**
 * A widget for details that span the entire tree row and have no columns                                                              
 */
class SDetailSingleItemRow : public SDetailTableRowBase
{
public:
	SLATE_BEGIN_ARGS( SDetailSingleItemRow )
		: _ColumnSizeData() {}

		SLATE_ARGUMENT( FDetailColumnSizeData, ColumnSizeData )
		SLATE_ARGUMENT( bool, AllowFavoriteSystem)
	SLATE_END_ARGS()

	/**
	 * Construct the widget
	 */
	void Construct( const FArguments& InArgs, FDetailLayoutCustomization* InCustomization, bool bHasMultipleColumns, TSharedRef<FDetailTreeNode> InOwnerTreeNode, const TSharedRef<STableViewBase>& InOwnerTableView );

protected:
	virtual bool OnContextMenuOpening( FMenuBuilder& MenuBuilder ) override;
private:
	void OnLeftColumnResized( float InNewWidth );
	void OnCopyProperty();
	void OnPasteProperty();
	bool CanPasteProperty() const;
	const FSlateBrush* GetBorderImage() const;
	TSharedRef<SWidget> CreateExtensionWidget( TSharedRef<SWidget> ValueWidget, FDetailLayoutCustomization& InCustomization, TSharedRef<FDetailTreeNode> InTreeNode );
	TSharedRef<SWidget> CreateKeyframeButton( FDetailLayoutCustomization& InCustomization, TSharedRef<FDetailTreeNode> InTreeNode );
	bool IsKeyframeButtonEnabled(TSharedRef<FDetailTreeNode> InTreeNode) const;
	FReply OnAddKeyframeClicked();
	bool IsHighlighted() const;

	const FSlateBrush* GetFavoriteButtonBrush() const;
	FReply OnFavoriteToggle();
	void AllowShowFavorite();
private:
	TWeakPtr<IDetailKeyframeHandler> KeyframeHandler;
	/** Customization for this widget */
	FDetailLayoutCustomization* Customization;
	FDetailColumnSizeData ColumnSizeData;
	bool bAllowFavoriteSystem;
};
