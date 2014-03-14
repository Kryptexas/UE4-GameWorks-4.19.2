// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class FClassDragDropOp : public FDragDropOperation
{
public:
	static FString GetTypeId() {static FString Type = TEXT("FClassDragDropOp"); return Type;}

	/** The classes to be dropped. */
	TArray< TWeakObjectPtr<UClass> > ClassesToDrop;
	
	/** The widget decorator to use */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE
	{
		// Just use the first class for the cursor decorator.
		const FSlateBrush* ClassIcon = FEditorStyle::GetBrush(*FString::Printf( TEXT( "ClassIcon.%s" ), *ClassesToDrop[0]->GetName() ) );

		// If the class icon is the default brush, do not put it in the cursor decoration window.
		if(ClassIcon != FEditorStyle::GetDefaultBrush())
		{
			return SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("Graph.ConnectorFeedback.Border"))
				.Content()
				[			
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew( SImage )
						.Image( ClassIcon )
					]
					+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock) 
							.Text( ClassesToDrop[0]->GetName() )
						]
				];
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
						.Text( ClassesToDrop[0]->GetName() )
					]
			];
	}

	static TSharedRef<FClassDragDropOp> New(TWeakObjectPtr<UClass> ClassToDrop)
	{
		TSharedRef<FClassDragDropOp> Operation = MakeShareable(new FClassDragDropOp);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FClassDragDropOp>(Operation);
		Operation->ClassesToDrop.Add(ClassToDrop);
		Operation->Construct();
		return Operation;
	}
};

struct FClassPackageData
{
	FString AssetName;
	FString GeneratedPackageName;

	FClassPackageData(const FString& InAssetName, const FString& InGeneratedPackageName)
	{
		AssetName = InAssetName;
		GeneratedPackageName = InGeneratedPackageName;
	}
};

class FUnloadedClassDragDropOp : public FDragDropOperation
{
public:
	static FString GetTypeId() {static FString Type = TEXT("FUnloadedClassDragDropOp"); return Type;}

	/** The assets to be dropped. */
	TSharedPtr< TArray< FClassPackageData > >	AssetsToDrop;
	
	/** The widget decorator to use */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE
	{
		// Create hover widget
		return SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Graph.ConnectorFeedback.Border"))
			.Content()
			[			
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock) 
					.Text( (*AssetsToDrop.Get())[0].AssetName )
				]
			];
	}

	static TSharedRef<FUnloadedClassDragDropOp> New(FClassPackageData AssetToDrop)
	{
		TSharedRef<FUnloadedClassDragDropOp> Operation = MakeShareable(new FUnloadedClassDragDropOp);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FUnloadedClassDragDropOp>(Operation);
		Operation->AssetsToDrop = MakeShareable(new TArray<FClassPackageData>);
		Operation->AssetsToDrop->Add(AssetToDrop);
		Operation->Construct();
		return Operation;
	}
};
