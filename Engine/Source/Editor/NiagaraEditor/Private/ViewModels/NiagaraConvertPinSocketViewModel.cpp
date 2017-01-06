// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraConvertPinSocketViewModel.h"
#include "NiagaraConvertPinViewModel.h"
#include "NiagaraConvertNodeViewModel.h"
#include "NiagaraNodeConvert.h"

#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "NiagaraConvertPinSocketViewModel.h"

FNiagaraConvertPinSocketViewModel::FNiagaraConvertPinSocketViewModel(
	TSharedRef<FNiagaraConvertPinViewModel> InOwnerPinViewModel, 
	TSharedPtr<FNiagaraConvertPinSocketViewModel> InOwnerPinSocketViewModel,
	FName InName, 
	EEdGraphPinDirection InDirection)
{
	OwnerPinViewModel = InOwnerPinViewModel;
	OwnerPinSocketViewModel = InOwnerPinSocketViewModel;
	Name = InName;
	Direction = InDirection;
	ConstructPathText();
	bIsConnectedNeedsRefresh = true;
}

FName FNiagaraConvertPinSocketViewModel::GetName() const
{
	return Name;
}

TArray<FName> FNiagaraConvertPinSocketViewModel::GetPath() const
{
	TArray<FName> PathNames;
	const FNiagaraConvertPinSocketViewModel* CurrentSocket = this;
	while (CurrentSocket != nullptr)
	{
		PathNames.Insert(CurrentSocket->GetName(), 0);
		CurrentSocket = CurrentSocket->GetOwnerPinSocketViewModel().Get();
	}
	return PathNames;
}

FText FNiagaraConvertPinSocketViewModel::GetPathText() const
{
	return PathText;
}

EEdGraphPinDirection FNiagaraConvertPinSocketViewModel::GetDirection() const
{
	return Direction;
}

bool FNiagaraConvertPinSocketViewModel::GetIsConnected() const
{
	if (bIsConnectedNeedsRefresh)
	{
		RefreshIsConnected();
	}
	return bIsConnected;
}

bool FNiagaraConvertPinSocketViewModel::CanBeConnected() const
{
	return ChildSockets.Num() == 0;
}

EVisibility FNiagaraConvertPinSocketViewModel::GetSocketIconVisibility() const
{
	return CanBeConnected()
		? EVisibility::Visible
		: EVisibility::Hidden;
}

FVector2D FNiagaraConvertPinSocketViewModel::GetAbsoluteConnectionPosition() const
{
	return AbsoluteConnectionPosition;
}

void FNiagaraConvertPinSocketViewModel::SetAbsoluteConnectionPosition(FVector2D Position)
{
	AbsoluteConnectionPosition = Position;
}

const TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>>& FNiagaraConvertPinSocketViewModel::GetChildSockets() const
{
	return ChildSockets;
}

void FNiagaraConvertPinSocketViewModel::SetChildSockets(const TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>>& InChildSockets)
{
	ChildSockets = InChildSockets;
}

TSharedPtr<FNiagaraConvertPinSocketViewModel> FNiagaraConvertPinSocketViewModel::GetOwnerPinSocketViewModel() const
{
	return OwnerPinSocketViewModel.Pin();
}

TSharedPtr<FNiagaraConvertPinViewModel> FNiagaraConvertPinSocketViewModel::GetOwnerPinViewModel() const
{
	return OwnerPinViewModel.Pin();
}

TSharedPtr<FNiagaraConvertNodeViewModel> FNiagaraConvertPinSocketViewModel::GetOwnerConvertNodeViewModel() const
{
	TSharedPtr<FNiagaraConvertPinViewModel> PinnedOwnerPinViewModel = OwnerPinViewModel.Pin();
	if (PinnedOwnerPinViewModel.IsValid())
	{
		return PinnedOwnerPinViewModel->GetOwnerConvertNodeViewModel();
	}
	return TSharedPtr<FNiagaraConvertNodeViewModel>();
}

void FNiagaraConvertPinSocketViewModel::SetIsBeingDragged(bool bInIsBeingDragged)
{
	bIsBeingDragged = bInIsBeingDragged;
	TSharedPtr<FNiagaraConvertNodeViewModel> OwnerConvertNodeViewModel = GetOwnerConvertNodeViewModel();
	if (OwnerConvertNodeViewModel.IsValid())
	{
		if (bIsBeingDragged)
		{
			OwnerConvertNodeViewModel->SetDraggedSocketViewModel(this->AsShared());
		}
		else
		{
			if (OwnerConvertNodeViewModel->GetDraggedSocketViewModel().Get() == this)
			{
				OwnerConvertNodeViewModel->SetDraggedSocketViewModel(TSharedPtr<FNiagaraConvertPinSocketViewModel>());
			}
		}
	}
}

FVector2D FNiagaraConvertPinSocketViewModel::GetAbsoluteDragPosition() const
{
	return AbsoluteDragPosition;
}

void FNiagaraConvertPinSocketViewModel::SetAbsoluteDragPosition(FVector2D InAbsoluteDragPosition)
{
	AbsoluteDragPosition = InAbsoluteDragPosition;
}

bool FNiagaraConvertPinSocketViewModel::CanConnect(TSharedRef<FNiagaraConvertPinSocketViewModel> OtherSocket, FText& ConnectionMessage)
{
	TSharedPtr<FNiagaraConvertNodeViewModel> OwnerConvertNodeViewModel = GetOwnerConvertNodeViewModel();
	if (OwnerConvertNodeViewModel.IsValid())
	{
		return OwnerConvertNodeViewModel->CanConnectSockets(OtherSocket, this->AsShared(), ConnectionMessage);
	}
	else
	{
		ConnectionMessage = LOCTEXT("InvalidSocketConnectionMessage", "Can not connect because socket is in an invalid state.");
		return false;
	}
}

void FNiagaraConvertPinSocketViewModel::GetConnectedSockets(TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>>& ConnectedSockets)
{
	TSharedPtr<FNiagaraConvertNodeViewModel> OwnerConvertNodeViewModel = GetOwnerConvertNodeViewModel();
	if (OwnerConvertNodeViewModel.IsValid())
	{
		OwnerConvertNodeViewModel->GetConnectedSockets(this->AsShared(), ConnectedSockets);
	}
}

void FNiagaraConvertPinSocketViewModel::Connect(TSharedRef<FNiagaraConvertPinSocketViewModel> OtherSocket)
{
	TSharedPtr<FNiagaraConvertNodeViewModel> OwnerConvertNodeViewModel = GetOwnerConvertNodeViewModel();
	if (OwnerConvertNodeViewModel.IsValid())
	{
		FText ConnectionMessage;
		if (OwnerConvertNodeViewModel->CanConnectSockets(OtherSocket, this->AsShared(), ConnectionMessage))
		{
			OwnerConvertNodeViewModel->ConnectSockets(OtherSocket, this->AsShared());
			bIsConnectedNeedsRefresh = true;
		}
	}
}

void FNiagaraConvertPinSocketViewModel::DisconnectAll()
{
	FScopedTransaction DisconnectSpecificTransaction(LOCTEXT("DisconnectAllTransaction", "Break all inner links for pin."));
	TSharedPtr<FNiagaraConvertNodeViewModel> OwnerConvertNodeViewModel = GetOwnerConvertNodeViewModel();
	if (OwnerConvertNodeViewModel.IsValid())
	{
		OwnerConvertNodeViewModel->DisconnectSocket(this->AsShared());
	}
}

void FNiagaraConvertPinSocketViewModel::DisconnectSpecific(TSharedRef<FNiagaraConvertPinSocketViewModel> OtherSocket)
{
	FScopedTransaction DisconnectSpecificTransaction(LOCTEXT("DisconnectSpecificTransaction", "Break specific inner link for pin."));
	TSharedPtr<FNiagaraConvertNodeViewModel> OwnerConvertNodeViewModel = GetOwnerConvertNodeViewModel();
	if (OwnerConvertNodeViewModel.IsValid())
	{
		OwnerConvertNodeViewModel->DisconnectSockets(this->AsShared(), OtherSocket);
	}
}

void FNiagaraConvertPinSocketViewModel::ConstructPathText()
{
	TArray<FName> PathNames = GetPath();
	TArray<FString> PathStrings;
	for (const FName& PathName : PathNames)
	{
		PathStrings.Add(PathName.ToString());
	}
	PathText = FText::FromString(FString::Join(PathStrings, TEXT(".")));
}

void FNiagaraConvertPinSocketViewModel::RefreshIsConnected() const
{
	TSharedPtr<FNiagaraConvertNodeViewModel> OwnerConvertNodeViewModel = GetOwnerConvertNodeViewModel();
	bIsConnected = OwnerConvertNodeViewModel.IsValid() && OwnerConvertNodeViewModel->IsSocketConnected(this->AsShared());
}

#undef LOCTEXT_NAMESPACE