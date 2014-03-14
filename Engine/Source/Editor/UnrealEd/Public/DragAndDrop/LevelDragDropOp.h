// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class FLevelDragDropOp : public FDragDropOperation
{
public:
	static FString GetTypeId() {static FString Type = TEXT("FLevelDragDropOp"); return Type;}

	/** The levels to be dropped. */
	TArray<TWeakObjectPtr<ULevel>> LevelsToDrop;
		
	/** The streaming levels to be dropped. */
	TArray<TWeakObjectPtr<ULevelStreaming>> StreamingLevelsToDrop;

	/** Whether content is good to drop on current site, used by decorator */
	bool bGoodToDrop;
		
	/** The widget decorator to use */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE
	{
		FString LevelName(TEXT("None"));
		
		if (LevelsToDrop.Num())
		{
			if (LevelsToDrop[0].IsValid())
			{
				LevelName = LevelsToDrop[0]->GetOutermost()->GetName();
			}
		}
		else if (StreamingLevelsToDrop.Num())
		{
			if (StreamingLevelsToDrop[0].IsValid())
			{
				LevelName = StreamingLevelsToDrop[0]->PackageName.ToString();
			}
		}
		
		return SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Graph.ConnectorFeedback.Border"))
			.Content()
			[			
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
					[
						SNew(STextBlock) 
						.Text( LevelName )
					]
			];
	}

	static TSharedRef<FLevelDragDropOp> New(const TArray<TWeakObjectPtr<ULevelStreaming>>& LevelsToDrop)
	{
		TSharedRef<FLevelDragDropOp> Operation = MakeShareable(new FLevelDragDropOp);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FLevelDragDropOp>(Operation);
		Operation->StreamingLevelsToDrop.Append(LevelsToDrop);
		Operation->bGoodToDrop = true;
		Operation->Construct();
		return Operation;
	}

	static TSharedRef<FLevelDragDropOp> New(const TArray<TWeakObjectPtr<ULevel>>& LevelsToDrop)
	{
		TSharedRef<FLevelDragDropOp> Operation = MakeShareable(new FLevelDragDropOp);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FLevelDragDropOp>(Operation);
		Operation->LevelsToDrop.Append(LevelsToDrop);
		Operation->bGoodToDrop = true;
		Operation->Construct();
		return Operation;
	}
};
