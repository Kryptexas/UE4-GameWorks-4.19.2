// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SharedPointer.h"
#include "EdGraph/EdGraphNode.h"
#include "Visibility.h"

class FNiagaraConvertPinViewModel;
class FNiagaraConvertNodeViewModel;

/** A view model for a connectible socket representing a property on a pin on a convert node. */
class FNiagaraConvertPinSocketViewModel : public TSharedFromThis<FNiagaraConvertPinSocketViewModel>
{
public:
	FNiagaraConvertPinSocketViewModel(
		TSharedRef<FNiagaraConvertPinViewModel> InOwnerPinViewModel, 
		TSharedPtr<FNiagaraConvertPinSocketViewModel> InOwnerPinSocketViewModel, 
		FName InName, 
		EEdGraphPinDirection InDirection);

	/** Gets the name of this socket which is the name of the property it represents. */
	FName GetName() const;

	/** Gets the path to this socket from the pin using the socket names. */
	TArray<FName> GetPath() const;

	/** Gets the path to this socket at text. */
	FText GetPathText() const;

	/** Gets the direction of this socket. */
	EEdGraphPinDirection GetDirection() const;

	/** Gets whether or not this socket is connected. */
	bool GetIsConnected() const;

	/** Gets whether or not this socket can be connected. */
	bool CanBeConnected() const;

	/** Gets the visibility of the connection icon for this socket. */
	EVisibility GetSocketIconVisibility() const;

	/** Gets the connection position for this socket in absolute coordinate space. */
	FVector2D GetAbsoluteConnectionPosition() const;

	/** Sets the connection position for this socket in absolute coordinate space. */
	void SetAbsoluteConnectionPosition(FVector2D Position);

	/** Gets the child sockets for this socket. */
	const TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>>& GetChildSockets() const;

	/** Sets the child sockets for this socket. */
	void SetChildSockets(const TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>>& InChildSockets);

	/** Gets the view model for this socket that owns this socket view model.  May be null if this
		is a root socket. */
	TSharedPtr<FNiagaraConvertPinSocketViewModel> GetOwnerPinSocketViewModel() const;

	/** Gets the view model for the pin that owns this socket view model. */
	TSharedPtr<FNiagaraConvertPinViewModel> GetOwnerPinViewModel() const;

	/** Gets the view model for the convert node that owns the pin that owns this socket view model. */
	TSharedPtr<FNiagaraConvertNodeViewModel> GetOwnerConvertNodeViewModel() const;

	/** Sets whether or not this socket is being dragged. */
	void SetIsBeingDragged(bool bInIsBeingDragged);

	/** Gets the absolute position of this socket when it is being dragged. */
	FVector2D GetAbsoluteDragPosition() const;

	/** Sets the absolute position of this socket when it is being dragged. */
	void SetAbsoluteDragPosition(FVector2D InAbsoluteDragPosition);

	/** Gets whether or not this socket can be connected to another socket and provides a message about the connection. */
	bool CanConnect(TSharedRef<FNiagaraConvertPinSocketViewModel> OtherSocket, FText& ConnectionMessage);

	/** Gets all sockets which are connected to this socket. */
	void GetConnectedSockets(TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>>& ConnectedSockets);

	/** Connects this socket to another socket. */
	void Connect(TSharedRef<FNiagaraConvertPinSocketViewModel> OtherSocket);

	/** Disconnects all connections to this socket. */
	void DisconnectAll();

	/** Disconnects this socket for a specific socket. */
	void DisconnectSpecific(TSharedRef<FNiagaraConvertPinSocketViewModel> OtherSocket);

private:
	/** Builds text representing the path to this socket from it's owning pin. */
	void ConstructPathText();

	/** Refreshes whether or not this pin is connected. */
	void RefreshIsConnected() const;

private:
	/** The pin view model that owns this socket view model. */
	TWeakPtr<FNiagaraConvertPinViewModel> OwnerPinViewModel;

	/** The socket view model which owns this view model if it's not a root socket. */
	TWeakPtr<FNiagaraConvertPinSocketViewModel> OwnerPinSocketViewModel;

	/** The name of this socket from the property it represents. */
	FName Name;

	/** The direction of this socket. */
	EEdGraphPinDirection Direction;

	/** The child sockets for this socket. */
	TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>> ChildSockets;

	/** The path to this socket as text. */
	FText PathText;

	/** The connection position of this socket in absolute space. */
	FVector2D AbsoluteConnectionPosition;

	/** Whether or not the bIsConnected variable should be refreshed before it is used. */
	bool bIsConnectedNeedsRefresh;

	/** Whether or not this socket is connected. */
	mutable bool bIsConnected;

	/** Whether or not this socket is being dragged. */
	bool bIsBeingDragged;

	/** The absolute drag position of this socket if it is being dragged. */
	FVector2D AbsoluteDragPosition;
};