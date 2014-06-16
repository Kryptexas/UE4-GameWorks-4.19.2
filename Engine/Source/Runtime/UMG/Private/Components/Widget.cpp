// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UWidget

UWidget::UWidget(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsEnabled = true;
	bIsVariable = true;
	bDesignTime = false;
	Visiblity = ESlateVisibility::Visible;
}

bool UWidget::GetIsEnabled() const
{
	return MyWidget.IsValid() ? MyWidget->IsEnabled() : bIsEnabled;
}

void UWidget::SetIsEnabled(bool bInIsEnabled)
{
	bIsEnabled = bInIsEnabled;
	if ( MyWidget.IsValid() )
	{
		MyWidget->SetEnabled(bInIsEnabled);
	}
}

TEnumAsByte<ESlateVisibility::Type> UWidget::GetVisibility()
{
	if ( MyWidget.IsValid() )
	{
		return UWidget::ConvertRuntimeToSerializedVisiblity(MyWidget->GetVisibility());
	}

	return Visiblity;
}

void UWidget::SetVisibility(TEnumAsByte<ESlateVisibility::Type> InVisibility)
{
	Visiblity = InVisibility;

	if ( MyWidget.IsValid() )
	{
		MyWidget->SetVisibility(UWidget::ConvertSerializedVisibilityToRuntime(InVisibility));
	}
}

void UWidget::SetToolTipText(const FText& InToolTipText)
{
	ToolTipText = InToolTipText;

	if ( MyWidget.IsValid() )
	{
		MyWidget->SetToolTipText(InToolTipText);
	}
}

bool UWidget::IsHovered() const
{
	return MyWidget->IsHovered();
}

TSharedRef<SWidget> UWidget::GetWidget() const
{
	if ( !MyWidget.IsValid() )
	{
		// We lie a bit about this being a const function.  If this is the first call it's not really const
		// and we need to construct and cache the widget for the first run.  But instead of forcing everyone
		// downstream to make RebuildWidget const and force every implementation to make things mutable, we
		// just blow away the const here.
		UWidget* MutableThis = const_cast<UWidget*>(this);
		
		MyWidget = MutableThis->RebuildWidget();
		MutableThis->SyncronizeProperties();
	}

	return MyWidget.ToSharedRef();
}

TSharedRef<SWidget> UWidget::BuildDesignTimeWidget(TSharedRef<SWidget> WrapWidget)
{
	if (IsDesignTime())
	{
		return SNew(SOverlay)
		
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			WrapWidget
		]
		
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.Visibility( EVisibility::HitTestInvisible )
			.BorderImage( FCoreStyle::Get().GetBrush("FocusRectangle") )
		];
	}
	else
	{
		return WrapWidget;
	}
}

#if WITH_EDITOR

TSharedRef<SWidget> UWidget::GetToolboxPreviewWidget() const
{
	return SNew(SImage);
	//.Image( FEditorStyle::GetBrush("UMGEditor.ToolboxPreviewWidget") );
}

void UWidget::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if ( MyWidget.IsValid() )
	{
		SyncronizeProperties();
	}
}

#endif

bool UWidget::Modify(bool bAlwaysMarkDirty)
{
	bool Modified = Super::Modify(bAlwaysMarkDirty);

	if ( Slot )
		Modified &= Slot->Modify(bAlwaysMarkDirty);

	return Modified;
}

TSharedRef<SWidget> UWidget::RebuildWidget()
{
	ensureMsg(false, TEXT("You must implement RebuildWidget() in your child class"));
	return SNew(SSpacer);
}

void UWidget::SyncronizeProperties()
{
	MyWidget->SetEnabled(OPTIONAL_BINDING(bool, bIsEnabled));
	MyWidget->SetVisibility(OPTIONAL_BINDING_CONVERT(ESlateVisibility::Type, Visiblity, EVisibility, ConvertVisibility));
	//MyWidget->SetCursor(OPTIONAL_BINDING(EMouseCursor)

	if ( !ToolTipText.IsEmpty() )
	{
			MyWidget->SetToolTipText(OPTIONAL_BINDING(FText, ToolTipText));
	}
}

bool UWidget::IsDesignTime() const
{
	return bDesignTime;
}

void UWidget::IsDesignTime(bool bInDesignTime)
{
	bDesignTime = bInDesignTime;
}

EVisibility UWidget::ConvertSerializedVisibilityToRuntime(TEnumAsByte<ESlateVisibility::Type> Input)
{
	switch ( Input )
	{
	case ESlateVisibility::Visible:
		return EVisibility::Visible;
	case ESlateVisibility::Collapsed:
		return EVisibility::Collapsed;
	case ESlateVisibility::Hidden:
		return EVisibility::Hidden;
	case ESlateVisibility::HitTestInvisible:
		return EVisibility::HitTestInvisible;
	case ESlateVisibility::SelfHitTestInvisible:
		return EVisibility::SelfHitTestInvisible;
	default:
		check(false);
		return EVisibility::Visible;
	}
}

TEnumAsByte<ESlateVisibility::Type> UWidget::ConvertRuntimeToSerializedVisiblity(const EVisibility& Input)
{
	if ( Input == EVisibility::Visible )
	{
		return ESlateVisibility::Visible;
	}
	else if ( Input == EVisibility::Collapsed )
	{
		return ESlateVisibility::Collapsed;
	}
	else if ( Input == EVisibility::Hidden )
	{
		return ESlateVisibility::Hidden;
	}
	else if ( Input == EVisibility::HitTestInvisible )
	{
		return ESlateVisibility::HitTestInvisible;
	}
	else if ( Input == EVisibility::SelfHitTestInvisible )
	{
		return ESlateVisibility::SelfHitTestInvisible;
	}
	else
	{
		check(false);
		return ESlateVisibility::Visible;
	}
}

FSizeParam UWidget::ConvertSerializedSizeParamToRuntime(const FSlateChildSize& Input)
{
	switch ( Input.SizeRule )
	{
	default:
	case ESlateSizeRule::Automatic:
		return FAuto();
	case ESlateSizeRule::Fill:
		return FStretch(Input.Value);
	}
}
