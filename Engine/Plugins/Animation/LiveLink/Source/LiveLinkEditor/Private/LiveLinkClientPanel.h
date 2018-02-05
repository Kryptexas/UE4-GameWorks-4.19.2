// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SListView.h"
#include "STreeView.h"
#include "EditorUndoClient.h"

struct FLiveLinkSourceUIEntry;
struct FLiveLinkSubjectUIEntry;
class FLiveLinkClient;
class ULiveLinkSourceFactory;
class ITableRow;
class STableViewBase;
class FMenuBuilder;
class FUICommandList;

typedef TSharedPtr<FLiveLinkSourceUIEntry> FLiveLinkSourceUIEntryPtr;
typedef TSharedPtr<FLiveLinkSubjectUIEntry> FLiveLinkSubjectUIEntryPtr;

class SLiveLinkClientPanel : public SCompoundWidget, public FGCObject, public FEditorUndoClient
{
	SLATE_BEGIN_ARGS(SLiveLinkClientPanel)
	{
	}

	SLATE_END_ARGS()

	~SLiveLinkClientPanel();

	void Construct(const FArguments& Args, FLiveLinkClient* InClient);

	// FGCObject interface
	void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End of FGCObject interface

	// FEditorUndoClient interface
	virtual void PostUndo(bool bSuccess);
	virtual void PostRedo(bool bSuccess);
	// End FEditorUndoClient interface

private:

	void BindCommands();

	void RefreshSourceData(bool bRefreshUI);

	TSharedRef< SWidget > GenerateSourceMenu();

	void RetrieveFactorySourcePanel(FMenuBuilder& MenuBuilder, ULiveLinkSourceFactory* FactoryCDO);

	FReply OnCloseSourceSelectionPanel(ULiveLinkSourceFactory* FactoryCDO, bool bMakeSource);

	void HandleOnAddFromFactory(ULiveLinkSourceFactory* InSourceFactory);

	// Starts adding a new virtual subject to the client
	void AddVirtualSubject();

	// Handles adding a virtual subject after the user has picked a name
	void HandleAddVirtualSubject(const FText& NewSubjectName, ETextCommit::Type CommitInfo);

	void HandleRemoveSource();

	bool CanRemoveSource();

	void HandleRemoveAllSources();

	// Registered with the client and called when client's sources change
	void OnSourcesChangedHandler();

	// Registered with the client and called when client's subjects change
	void OnSubjectsChangedHandler();

	// Controls whether the editor performance throttling warning should be visible
	EVisibility ShowEditorPerformanceThrottlingWarning() const;

	// Handle disabling of editor performance throttling
	FReply DisableEditorPerformanceThrottling();

private:
	const TArray<FLiveLinkSourceUIEntryPtr>& GetCurrentSources() const;

	TSharedRef<ITableRow> MakeSourceListViewWidget(FLiveLinkSourceUIEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable) const;
	
	void OnSourceListSelectionChanged(FLiveLinkSourceUIEntryPtr Entry, ESelectInfo::Type SelectionType) const;
	
	void OnPropertyChanged(const FPropertyChangedEvent& InEvent);

	// Helper functions for building the subject tree UI
	TSharedRef<ITableRow> MakeTreeRowWidget(FLiveLinkSubjectUIEntryPtr InInfo, const TSharedRef<STableViewBase>& OwnerTable);
	void GetChildrenForInfo(FLiveLinkSubjectUIEntryPtr InInfo, TArray< FLiveLinkSubjectUIEntryPtr >& OutChildren);
	void RebuildSubjectList();

	// Handler for the subject tree selection changing	
	void OnSelectionChanged(FLiveLinkSubjectUIEntryPtr BoneInfo, ESelectInfo::Type SelectInfo);

	//Source list widget
	TSharedPtr<SListView<FLiveLinkSourceUIEntryPtr>> ListView;

	//Source list items
	TArray<FLiveLinkSourceUIEntryPtr> SourceData;

	//Subject tree widget
	TSharedPtr<STreeView<FLiveLinkSubjectUIEntryPtr>> SubjectsTreeView;

	//Subject tree items
	TArray<FLiveLinkSubjectUIEntryPtr> SubjectData;

	TSharedPtr<FUICommandList> CommandList;

	FLiveLinkClient* Client;

	TMap<ULiveLinkSourceFactory*, TSharedPtr<SWidget>> SourcePanels;

	// Reference to connection settings struct details panel
	TSharedPtr<class IDetailsView> SettingsDetailsView;

	// Handle to delegate registered with client so we can update when a source disappears
	FDelegateHandle OnSourcesChangedHandle;

	// Handle to delegate registered with client so we can update when subject state changes
	FDelegateHandle OnSubjectsChangedHandle;

	//Map to cover 
	TMap<UClass*, UObject*> DetailsPanelEditorObjects;
};