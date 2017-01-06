// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraConvertNodeViewModel.h"
#include "NiagaraNodeConvert.h"
#include "NiagaraConvertPinViewModel.h"
#include "NiagaraConvertPinSocketViewModel.h"
#include "EdGraphSchema_Niagara.h"

#define LOCTEXT_NAMESPACE "NiagaraConvertNodeViewModel"

FNiagaraConvertNodeViewModel::FNiagaraConvertNodeViewModel(UNiagaraNodeConvert& InConvertNode)
	: ConvertNode(InConvertNode)
	, bPinViewModelsNeedRefresh(true)
	, bConnectionViewModelsNewRefresh(true)
{
}

const TArray<TSharedRef<FNiagaraConvertPinViewModel>>& FNiagaraConvertNodeViewModel::GetInputPinViewModels()
{
	if (bPinViewModelsNeedRefresh)
	{
		RefreshPinViewModels();
	}
	return InputPinViewModels;
}

const TArray<TSharedRef<FNiagaraConvertPinViewModel>>& FNiagaraConvertNodeViewModel::GetOutputPinViewModels()
{
	if (bPinViewModelsNeedRefresh)
	{
		RefreshPinViewModels();
	}
	return OutputPinViewModels;
}

const TArray<TSharedRef<FNiagaraConvertConnectionViewModel>> FNiagaraConvertNodeViewModel::GetConnectionViewModels()
{
	if (bConnectionViewModelsNewRefresh)
	{
		RefreshConnectionViewModels();
	}
	return ConnectionViewModels;
}

TSharedPtr<FNiagaraConvertPinSocketViewModel> FNiagaraConvertNodeViewModel::GetDraggedSocketViewModel() const
{
	return DraggedSocketViewModel;
}

void FNiagaraConvertNodeViewModel::SetDraggedSocketViewModel(TSharedPtr<FNiagaraConvertPinSocketViewModel> DraggedSocket)
{
	DraggedSocketViewModel = DraggedSocket;
}

bool FNiagaraConvertNodeViewModel::IsSocketConnected(TSharedRef<const FNiagaraConvertPinSocketViewModel> Socket) const
{
	TSharedPtr<FNiagaraConvertPinViewModel> SocketPinViewModel = Socket->GetOwnerPinViewModel();
	if (SocketPinViewModel.IsValid())
	{
		TArray<FName> SocketPath = Socket->GetPath();
		for (FNiagaraConvertConnection& Connection : ConvertNode.GetConnections())
		{
			if (Socket->GetDirection() == EGPD_Input)
			{
				if (SocketPinViewModel->GetPinId() == Connection.SourcePinId && SocketPath == Connection.SourcePath)
				{
					return true;
				}
			}
			else // EGPD_Output
			{
				if (SocketPinViewModel->GetPinId() == Connection.DestinationPinId && SocketPath == Connection.DestinationPath)
				{
					return true;
				}
			}
		}
	}
	return false;
}

void FNiagaraConvertNodeViewModel::GetConnectedSockets(TSharedRef<const FNiagaraConvertPinSocketViewModel> Socket, TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>>& ConnectedSockets)
{
	TSharedPtr<FNiagaraConvertPinViewModel> SocketPinViewModel = Socket->GetOwnerPinViewModel();
	if (SocketPinViewModel.IsValid())
	{
		TArray<FName> SocketPath = Socket->GetPath();
		EEdGraphPinDirection ConnectedPinDirection = Socket->GetDirection() == EGPD_Input ? EGPD_Output : EGPD_Input;
		for (FNiagaraConvertConnection& Connection : ConvertNode.GetConnections())
		{
			FGuid ConnectedPinId;
			TArray<FName>* ConnectedPinPath;
			if (Socket->GetDirection() == EGPD_Input)
			{
				ConnectedPinId = Connection.DestinationPinId;
				ConnectedPinPath = &Connection.DestinationPath;
			}
			else // EGPD_Output
			{
				ConnectedPinId = Connection.SourcePinId;
				ConnectedPinPath = &Connection.SourcePath;
			}

			TSharedPtr<FNiagaraConvertPinSocketViewModel> ConnectedSocket = GetSocket(ConnectedPinId, *ConnectedPinPath, ConnectedPinDirection);
			if (ConnectedSocket.IsValid())
			{
				ConnectedSockets.Add(ConnectedSocket.ToSharedRef());
			}
		}
	}
}

bool FNiagaraConvertNodeViewModel::CanConnectSockets(TSharedRef<FNiagaraConvertPinSocketViewModel> SocketA, TSharedRef<FNiagaraConvertPinSocketViewModel> SocketB, FText& ConnectionMessage)
{
	if (SocketA->GetOwnerConvertNodeViewModel().Get() != this || SocketB->GetOwnerConvertNodeViewModel().Get() != this)
	{
		ConnectionMessage = LOCTEXT("DifferentConvertNodeConnectionMessage", "Can only connect pins from the same convert node.");
		return false;
	}

	if (SocketA->GetDirection() == SocketB->GetDirection())
	{
		ConnectionMessage = LOCTEXT("SamePinDirectionConnectionMessage", "Can only connect pins with different directions.");
		return false;
	}

	if (SocketA->GetOwnerPinViewModel().IsValid() == false || SocketB->GetOwnerPinViewModel().IsValid() == false)
	{
		ConnectionMessage = LOCTEXT("InvalidPinStateConnectionMessage", "Can not connect due to invalid pin state.");
		return false;
	}

	ConnectionMessage = FText::Format(LOCTEXT("ConnectFormat", "Connect {0} to {1}"), SocketA->GetPathText(), SocketB->GetPathText());
	return true;
}

void FNiagaraConvertNodeViewModel::ConnectSockets(TSharedRef<FNiagaraConvertPinSocketViewModel> SocketA, TSharedRef<FNiagaraConvertPinSocketViewModel> SocketB)
{
	TSharedRef<FNiagaraConvertPinSocketViewModel> InputSocket = SocketA->GetDirection() == EGPD_Input ? SocketA : SocketB;
	TSharedRef<FNiagaraConvertPinSocketViewModel> OutputSocket = SocketA->GetDirection() == EGPD_Output ? SocketA : SocketB;

	FGuid InputPinId = InputSocket->GetOwnerPinViewModel()->GetPinId();
	TArray<FName> InputPath = InputSocket->GetPath();

	FGuid OutputPinId = OutputSocket->GetOwnerPinViewModel()->GetPinId();
	TArray<FName> OutputPath = OutputSocket->GetPath();

	ConvertNode.Modify();

	// Remove any existing connections to the same destination.
	auto RemovePredicate = [&](FNiagaraConvertConnection& Connection)
	{
		return Connection.DestinationPinId == OutputPinId && Connection.DestinationPath == OutputPath;
	};

	ConvertNode.GetConnections().RemoveAll(RemovePredicate);
	ConvertNode.GetConnections().Add(FNiagaraConvertConnection(InputPinId, InputPath, OutputPinId, OutputPath));
	InvalidateConnectionViewModels();
}

void FNiagaraConvertNodeViewModel::DisconnectSocket(TSharedRef<FNiagaraConvertPinSocketViewModel> Socket)
{
	TSharedPtr<FNiagaraConvertPinViewModel> OwnerPin = Socket->GetOwnerPinViewModel();
	if (OwnerPin.IsValid())
	{
		ConvertNode.Modify();

		TArray<FName> Path = Socket->GetPath();
		auto RemovePredicate = [&](FNiagaraConvertConnection& Connection)
		{
			return (Connection.SourcePinId == OwnerPin->GetPinId() && Connection.SourcePath == Path) ||
				(Connection.DestinationPinId == OwnerPin->GetPinId() && Connection.DestinationPath == Path);
		};

		ConvertNode.GetConnections().RemoveAll(RemovePredicate);
		InvalidateConnectionViewModels();
	}
}

void FNiagaraConvertNodeViewModel::DisconnectSockets(TSharedRef<FNiagaraConvertPinSocketViewModel> SocketA, TSharedRef<FNiagaraConvertPinSocketViewModel> SocketB)
{
	TSharedPtr<FNiagaraConvertPinViewModel> OwnerPinA = SocketA->GetOwnerPinViewModel();
	TSharedPtr<FNiagaraConvertPinViewModel> OwnerPinB = SocketB->GetOwnerPinViewModel();
	if (OwnerPinA.IsValid() && OwnerPinB.IsValid())
	{
		ConvertNode.Modify();

		TArray<FName> PathA = SocketA->GetPath();
		TArray<FName> PathB = SocketB->GetPath();

		auto RemovePredicate = [&](FNiagaraConvertConnection& Connection)
		{
			bool SourceMatchesA = Connection.SourcePinId == OwnerPinA->GetPinId() && Connection.SourcePath == PathA;
			bool SourceMatchesB = Connection.SourcePinId == OwnerPinB->GetPinId() && Connection.SourcePath == PathB;
			bool DestinationMatchesA = Connection.DestinationPinId == OwnerPinA->GetPinId() && Connection.DestinationPath == PathA;
			bool DestinationMatchesB = Connection.DestinationPinId == OwnerPinB->GetPinId() && Connection.DestinationPath == PathB;
			return (SourceMatchesA && DestinationMatchesB) || (SourceMatchesB && DestinationMatchesA);
		};

		ConvertNode.GetConnections().RemoveAll(RemovePredicate);
		InvalidateConnectionViewModels();
	}
}

void FNiagaraConvertNodeViewModel::RefreshPinViewModels()
{
	InputPinViewModels.Empty();
	OutputPinViewModels.Empty();

	TArray<UEdGraphPin*> InputPins;
	TArray<UEdGraphPin*> OutputPins;
	ConvertNode.GetInputPins(InputPins);
	ConvertNode.GetOutputPins(OutputPins);

	for (UEdGraphPin* InputPin : InputPins)
	{
		if (InputPin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryType)
		{
			InputPinViewModels.Add(MakeShareable(new FNiagaraConvertPinViewModel(this->AsShared(), *InputPin)));
		}
	}

	for (UEdGraphPin* OutputPin : OutputPins)
	{
		if (OutputPin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryType)
		{
			OutputPinViewModels.Add(MakeShareable(new FNiagaraConvertPinViewModel(this->AsShared(), *OutputPin)));
		}
	}

	bPinViewModelsNeedRefresh = false;
}

void FNiagaraConvertNodeViewModel::InvalidateConnectionViewModels()
{
	bConnectionViewModelsNewRefresh = true;
}

void FNiagaraConvertNodeViewModel::RefreshConnectionViewModels()
{
	ConnectionViewModels.Empty();

	for (const FNiagaraConvertConnection& Connection : ConvertNode.GetConnections())
	{
		TSharedPtr<FNiagaraConvertPinSocketViewModel> SourceSocket = GetSocket(Connection.SourcePinId, Connection.SourcePath, EGPD_Input);
		TSharedPtr<FNiagaraConvertPinSocketViewModel> DestinationSocket = GetSocket(Connection.DestinationPinId, Connection.DestinationPath, EGPD_Output);
		if (SourceSocket.IsValid() && DestinationSocket.IsValid())
		{
			ConnectionViewModels.Add(MakeShareable(new FNiagaraConvertConnectionViewModel(SourceSocket.ToSharedRef(), DestinationSocket.ToSharedRef())));
		}
	}

	bConnectionViewModelsNewRefresh = false;
}

TSharedPtr<FNiagaraConvertPinSocketViewModel> GetSocketByPathRecursive(const TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>>& SocketViewModels, const TArray<FName>& Path, int32 PathIndex)
{
	if (PathIndex < Path.Num())
	{
		for (const TSharedRef<FNiagaraConvertPinSocketViewModel>& SocketViewModel : SocketViewModels)
		{
			if (SocketViewModel->GetName() == Path[PathIndex])
			{
				if (PathIndex == Path.Num() - 1)
				{
					return SocketViewModel;
				}
				else
				{
					return GetSocketByPathRecursive(SocketViewModel->GetChildSockets(), Path, PathIndex + 1);
				}
			}
		}
	}
	return TSharedPtr<FNiagaraConvertPinSocketViewModel>();
}

TSharedPtr<FNiagaraConvertPinSocketViewModel> FNiagaraConvertNodeViewModel::GetSocket(FGuid PinId, const TArray<FName> Path, EEdGraphPinDirection Direction)
{
	const TArray<TSharedRef<FNiagaraConvertPinViewModel>> PinViewModels = Direction == EGPD_Input
		? GetInputPinViewModels()
		: GetOutputPinViewModels();

	TSharedPtr<FNiagaraConvertPinViewModel> PathPinViewModel;
	for (TSharedRef<FNiagaraConvertPinViewModel> PinViewModel : PinViewModels)
	{
		if (PinViewModel->GetPinId() == PinId)
		{
			PathPinViewModel = PinViewModel;
			break;
		}
	}

	if (PathPinViewModel.IsValid())
	{
		return GetSocketByPathRecursive(PathPinViewModel->GetSocketViewModels(), Path, 0);
	}
	else
	{
		return TSharedPtr<FNiagaraConvertPinSocketViewModel>();
	}
}

#undef LOCTEXT_NAMESPACE