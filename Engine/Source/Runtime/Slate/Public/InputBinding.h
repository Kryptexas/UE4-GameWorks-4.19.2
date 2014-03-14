// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Delegate.h"

/** Defines FExecuteAction delegate interface */
DECLARE_DELEGATE( FExecuteAction );

/** Defines FCanExecuteAction delegate interface.  Returns true when an action is able to execute. */
DECLARE_DELEGATE_RetVal( bool, FCanExecuteAction );

/** Defines FIsActionChecked delegate interface.  Returns true if the action is currently toggled on. */
DECLARE_DELEGATE_RetVal( bool, FIsActionChecked );

/** Defines FIsActionButtonVisible delegate interface.  Returns true when UI buttons associated with the action should be visible. */
DECLARE_DELEGATE_RetVal( bool, FIsActionButtonVisible );

namespace EModifierKey
{
	typedef uint8 Type;

	const Type	None	= 0;
	const Type	Control = 1 << 0;
	const Type	Alt		= 1 << 1;
	const Type	Shift	= 1 << 2;
};

/**
 * Raw input gesture that defines input that must be valid when           
 */
struct SLATE_API FInputGesture
{
	/** Key that must be pressed */
	FKey Key;
	/** True if control must be pressed */
	uint32 bCtrl:1;
	/** True if alt must be pressed */
	uint32 bAlt:1;
	/** True if shift must be pressed */
	uint32 bShift:1;

	FInputGesture()
	{
		bCtrl = bAlt = bShift = 0;
	}

	FInputGesture( EModifierKey::Type InModifierKeys, const FKey InKey )
		: Key( InKey )
	{
		bCtrl = (InModifierKeys & EModifierKey::Control) != 0;
		bAlt = (InModifierKeys & EModifierKey::Alt) != 0;
		bShift = (InModifierKeys & EModifierKey::Shift) != 0;
	}

	FInputGesture( const FKey InKey )
		: Key( InKey )
	{
		bCtrl = bAlt = bShift = 0;
	}

	FInputGesture( const FKey InKey, EModifierKey::Type InModifierKeys )
		: Key( InKey )
	{
		bCtrl = (InModifierKeys & EModifierKey::Control) != 0;
		bAlt = (InModifierKeys & EModifierKey::Alt) != 0;
		bShift = (InModifierKeys & EModifierKey::Shift) != 0;
	}

	FInputGesture( const FInputGesture& Other )
		: Key( Other.Key )
		, bCtrl( Other.bCtrl )
		, bAlt( Other.bAlt )
		, bShift( Other.bShift )
	{
	}

	/**
 	 * Sets this gesture to a new key and modifier state based on the provided template
	 * Should not be called directly.  Only used by the keybinding editor to set user defined keys
	 */
	void Set( FInputGesture InTemplate )
	{
		Key = InTemplate.Key;
		bCtrl = InTemplate.bCtrl;
		bAlt = InTemplate.bAlt;
		bShift = InTemplate.bShift;
	}

	/** 
	 * @return A localized string that represents the gesture 
	 */
	FText GetInputText() const;
	
	/**
	 * @return The key represented as a string
	 */
	FText GetKeyText() const;

	/** 
	 * @return True if any modifier keys must be pressed in the gesture
	 */
	bool HasAnyModifierKeys() const { return bCtrl || bAlt || bShift; }

	/**
	 * Determines if this gesture is valid.  A gesture is valid if it has a non modifier key that must be pressed
	 * and zero or more modifier keys that must be pressed
	 *
	 * @return true if the gesture is valid
	 */
	bool IsValidGesture() const
	{
		return (Key.IsValid() && !Key.IsModifierKey());
	}

	/**
	 * Comparison operators                 
	 */
	bool operator!=( const FInputGesture& Other ) const
	{
		return Key != Other.Key || bCtrl != Other.bCtrl || bAlt != Other.bAlt || bShift != Other.bShift;
	}

	bool operator==( const FInputGesture& Other ) const
	{
		return Key == Other.Key && bCtrl == Other.bCtrl && bAlt == Other.bAlt && bShift == Other.bShift;
	}

	friend uint32 GetTypeHash( const FInputGesture& Gesture )
	{
		return GetTypeHash( Gesture.Key ) ^ ( (Gesture.bCtrl << 2) | (Gesture.bShift << 1) | Gesture.bAlt);
	}
};


/** Types of user interfaces that can be associated with a user interface action */
namespace EUserInterfaceActionType
{
	enum Type
	{
		/** Momentary buttons or menu items.  These support enable state, and execute a delegate when clicked. */
		Button,

		/** Toggleable buttons or menu items that store on/off state.  These support enable state, and execute a delegate when toggled. */
		ToggleButton,
		
		/** Radio buttons are similar to toggle buttons in that they are for menu items that store on/off state.  However they should be used to indicate that menu items in a group can only be in one state */
		RadioButton,

		/** Similar to Button but will display a readonly checkbox next to the item. */
		Check
	};
};


struct FUIAction
{
	/**
	 * Default constructor
	 */
	FUIAction()
	{
	}

	/**
	 * Constructor that takes delegates to initialize the action with
	 *
     * @param	ExecuteAction		The delegate to call when the action should be executed
	 */
	FUIAction( FExecuteAction InitExecuteAction )
		: ExecuteAction( InitExecuteAction ),
		  CanExecuteAction( FCanExecuteAction::CreateRaw( &FSlateApplication::Get(), &FSlateApplication::IsNormalExecution )),
		  IsCheckedDelegate(),
		  IsActionVisibleDelegate()
	{

	}

	/**
	 * Constructor that takes delegates to initialize the action with
	 *
     * @param	ExecuteAction		The delegate to call when the action should be executed
     * @param	CanExecuteAction	The delegate to call to see if the action can be executed
	 */
	FUIAction( FExecuteAction InitExecuteAction, FCanExecuteAction InitCanExecuteAction )
		: ExecuteAction( InitExecuteAction ),
		  CanExecuteAction( InitCanExecuteAction ),
		  IsCheckedDelegate(),
		  IsActionVisibleDelegate()
	{
	}

	/**
	 * Constructor that takes delegates to initialize the action with
	 *
     * @param	ExecuteAction		The delegate to call when the action should be executed
     * @param	CanExecuteAction	The delegate to call to see if the action can be executed
     * @param	IsCheckedDelegate	The delegate to call to see if the action should appear checked when visualized
	 */
	FUIAction( FExecuteAction InitExecuteAction, FCanExecuteAction InitCanExecuteAction, FIsActionChecked InitIsCheckedDelegate )
		: ExecuteAction( InitExecuteAction ),
		  CanExecuteAction( InitCanExecuteAction ),
		  IsCheckedDelegate( InitIsCheckedDelegate ),
		  IsActionVisibleDelegate()
	{
	}

	/**
	 * Constructor that takes delegates to initialize the action with
	 *
     * @param	ExecuteAction			The delegate to call when the action should be executed
     * @param	CanExecuteAction		The delegate to call to see if the action can be executed
     * @param	IsCheckedDelegate		The delegate to call to see if the action should appear checked when visualized
	 * @param	IsActionVisibleDelegate	The delegate to call to see if the action should be visible
	 */
	FUIAction( FExecuteAction InitExecuteAction, FCanExecuteAction InitCanExecuteAction, FIsActionChecked InitIsCheckedDelegate, FIsActionButtonVisible InitIsActionVisibleDelegate )
		: ExecuteAction( InitExecuteAction ),
		  CanExecuteAction( InitCanExecuteAction ),
		  IsCheckedDelegate( InitIsCheckedDelegate ),
		  IsActionVisibleDelegate( InitIsActionVisibleDelegate )
	{
	}


	/**
	 * Checks to see if its currently safe to execute this action.
	 *
	 * @return	True if this action can be executed and we should reflect that state visually in the UI
	 */
	bool CanExecute() const
	{
		// Fire the 'can execute' delegate if we have one, otherwise always return true
		return CanExecuteAction.IsBound() ? CanExecuteAction.Execute() : true;
	}

	/**
	 * Executes this action
	 */
	bool Execute() const
	{
		// It's up to the programmer to ensure that the action is still valid by the time the user clicks on the
		// button.  Otherwise the user won't know why the action didn't take place!
		if( CanExecute() )
		{
			return ExecuteAction.ExecuteIfBound();
		}

		return false;
	}

	/**
	 * Queries the checked state for this action.  This is only valid for actions that are toggleable!
	 *
	 * @return	Checked state for this action
	 */
	bool IsChecked() const
	{
		if( IsCheckedDelegate.IsBound() )
		{
			const bool bIsChecked = IsCheckedDelegate.Execute();
			if( bIsChecked )
			{
				return true;
			}
		}

		return false;
	}

	bool IsBound() const
	{
		return ExecuteAction.IsBound();
	}

	/**
	 * Queries the visibility for this action.
	 *
	 * @return	Visibility state for this action
	 */
	EVisibility IsVisible() const
	{
		if (IsActionVisibleDelegate.IsBound())
		{
			const bool bIsVisible = IsActionVisibleDelegate.Execute();
			return bIsVisible ? EVisibility::Visible : EVisibility::Collapsed;
		}

		return EVisibility::Visible;
	}

	/**
	 * Equality operator
	 */
	bool operator==( const FUIAction& Other ) const
	{
		return
			(ExecuteAction == Other.ExecuteAction) &&
			(CanExecuteAction == Other.CanExecuteAction) &&
			(IsCheckedDelegate == Other.IsCheckedDelegate) &&
			(IsActionVisibleDelegate == Other.IsActionVisibleDelegate);
	}
	

	FExecuteAction ExecuteAction;
	FCanExecuteAction CanExecuteAction;
	FIsActionChecked IsCheckedDelegate;
	FIsActionButtonVisible IsActionVisibleDelegate;
};

class FUICommandInfo;

class SLATE_API FUICommandInfoDecl
{
	friend class FBindingContext;
public:
	FUICommandInfoDecl& DefaultGesture( const FInputGesture& InDefaultGesture );
	FUICommandInfoDecl& UserInterfaceType( EUserInterfaceActionType::Type InType );
	FUICommandInfoDecl& Icon( const FSlateIcon& InIcon );
	FUICommandInfoDecl& Description( const FText& InDesc );

	operator TSharedPtr<FUICommandInfo>() const;
	operator TSharedRef<FUICommandInfo>() const;
public:
	FUICommandInfoDecl( const TSharedRef<class FBindingContext>& InContext, const FName InCommandName, const FText& InLabel, const FText& InDesc  );
private:
	TSharedPtr<class FUICommandInfo> Info;
	const TSharedRef<FBindingContext>& Context;
};

/**
 * Represents a context in which input bindings are valid
 */
class SLATE_API FBindingContext : public TSharedFromThis<FBindingContext>
{
public:
	/**
	 * Constructor
	 *
	 * @param InContextName		The name of the context
	 * @param InContextDesc		The localized description of the context
	 * @param InContextParent	Optional parent context.  Bindings are not allowed to be the same between parent and child contexts
	 * @param InStyleSetName	The style set to find the icons in, eg) FCoreStyle::Get().GetStyleSetName()
	 */
	FBindingContext( const FName InContextName, const FText& InContextDesc, const FName InContextParent, const FName InStyleSetName )
		: ContextName( InContextName )
		, ContextParent( InContextParent )
		, ContextDesc( InContextDesc )
		, StyleSetName( InStyleSetName )
	{
		check(!InStyleSetName.IsNone());
	}

	FBindingContext( const FBindingContext &Other )
		: ContextName( Other.ContextName )
		, ContextParent( Other.ContextParent )
		, ContextDesc( Other.ContextDesc )
		, StyleSetName( Other.StyleSetName )
	{}

	/**
	 * Creates a new command declaration used to populate commands with data
	 */
	FUICommandInfoDecl NewCommand( const FName InCommandName, const FText& InCommandLabel, const FText& InCommandDesc );

	/**
	 * @return The name of the context
	 */
	FName GetContextName() const { return ContextName; }

	/**
	 * @return The name of the parent context (or NAME_None if there isnt one)
	 */
	FName GetContextParent() const { return ContextParent; }

	/**
	 * @return The name of the style set to find the icons in
	 */
	FName GetStyleSetName() const { return StyleSetName; }

	/**
 	 * @return The localized description of this context
	 */
	const FText& GetContextDesc() const { return ContextDesc; }

	friend uint32 GetTypeHash( const FBindingContext& Context )
	{
		return GetTypeHash( Context.ContextName );
	}

	bool operator==( const FBindingContext& Other ) const
	{
		return ContextName == Other.ContextName;
	}

	/** A delegate that is called when commands are registered or unregistered with a binding context */
	static FSimpleMulticastDelegate CommandsChanged; 
private:
	/** The name of the context */
	FName ContextName;
	/** The name of the parent context */
	FName ContextParent;
	/** The description of the context */
	FText ContextDesc;
	/** The style set to find the icons in */
	FName StyleSetName;
};

class SLATE_API FUICommandInfo
{
	friend class FInputBindingManager;
	friend class FUICommandInfoDecl;
public:
	/** Default constructor */
	FUICommandInfo( const FName InBindingContext )
		: ActiveGesture( new FInputGesture )
		, DefaultGesture( EKeys::Invalid, EModifierKey::None )
		, BindingContext( InBindingContext )
		, UserInterfaceType( EUserInterfaceActionType::Button )
	{
	}

	/**
	 * Returns the friendly, localized string name of the gesture that is required to perform the command
	 *
	 * @param	bAllGestures	If true a comma separated string with all secondary active gestures will be returned.  Otherwise just the main gesture string is returned
	 * @return	Localized friendly text for the gesture
	 */
	const FText GetInputText() const;

	/**
	 * @return	Returns the active gesture for this command
	 */
	const TSharedRef<const FInputGesture> GetActiveGesture() const { return ActiveGesture; }

	const FInputGesture& GetDefaultGesture() const { return DefaultGesture; }

	/** Utility function to make an FUICommandInfo */
	static void MakeCommandInfo( const TSharedRef<class FBindingContext>& InContext, TSharedPtr< FUICommandInfo >& OutCommand, const FName InCommandName, const FText& InCommandLabel, const FText& InCommandDesc, const FSlateIcon& InIcon, const EUserInterfaceActionType::Type InUserInterfaceType, const FInputGesture& InDefaultGesture );

	/** @return The display label for this command */
	const FText& GetLabel() const { return Label; }

	/** @return The description of this command */
	const FText& GetDescription() const { return Description; }

	/** @return The icon to used when this command is displayed in UI that shows icons */
	const FSlateIcon& GetIcon() const { return Icon; }

	/** @return The type of command this is.  Used to determine what UI to create for it */
	EUserInterfaceActionType::Type GetUserInterfaceType() const { return UserInterfaceType; }
	
	/** @return The name of the command */
	FName GetCommandName() const { return CommandName; }

	/** @return The name of the context where the command is valid */
	FName GetBindingContext() const { return BindingContext; }

	/** Sets the new active gesture for this command */
	void SetActiveGesture( const FInputGesture& NewGesture );

	/** Removes the active gesture from this command */
	void RemoveActiveGesture();

	/** 
	 * Makes a tooltip for this command.
	 * @param	InText	Optional dynamic text to be displayed in the tooltip.
	 * @return	The tooltip widget
	 */
	TSharedRef<class SToolTip> MakeTooltip( const TAttribute<FText>& InText = TAttribute<FText>() , const TAttribute< EVisibility >& InToolTipVisibility = TAttribute<EVisibility>()) const;

private:
	/** Input command that executes this action */
	TSharedRef<FInputGesture> ActiveGesture;

	/** Default display name of the command */
	FText Label;

	/** Localized help text for this command */
	FText Description;

	/** The default input gesture for this command (can be invalid) */
	FInputGesture DefaultGesture;

	/** Brush name for icon to use in tool bars and menu items to represent this command */
	FSlateIcon Icon;

	/** Brush name for icon to use in tool bars and menu items to represent this command in its toggled on (checked) state*/
	FName UIStyle;

	/** Name of the command */
	FName CommandName;

	/** The context in which this command is active */
	FName BindingContext;

	/** The type of user interface to associated with this action */
	EUserInterfaceActionType::Type UserInterfaceType;

};

/**
 * A drag drop operation for UI Commands
 */
class SLATE_API FUICommandDragDropOp : public FDragDropOperation
{
public:
	static FString GetTypeId() {static FString Type = TEXT("FUICommandDragDropOp"); return Type;}

	static TSharedRef<FUICommandDragDropOp> New( TSharedRef<const FUICommandInfo> InCommandInfo, FName InOriginMultiBox, TSharedPtr<SWidget> CustomDectorator, FVector2D DecoratorOffset );

	FUICommandDragDropOp( TSharedRef<const FUICommandInfo> InUICommand, FName InOriginMultiBox, TSharedPtr<SWidget> InCustomDecorator, FVector2D DecoratorOffset )
		: UICommand( InUICommand )
		, OriginMultiBox( InOriginMultiBox )
		, CustomDecorator( InCustomDecorator )
		, Offset( DecoratorOffset )
	{}

	/**
	 * Sets a delegate that will be called when the command is dropped
	 */
	void SetOnDropNotification( FSimpleDelegate InOnDropNotification ) { OnDropNotification = InOnDropNotification; }

	/** FDragDropOperation interface */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE;
	virtual void OnDragged( const class FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual void OnDrop( bool bDropWasHandled, const FPointerEvent& MouseEvent ) OVERRIDE;
public:
	/** UI command being dragged */
	TSharedPtr< const FUICommandInfo > UICommand;
	/** Multibox the UI command was dragged from if any*/
	FName OriginMultiBox;
	/** Custom decorator to display */
	TSharedPtr<SWidget> CustomDecorator;
	/** Offset from the cursor where the decorator should be displayed */
	FVector2D Offset;
	/** Delegate called when the command is dropped */
	FSimpleDelegate OnDropNotification;
};


typedef TMap< FName, TSharedPtr<FUICommandInfo> > FCommandInfoMap;
typedef TMap< FInputGesture, FName > FGestureMap;

/** Delegate for alerting subscribers the input manager records a user-defined gesture */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUserDefinedGestureChanged, const FUICommandInfo& );

/**
 * Manager responsible for creating and processing input bindings.                  
 */
class SLATE_API FInputBindingManager
{
public:
	/**
	 * @return The instance of this manager                   
	 */
	static FInputBindingManager& Get();


	/**
	 * Returns a list of all known input contexts
	 *
	 * @param OutInputContexts	The generated list of contexts
	 * @return A list of all known input contexts                   
	 */
	void GetKnownInputContexts( TArray< TSharedPtr<FBindingContext> >& OutInputContexts ) const;

	/**
	 * Look up a binding context by name.
	 */
	TSharedPtr<FBindingContext> GetContextByName( const FName& InContextName );

	/**
	 * Remove the context with this name
	 */
	void RemoveContextByName( const FName& InContextName );

	/**
	 * Creates an input command from the specified user interface action
	 * 
	 * @param InBindingContext		The context where the command is valid
	 * @param InUserInterfaceAction	The user interface action to create the input command from
	 *								Will automatically bind the user interface action to the command specified 
	 *								by the action so when the command's gesture is pressed, the action is executed
	 */
	void CreateInputCommand( const TSharedRef<FBindingContext>& InBindingContext, TSharedRef<FUICommandInfo> InUICommandInfo );


	/**
     * Returns a command info that is has the same active gesture as the provided gesture and is in the same binding context or parent context
     *
     * @param	InBindingContext		The context in which the command is valid
     * @param	InGesture				The gesture to match against commands
	 * @param	bCheckDefault			Whether or not to check the default gesture.  Will check the active gesture if false
     * @return	A pointer to the command info which has the InGesture as its active gesture or null if one cannot be found
     */
	const TSharedPtr<FUICommandInfo> GetCommandInfoFromInputGesture( const FName InBindingContext, const FInputGesture& InGesture, bool bCheckDefault ) const;

	/** 
	 * Finds the command in the provided context which uses the provided input gesture
	 * 
	 * @param InBindingContext	The binding context name
	 * @param InGesture			The gesture to check against when looking for commands
	 * @param bCheckDefault		Whether or not to check the default gesture of commands instead of active gestures
	 * @param OutActiveGestureIndex	The index into the commands active gesture array where the passed in gesture was matched.  INDEX_NONE if bCheckDefault is true or nothing was found.
	 */
	const TSharedPtr<FUICommandInfo> FindCommandInContext( const FName InBindingContext, const FInputGesture& InGesture, bool bCheckDefault ) const;

	/** 
	 * Finds the command in the provided context which has the provided name 
	 * 
	 * @param InBindingContext	The binding context name
	 * @param CommandName		The name of the command to find
	 */
	const TSharedPtr<FUICommandInfo> FindCommandInContext( const FName InBindingContext, const FName CommandName ) const;

	/**
	 * Called when the active gesture is changed on a command
	 *
	 * @param CommandInfo	The command that had the active gesture changed. 
	 */
	virtual void NotifyActiveGestureChanged( const FUICommandInfo& CommandInfo );
	
    /**
     * Saves the user defined gestures to a json file
     */
	void SaveInputBindings();

    /**
     * Removes any user defined gestures
     */
	void RemoveUserDefinedGestures();

	/**
	 * Returns all known command infos for a given binding context
	 *
	 * @param InBindingContext	The binding context to get command infos from
	 * @param OutCommandInfos	The list of command infos for the binding context
	 */
	void GetCommandInfosFromContext( const FName InBindingContext, TArray< TSharedPtr<FUICommandInfo> >& OutCommandInfos ) const;

	/** Registers a delegate to be called when a user-defined gesture is edited */
	void RegisterUserDefinedGestureChanged(const FOnUserDefinedGestureChanged::FDelegate& Delegate)
	{
		OnUserDefinedGestureChanged.Add(Delegate);
	}

	/** Unregisters a delegate to be called when a user-defined gesture is edited */
	void UnregisterUserDefinedGestureChanged(const FOnUserDefinedGestureChanged::FDelegate& Delegate)
	{
		OnUserDefinedGestureChanged.Remove(Delegate);
	}

private:
	FInputBindingManager();
	/**
     	 * Gets the user defined gesture (if any) from the provided command name
     	 * 
         * @param InBindingContext	The context in which the command is active
         * @param InCommandName		The name of the command to get the gesture from
         */
	bool GetUserDefinedGesture( const FName InBindingContext, const FName InCommandName, FInputGesture& OutUserDefinedGesture );

	/**
	 *	Checks a binding context for duplicate gestures 
	 */
	void CheckForDuplicateDefaultGestures( const FBindingContext& InBindingContext, TSharedPtr<FUICommandInfo> InCommandInfo ) const;

	/**
	 * Recursively finds all child contexts of the provided binding context.  
	 *
	 * @param InBindingContext	The binding context to search
	 * @param AllChildren		All the children of InBindingContext. InBindingContext is the first element in AllChildren
	 */
	void GetAllChildContexts( const FName InBindingContext, TArray<FName>& AllChildren ) const;

private:
	struct FContextEntry
	{
		/** A list of commands associated with the context */
		FCommandInfoMap CommandInfoMap;

		/** Gesture to command info map */
		FGestureMap GestureToCommandInfoMap;

		/** The binding context for this entry*/
		TSharedPtr< FBindingContext > BindingContext;
	};

	/** A mapping of context name to the associated entry map */
	TMap< FName, FContextEntry > ContextMap;
	
	/** A mapping of contexts to their child contexts */
	TMultiMap< FName, FName > ParentToChildMap;

	/** User defined gesture overrides for commands */
	TSharedPtr< class FUserDefinedGestures > UserDefinedGestures;

	/** Delegate called when a user-defined gesture is edited */
	FOnUserDefinedGestureChanged OnUserDefinedGestureChanged;
};

class SLATE_API FUICommandList : public TSharedFromThis<FUICommandList>
{
public:
	/** Determines if this UICommandList is capable of producing an action for the supplied command */
	DECLARE_DELEGATE_RetVal_OneParam(bool, FCanProduceActionForCommand, const TSharedRef<const FUICommandInfo>& /*Command*/);

	/**
     * Maps a command info to a series of delegates that are executed by a multibox or mouse/keyboard input
     *
     * @param InUICommandInfo	The command info to map
     * @param ExecuteAction		The delegate to call when the command should be executed
     */
	void MapAction( const TSharedPtr< const FUICommandInfo > InUICommandInfo, FExecuteAction ExecuteAction );

	/**
     * Maps a command info to a series of delegates that are executed by a multibox or mouse/keyboard input
     *
     * @param InUICommandInfo	The command info to map
     * @param ExecuteAction		The delegate to call when the command should be executed
     * @param CanExecuteAction	The delegate to call to see if the command can be executed
     */
	void MapAction( const TSharedPtr< const FUICommandInfo > InUICommandInfo, FExecuteAction ExecuteAction, FCanExecuteAction CanExecuteAction );

	/**
     * Maps a command info to a series of delegates that are executed by a multibox or mouse/keyboard input
     *
     * @param InUICommandInfo	The command info to map
     * @param ExecuteAction		The delegate to call when the command should be executed
     * @param CanExecuteAction	The delegate to call to see if the command can be executed
     * @param IsCheckedDelegate	The delegate to call to see if the command should appear checked when visualized in a multibox
     */
	void MapAction( const TSharedPtr< const FUICommandInfo > InUICommandInfo, FExecuteAction ExecuteAction, FCanExecuteAction CanExecuteAction, FIsActionChecked IsCheckedDelegate );

	/**
     * Maps a command info to a series of delegates that are executed by a multibox or mouse/keyboard input
     *
     * @param InUICommandInfo	The command info to map
     * @param ExecuteAction		The delegate to call when the command should be executed
     * @param CanExecuteAction	The delegate to call to see if the command can be executed
     * @param IsCheckedDelegate	The delegate to call to see if the command should appear checked when visualized in a multibox
	 * @param IsVisibleDelegate	The delegate to call to see if the command should appear or be hidden when visualized in a multibox
     */
	void MapAction( const TSharedPtr< const FUICommandInfo > InUICommandInfo, FExecuteAction ExecuteAction, FCanExecuteAction CanExecuteAction, FIsActionChecked IsCheckedDelegate, FIsActionButtonVisible IsVisibleDelegate );

	/**
     * Maps a command info to a series of delegates that are executed by a multibox or mouse/keyboard input
     *
     * @param InUICommandInfo	The command info to map
     * @param InUIAction		Action to map to this command
     */
	void MapAction( const TSharedPtr< const FUICommandInfo > InUICommandInfo, const FUIAction& InUIAction );

	/**
	 * Append commands in InCommandsToAppend to this command list.
	 */
	void Append( const TSharedRef<FUICommandList>& InCommandsToAppend );

	/**
     * Executes the action associated with the provided command info
     * Note: It is assumed at this point that CanExecuteAction was already checked
     *
     * @param InUICommandInfo	The command info execute
     */
	bool ExecuteAction( const TSharedRef< const FUICommandInfo > InUICommandInfo ) const;

	/**
     * Calls the CanExecuteAction associated with the provided command info to see if ExecuteAction can be called
     *
     * @param InUICommandInfo	The command info execute
     */
	bool CanExecuteAction( const TSharedRef< const FUICommandInfo > InUICommandInfo ) const;

	/**
	 * Attempts to execute the action associated with the provided command info
	 * Note: This will check if the action can be executed before finally executing the action
	 *
	 * @param InUICommandInfo	The command info execute
	 */
	bool TryExecuteAction( const TSharedRef< const FUICommandInfo > InUICommandInfo ) const;

	/**
     * Calls the IsVisible delegate associated with the provided command info to see if the command should be visible in a toolbar
     *
     * @param InUICommandInfo	The command info execute
     */
	EVisibility GetVisibility( const TSharedRef< const FUICommandInfo > InUICommandInfo ) const;

	/**
     * Calls the IsChecked delegate to see if the visualization of this command in a multibox should appear checked
     *
     * @param InUICommandInfo	The command info execute
     */
	bool IsChecked( const TSharedRef< const FUICommandInfo > InUICommandInfo ) const;

	/**
	 * Processes any UI commands which are activated by the specified keyboard event
	 *
	 * @param InKeyboardEvent	The keyboard event to check
	 *
	 * @return true if an action was processed
	 */
	bool ProcessCommandBindings( const FKeyboardEvent& InKeyboardEvent ) const;

	/**
	 * Processes any UI commands which are activated by the specified mouse event
	 *
	 * @param InKeyboardEvent	The mouse event to check
	 *
	 * @return true if an action was processed
	 */
	bool ProcessCommandBindings( const FPointerEvent& InMouseEvent ) const;

	/** Sets the delegate that determines if this UICommandList is capable of producing an action for the supplied command */
	void SetCanProduceActionForCommand( const FCanProduceActionForCommand& NewCanProduceActionForCommand ) { CanProduceActionForCommand = NewCanProduceActionForCommand; }

	/** 
	  * Attempts to find an action for the specified command in the current UICommandList. This is a wrapper for GetActionForCommandRecursively.
	  *
	  * @param Command				The UI command for which you are discovering an action
	  */
	const FUIAction* GetActionForCommand(TSharedPtr<const FUICommandInfo> Command) const;
private:
	/**
	 * Helper function to execute delegate or exec command associated with a command (if valid)
	 *
	 * @param Key		The current key that is pressed
	 * @param bCtrl		True if control is pressed
	 * @param bAlt		True if alt is pressed
	 * @param bShift	True if shift is pressed
	 * @param bRepeat	True if command is repeating (held)
	 * @return True if a command was executed, False otherwise
	 */
	bool ConditionalProcessCommandBindings( const FKey Key, bool bCtrl, bool bAlt, bool bShift, bool bRepeat ) const;

	/** 
	  * Attempts to find an action for the specified command in the current UICommandList. If it is not found, the action for the
	  * specified command is discovered in the children recursively then the parents recursively.
	  *
	  * @param Command				The UI command for which you are discovering an action
	  * @param bIncludeChildren		If true, children of this command list will be searched in the event that the action is not found
	  * @param bIncludeParents		If true, parents of this command list will be searched in the event that the action is not found
	  * @param OutVisitedLists		The set of visited lists during recursion. This is used to prevent cycles.
	  */
	const FUIAction* GetActionForCommandRecursively(const TSharedRef<const FUICommandInfo>& Command, bool bIncludeChildren, bool bIncludeParents, TSet<TSharedRef<const FUICommandList>>& InOutVisitedLists) const;

	/** Returns all contexts associated with this list. This is a wrapper for GatherContextsForListRecursively */
	void GatherContextsForList(TSet<FName>& OutAllContexts) const;

	/** Returns all contexts associated with this list. */
	void GatherContextsForListRecursively(TSet<FName>& OutAllContexts, TSet<TSharedRef<const FUICommandList>>& InOutVisitedLists) const;

private:
	typedef TMap< const TSharedPtr< const FUICommandInfo >, FUIAction > FUIBindingMap;
	
	/** Known contexts in this list.  Each context must be known so we can quickly look up commands from bindings */
	TSet<FName> ContextsInList;

	/** Mapping of command to action */
	FUIBindingMap UICommandBindingMap;

	/** The list of parent and children UICommandLists */
	TArray<TWeakPtr<FUICommandList>> ParentUICommandLists;
	TArray<TWeakPtr<FUICommandList>> ChildUICommandLists;

	/** Determines if this UICommandList is capable of producing an action for the supplied command */
	FCanProduceActionForCommand CanProduceActionForCommand;
};
