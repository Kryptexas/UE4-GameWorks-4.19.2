// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "SListView.h"
#include "IMessageContext.h"
#include "MessageEndpoint.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Guid.h"

struct FLiveLinkPongMessage;
struct FMessageAddress;
class ITableRow;
class STableViewBase;

struct FProviderPollResult
{
public:
	FProviderPollResult(const FMessageAddress& InAddress, const FString& InName, const FString& InMachineName)
		: Address(InAddress)
		, Name(InName)
		, MachineName(InMachineName)
	{}

	FMessageAddress Address;

	FString			Name;
	FString			MachineName;
};

typedef TSharedPtr<FProviderPollResult> FProviderPollResultPtr;

class SLiveLinkMessageBusSourceEditor : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SLiveLinkMessageBusSourceEditor)
	{
	}

	SLATE_END_ARGS()

	~SLiveLinkMessageBusSourceEditor();

	void Construct(const FArguments& Args);

	FProviderPollResultPtr GetSelectedSource() const { return SelectedResult; }

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);

private:

	void HandlePongMessage(const FLiveLinkPongMessage& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);

	TSharedRef<ITableRow> MakeSourceListViewWidget(FProviderPollResultPtr PollResult, const TSharedRef<STableViewBase>& OwnerTable) const;

	void OnSourceListSelectionChanged(FProviderPollResultPtr PollResult, ESelectInfo::Type SelectionType);

	TSharedPtr<SListView<FProviderPollResultPtr>> ListView;

	TArray<FProviderPollResultPtr> PollData;

	FProviderPollResultPtr SelectedResult;

	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;

	FGuid CurrentPollRequest;

	// Time since our UI was last ticked, allow us to refresh if we haven't been onscreen for a while
	double LastTickTime;
};