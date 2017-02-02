// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/DragAndDrop.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "EditorStyleSet.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "DragAndDrop/DecoratedDragDropOp.h"

class FAssetPathDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FAssetPathDragDropOp, FDecoratedDragDropOp)

	/** Data for the asset this item represents */
	TArray<FString> PathNames;

	static TSharedRef<FAssetPathDragDropOp> New(const TArray<FString>& InPathNames)
	{
		TSharedRef<FAssetPathDragDropOp> Operation = MakeShareable(new FAssetPathDragDropOp);
		
		Operation->MouseCursor = EMouseCursor::GrabHandClosed;
		Operation->PathNames = InPathNames;
		Operation->Construct();

		return Operation;
	}
	
public:
	FText GetDecoratorText() const
	{
		if ( CurrentHoverText.IsEmpty() && PathNames.Num() > 0 )
		{
			return (PathNames.Num() == 1)
				? FText::FromString(PathNames[0])
				: FText::Format(NSLOCTEXT("ContentBrowser", "FolderDescriptionMulti", "{0} and {1} {1}|plural(one=other,other=others)"), FText::FromString(PathNames[0]), PathNames.Num() - 1);
		}

		return CurrentHoverText;
	}

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override
	{
		return 
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ContentBrowser.AssetDragDropTooltipBackground"))
			.Content()
			[
				SNew(SHorizontalBox)

				// Left slot is folder icon
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderClosed"))
				]

				// Right slot is for tooltip
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(3.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SImage) 
						.Image(this, &FAssetPathDragDropOp::GetIcon)
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0,0,3,0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock) 
						.Text(this, &FAssetPathDragDropOp::GetDecoratorText)
					]
				]
			];
	}
};
