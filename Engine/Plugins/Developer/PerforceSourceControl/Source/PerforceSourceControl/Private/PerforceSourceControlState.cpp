// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PerforceSourceControlPrivatePCH.h"
#include "PerforceSourceControlState.h"

#define LOCTEXT_NAMESPACE "PerforceSourceControl.State"

int32 FPerforceSourceControlState::GetHistorySize() const
{
	return History.Num();
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FPerforceSourceControlState::GetHistoryItem( int32 HistoryIndex ) const
{
	check(History.IsValidIndex(HistoryIndex));
	return History[HistoryIndex];
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FPerforceSourceControlState::FindHistoryRevision( int32 RevisionNumber ) const
{
	for(auto Iter(History.CreateConstIterator()); Iter; Iter++)
	{
		if((*Iter)->GetRevisionNumber() == RevisionNumber)
		{
			return *Iter;
		}
	}

	return NULL;
}

FName FPerforceSourceControlState::GetIconName() const
{
	switch(State)
	{
	default:
	case EPerforceState::DontCare:
		return NAME_None;
	case EPerforceState::CheckedOut:
		return FName("Perforce.CheckedOut");
	case EPerforceState::ReadOnly:
		return NAME_None;
	case EPerforceState::NotCurrent:
		return FName("Perforce.NotAtHeadRevision");
	case EPerforceState::NotInDepot:
		return FName("Perforce.NotInDepot");
	case EPerforceState::CheckedOutOther:
		return FName("Perforce.CheckedOutByOtherUser");
	case EPerforceState::Ignore:
		return NAME_None;
	case EPerforceState::OpenForAdd:
		return FName("Perforce.OpenForAdd");
	case EPerforceState::MarkedForDelete:
		return NAME_None;
	}
}

FName FPerforceSourceControlState::GetSmallIconName() const
{
	switch(State)
	{
	default:
	case EPerforceState::DontCare:
		return NAME_None;
	case EPerforceState::CheckedOut:
		return FName("Perforce.CheckedOut_Small");
	case EPerforceState::ReadOnly:
		return NAME_None;
	case EPerforceState::NotCurrent:
		return FName("Perforce.NotAtHeadRevision_Small");
	case EPerforceState::NotInDepot:
		return FName("Perforce.NotInDepot_Small");
	case EPerforceState::CheckedOutOther:
		return FName("Perforce.CheckedOutByOtherUser_Small");
	case EPerforceState::Ignore:
		return NAME_None;
	case EPerforceState::OpenForAdd:
		return FName("Perforce.OpenForAdd_Small");
	case EPerforceState::MarkedForDelete:
		return NAME_None;
	}
}

FText FPerforceSourceControlState::GetDisplayName() const
{
	switch(State)
	{
	default:
	case EPerforceState::DontCare:
		return LOCTEXT("Unknown", "Unknown");
	case EPerforceState::CheckedOut:
		return LOCTEXT("CheckedOut", "Checked out");
	case EPerforceState::ReadOnly:
		return LOCTEXT("ReadOnly", "Read only");
	case EPerforceState::NotCurrent:
		return LOCTEXT("NotCurrent", "Not current");
	case EPerforceState::NotInDepot:
		return LOCTEXT("NotInDepot", "Not in depot");
	case EPerforceState::CheckedOutOther:
		return FText::Format( LOCTEXT("CheckedOutOther", "Checked out by: {0}"), FText::FromString(OtherUserCheckedOut) );
	case EPerforceState::Ignore:
		return LOCTEXT("Ignore", "Ignore");
	case EPerforceState::OpenForAdd:
		return LOCTEXT("OpenedForAdd", "Opened for add");
	case EPerforceState::MarkedForDelete:
		return LOCTEXT("MarkedForDelete", "Marked for delete");
	}
}

FText FPerforceSourceControlState::GetDisplayTooltip() const
{
	switch(State)
	{
	default:
	case EPerforceState::DontCare:
		return LOCTEXT("Unknown_Tooltip", "The file(s) status is unknown");
	case EPerforceState::CheckedOut:
		return LOCTEXT("CheckedOut_Tooltip", "The file(s) are checked out");
	case EPerforceState::ReadOnly:
		return LOCTEXT("ReadOnly_Tooltip", "The file(s) are marked locally as read-only");
	case EPerforceState::NotCurrent:
		return LOCTEXT("NotCurrent_Tooltip", "The file(s) are not at the head revision");
	case EPerforceState::NotInDepot:
		return LOCTEXT("NotInDepot_Tooltip", "The file(s) are not present in the Perforce depot");
	case EPerforceState::CheckedOutOther:
		return FText::Format( LOCTEXT("CheckedOutOther_Tooltip", "Checked out by: {0}"), FText::FromString(OtherUserCheckedOut) );
	case EPerforceState::Ignore:
		return LOCTEXT("Ignore_Tooltip", "The file(s) are ignored by Perforce");
	case EPerforceState::OpenForAdd:
		return LOCTEXT("OpenedForAdd_Tooltip", "The file(s) are opened for add");
	case EPerforceState::MarkedForDelete:
		return LOCTEXT("MarkedForDelete_Tooltip", "The file(s) are marked for delete");
	}
}

const FString& FPerforceSourceControlState::GetFilename() const
{
	return LocalFilename;
}

const FDateTime& FPerforceSourceControlState::GetTimeStamp() const
{
	return TimeStamp;
}

bool FPerforceSourceControlState::CanCheckout() const
{
	bool bCanDoCheckout = false;

	if ( State == EPerforceState::ReadOnly )
	{
		bCanDoCheckout = State == EPerforceState::ReadOnly;
	}
	// For non-binary, non-exclusive checkout files, we can check out and merge later
	else if ( !bBinary && !bExclusiveCheckout )	
	{
		bCanDoCheckout = State == EPerforceState::CheckedOutOther ||
						State == EPerforceState::NotCurrent;
	}

	return bCanDoCheckout;
}

bool FPerforceSourceControlState::IsCheckedOut() const
{
	return State == EPerforceState::CheckedOut;
}

bool FPerforceSourceControlState::IsCheckedOutOther(FString* Who) const
{
	if(Who != NULL)
	{
		*Who = OtherUserCheckedOut;
	}
	return State == EPerforceState::CheckedOutOther;
}

bool FPerforceSourceControlState::IsCurrent() const
{
	return State != EPerforceState::NotCurrent;
}

bool FPerforceSourceControlState::IsSourceControlled() const
{
	return State != EPerforceState::NotInDepot;
}

bool FPerforceSourceControlState::IsAdded() const
{
	return State == EPerforceState::OpenForAdd;
}

bool FPerforceSourceControlState::IsDeleted() const
{
	return State == EPerforceState::MarkedForDelete;
}

bool FPerforceSourceControlState::IsIgnored() const
{
	return State == EPerforceState::Ignore;
}

bool FPerforceSourceControlState::CanEdit() const
{
	return State == EPerforceState::CheckedOut || State == EPerforceState::OpenForAdd;
}

bool FPerforceSourceControlState::IsUnknown() const
{
	return State == EPerforceState::DontCare;
}

bool FPerforceSourceControlState::IsModified() const
{
	return bModifed;
}

#undef LOCTEXT_NAMESPACE
