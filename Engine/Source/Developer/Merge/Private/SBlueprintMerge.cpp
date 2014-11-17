// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"
#include "SBlueprintMerge.h"
#include "DiffResults.h"
#include "GraphDiffControl.h"
#include "ISourceControlModule.h"

#define LOCTEXT_NAMESPACE "SBlueprintMerge"

// todo doc: where doe this utility function belong!
static UPackage* CreateTempPackage(FString Name)
{
	static uint32 TempUid = 0;
	FString TempPackageName = FString::Printf(TEXT("/Temp/BpMerge-%u-%s"), TempUid++, *Name);
	return CreatePackage(NULL, *TempPackageName);
}

static UBlueprint* DuplicateBlueprint( const UBlueprint* const BlueprintToClone)
{
	UPackage* TempPackage = CreateTempPackage(BlueprintToClone->GetName());
	TArray<UObject*> Subobjects;
	GetObjectsWithOuter(TempPackage, Subobjects, true);
	check(Subobjects.Num() == 0);
	EObjectFlags FlagMask = RF_AllFlags & ~RF_Transient & ~RF_PendingKill;
	return Cast<UBlueprint>(StaticDuplicateObject(BlueprintToClone, TempPackage, *BlueprintToClone->GetName(), FlagMask));
}

template< class Predicate >
UObject* FindOuter( UObject& Obj, Predicate P )
{
	UObject* Iter = &Obj;
	while( Iter )
	{
		const UObject* ImmutableIter = Iter;
		if( P(*ImmutableIter) )
		{
			break;
		}
		Iter = Iter->GetOuter();
	}
	return Iter;
}

static bool PackagesAppearEquivalent( const UPackage* A, const UPackage* B )
{
	TArray<UObject*> SubobjectsA;
	GetObjectsWithOuter(A, SubobjectsA, true);
	TArray<UObject*> SubobjectsB;
	GetObjectsWithOuter(B, SubobjectsB, true);

	auto IsTransientOrPendingKill = [](UObject const& Obj){ return Obj.HasAnyFlags(RF_Transient | RF_PendingKill); };
	TSet<FString> SubobjectANames;
	for( const auto& I : SubobjectsA )
	{
		if (!FindOuter(*I, IsTransientOrPendingKill ) )
		{
			SubobjectANames.Add(I->GetName());
		}
	}
	TSet<FString> SubobjectBNames;
	for (const auto& I : SubobjectsB)
	{
		if (!FindOuter(*I, IsTransientOrPendingKill))
		{
			SubobjectBNames.Add(I->GetName());
		}
	}

	TArray<UObject*> DifferentObjects;
	TSet<FString> AB = SubobjectANames.Difference(SubobjectBNames);
	TSet<FString> BA = SubobjectBNames.Difference(SubobjectANames);
	TSet<FString> Difference = AB.Union( BA );
	for(const auto& I : Difference )
	{
		auto FindByName = [&I]( const UObject* Candidate ) { return Candidate->GetName() == I; };
		UObject** FoundObject = SubobjectsA.FindByPredicate( FindByName );
		if( !FoundObject )
		{
			FoundObject = SubobjectsB.FindByPredicate( FindByName );
		}

		if( FoundObject )
		{
			DifferentObjects.Add( *FoundObject );
		}
		else
		{
			verify(false);
		}
	}

	return Difference.Num() == 0;
}

/** 
 * Commands used to navigate the lists of differences between the local revision and the common base
 * or the remote revision and the common base.
 */
class FMergeDifferencesListCommands : public TCommands<FMergeDifferencesListCommands>
{
public:
	FMergeDifferencesListCommands()
		: TCommands<FMergeDifferencesListCommands>("MergeList", NSLOCTEXT("Merge", "Blueprint", "Blueprint Merge"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}

	/** Go to previous or next difference, focusing on either the remote or local diff */
	TSharedPtr<FUICommandInfo> LocalPrevious;
	TSharedPtr<FUICommandInfo> LocalNext;
	TSharedPtr<FUICommandInfo> RemotePrevious;
	TSharedPtr<FUICommandInfo> RemoteNext;

	virtual void RegisterCommands() override
	{
		UI_COMMAND(LocalPrevious, "LocalPrevious", "Go to previous local difference", EUserInterfaceActionType::Button, FInputGesture(EKeys::F7, EModifierKey::Control));
		UI_COMMAND(LocalNext, "LocalNext", "Go to next local difference", EUserInterfaceActionType::Button, FInputGesture(EKeys::F7));
		UI_COMMAND(RemotePrevious, "RemotePrevious", "Go to previous remote difference", EUserInterfaceActionType::Button, FInputGesture(EKeys::F8, EModifierKey::Control));
		UI_COMMAND(RemoteNext, "RemoteNext", "Go to next remote difference", EUserInterfaceActionType::Button, FInputGesture(EKeys::F8));
	}
};

void SBlueprintMerge::Construct(const FArguments& InArgs)
{
	FMergeDifferencesListCommands::Register();

	PanelLocal.Blueprint = InArgs._BlueprintLocal;
	BlueprintResult = DuplicateBlueprint(InArgs._BlueprintLocal);
	(void)PackagesAppearEquivalent( BlueprintResult->GetOutermost(), PanelLocal.Blueprint->GetOutermost() );
	//check( PackagesAppearEquivalent( BlueprintResult->GetOutermost(), PanelLocal.Blueprint->GetOutermost() ) );
	OwningWindow = InArgs._OwningWindow;

	return SBlueprintDiff::Construct( InArgs._BaseArgs );
}

TSharedRef<ITableRow> SBlueprintMerge::OnGenerateRow( TSharedPtr<struct FDiffSingleResult> ParamItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	FDiffSingleResult const& Result = *ParamItem;
	FString ToolTip = Result.ToolTip;
	FLinearColor Color = Result.DisplayColor;
	FString Text = Result.DisplayString;
	if (Text.Len() == 0)
	{
		Text = LOCTEXT("DIF_UnknownDiff", "Unknown Diff").ToString();
		ToolTip = LOCTEXT("DIF_Confused", "There is an unspecified difference").ToString();
	}
	return SNew(STableRow< TSharedPtr<struct FDiffSingleResult> >, OwnerTable)
		.Content()
		[
			SNew(STextBlock)
			.ToolTipText(ToolTip)
			.ColorAndOpacity(Color)
			.Text(Text)
		];
}

void SBlueprintMerge::OnDiffListSelectionChanged(TSharedPtr<struct FDiffSingleResult> Item, ESelectInfo::Type SelectionType)
{
	if( !Item.IsValid() )
	{
		return;
	}

	//focus the graph onto the diff that was clicked on
	FDiffSingleResult const& Result = *Item;
	if (Result.Pin1)
	{
		PanelNew.GraphEditor.Pin()->ClearSelectionSet();
		PanelOld.GraphEditor.Pin()->ClearSelectionSet();

		if (Result.Pin1)
		{
			Result.Pin1->bIsDiffing = true;
			GetGraphEditorForGraph(Result.Pin1->GetOwningNode()->GetGraph())->JumpToPin(Result.Pin1);
		}

		if (Result.Pin2)
		{
			Result.Pin2->bIsDiffing = true;
			GetGraphEditorForGraph(Result.Pin2->GetOwningNode()->GetGraph())->JumpToPin(Result.Pin2);
		}
	}
	else if (Result.Node1)
	{
		PanelNew.GraphEditor.Pin()->ClearSelectionSet();
		PanelOld.GraphEditor.Pin()->ClearSelectionSet();

		if (Result.Node2)
		{
			GetGraphEditorForGraph(Result.Node2->GetGraph())->JumpToNode(Result.Node2, false);
		}
		GetGraphEditorForGraph(Result.Node1->GetGraph())->JumpToNode(Result.Node1, false);
	}
}

void SBlueprintMerge::ResetGraphEditors()
{
	auto GraphEditor = PanelLocal.GraphEditor.Pin();
	if (GraphEditor != TSharedPtr<SGraphEditor>())
	{
		if (bLockViews)
		{
			GraphEditor->LockToGraphEditor(PanelNew.GraphEditor);
			GraphEditor->LockToGraphEditor(PanelOld.GraphEditor);

			auto OldGraph = PanelOld.GraphEditor.Pin();
			if( OldGraph != TSharedPtr<SGraphEditor>() )
			{
				OldGraph->LockToGraphEditor( GraphEditor );
			}
			auto NewGraph = PanelNew.GraphEditor.Pin();
			if( NewGraph!= TSharedPtr<SGraphEditor>() )
			{
				NewGraph->LockToGraphEditor(GraphEditor);
			}
		}
		else
		{
			GraphEditor->UnlockFromGraphEditor(PanelNew.GraphEditor);
			GraphEditor->UnlockFromGraphEditor(PanelOld.GraphEditor);

			auto OldGraph = PanelOld.GraphEditor.Pin();
			if (OldGraph != TSharedPtr<SGraphEditor>())
			{
				OldGraph->UnlockFromGraphEditor(GraphEditor);
			}
			auto NewGraph = PanelNew.GraphEditor.Pin();
			if (NewGraph != TSharedPtr<SGraphEditor>())
			{
				NewGraph->UnlockFromGraphEditor(GraphEditor);
			}
		}
	}
	return SBlueprintDiff::ResetGraphEditors();
}

TSharedRef<SWidget> SBlueprintMerge::GenerateDiffWindow()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SSplitter).Orientation(Orient_Vertical)
			+ SSplitter::Slot()
			.Value(.7f)
			[
				SNew(SSplitter)
				+ SSplitter::Slot()
				.Value(0.5f)
				[
					//left blueprint
					SAssignNew(PanelLocal.GraphEditorBorder, SBorder)
					.VAlign(VAlign_Fill)
					[
						SBlueprintDiff::DefaultEmptyPanel()
					]
				]
				+ SSplitter::Slot()
				.Value(0.5f)
				[
					//middle (base) blueprint
					SAssignNew(PanelOld.GraphEditorBorder, SBorder)
					.VAlign(VAlign_Fill)
					[
						SBlueprintDiff::DefaultEmptyPanel()
					]
				]
			+ SSplitter::Slot()
				.Value(0.5f)
				[
					//right blueprint
					SAssignNew(PanelNew.GraphEditorBorder, SBorder)
					.VAlign(VAlign_Fill)
					[
						SBlueprintDiff::DefaultEmptyPanel()
					]
				]
			]
			+ SSplitter::Slot()
			.Value(.3f)
			[
				SAssignNew(EditorBorder, SBorder)
			]
		]
	;
}

TSharedRef<SWidget> SBlueprintMerge::GenerateToolbar()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(SButton)
				.OnClicked(this, &SBlueprintMerge::OnAcceptResultClicked)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AcceptResult", "Accept Result"))
					.ToolTipText( LOCTEXT( "AcceptResultToolTip", "Overwrite the local blueprint with the blueprint constructed in the lower portion of the merge tool." ) )
				]
			]
			+ SVerticalBox::Slot()
			[
				SNew(SButton)
				.OnClicked(this, &SBlueprintMerge::OnCancelClicked)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CancelMerge", "Cancel"))
					.ToolTipText(LOCTEXT("CancelMergeToolTip", "Discards the changes made in the merge tool. Local file will remain unresolved so the merge tool can be run again if desired."))
				]
			]
			+ SVerticalBox::Slot()
			[
				SNew(SButton)
				.OnClicked(this, &SBlueprintMerge::OnTakeLocalClicked)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("LocalChanges", "Take Local Changes Only"))
					.ToolTipText(LOCTEXT("LocalChangesToolTip", "Resolve the conflict by discarding the changes made remotely. Conflict not fully resolved until the result is accepted."))
				]
			]
			+ SVerticalBox::Slot()
			[
				SNew(SButton)
				.OnClicked(this, &SBlueprintMerge::OnTakeRemoteClicked)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RemoteChanges", "Take Remote Changes Only"))
					.ToolTipText(LOCTEXT("RemoteChangesToolTip", "Resolve the conflict by discarding the changes made locally. Conflict not fully resolved until the result is accepted."))
				]
			]
			+ SVerticalBox::Slot()
			[
				SNew(SButton)
				.OnClicked(this, &SBlueprintMerge::OnTakeBaseClicked)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("BaseChanges", "Take Base Only (Revert All)"))
					.ToolTipText(LOCTEXT("BaseChangesToolTip", "Resolve the conflict by discarding the both local and remote changes. Conflict not fully resolved until the result is accepted."))
				]
			]
			+ SVerticalBox::Slot()
			[
				SNew(SButton)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AutoMerge", "Automatically Merge"))
					.ToolTipText(LOCTEXT("AutoMergeChangesToolTip", "Resolve the conflict by attempting to automatically apply both local and remote changes. Conflict not fully resolved until the result is accepted."))
				]
			]
		]
		+ SVerticalBox::Slot()
			.AutoHeight()
		[
			SAssignNew(DiffListBorder, SBorder)
		];
}

// @cr doc const shallowness
static UEdGraph* FindGraphByName(UBlueprint const& FromBlueprint, const FString& GraphName)
{
	TArray<UEdGraph*> Graphs;
	FromBlueprint.GetAllGraphs(Graphs);

	UEdGraph* Ret = NULL;
	if (UEdGraph** Result = Graphs.FindByPredicate(FMatchName(GraphName)))
	{
		Ret = *Result;
	}
	return Ret;
}

void SBlueprintMerge::HandleGraphChanged(const FString& GraphName)
{
	UEdGraph* GraphOld = FindGraphByName(*PanelOld.Blueprint, GraphName);
	UEdGraph* GraphNew = FindGraphByName(*PanelNew.Blueprint, GraphName);
	UEdGraph* GraphLocal = FindGraphByName(*PanelLocal.Blueprint, GraphName);
	UEdGraph* GraphResult = FindGraphByName(*BlueprintResult, GraphName);

	PanelOld.GeneratePanel(GraphOld, NULL);
	PanelNew.GeneratePanel(GraphNew, GraphOld);
	PanelLocal.GeneratePanel(GraphLocal, GraphOld);

	{
		// Set up a normal editor window so that the user can tweak the result of the merge:
		// @todo doc: need to copy the target blueprint and destructively edit that one, not 
		// the base blueprint..
		if (GraphResult)
		{
			SGraphEditor::FGraphEditorEvents InEvents;

			//FGraphAppearanceInfo AppearanceInfo;
			//AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_BlueprintDif", "DIFF").ToString();

			/*if (!GraphEditorCommands.IsValid())
			{
				GraphEditorCommands = MakeShareable(new FUICommandList());

				GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
					FExecuteAction::CreateRaw(this, &FDiffPanel::CopySelectedNodes),
					FCanExecuteAction::CreateRaw(this, &FDiffPanel::CanCopyNodes)
					);
			}*/

			auto Editor = SNew(SGraphEditor)
				.GraphToEdit(GraphResult)
				.TitleBarEnabledOnly(false)
				//.Appearance(AppearanceInfo)
				.GraphEvents(InEvents);
			EditorBorder->SetContent(Editor);
		}
		else
		{
			EditorBorder->ClearContent();
		}
	}
}

template< typename T >
static void CopyToShared( const TArray<T>& Source, TArray< TSharedPtr<T> >& Dest )
{
	check( Dest.Num() == 0 );
	for( auto i : Source )
	{
		Dest.Add( MakeShareable(new T( i )) );
	}
}

static void WriteBackup( UPackage& Package, const FString& Directory, const FString& Filename )
{
	if( GEditor )
	{
		const FString DestinationFilename = Directory + FString("/") + Filename;
		FString OriginalFilename;
		if( FPackageName::DoesPackageExist(Package.GetName(), NULL, &OriginalFilename) )
		{
			IFileManager::Get().Copy(*DestinationFilename, *OriginalFilename);
		}
	}
}

void SBlueprintMerge::OnSelectionChanged(SBlueprintDiff::FGraphToDiff Item, ESelectInfo::Type SelectionType)
{
	if (SelectionType != ESelectInfo::OnMouseClick || !Item.IsValid())
	{
		return;
	}

	FString GraphName = Item->GetGraphOld()->GetName();
	DisablePinDiffFocus(); // @todo doc: Not sure what this does..
	HandleGraphChanged(GraphName);

	// Ensure that regenerated graphs are properly locked/unlocked:
	ResetGraphEditors();

	// We'll diff the other two graphs against this shared base:
	UEdGraph* GraphBase = FindGraphByName(*PanelOld.Blueprint, GraphName);

	// Diff local version vs base:
	TArray<FDiffSingleResult> FoundDiffsLocal;
	UEdGraph* GraphLocal = FindGraphByName(*PanelLocal.Blueprint, GraphName);
	FGraphDiffControl::DiffGraphs(GraphLocal, GraphBase, FoundDiffsLocal);

	// Diff remote version vs. base:
	TArray<FDiffSingleResult> FoundDiffsRemote;
	UEdGraph* GraphRemote = FindGraphByName(*PanelNew.Blueprint, GraphName);
	FGraphDiffControl::DiffGraphs(GraphRemote, GraphBase, FoundDiffsRemote);

	// Display results of the two diffs:
	FMergeDifferencesListCommands const& Commands = FMergeDifferencesListCommands::Get();
	LocalDiffResults.Empty();
	TSharedRef< SWidget > DiffsLocal = GenerateDiffView(FoundDiffsLocal, Commands.LocalNext, Commands.LocalPrevious, LocalDiffResults );
	RemoteDiffResults.Empty();
	TSharedRef< SWidget > DiffsRemote = GenerateDiffView(FoundDiffsRemote, Commands.RemoteNext, Commands.RemotePrevious, RemoteDiffResults );

	DiffListBorder->SetContent( SNew( SVerticalBox )
		+ SVerticalBox::Slot()[DiffsLocal]
		+ SVerticalBox::Slot()[DiffsRemote] );
}

SGraphEditor* SBlueprintMerge::GetGraphEditorForGraph(UEdGraph* Graph) const
{
	if (PanelLocal.GraphEditor.Pin()->GetCurrentGraph() == Graph)
	{
		return PanelLocal.GraphEditor.Pin().Get();
	}
	return SBlueprintDiff::GetGraphEditorForGraph(Graph);
}

TSharedRef< SWidget > SBlueprintMerge::GenerateDiffView(TArray<FDiffSingleResult>& Diffs, TSharedPtr< const FUICommandInfo > CommandNext, TSharedPtr< const FUICommandInfo > CommandPrev, TArray< TSharedPtr<FDiffSingleResult> >& SharedResults )
{
	struct SortDiff
	{
		bool operator () (const FDiffSingleResult& A, const FDiffSingleResult& B) const
		{
			return A.Diff < B.Diff;
		}
	};

	Sort(Diffs.GetTypedData(), Diffs.Num(), SortDiff());

	TSharedPtr<FUICommandList> KeyCommands = MakeShareable(new FUICommandList);

	//KeyCommands->MapAction(CommandPrev, FExecuteAction::CreateSP(this, &FListItemGraphToDiff::PrevDiff));
	//KeyCommands->MapAction(CommandNext, FExecuteAction::CreateSP(this, &FListItemGraphToDiff::NextDiff));

	FToolBarBuilder ToolbarBuilder(KeyCommands, FMultiBoxCustomization::None);
	ToolbarBuilder.AddToolBarButton(CommandPrev, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintDif.PrevDiff"));
	ToolbarBuilder.AddToolBarButton(CommandNext, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintDif.NextDiff"));

	CopyToShared(Diffs, SharedResults);

	TSharedRef<SHorizontalBox> Result = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.MaxWidth(350.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(0.f)
			.AutoHeight()
			[
				ToolbarBuilder.MakeWidget()
			]
			+ SVerticalBox::Slot()
				.Padding(0.f)
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("PropertyWindow.CategoryBackground"))
					.Padding(FMargin(2.0f))
					.ForegroundColor(FEditorStyle::GetColor("PropertyWindow.CategoryForeground"))
					.ToolTipText(LOCTEXT("BlueprintDifDifferencesToolTip", "List of differences found between revisions, click to select"))
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("RevisionDifferences", "Revision Differences"))
					]
				]
			+ SVerticalBox::Slot()
				.Padding(1.f)
				.FillHeight(1.f)
				[
					SNew(SListView< TSharedPtr<FDiffSingleResult> >)
					.ItemHeight(24)
					.ListItemsSource(&SharedResults)
					.OnGenerateRow(this, &SBlueprintMerge::OnGenerateRow)
					.SelectionMode(ESelectionMode::Single)
					.OnSelectionChanged( this, &SBlueprintMerge::OnDiffListSelectionChanged)
				]
		];
	return Result;
}

FReply SBlueprintMerge::OnAcceptResultClicked()
{
	// Because merge operations are so destructive and can be confusing, lets write backups of the files involved:
	const FString BackupSubDir = FPaths::GameSavedDir() / TEXT("Backup") / TEXT("Resolve_Backup[") + FDateTime::Now().ToString(TEXT("%Y-%m-%d-%H-%M-%S")) + TEXT("]");
	WriteBackup(*PanelNew.Blueprint->GetOutermost(), BackupSubDir, TEXT("RemoteAsset") + FPackageName::GetAssetPackageExtension() );
	WriteBackup(*PanelOld.Blueprint->GetOutermost(), BackupSubDir, TEXT("CommonBaseAsset") + FPackageName::GetAssetPackageExtension() );
	WriteBackup(*PanelLocal.Blueprint->GetOutermost(), BackupSubDir, TEXT("LocalAsset") + FPackageName::GetAssetPackageExtension());

	UPackage* Package = PanelLocal.Blueprint->GetOutermost();
	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);

	// Perform the resolve with the SCC plugin, we do this first so that the editor doesn't warn about writing to a file that is unresolved:
	ISourceControlModule::Get().GetProvider().Execute(ISourceControlOperation::Create<FResolve>(), SourceControlHelpers::PackageFilenames(PackagesToSave), EConcurrency::Synchronous);

	// cr doc: this is too simple to be correct..
	PanelLocal.Blueprint->RemoveGeneratedClasses();
	StaticDuplicateObject( BlueprintResult, Package, *PanelLocal.Blueprint->GetName() );

	auto OwningWindowPinned = OwningWindow.Pin();
	if (OwningWindowPinned.IsValid())
	{
		OwningWindowPinned->RequestDestroyWindow();
	}

	const auto Result = FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, /*bCheckDirty=*/ false, /*bPromptToSave=*/ false);
	if (Result != FEditorFileUtils::PR_Success)
	{
		static const float MessageTime = 5.0f;
		const FText ErrorMessage = FText::Format( LOCTEXT("MergeWriteFailedError", "Failed to write merged files, please look for backups in {0}"), FText::FromString(BackupSubDir) );

		FNotificationInfo ErrorNotification(ErrorMessage);
		FSlateNotificationManager::Get().AddNotification( ErrorNotification );
	}

	return FReply::Handled();
}

FReply SBlueprintMerge::OnCancelClicked()
{
	auto OwningWindowPinned = OwningWindow.Pin();
	if( OwningWindowPinned.IsValid() )
	{
		OwningWindowPinned->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SBlueprintMerge::OnTakeLocalClicked()
{
	StageBlueprint(PanelLocal.Blueprint);
	return FReply::Handled();
}

FReply SBlueprintMerge::OnTakeRemoteClicked()
{
	StageBlueprint(PanelNew.Blueprint);
	return FReply::Handled();
}

FReply SBlueprintMerge::OnTakeBaseClicked()
{
	StageBlueprint(PanelOld.Blueprint);
	return FReply::Handled();
}

void SBlueprintMerge::StageBlueprint(UBlueprint const* DesiredBP)
{
	// User has decided to discard the remote changes, update the result window
	// with this result:
	if (BlueprintResult)
	{
		//delete BlueprintResult;... yeah.. i have no idea what's keeping this stuff alive, best not to delete it
	}

	BlueprintResult = DuplicateBlueprint(DesiredBP);

	(void)PackagesAppearEquivalent(BlueprintResult->GetOutermost(), DesiredBP->GetOutermost());
	//check( PackagesAppearEquivalent( BlueprintResult->GetOutermost(), DesiredBP->GetOutermost() ) );

	SGraphEditor::FGraphEditorEvents InEvents;
	TArray< FGraphToDiff>  Selected = GraphsToDiff->GetSelectedItems();
	check(Selected.Num() <= 1); // We set this control up to not support multiselect

	UEdGraph* GraphResult = NULL;

	if (Selected.Num() == 1)
	{
		GraphResult = FindGraphByName(*BlueprintResult, Selected[0]->GetGraphNew()->GetName());
	}

	if( GraphResult )
	{
		auto Editor = SNew(SGraphEditor)
			.GraphToEdit(GraphResult)
			.TitleBarEnabledOnly(false)
			//.Appearance(AppearanceInfo)
			.GraphEvents(InEvents);
		EditorBorder->SetContent(Editor);
	}
	else
	{
		EditorBorder->ClearContent();
	}
}

#undef LOCTEXT_NAMESPACE
