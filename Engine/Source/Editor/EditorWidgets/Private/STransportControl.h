// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


class STransportControl : public ITransportControl, public FTickableEditorObject
{
public:
	SLATE_BEGIN_ARGS(STransportControl)
		: _TransportArgs() {}

		SLATE_ARGUMENT(FTransportControlArgs, TransportArgs)
	SLATE_END_ARGS()
		
	/**
	 * Construct the widget
	 * 
	 * @param InArgs   A declaration from which to construct the widget
	 */
	void Construct( const FArguments& InArgs );

	virtual ~STransportControl() {}

	using SWidget::Tick;

	// Begin FTickableObjectBase implementation
	virtual bool IsTickable() const override;
	virtual void Tick( float DeltaTime ) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT( STransportControl, STATGROUP_Tickables ); }
	// End FTickableObjectBase
private:
	const FSlateBrush* GetForwardStatusIcon() const;
	FText GetForwardStatusTooltip() const;
	FText GetRecordStatusTooltip() const;
	const FSlateBrush* GetBackwardStatusIcon() const;
	FSlateColor GetLoopStatusColor() const;

	/** Executes the OnTickPlayback delegate */
	EActiveTickReturnType TickPlayback( double InCurrentTime, float InDeltaTime );

	FReply OnToggleLooping();

private:
	/** The handle to the active tick */
	TWeakPtr<FActiveTickHandle> ActiveTickHandle;

	/** Whether the active tick is currently registered */
	bool bIsActiveTickRegistered;

	FTransportControlArgs TransportControlArgs;
};
