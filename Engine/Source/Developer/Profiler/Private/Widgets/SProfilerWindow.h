// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/*-----------------------------------------------------------------------------
	Type definitions
-----------------------------------------------------------------------------*/

/** Type definition for shared pointers to instances of SNotificationItem. */
typedef TSharedPtr<class SNotificationItem> SNotificationItemPtr;

/** Type definition for shared references to instances of SNotificationItem. */
typedef TSharedRef<class SNotificationItem> SNotificationItemRef;

/** Type definition for weak references to instances of SNotificationItem. */
typedef TWeakPtr<class SNotificationItem> SNotificationItemWeak;

/*-----------------------------------------------------------------------------
	SProfilerWindow class
-----------------------------------------------------------------------------*/

/**
 * Implements the profiler window.
 */
class SProfilerWindow : public SCompoundWidget
{
public:
	/** Default constructor. */
	SProfilerWindow();

	/** Virtual destructor. */
	virtual ~SProfilerWindow();

	SLATE_BEGIN_ARGS(SProfilerWindow){}
	SLATE_END_ARGS()
	
	/**
	 * Constructs this widget.
	 *
	 * @param InArgs					- the Slate argument list
	 * @param InSessionManager			- the session manager to use
	 */
	void Construct( const FArguments& InArgs, const ISessionManagerRef& InSessionManager );

	void ManageEventGraphTab( const FGuid ProfilerInstanceID, const bool bCreateFakeTab, const FString TabName );
	void UpdateEventGraph( const FGuid ProfilerInstanceID, const FEventGraphDataRef AverageEventGraph, const FEventGraphDataRef MaximumEventGraph, bool bInitial );

	void ManageLoadingProgressNotificationState( const FString& Filename, const EProfilerNotificationTypes::Type NotificatonType, const ELoadingProgressStates::Type ProgressState, const float DataLoadingProgress );

	void OpenProfilerSettings();
	void CloseProfilerSettings();

protected:
	/** Callback for determining the visibility of the 'Select a session' overlay. */
	EVisibility IsSessionOverlayVissible() const;

	/** Callback for getting the enabled state of the profiler window. */
	bool IsProfilerEnabled() const;

	void SendingServiceSideCapture_Cancel( const FString Filename );

	void SendingServiceSideCapture_Load( const FString Filename );

private:
	/**
	 * Called after a key is pressed when this widget has keyboard focus
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param  InKeyboardEvent  Keyboard event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;

	/**
	 * Called when the user is dropping something onto a widget; terminates drag and drop.
	 *
	 * @param MyGeometry      The geometry of the widget receiving the event.
	 * @param DragDropEvent   The drag and drop event.
	 *
	 * @return A reply that indicated whether this event was handled.
	 */
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;

	/**
	 * Called during drag and drop when the the mouse is being dragged over a widget.
	 *
	 * @param MyGeometry      The geometry of the widget receiving the event.
	 * @param DragDropEvent   The drag and drop event.
	 *
	 * @return A reply that indicated whether this event was handled.
	 */
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )  OVERRIDE;

	/** Holds all widgets for the profiler window like menu bar, toolbar and tabs. */
	TSharedPtr<SVerticalBox> MainContentPanel;

	/** Holds all event graphs. */
	TSharedPtr<SVerticalBox> EventGraphPanel;

	/** Holds the session manager. */
	ISessionManagerPtr SessionManager;

	/** Widget for the panel which contains all graphs and event graphs. */
	TSharedPtr<SProfilerGraphPanel> GraphPanel;

	/** Widget for the non-intrusive notifications. */
	TSharedPtr<SNotificationList> NotificationList;

	/** Overlay slot which contains the profiler settings widget. */
	SOverlay::FOverlaySlot* OverlaySettingsSlot;

	/** Holds all active and visible notifications, stored as FGuid -> SNotificationItemWeak . */
	TMap< FString, SNotificationItemWeak > ActiveNotifications;

	/** Active event graphs, one event graph for each profiler instance, stored as FGuid -> SEventGraph. */
	TMap< FGuid, TSharedRef<class SEventGraph> > ActiveEventGraphs;
};