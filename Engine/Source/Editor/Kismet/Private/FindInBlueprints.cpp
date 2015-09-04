// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintEditorCommands.h"
#include "BlueprintEditor.h"
#include "FindInBlueprints.h"
#include "FindInBlueprintManager.h"
#include "Editor/UnrealEd/Public/Kismet2/DebuggerCommands.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistryModule.h"
#include "Json.h"
#include "IDocumentation.h"
#include "SSearchBox.h"
#include "GenericCommands.h"
#include "ImaginaryBlueprintData.h"
#include "FiBSearchInstance.h"

#define LOCTEXT_NAMESPACE "FindInBlueprints"


FText FindInBlueprintsHelpers::AsFText(TSharedPtr< FJsonValue > InJsonValue, const TMap<int32, FText>& InLookupTable)
{
	if (const FText* LookupText = InLookupTable.Find(FCString::Atoi(*InJsonValue->AsString())))
	{
		return *LookupText;
	}
	// Let's never get here.
	return LOCTEXT("FiBSerializationError", "There was an error in serialization!");
}

FText FindInBlueprintsHelpers::AsFText(int32 InValue, const TMap<int32, FText>& InLookupTable)
{
	if (const FText* LookupText = InLookupTable.Find(InValue))
	{
		return *LookupText;
	}
	// Let's never get here.
	return LOCTEXT("FiBSerializationError", "There was an error in serialization!");
}

bool FindInBlueprintsHelpers::IsTextEqualToString(FText InText, FString InString)
{
	return InString == InText.ToString() || InString == InText.BuildSourceString();
}

FString FindInBlueprintsHelpers::GetPinTypeAsString(const FEdGraphPinType& InPinType)
{
	FString Result = InPinType.PinCategory;
	if(UObject* SubCategoryObject = InPinType.PinSubCategoryObject.Get()) 
	{
		Result += FString(" '") + SubCategoryObject->GetName() + "'";
	}
	else
	{
		Result += FString(" '") + InPinType.PinSubCategory + "'";
	}

	return Result;
}

bool FindInBlueprintsHelpers::ParsePinType(FText InKey, FText InValue, FEdGraphPinType& InOutPinType)
{
	bool bParsed = true;

	if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_PinCategory) == 0)
	{
		InOutPinType.PinCategory = InValue.ToString();
	}
	else if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_PinSubCategory) == 0)
	{
		InOutPinType.PinSubCategory = InValue.ToString();
	}
	else if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_ObjectClass) == 0)
	{
		InOutPinType.PinSubCategory = InValue.ToString();
	}
	else if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_IsArray) == 0)
	{
		InOutPinType.bIsArray = InValue.ToString().ToBool();
	}
	else if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_IsReference) == 0)
	{
		InOutPinType.bIsReference = InValue.ToString().ToBool();
	}
	else
	{
		bParsed = false;
	}

	return bParsed;
}


//////////////////////////////////////////////////////////////////////////
// FBlueprintSearchResult

FFindInBlueprintsResult::FFindInBlueprintsResult(const FText& InDisplayText ) 
	: DisplayText(InDisplayText)
{
}

FFindInBlueprintsResult::FFindInBlueprintsResult( const FText& InDisplayText, TSharedPtr<FFindInBlueprintsResult> InParent) 
	: Parent(InParent), DisplayText(InDisplayText)
{
}

FReply FFindInBlueprintsResult::OnClick()
{
	// If there is a parent, handle it using the parent's functionality
	if(Parent.IsValid())
	{
		return Parent.Pin()->OnClick();
	}
	else
	{
		// As a last resort, find the parent Blueprint, and open that, it will get the user close to what they want
		UBlueprint* Blueprint = GetParentBlueprint();
		if(Blueprint)
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Blueprint, false);
		}
	}
	
	return FReply::Handled();
}

FText FFindInBlueprintsResult::GetCategory() const
{
	return FText::GetEmpty();
}

TSharedRef<SWidget> FFindInBlueprintsResult::CreateIcon() const
{
	FLinearColor IconColor = FLinearColor::White;
	const FSlateBrush* Brush = NULL;

	return 	SNew(SImage)
			.Image(Brush)
			.ColorAndOpacity(IconColor)
			.ToolTipText( GetCategory() );
}

FString FFindInBlueprintsResult::GetCommentText() const
{
	return CommentText;
}

UBlueprint* FFindInBlueprintsResult::GetParentBlueprint() const
{
	UBlueprint* ResultBlueprint = nullptr;
	if (Parent.IsValid())
	{
		ResultBlueprint = Parent.Pin()->GetParentBlueprint();
	}
	else
	{
		GIsEditorLoadingPackage = true;
		UObject* Object = LoadObject<UObject>(NULL, *DisplayText.ToString(), NULL, 0, NULL);
		GIsEditorLoadingPackage = false;

		if(UBlueprint* BlueprintObj = Cast<UBlueprint>(Object))
		{
			ResultBlueprint = BlueprintObj;
		}
		else if(UWorld* WorldObj = Cast<UWorld>(Object))
		{
			if(WorldObj->PersistentLevel)
			{
				ResultBlueprint = Cast<UBlueprint>((UObject*)WorldObj->PersistentLevel->GetLevelScriptBlueprint(true));
			}
		}

	}
	return ResultBlueprint;
}

void FFindInBlueprintsResult::ExpandAllChildren( TSharedPtr< STreeView< TSharedPtr< FFindInBlueprintsResult > > > InTreeView )
{
	if( Children.Num() )
	{
		InTreeView->SetItemExpansion(this->AsShared(), true);
		for( int32 i = 0; i < Children.Num(); i++ )
		{
			Children[i]->ExpandAllChildren(InTreeView);
		}
	}
}

FText FFindInBlueprintsResult::GetDisplayString() const
{
	return DisplayText;
}

//////////////////////////////////////////////////////////
// FFindInBlueprintsGraphNode

FFindInBlueprintsGraphNode::FFindInBlueprintsGraphNode(const FText& InValue, TSharedPtr<FFindInBlueprintsResult> InParent)
	: FFindInBlueprintsResult(InValue, InParent)
	, Class(nullptr)
{
}

FReply FFindInBlueprintsGraphNode::OnClick()
{
	UBlueprint* Blueprint = GetParentBlueprint();
	if(Blueprint)
	{
		UEdGraphNode* OutNode = NULL;
		if(	UEdGraphNode* GraphNode = FBlueprintEditorUtils::GetNodeByGUID(Blueprint, NodeGuid) )
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(GraphNode, /*bRequestRename=*/false);
			return FReply::Handled();
		}
	}

	return FFindInBlueprintsResult::OnClick();
}

TSharedRef<SWidget> FFindInBlueprintsGraphNode::CreateIcon() const
{
	return 	SNew(SImage)
		.Image(GlyphBrush)
		.ColorAndOpacity(GlyphColor)
		.ToolTipText( GetCategory() );
}

void FFindInBlueprintsGraphNode::ParseSearchInfo(FText InKey, FText InValue)
{
	if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_NodeGuid) == 0)
	{
		FString NodeGUIDAsString = InValue.ToString();
		FGuid::Parse(NodeGUIDAsString, NodeGuid);
	}

	if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_ClassName) == 0)
	{
		ClassName = InValue.ToString();
	}
	else if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_Name) == 0)
	{
		DisplayText = InValue;
	}
	else if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_Comment) == 0)
	{
		CommentText = InValue.ToString();
	}
	else if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_Glyph) == 0)
	{
		GlyphBrush = FEditorStyle::GetBrush(*InValue.ToString());
	}
	else if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_GlyphColor) == 0)
	{
		GlyphColor.InitFromString(InValue.ToString());
	}
}

FText FFindInBlueprintsGraphNode::GetCategory() const
{
	if(Class == UK2Node_CallFunction::StaticClass())
	{
		return LOCTEXT("CallFuctionCat", "Function Call");
	}
	else if(Class == UK2Node_MacroInstance::StaticClass())
	{
		return LOCTEXT("MacroCategory", "Macro");
	}
	else if(Class == UK2Node_Event::StaticClass())
	{
		return LOCTEXT("EventCat", "Event");
	}
	else if(Class == UK2Node_VariableGet::StaticClass())
	{
		return LOCTEXT("VariableGetCategory", "Variable Get");
	}
	else if(Class == UK2Node_VariableSet::StaticClass())
	{
		return LOCTEXT("VariableSetCategory", "Variable Set");
	}

	return LOCTEXT("NodeCategory", "Node");
}

void FFindInBlueprintsGraphNode::FinalizeSearchData()
{
	if(!ClassName.IsEmpty())
	{
		Class = FindObject<UClass>(ANY_PACKAGE, *ClassName, true);
		ClassName.Empty();
	}
}

//////////////////////////////////////////////////////////
// FFindInBlueprintsPin

FFindInBlueprintsPin::FFindInBlueprintsPin(const FText& InValue, TSharedPtr<FFindInBlueprintsResult> InParent, FString InSchemaName)
	: FFindInBlueprintsResult(InValue, InParent)
	, SchemaName(InSchemaName)
	, IconColor(FSlateColor::UseForeground())
{
}

TSharedRef<SWidget> FFindInBlueprintsPin::CreateIcon() const
{
	const FSlateBrush* Brush = NULL;

	if( PinType.bIsArray )
	{
		Brush = FEditorStyle::GetBrush( TEXT("GraphEditor.ArrayPinIcon") );
	}
	else if( PinType.bIsReference )
	{
		Brush = FEditorStyle::GetBrush( TEXT("GraphEditor.RefPinIcon") );
	}
	else
	{
		Brush = FEditorStyle::GetBrush( TEXT("GraphEditor.PinIcon") );
	}

	return 	SNew(SImage)
		.Image(Brush)
		.ColorAndOpacity(IconColor)
		.ToolTipText(FText::FromString(FindInBlueprintsHelpers::GetPinTypeAsString(PinType)));
}

void FFindInBlueprintsPin::ParseSearchInfo(FText InKey, FText InValue)
{
	if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_Name) == 0)
	{
		DisplayText = InValue;
	}
	else
	{
		FindInBlueprintsHelpers::ParsePinType(InKey, InValue, PinType);
	}
}

FText FFindInBlueprintsPin::GetCategory() const
{
	return LOCTEXT("PinCategory", "Pin");
}

void FFindInBlueprintsPin::FinalizeSearchData()
{
	if(!PinType.PinSubCategory.IsEmpty())
	{
		PinType.PinSubCategoryObject = FindObject<UClass>(ANY_PACKAGE, *PinType.PinSubCategory, true);
		if(!PinType.PinSubCategoryObject.IsValid())
		{
			PinType.PinSubCategoryObject = FindObject<UScriptStruct>(UObject::StaticClass(), *PinType.PinSubCategory);
		}

		if (PinType.PinSubCategoryObject.IsValid())
		{
			PinType.PinSubCategory.Empty();
		}
	}

	if(!SchemaName.IsEmpty())
	{
		UEdGraphSchema* Schema = nullptr;
		UClass* SchemaClass = FindObject<UClass>(ANY_PACKAGE, *SchemaName, true);
		if(SchemaClass)
		{
			Schema = SchemaClass->GetDefaultObject<UEdGraphSchema>();
		} 
		IconColor = Schema->GetPinTypeColor(PinType);

		SchemaName.Empty();
	}
}

//////////////////////////////////////////////////////////
// FFindInBlueprintsProperty

FFindInBlueprintsProperty::FFindInBlueprintsProperty(const FText& InValue, TSharedPtr<FFindInBlueprintsResult> InParent)
	: FFindInBlueprintsResult(InValue, InParent)
	, bIsSCSComponent(false)
{
}

TSharedRef<SWidget> FFindInBlueprintsProperty::CreateIcon() const
{
	FLinearColor IconColor = FLinearColor::White;
	const FSlateBrush* Brush = FEditorStyle::GetBrush(UK2Node_Variable::GetVarIconFromPinType(PinType, IconColor));
	IconColor = UEdGraphSchema_K2::StaticClass()->GetDefaultObject<UEdGraphSchema_K2>()->GetPinTypeColor(PinType);

	return 	SNew(SImage)
		.Image(Brush)
		.ColorAndOpacity(IconColor)
		.ToolTipText( FText::FromString(FindInBlueprintsHelpers::GetPinTypeAsString(PinType)) );
}

void FFindInBlueprintsProperty::ParseSearchInfo(FText InKey, FText InValue)
{
	if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_Name) == 0)
	{
		DisplayText = InValue;
	}
	else if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_IsSCSComponent) == 0)
	{
		bIsSCSComponent = true;
	}
	else
	{
		FindInBlueprintsHelpers::ParsePinType(InKey, InValue, PinType);
	}
}

FText FFindInBlueprintsProperty::GetCategory() const
{
	if(bIsSCSComponent)
	{
		return LOCTEXT("Component", "Component");
	}
	return LOCTEXT("Variable", "Variable");
}

void FFindInBlueprintsProperty::FinalizeSearchData()
{
	if(!PinType.PinSubCategory.IsEmpty())
	{
		PinType.PinSubCategoryObject = FindObject<UClass>(ANY_PACKAGE, *PinType.PinSubCategory, true);
		if(!PinType.PinSubCategoryObject.IsValid())
		{
			PinType.PinSubCategoryObject = FindObject<UScriptStruct>(UObject::StaticClass(), *PinType.PinSubCategory);
		}

		if (PinType.PinSubCategoryObject.IsValid())
		{
			PinType.PinSubCategory.Empty();
		}
	}
}

//////////////////////////////////////////////////////////
// FFindInBlueprintsGraph

FFindInBlueprintsGraph::FFindInBlueprintsGraph(const FText& InValue, TSharedPtr<FFindInBlueprintsResult> InParent, EGraphType InGraphType)
	: FFindInBlueprintsResult(InValue, InParent)
	, GraphType(InGraphType)
{
}

FReply FFindInBlueprintsGraph::OnClick()
{
	UBlueprint* Blueprint = GetParentBlueprint();
	if(Blueprint)
	{
		TArray<UEdGraph*> BlueprintGraphs;
		Blueprint->GetAllGraphs(BlueprintGraphs);

		for( auto Graph : BlueprintGraphs)
		{
			FGraphDisplayInfo DisplayInfo;
			Graph->GetSchema()->GetGraphDisplayInformation(*Graph, DisplayInfo);

			if(DisplayInfo.PlainName.EqualTo(DisplayText))
			{
				FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Graph);
				break;
			}
		}
	}
	else
	{
		return FFindInBlueprintsResult::OnClick();
	}
	return FReply::Handled();
}

TSharedRef<SWidget> FFindInBlueprintsGraph::CreateIcon() const
{
	const FSlateBrush* Brush = NULL;
	if(GraphType == GT_Function)
	{
		Brush = FEditorStyle::GetBrush(TEXT("GraphEditor.Function_16x"));
	}
	else if(GraphType == GT_Macro)
	{
		Brush = FEditorStyle::GetBrush(TEXT("GraphEditor.Macro_16x"));
	}

	return 	SNew(SImage)
		.Image(Brush)
		.ToolTipText( GetCategory() );
}

void FFindInBlueprintsGraph::ParseSearchInfo(FText InKey, FText InValue)
{
	if(InKey.CompareTo(FFindInBlueprintSearchTags::FiB_Name) == 0)
	{
		DisplayText = InValue;
	}
}

FText FFindInBlueprintsGraph::GetCategory() const
{
	if(GraphType == GT_Function)
	{
		return LOCTEXT("FunctionGraphCategory", "Function");
	}
	else if(GraphType == GT_Macro)
	{
		return LOCTEXT("MacroGraphCategory", "Macro");
	}
	return LOCTEXT("GraphCategory", "Graph");
}

////////////////////////////////////
// FStreamSearch

/**
 * Async task for searching Blueprints
 */
class FStreamSearch : public FRunnable
{
public:
	/** Constructor */
	FStreamSearch(const FString& InSearchValue )
		: SearchValue(InSearchValue)
		, bThreadCompleted(false)
		, StopTaskCounter(0)
	{
		// Add on a Guid to the thread name to ensure the thread is uniquely named.
		Thread = FRunnableThread::Create( this, *FString::Printf(TEXT("FStreamSearch%s"), *FGuid::NewGuid().ToString()), 0, TPri_BelowNormal );
	}

	/** Begin FRunnable Interface */
	virtual bool Init() override
	{
		return true;
	}

	virtual uint32 Run() override
	{
		FFindInBlueprintSearchManager::Get().BeginSearchQuery(this);

		TFunction<void(const FSearchResult&)> OnResultReady = [this](const FSearchResult& Result) {
			FScopeLock ScopeLock(&SearchCriticalSection);
			ItemsFound.Add(Result);
		};

		// Searching comes to an end if it is requested using the StopTaskCounter or continuing the search query yields no results
		FSearchData QueryResult;
		while (FFindInBlueprintSearchManager::Get().ContinueSearchQuery(this, QueryResult))
		{
			// This work cannot be spun off to a use the TaskGraph system for spreading the work over multiple threads/cores as the stack limit is too small for the work being done.
			if (QueryResult.Value.Len() > 0)
			{
				TSharedPtr< FImaginaryBlueprint> ImaginaryBlueprint(new FImaginaryBlueprint(FPaths::GetBaseFilename(QueryResult.BlueprintPath.ToString()), QueryResult.BlueprintPath.ToString(), QueryResult.ParentClass, QueryResult.Interfaces, QueryResult.Value));
				TSharedPtr< FFiBSearchInstance > SearchInstance(new FFiBSearchInstance);
				FSearchResult SearchResult = SearchInstance->StartSearchQuery(*SearchValue, ImaginaryBlueprint);

				// If there are children, add the item to the search results
				if(SearchResult.IsValid() && SearchResult->Children.Num() != 0)
				{
					OnResultReady(SearchResult);
				}
			}

			if (StopTaskCounter.GetValue())
			{
				// Ensure that the FiB Manager knows that we are done searching
				FFindInBlueprintSearchManager::Get().EnsureSearchQueryEnds(this);
			}
		}

		bThreadCompleted = true;

		return 0;
	}

	virtual void Stop() override
	{
		StopTaskCounter.Increment();
	}

	virtual void Exit() override
	{

	}
	/** End FRunnable Interface */

	/** Brings the thread to a safe stop before continuing. */
	void EnsureCompletion()
	{
		{
			FScopeLock CritSectionLock(&SearchCriticalSection);
			ItemsFound.Empty();
		}

		Stop();
		Thread->WaitForCompletion();
		delete Thread;
		Thread = NULL;
	}

	/** Returns TRUE if the thread is done with it's work. */
	bool IsComplete() const
	{
		return bThreadCompleted;
	}

	/**
	 * Appends the items filtered through the search filter to the passed array
	 *
	 * @param OutItemsFound		All the items found since last queried
	 */
	void GetFilteredItems(TArray<FSearchResult>& OutItemsFound)
	{
		FScopeLock ScopeLock(&SearchCriticalSection);
		OutItemsFound.Append(ItemsFound);
		ItemsFound.Empty();
	}

	/** Helper function to query the percent complete this search is */
	float GetPercentComplete() const
	{
		return FFindInBlueprintSearchManager::Get().GetPercentComplete(this);
	}

public:
	/** Thread to run the cleanup FRunnable on */
	FRunnableThread* Thread;

	/** A list of items found, cleared whenever the main thread pulls them to display to screen */
	TArray<FSearchResult> ItemsFound;

	/** The search value to filter results by */
	FString SearchValue;

	/** Prevents searching while other threads are pulling search results */
	FCriticalSection SearchCriticalSection;

	// Whether the thread has finished running
	bool bThreadCompleted;

	/** > 0 if we've been asked to abort work in progress at the next opportunity */
	FThreadSafeCounter StopTaskCounter;
};

//////////////////////////////////////////////////////////////////////////
// SBlueprintSearch

void SFindInBlueprints::Construct( const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor)
{
	BlueprintEditorPtr = InBlueprintEditor;

	RegisterCommands();

	bIsInFindWithinBlueprintMode = true;
	
	this->ChildSlot
		[
			SAssignNew(MainVerticalBox, SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SAssignNew(SearchTextField, SSearchBox)
					.HintText(LOCTEXT("BlueprintSearchHint", "Enter function or event name to find references..."))
					.OnTextChanged(this, &SFindInBlueprints::OnSearchTextChanged)
					.OnTextCommitted(this, &SFindInBlueprints::OnSearchTextCommitted)
				]
				+SHorizontalBox::Slot()
				.Padding(2.f, 0.f)
				.AutoWidth()
				[
					SNew(SCheckBox)
					.OnCheckStateChanged(this, &SFindInBlueprints::OnFindModeChanged)
					.IsChecked(this, &SFindInBlueprints::OnGetFindModeChecked)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("BlueprintSearchModeChange", "Find In Current Blueprint Only"))
					]
				]
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
				[
					SAssignNew(TreeView, STreeViewType)
					.ItemHeight(24)
					.TreeItemsSource( &ItemsFound )
					.OnGenerateRow( this, &SFindInBlueprints::OnGenerateRow )
					.OnGetChildren( this, &SFindInBlueprints::OnGetChildren )
					.OnMouseButtonDoubleClick(this,&SFindInBlueprints::OnTreeSelectionDoubleClicked)
					.SelectionMode( ESelectionMode::Multi )
					.OnContextMenuOpening(this, &SFindInBlueprints::OnContextMenuOpening)
				]
			]

			+SVerticalBox::Slot()
				.AutoHeight()
			[
				SNew(SHorizontalBox)

				// Text
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 2)
				[
					SNew(STextBlock)
					.Font( FEditorStyle::GetFontStyle("AssetDiscoveryIndicator.MainStatusFont") )
					.Text( LOCTEXT("SearchResults", "Searching...") )
					.Visibility(this, &SFindInBlueprints::GetSearchbarVisiblity)
				]

				// Progress bar
				+SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(SProgressBar)
						.Visibility(this, &SFindInBlueprints::GetSearchbarVisiblity).Percent( this, &SFindInBlueprints::GetPercentCompleteSearch )
				]
			]
		];
}

void SFindInBlueprints::ConditionallyAddCacheBar()
{
	FFindInBlueprintSearchManager& FindInBlueprintManager = FFindInBlueprintSearchManager::Get();

	// Do not add a second cache bar and do not add it when there are no uncached Blueprints
	if(FindInBlueprintManager.GetNumberUncachedBlueprints() > 0 || FindInBlueprintManager.GetFailedToCacheCount() > 0)
	{
		if(MainVerticalBox.IsValid() && !CacheBarSlot.IsValid())
		{
			// Create a single string of all the Blueprint paths that failed to cache, on separate lines
			FString PackageList;
			TArray<FName> FailedToCacheList = FFindInBlueprintSearchManager::Get().GetFailedToCachePathList();
			for (FName Package : FailedToCacheList)
			{
				PackageList += Package.ToString() + TEXT("\n");
			}

			// Lambda to put together the popup menu detailing the failed to cache paths
			auto OnDisplayCacheFailLambda = [](TWeakPtr<SWidget> InParentWidget, FString InPackageList)->FReply
			{
				if (InParentWidget.IsValid())
				{
					TSharedRef<SWidget> DisplayWidget = 
						SNew(SBox)
						.MaxDesiredHeight(512)
						.MaxDesiredWidth(512)
						.Content()
						[
							SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							[
								SNew(SScrollBox)
								+SScrollBox::Slot()
								[
									SNew(SMultiLineEditableText)
									.AutoWrapText(true)
									.IsReadOnly(true)
									.Text(FText::FromString(InPackageList))
								]
							]
						];

					FSlateApplication::Get().PushMenu(
						InParentWidget.Pin().ToSharedRef(),
						FWidgetPath(),
						DisplayWidget,
						FSlateApplication::Get().GetCursorPos(),
						FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup)
						);	
				}
				return FReply::Handled();
			};

			MainVerticalBox.Pin()->AddSlot()
				.AutoHeight()
				[
					SAssignNew(CacheBarSlot, SBorder)
					.Visibility( this, &SFindInBlueprints::GetCachingBarVisibility )
					.BorderBackgroundColor( this, &SFindInBlueprints::GetCachingBarColor )
					.BorderImage( FCoreStyle::Get().GetBrush("ErrorReporting.Box") )
					.Padding( FMargin(3,1) )
					[
						SNew(SVerticalBox)

						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.VAlign(EVerticalAlignment::VAlign_Center)
							.AutoWidth()
							[
								SNew(STextBlock)
								.Text(this, &SFindInBlueprints::GetUncachedBlueprintWarningText)
								.ColorAndOpacity( FCoreStyle::Get().GetColor("ErrorReporting.ForegroundColor") )
							]

							// Cache All button
							+SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(EVerticalAlignment::VAlign_Center)
								.Padding(6.0f, 2.0f, 4.0f, 2.0f)
								[
									SNew(SButton)
									.Text(LOCTEXT("IndexAllBlueprints", "Index All"))
									.OnClicked( this, &SFindInBlueprints::OnCacheAllBlueprints )
									.Visibility( this, &SFindInBlueprints::GetCacheAllButtonVisibility )
									.ToolTip(IDocumentation::Get()->CreateToolTip(
									LOCTEXT("IndexAlLBlueprints_Tooltip", "Loads all non-indexed Blueprints and saves them with their search data. This can be a very slow process and the editor may become unresponsive."),
									NULL,
									TEXT("Shared/Editors/BlueprintEditor"),
									TEXT("FindInBlueprint_IndexAll")))
								]


							// View of failed Blueprint paths
							+SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(4.0f, 2.0f, 0.0f, 2.0f)
								[
									SNew(SButton)
									.Text(LOCTEXT("ShowFailedPackages", "Show Failed Packages"))
									.OnClicked(FOnClicked::CreateLambda(OnDisplayCacheFailLambda, TWeakPtr<SWidget>(SharedThis(this)), PackageList))
									.Visibility( this, &SFindInBlueprints::GetFailedToCacheListVisibility )
									.ToolTip(IDocumentation::Get()->CreateToolTip(
									LOCTEXT("FailedCache_Tooltip", "Displays a list of packages that failed to save."),
									NULL,
									TEXT("Shared/Editors/BlueprintEditor"),
									TEXT("FindInBlueprint_FailedCache")))
								]

							// Cache progress bar
							+SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.Padding(4.0f, 2.0f, 4.0f, 2.0f)
								[
									SNew(SProgressBar)
									.Percent( this, &SFindInBlueprints::GetPercentCompleteCache )
									.Visibility( this, &SFindInBlueprints::GetCachingProgressBarVisiblity )
								]

							// Cancel button
							+SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(4.0f, 2.0f, 0.0f, 2.0f)
								[
									SNew(SButton)
									.Text(LOCTEXT("CancelCacheAll", "Cancel"))
									.OnClicked( this, &SFindInBlueprints::OnCancelCacheAll )
									.Visibility( this, &SFindInBlueprints::GetCachingProgressBarVisiblity )
									.ToolTipText( LOCTEXT("CancelCacheAll_Tooltip", "Stops the caching process from where ever it is, can be started back up where it left off when needed.") )
								]

							// "X" to remove the bar
							+SHorizontalBox::Slot()
								.HAlign(HAlign_Right)
								[
									SNew(SButton)
									.ButtonStyle( FCoreStyle::Get(), "NoBorder" )
									.ContentPadding(0)
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									.OnClicked( this, &SFindInBlueprints::OnRemoveCacheBar )
									.ForegroundColor( FSlateColor::UseForeground() )
									[
										SNew(SImage)
										.Image( FCoreStyle::Get().GetBrush("EditableComboBox.Delete") )
										.ColorAndOpacity( FSlateColor::UseForeground() )
									]
								]
						]

						+SVerticalBox::Slot()
							.AutoHeight()
							.Padding(8.0f, 0.0f, 0.0f, 2.0f)
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
									.Text(this, &SFindInBlueprints::GetCurrentCacheBlueprintName)
									.Visibility( this, &SFindInBlueprints::GetCachingBlueprintNameVisiblity )
									.ColorAndOpacity( FCoreStyle::Get().GetColor("ErrorReporting.ForegroundColor") )
								]

								+SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
									.Text(LOCTEXT("FiBUnresponsiveEditorWarning", "NOTE: the editor may become unresponsive for some time!"))
									.TextStyle(&FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>( "SmallText" ))
								]
							]
					]
				];
		}
	}
	else
	{
		// Because there are no uncached Blueprints, remove the bar
		OnRemoveCacheBar();
	}
}

FReply SFindInBlueprints::OnRemoveCacheBar()
{
	if(MainVerticalBox.IsValid() && CacheBarSlot.IsValid())
	{
		MainVerticalBox.Pin()->RemoveSlot(CacheBarSlot.Pin().ToSharedRef());
	}

	return FReply::Handled();
}

SFindInBlueprints::~SFindInBlueprints()
{
	if(StreamSearch.IsValid())
	{
		StreamSearch->Stop();
		StreamSearch->EnsureCompletion();
	}

	FFindInBlueprintSearchManager::Get().CancelCacheAll(this);
}

EActiveTimerReturnType SFindInBlueprints::UpdateSearchResults( double InCurrentTime, float InDeltaTime )
{
	if ( StreamSearch.IsValid() )
	{
		bool bShouldShutdownThread = false;
		bShouldShutdownThread = StreamSearch->IsComplete();

		TArray<FSearchResult> BackgroundItemsFound;

		StreamSearch->GetFilteredItems( BackgroundItemsFound );
		if ( BackgroundItemsFound.Num() )
		{
			for ( auto& Item : BackgroundItemsFound )
			{
				Item->ExpandAllChildren( TreeView );
				ItemsFound.Add( Item );
			}
			TreeView->RequestTreeRefresh();
		}

		// If the thread is complete, shut it down properly
		if ( bShouldShutdownThread )
		{
			if ( ItemsFound.Num() == 0 )
			{
				// Insert a fake result to inform user if none found
				ItemsFound.Add( FSearchResult( new FFindInBlueprintsResult( LOCTEXT( "BlueprintSearchNoResults", "No Results found" ) ) ) );
				TreeView->RequestTreeRefresh();
			}

			// Add the cache bar if needed.
			ConditionallyAddCacheBar();

			StreamSearch->EnsureCompletion();
			StreamSearch.Reset();
		}
	}

	return StreamSearch.IsValid() ? EActiveTimerReturnType::Continue : EActiveTimerReturnType::Stop;
}

void SFindInBlueprints::RegisterCommands()
{
	TSharedPtr<FUICommandList> ToolKitCommandList = BlueprintEditorPtr.Pin()->GetToolkitCommands();

	ToolKitCommandList->MapAction( FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &SFindInBlueprints::OnCopyAction) );

	ToolKitCommandList->MapAction( FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateSP(this, &SFindInBlueprints::OnSelectAllAction) );
}

void SFindInBlueprints::FocusForUse(bool bSetFindWithinBlueprint, FString NewSearchTerms, bool bSelectFirstResult)
{
	// NOTE: Careful, GeneratePathToWidget can be reentrant in that it can call visibility delegates and such
	FWidgetPath FilterTextBoxWidgetPath;
	FSlateApplication::Get().GeneratePathToWidgetUnchecked( SearchTextField.ToSharedRef(), FilterTextBoxWidgetPath );

	// Set keyboard focus directly
	FSlateApplication::Get().SetKeyboardFocus( FilterTextBoxWidgetPath, EFocusCause::SetDirectly );

	// Set the filter mode
	bIsInFindWithinBlueprintMode = bSetFindWithinBlueprint;

	if (!NewSearchTerms.IsEmpty())
	{
		SearchTextField->SetText(FText::FromString(NewSearchTerms));
		InitiateSearch();

		// Select the first result
		if (bSelectFirstResult && ItemsFound.Num())
		{
			auto ItemToFocusOn = ItemsFound[0];

			// We want the first childmost item to select, as that is the item that is most-likely to be what was searched for (parents being graphs).
			// Will fail back upward as neccessary to focus on a focusable item
			while(ItemToFocusOn->Children.Num())
			{
				ItemToFocusOn = ItemToFocusOn->Children[0];
			}
			TreeView->SetSelection(ItemToFocusOn);
			ItemToFocusOn->OnClick();
		}
	}
}

void SFindInBlueprints::OnSearchTextChanged( const FText& Text)
{
	SearchValue = Text.ToString();
}

void SFindInBlueprints::OnSearchTextCommitted( const FText& Text, ETextCommit::Type CommitType )
{
	if (CommitType == ETextCommit::OnEnter)
	{
		InitiateSearch();
	}
}

void SFindInBlueprints::OnFindModeChanged(ECheckBoxState CheckState)
{
	bIsInFindWithinBlueprintMode = CheckState == ECheckBoxState::Checked;
}

ECheckBoxState SFindInBlueprints::OnGetFindModeChecked() const
{
	return bIsInFindWithinBlueprintMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SFindInBlueprints::InitiateSearch()
{
	if(ItemsFound.Num())
	{
		// Reset the scroll to the top
		TreeView->RequestScrollIntoView(ItemsFound[0]);
	}

	ItemsFound.Empty();

	if (SearchValue.Len() > 0)
	{
		OnRemoveCacheBar();

		TreeView->RequestTreeRefresh();

		HighlightText = FText::FromString( SearchValue );
		if (bIsInFindWithinBlueprintMode)
		{
			if(StreamSearch.IsValid() && !StreamSearch->IsComplete())
			{
				StreamSearch->Stop();
				StreamSearch->EnsureCompletion();
				StreamSearch.Reset();
			}

			UBlueprint* Blueprint = BlueprintEditorPtr.Pin()->GetBlueprintObj();
			FString ParentClass;
			if (UProperty* ParentClassProp = Blueprint->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UBlueprint, ParentClass)))
			{
				ParentClassProp->ExportTextItem(ParentClass, ParentClassProp->ContainerPtrToValuePtr<uint8>(Blueprint), nullptr, Blueprint, 0);
			}

			TArray<FString> Interfaces;

			for (FBPInterfaceDescription& InterfaceDesc : Blueprint->ImplementedInterfaces)
			{
				Interfaces.Add(InterfaceDesc.Interface->GetPathName());
			}
			TSharedPtr< FImaginaryBlueprint> ImaginaryBlueprint(new FImaginaryBlueprint(Blueprint->GetName(), Blueprint->GetPathName(), ParentClass, Interfaces, FFindInBlueprintSearchManager::Get().QuerySingleBlueprint(Blueprint)));
			TSharedPtr< FFiBSearchInstance > SearchInstance(new FFiBSearchInstance);
			FSearchResult SearchResult = RootSearchResult = SearchInstance->StartSearchQuery(SearchValue, ImaginaryBlueprint);

			if (SearchResult.IsValid())
			{
				ItemsFound = SearchResult->Children;
			}

			if(ItemsFound.Num() == 0)
			{
				// Insert a fake result to inform user if none found
				ItemsFound.Add(FSearchResult(new FFindInBlueprintsResult(LOCTEXT("BlueprintSearchNoResults", "No Results found"))));
				HighlightText = FText::GetEmpty();
			}
			else
			{
				for(auto Item : ItemsFound)
				{
					Item->ExpandAllChildren(TreeView);
				}
			}

			TreeView->RequestTreeRefresh();
		}
		else
		{
			LaunchStreamThread(SearchValue);
		}
	}
}

void SFindInBlueprints::LaunchStreamThread(const FString& InSearchValue)
{
	if(StreamSearch.IsValid() && !StreamSearch->IsComplete())
	{
		StreamSearch->Stop();
		StreamSearch->EnsureCompletion();
	}
	else
	{
		// If the stream search wasn't already running, register the active timer
		RegisterActiveTimer( 0.f, FWidgetActiveTimerDelegate::CreateSP( this, &SFindInBlueprints::UpdateSearchResults ) );
	}

	StreamSearch = MakeShareable(new FStreamSearch(InSearchValue));
}

TSharedRef<ITableRow> SFindInBlueprints::OnGenerateRow( FSearchResult InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	// Finalize the search data, this does some non-thread safe actions that could not be done on the separate thread.
	InItem->FinalizeSearchData();

	bool bIsACategoryWidget = !bIsInFindWithinBlueprintMode && !InItem->Parent.IsValid();

	if (bIsACategoryWidget)
	{
		return SNew( STableRow< TSharedPtr<FFindInBlueprintsResult> >, OwnerTable )
			[
				SNew(SBorder)
				.VAlign(VAlign_Center)
				.BorderImage(FEditorStyle::GetBrush("PropertyWindow.CategoryBackground"))
				.Padding(FMargin(2.0f))
				.ForegroundColor(FEditorStyle::GetColor("PropertyWindow.CategoryForeground"))
				[
					SNew(STextBlock)
					.Text(InItem.Get(), &FFindInBlueprintsResult::GetDisplayString)
					.ToolTipText(LOCTEXT("BlueprintCatSearchToolTip", "Blueprint"))
				]
			];
	}
	else // Functions/Event/Pin widget
	{
		FText CommentText = FText::GetEmpty();

		if(!InItem->GetCommentText().IsEmpty())
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Comment"), FText::FromString(InItem->GetCommentText()));

			CommentText = FText::Format(LOCTEXT("NodeComment", "Node Comment:[{Comment}]"), Args);
		}

		FFormatNamedArguments Args;
		Args.Add(TEXT("Category"), InItem->GetCategory());
		Args.Add(TEXT("DisplayTitle"), InItem->DisplayText);

		FText Tooltip = FText::Format(LOCTEXT("BlueprintResultSearchToolTip", "{Category} : {DisplayTitle}"), Args);

		return SNew( STableRow< TSharedPtr<FFindInBlueprintsResult> >, OwnerTable )
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					InItem->CreateIcon()
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2,0)
				[
					SNew(STextBlock)
						.Text(InItem.Get(), &FFindInBlueprintsResult::GetDisplayString)
						.HighlightText(HighlightText)
						.ToolTipText(Tooltip)
				]
				+SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(2,0)
				[
					SNew(STextBlock)
					.Text( CommentText )
					.ColorAndOpacity(FLinearColor::Yellow)
					.HighlightText(HighlightText)
				]
			];
	}
}

void SFindInBlueprints::OnGetChildren( FSearchResult InItem, TArray< FSearchResult >& OutChildren )
{
	OutChildren += InItem->Children;
}

void SFindInBlueprints::OnTreeSelectionDoubleClicked( FSearchResult Item )
{
	if(Item.IsValid())
	{
		Item->OnClick();
	}
}

TOptional<float> SFindInBlueprints::GetPercentCompleteSearch() const
{
	if(StreamSearch.IsValid())
	{
		return StreamSearch->GetPercentComplete();
	}
	return 0.0f;
}

EVisibility SFindInBlueprints::GetSearchbarVisiblity() const
{
	return StreamSearch.IsValid()? EVisibility::Visible : EVisibility::Collapsed;
}

FReply SFindInBlueprints::OnCacheAllBlueprints()
{
	if(!FFindInBlueprintSearchManager::Get().IsCacheInProgress())
	{
		// Request from the SearchManager a delegate to use for ticking the cache system.
		FWidgetActiveTimerDelegate WidgetActiveTimer;
		FFindInBlueprintSearchManager::Get().CacheAllUncachedBlueprints(SharedThis(this), WidgetActiveTimer);
		RegisterActiveTimer(0.f, WidgetActiveTimer);
	}

	return FReply::Handled();
}

FReply SFindInBlueprints::OnCancelCacheAll()
{
	FFindInBlueprintSearchManager::Get().CancelCacheAll(this);

	// Resubmit the last search
	OnSearchTextCommitted(SearchTextField->GetText(), ETextCommit::OnEnter);

	return FReply::Handled();
}

int32 SFindInBlueprints::GetCurrentCacheIndex() const
{
	return FFindInBlueprintSearchManager::Get().GetCurrentCacheIndex();
}

TOptional<float> SFindInBlueprints::GetPercentCompleteCache() const
{
	return FFindInBlueprintSearchManager::Get().GetCacheProgress();
}

EVisibility SFindInBlueprints::GetCachingProgressBarVisiblity() const
{
	return IsCacheInProgress()? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility SFindInBlueprints::GetCacheAllButtonVisibility() const
{
	return IsCacheInProgress()? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SFindInBlueprints::GetCachingBarVisibility() const
{
	FFindInBlueprintSearchManager& FindInBlueprintManager = FFindInBlueprintSearchManager::Get();
	return (FindInBlueprintManager.GetNumberUncachedBlueprints() > 0 || FindInBlueprintManager.GetFailedToCacheCount())? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SFindInBlueprints::GetCachingBlueprintNameVisiblity() const
{
	return IsCacheInProgress()? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SFindInBlueprints::GetFailedToCacheListVisibility() const
{
	return FFindInBlueprintSearchManager::Get().GetFailedToCacheCount()? EVisibility::Visible : EVisibility::Collapsed;
}

bool SFindInBlueprints::IsCacheInProgress() const
{
	return FFindInBlueprintSearchManager::Get().IsCacheInProgress();
}

FSlateColor SFindInBlueprints::GetCachingBarColor() const
{
	// The caching bar's default color is a darkish red
	FSlateColor ReturnColor = FSlateColor(FLinearColor(0.4f, 0.0f, 0.0f));
	if(IsCacheInProgress())
	{
		// It turns yellow when in progress
		ReturnColor = FSlateColor(FLinearColor(0.4f, 0.4f, 0.0f));
	}
	return ReturnColor;
}

FText SFindInBlueprints::GetUncachedBlueprintWarningText() const
{
	FFindInBlueprintSearchManager& FindInBlueprintManager = FFindInBlueprintSearchManager::Get();

	int32 FailedToCacheCount = FindInBlueprintManager.GetFailedToCacheCount();

	// The number of unindexed Blueprints is the total of those that failed to cache and those that haven't been attempted yet.
	FFormatNamedArguments Args;
	Args.Add(TEXT("Count"), FindInBlueprintManager.GetNumberUncachedBlueprints() + FailedToCacheCount);

	FText ReturnDisplayText;
	if(IsCacheInProgress())
	{
		Args.Add(TEXT("CurrentIndex"), FindInBlueprintManager.GetCurrentCacheIndex());

		ReturnDisplayText = FText::Format(LOCTEXT("CachingBlueprints", "Indexing Blueprints... {CurrentIndex}/{Count}"), Args);
	}
	else
	{
		ReturnDisplayText = FText::Format(LOCTEXT("UncachedBlueprints", "Search incomplete. {Count} Blueprints are not indexed!"), Args);

		if (FailedToCacheCount > 0)
		{
			FFormatNamedArguments ArgsWithCacheFails;
			Args.Add(TEXT("BaseMessage"), ReturnDisplayText);
			Args.Add(TEXT("CacheFails"), FailedToCacheCount);
			ReturnDisplayText = FText::Format(LOCTEXT("UncachedBlueprintsWithCacheFails", "{BaseMessage} {CacheFails} Blueprints failed to cache."), Args);
		}
	}

	return ReturnDisplayText;
}

FText SFindInBlueprints::GetCurrentCacheBlueprintName() const
{
	return FText::FromName(FFindInBlueprintSearchManager::Get().GetCurrentCacheBlueprintName());
}

void SFindInBlueprints::OnCacheComplete()
{
	// Resubmit the last search, which will also remove the bar if needed
	OnSearchTextCommitted(SearchTextField->GetText(), ETextCommit::OnEnter);
}

TSharedPtr<SWidget> SFindInBlueprints::OnContextMenuOpening()
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection,BlueprintEditorPtr.Pin()->GetToolkitCommands());

	MenuBuilder.BeginSection("BasicOperations");
	{
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().SelectAll);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
	}

	return MenuBuilder.MakeWidget();
}

void SFindInBlueprints::SelectAllItemsHelper(FSearchResult InItemToSelect)
{
	// Iterates over all children and recursively selects all items in the results
	TreeView->SetItemSelection(InItemToSelect, true);

	for( const auto Child : InItemToSelect->Children )
	{
		SelectAllItemsHelper(Child);
	}
}

void SFindInBlueprints::OnSelectAllAction()
{
	for( const auto Item : ItemsFound )
	{
		SelectAllItemsHelper(Item);
	}
}

void SFindInBlueprints::OnCopyAction()
{
	TArray< FSearchResult > SelectedItems = TreeView->GetSelectedItems();

	FString SelectedText;

	for( const auto SelectedItem : SelectedItems)
	{
		// Add indents for each layer into the tree the item is
		for(auto ParentItem = SelectedItem->Parent; ParentItem.IsValid(); ParentItem = ParentItem.Pin()->Parent)
		{
			SelectedText += TEXT("\t");
		}

		// Add the display string
		SelectedText += SelectedItem->GetDisplayString().ToString();

		// If there is a comment, add two indents and then the comment
		FString CommentText = SelectedItem->GetCommentText();
		if(!CommentText.IsEmpty())
		{
			SelectedText += TEXT("\t\t") + CommentText;
		}
		
		// Line terminator so the next item will be on a new line
		SelectedText += LINE_TERMINATOR;
	}

	// Copy text to clipboard
	FPlatformMisc::ClipboardCopy( *SelectedText );
}

#undef LOCTEXT_NAMESPACE
