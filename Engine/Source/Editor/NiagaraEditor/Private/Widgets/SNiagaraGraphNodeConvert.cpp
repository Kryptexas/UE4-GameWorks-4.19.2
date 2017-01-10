// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SNiagaraGraphNodeConvert.h"
#include "NiagaraNodeConvert.h"
#include "NiagaraConvertNodeViewModel.h"
#include "NiagaraConvertPinViewModel.h"
#include "NiagaraConvertPinSocketViewModel.h"
#include "SNiagaraConvertPinSocket.h"

#include "GraphEditorSettings.h"
#include "DrawElements.h"
#include "SGraphPin.h"

void SNiagaraGraphNodeConvert::Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode)
{
	GraphNode = InGraphNode;
	UpdateGraphNode();
}

TSharedRef<SWidget> ConstructPinSocketsRecursive(const TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>>& SocketViewModels)
{
	TSharedRef<SVerticalBox> SocketBox = SNew(SVerticalBox);
	for (const TSharedRef<FNiagaraConvertPinSocketViewModel> SocketViewModel : SocketViewModels)
	{
		if(SocketViewModel->CanBeConnected())
		{
			SocketBox->AddSlot()
			.AutoHeight()
			.Padding(0, 0, 0, 1)
			[
				SNew(SNiagaraConvertPinSocket, SocketViewModel)
			];
		}

		if (SocketViewModel->GetChildSockets().Num() > 0)
		{
			SocketBox->AddSlot()
			.AutoHeight()
			.Padding(0, 0, 0, 5)
			[
				ConstructPinSocketsRecursive(SocketViewModel->GetChildSockets())
			];
		}
	}
	return SocketBox;
}

TSharedRef<SWidget> ConstructPinSockets(TSharedRef<FNiagaraConvertPinViewModel> PinViewModel)
{
	return ConstructPinSocketsRecursive(PinViewModel->GetSocketViewModels());
}

void SNiagaraGraphNodeConvert::UpdateGraphNode()
{
	UNiagaraNodeConvert* ConvertNode = Cast<UNiagaraNodeConvert>(GraphNode);
	if (ConvertNode != nullptr)
	{
		ConvertNodeViewModel = MakeShareable(new FNiagaraConvertNodeViewModel(*ConvertNode));
	}
	SGraphNode::UpdateGraphNode();
}

void SNiagaraGraphNodeConvert::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	TSharedPtr<FNiagaraConvertPinViewModel> ConvertPinViewModel = GetViewModelForPinWidget(PinToAdd);
	if (ConvertPinViewModel.IsValid() == false)
	{
		SGraphNode::AddPin(PinToAdd);
	}
	else
	{
		PinToAdd->SetOwner(SharedThis(this));

		const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
		const bool bAdvancedParameter = (PinObj != nullptr) && PinObj->bAdvancedView;
		if (bAdvancedParameter)
		{
			PinToAdd->SetVisibility(TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced));
		}

		float WirePadding = 20;
		float SocketPinPadding = 30;

		if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
		{
			FMargin InputPinPadding = Settings->GetInputPinPadding();
			InputPinPadding.Bottom = 3;
			InputPinPadding.Right += WirePadding;

			FMargin InputSocketPadding = InputPinPadding;
			InputSocketPadding.Top = 0;
			InputSocketPadding.Left += SocketPinPadding;

			LeftNodeBox->AddSlot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(InputPinPadding)
				[
					PinToAdd
				];
			LeftNodeBox->AddSlot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Top)
				.Padding(InputSocketPadding)
				[
					ConstructPinSockets(ConvertPinViewModel.ToSharedRef())
				];

			InputPins.Add(PinToAdd);
		}
		else // Direction == EEdGraphPinDirection::EGPD_Output
		{
			FMargin OutputPinPadding = Settings->GetOutputPinPadding();
			OutputPinPadding.Bottom = 3;
			OutputPinPadding.Left += WirePadding;

			FMargin OutputSocketPadding = OutputPinPadding;
			OutputSocketPadding.Top = 0;
			OutputSocketPadding.Right += SocketPinPadding;

			RightNodeBox->AddSlot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(OutputPinPadding)
				[
					PinToAdd
				];
			RightNodeBox->AddSlot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				.Padding(OutputSocketPadding)
				[
					ConstructPinSockets(ConvertPinViewModel.ToSharedRef())
				];
			OutputPins.Add(PinToAdd);
		}
	}
}

float DirectionOffset = 100;

int32 SNiagaraGraphNodeConvert::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 NewLayerId = SGraphNode::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	NewLayerId++;
	for (const TSharedRef<FNiagaraConvertConnectionViewModel>& ConnectionViewModel : ConvertNodeViewModel->GetConnectionViewModels())
	{
		FVector2D LocalStart = AllottedGeometry.AbsoluteToLocal(ConnectionViewModel->SourceSocket->GetAbsoluteConnectionPosition()) * GetContentScale().X;
		FVector2D StartDirection = FVector2D(DirectionOffset, 0);

		FVector2D LocalEnd = AllottedGeometry.AbsoluteToLocal(ConnectionViewModel->DestinationSocket->GetAbsoluteConnectionPosition()) * (1 / GetContentScale().X);
		FVector2D EndDirection = FVector2D(DirectionOffset, 0);

		FSlateDrawElement::MakeSpline(
			OutDrawElements,
			NewLayerId,
			AllottedGeometry.ToPaintGeometry(),
			LocalStart,
			StartDirection,
			LocalEnd,
			EndDirection,
			MyClippingRect,
			2);
	}

	if (ConvertNodeViewModel->GetDraggedSocketViewModel().IsValid())
	{
		FVector2D LocalStart = AllottedGeometry.AbsoluteToLocal(ConvertNodeViewModel->GetDraggedSocketViewModel()->GetAbsoluteConnectionPosition());
		FVector2D StartDirection = FVector2D(DirectionOffset, 0);

		FVector2D LocalEnd = AllottedGeometry.AbsoluteToLocal(ConvertNodeViewModel->GetDraggedSocketViewModel()->GetAbsoluteDragPosition()) + Inverse(Args.GetWindowToDesktopTransform());
		FVector2D EndDirection = FVector2D(DirectionOffset, 0);

		FSlateDrawElement::MakeSpline(
			OutDrawElements,
			NewLayerId,
			AllottedGeometry.ToPaintGeometry(),
			LocalStart,
			StartDirection,
			LocalEnd,
			EndDirection,
			MyClippingRect,
			2);
	}

	return NewLayerId;
}

TSharedPtr<FNiagaraConvertPinViewModel> SNiagaraGraphNodeConvert::GetViewModelForPinWidget(TSharedRef<SGraphPin> GraphPin)
{
	const TArray<TSharedRef<FNiagaraConvertPinViewModel>>& ViewModels = GraphPin->GetDirection() == EGPD_Input
		? ConvertNodeViewModel->GetInputPinViewModels()
		: ConvertNodeViewModel->GetOutputPinViewModels();

	auto FindByPin = [GraphPin](const TSharedRef<FNiagaraConvertPinViewModel> PinViewModel) { return &PinViewModel->GetGraphPin() == GraphPin->GetPinObj(); };
	const TSharedRef<FNiagaraConvertPinViewModel>* ViewModelPtr = ViewModels.FindByPredicate(FindByPin);

	return ViewModelPtr != nullptr
		? *ViewModelPtr
		: TSharedPtr<FNiagaraConvertPinViewModel>();
}
