// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "Json.h"
#include "InputBinding.h"
#include "RemoteConfigIni.h"

/** The maximum number of allowed active gestures per command */
const uint32 MaxActiveGestures = 3;

FSimpleMulticastDelegate FBindingContext::CommandsChanged;

/**
 * Returns the friendly, localized string name of this key binding
 * @todo Slate: Got to be a better way to do this
 */
FText FInputGesture::GetInputText() const
{
	FText FormatString = NSLOCTEXT("UICommand", "ShortcutKeyWithNoAdditionalModifierKeys", "{Key}");

	if ( bCtrl && !bAlt && !bShift )
	{
#if PLATFORM_MAC
		FormatString = NSLOCTEXT("UICommand", "KeyName_Command", "Command+{Key}");
#else
		FormatString = NSLOCTEXT("UICommand", "KeyName_Control", "Ctrl+{Key}");
#endif
	}
	else if ( bCtrl && bAlt && !bShift )
	{
#if PLATFORM_MAC
		FormatString = NSLOCTEXT("UICommand", "KeyName_Command + KeyName_Alt", "Command+Alt+{Key}");
#else
		FormatString = NSLOCTEXT("UICommand", "KeyName_Control + KeyName_Alt", "Ctrl+Alt+{Key}");
#endif
	}
	else if ( bCtrl && !bAlt && bShift )
	{
#if PLATFORM_MAC
		FormatString = NSLOCTEXT("UICommand", "KeyName_Command + KeyName_Shift", "Command+Shift+{Key}");
#else
		FormatString = NSLOCTEXT("UICommand", "KeyName_Control + KeyName_Shift", "Ctrl+Shift+{Key}");
#endif
	}
	else if ( bCtrl && bAlt && bShift )
	{
#if PLATFORM_MAC
		FormatString = NSLOCTEXT("UICommand", "KeyName_Command + KeyName_Alt + KeyName_Shift", "Command+Alt+Shift+{Key}");
#else
		FormatString = NSLOCTEXT("UICommand", "KeyName_Control + KeyName_Alt + KeyName_Shift", "Ctrl+Alt+Shift+{Key}");
#endif
	}
	else if ( !bCtrl && bAlt && !bShift )
	{
		FormatString = NSLOCTEXT("UICommand", "KeyName_Alt", "Alt+{Key}");
	}
	else if ( !bCtrl && bAlt && bShift )
	{
		FormatString = NSLOCTEXT("UICommand", "KeyName_Alt + KeyName_Shift", "Alt+Shift+{Key}");
	}
	else if ( !bCtrl && !bAlt && bShift )
	{
		FormatString = NSLOCTEXT("UICommand", "KeyName_Shift", "Shift+{Key}");
	}

	FFormatNamedArguments Args;
	Args.Add( TEXT("Key"), GetKeyText() );

	return FText::Format( FormatString, Args );
}

FText FInputGesture::GetKeyText() const
{
	FText OutString;		// = KeyGetDisplayName(Key);
	if (Key.IsValid() && !Key.IsModifierKey())
	{
		OutString = Key.GetDisplayName();
	}
	return OutString;
}

FUICommandInfoDecl FBindingContext::NewCommand( const FName InCommandName, const FText& InCommandLabel, const FText& InCommandDesc )
{
	return FUICommandInfoDecl( this->AsShared(), InCommandName, InCommandLabel, InCommandDesc );
}

FUICommandInfoDecl::FUICommandInfoDecl( const TSharedRef<FBindingContext>& InContext, const FName InCommandName, const FText& InLabel, const FText& InDesc )
	: Context( InContext )
{
	Info = MakeShareable( new FUICommandInfo( InContext->GetContextName() ) );
	Info->CommandName = InCommandName;
	Info->Label = InLabel;
	Info->Description = InDesc;
}

FUICommandInfoDecl& FUICommandInfoDecl::DefaultGesture( const FInputGesture& InDefaultGesture )
{
	Info->DefaultGesture = InDefaultGesture;
	return *this;
}
FUICommandInfoDecl& FUICommandInfoDecl::UserInterfaceType( EUserInterfaceActionType::Type InType )
{
	Info->UserInterfaceType = InType;
	return *this;
}

FUICommandInfoDecl& FUICommandInfoDecl::Icon( const FSlateIcon& InIcon )
{
	Info->Icon = InIcon;
	return *this;
}

FUICommandInfoDecl& FUICommandInfoDecl::Description( const FText& InDescription )
{
	Info->Description = InDescription;
	return *this;
}

FUICommandInfoDecl::operator TSharedPtr<FUICommandInfo>() const
{
	FInputBindingManager::Get().CreateInputCommand( Context, Info.ToSharedRef() );
	return Info;
}

FUICommandInfoDecl::operator TSharedRef<FUICommandInfo>() const
{
	FInputBindingManager::Get().CreateInputCommand( Context, Info.ToSharedRef() );
	return Info.ToSharedRef();
}

const FText FUICommandInfo::GetInputText() const
{	
	return ActiveGesture->GetInputText();
}


void FUICommandInfo::MakeCommandInfo( const TSharedRef<class FBindingContext>& InContext, TSharedPtr< FUICommandInfo >& OutCommand, const FName InCommandName, const FText& InCommandLabel, const FText& InCommandDesc, const FSlateIcon& InIcon, const EUserInterfaceActionType::Type InUserInterfaceType, const FInputGesture& InDefaultGesture )
{
	ensureMsgf( !InCommandLabel.IsEmpty(), TEXT("Command labels cannot be empty") );

	OutCommand = MakeShareable( new FUICommandInfo( InContext->GetContextName() ) );
	OutCommand->CommandName = InCommandName;
	OutCommand->Label = InCommandLabel;
	OutCommand->Description = InCommandDesc;
	OutCommand->Icon = InIcon;
	OutCommand->UserInterfaceType = InUserInterfaceType;
	OutCommand->DefaultGesture = InDefaultGesture;
	FInputBindingManager::Get().CreateInputCommand( InContext, OutCommand.ToSharedRef() );
}

void FUICommandInfo::SetActiveGesture( const FInputGesture& NewGesture )
{
	ActiveGesture->Set( NewGesture );

	// Set the user defined gesture for this command so it can be saved to disk later
	FInputBindingManager::Get().NotifyActiveGestureChanged( *this );
}

void FUICommandInfo::RemoveActiveGesture()
{
	// Gesture already exists
	// Reset the other gesture that has the same binding
	ActiveGesture = MakeShareable( new FInputGesture() );
	FInputBindingManager::Get().NotifyActiveGestureChanged( *this );
}

TSharedRef<SToolTip> FUICommandInfo::MakeTooltip( const TAttribute<FText>& InText, const TAttribute< EVisibility >& InToolTipVisibility ) const
{
	return 
		SNew(SToolTip)
		.Visibility(InToolTipVisibility.IsBound() ? InToolTipVisibility : EVisibility::Visible)
		.Content()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(InText.IsBound() ? InText : GetDescription())
				.Font(FCoreStyle::Get().GetFontStyle( "ToolTip.Font" ))
				.ColorAndOpacity( FSlateColor::UseForeground() )
			]
			+SVerticalBox::Slot()
			[
				SNew(STextBlock)
				.Text(GetInputText())
				.Font(FCoreStyle::Get().GetFontStyle( "ToolTip.Font" ))
				.ColorAndOpacity( FSlateColor::UseSubduedForeground() )
			]
		];
}

TSharedRef<FUICommandDragDropOp> FUICommandDragDropOp::New( TSharedRef<const FUICommandInfo> InCommandInfo, FName InOriginMultiBox, TSharedPtr<SWidget> CustomDectorator, FVector2D DecoratorOffset )
{
	TSharedRef<FUICommandDragDropOp> Operation = MakeShareable( new FUICommandDragDropOp( InCommandInfo, InOriginMultiBox, CustomDectorator, DecoratorOffset ) );

	FSlateApplication::GetDragDropReflector().RegisterOperation<FUICommandDragDropOp>(Operation);

	Operation->Construct();

	return Operation;
}

void FUICommandDragDropOp::OnDragged( const class FDragDropEvent& DragDropEvent )
{
	CursorDecoratorWindow->SetOpacity( .85f );
	CursorDecoratorWindow->MoveWindowTo( DragDropEvent.GetScreenSpacePosition() + Offset );
}

void FUICommandDragDropOp::OnDrop( bool bDropWasHandled, const FPointerEvent& MouseEvent )
{
	FDragDropOperation::OnDrop( bDropWasHandled, MouseEvent );

	OnDropNotification.ExecuteIfBound();
}


TSharedPtr<SWidget> FUICommandDragDropOp::GetDefaultDecorator() const
{
	TSharedPtr<SWidget> Content;

	if( CustomDecorator.IsValid() )
	{
		Content = CustomDecorator;
	}
	else
	{
		Content =
			SNew( STextBlock )
			.Text( UICommand->GetLabel() );
	}


	// Create hover widget
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Content()
		[	
			Content.ToSharedRef()
		];
}

/**
 * @return The instance of this manager                   
 */
FInputBindingManager& FInputBindingManager::Get()
{
	static FInputBindingManager* Instance= NULL;
	if( Instance == NULL )
	{
		Instance = new FInputBindingManager();
	}

	return *Instance;
}

FInputBindingManager::FInputBindingManager()
{ 
}

class FUserDefinedGestures
{
public:
	void LoadGestures();
	void SaveGestures();
	bool GetUserDefinedGesture( const FName BindingContext, const FName CommandName, FInputGesture& OutUserDefinedGesture );
	void SetUserDefinedGesture( const FUICommandInfo& CommandInfo );
	/** Remove all user defined gestures */
	void RemoveAll();
private:
	TSharedPtr<FJsonObject> Gestures;
};

void FUserDefinedGestures::LoadGestures()
{
	if( !Gestures.IsValid() )
	{
		FString Content;
		GConfig->GetString(TEXT("UserDefinedGestures"), TEXT("Content"), Content, GEditorKeyBindingsIni);

		TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( FRemoteConfig::ReplaceIniSpecialCharWithChar(Content).ReplaceEscapedCharWithChar() );
		bool bResult = FJsonSerializer::Deserialize( Reader, Gestures );

		if (!Gestures.IsValid())
		{
			// Gestures have not been loaded from the ini file, try reading them from the txt file now
			FArchive* Ar = IFileManager::Get().CreateFileReader( *( FPaths::GameSavedDir() / TEXT("Preferences/EditorKeyBindings.txt") ) );
			if( Ar )
			{
				TSharedRef< TJsonReader<ANSICHAR> > TextReader = TJsonReaderFactory<ANSICHAR>::Create( Ar );
				bResult = FJsonSerializer::Deserialize( TextReader, Gestures );
				delete Ar;
			}
		}
		
		if( !Gestures.IsValid() )
		{
			Gestures = MakeShareable( new FJsonObject );
		}
	}
}

void FUserDefinedGestures::SaveGestures()
{
	if( Gestures.IsValid() ) 
	{
		FString Content;
		TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create( &Content );
		FJsonSerializer::Serialize( Gestures.ToSharedRef(), Writer );

		GConfig->SetString(TEXT("UserDefinedGestures"), TEXT("Content"), *FRemoteConfig::ReplaceIniCharWithSpecialChar(Content).ReplaceCharWithEscapedChar(), GEditorKeyBindingsIni);
	}
}

bool FUserDefinedGestures::GetUserDefinedGesture( const FName BindingContext, const FName CommandName, FInputGesture& OutUserDefinedGesture )
{
	bool bResult = false;

	if( Gestures.IsValid() )
	{
		const TSharedPtr<FJsonValue> ContextObj = Gestures->Values.FindRef( BindingContext.ToString() );
		if( ContextObj.IsValid() )
		{
			const TSharedPtr<FJsonValue> CommandObj = ContextObj->AsObject()->Values.FindRef( CommandName.ToString() );
			if( CommandObj.IsValid() )
			{
				TSharedPtr<FJsonObject> GestureObj = CommandObj->AsObject();

				const TSharedPtr<FJsonValue> KeyObj = GestureObj->Values.FindRef( TEXT("Key") );
				const TSharedPtr<FJsonValue> CtrlObj = GestureObj->Values.FindRef( TEXT("Control") );
				const TSharedPtr<FJsonValue> AltObj = GestureObj->Values.FindRef( TEXT("Alt") );
				const TSharedPtr<FJsonValue> ShiftObj = GestureObj->Values.FindRef( TEXT("Shift") );

				OutUserDefinedGesture.Key = *KeyObj->AsString();
				OutUserDefinedGesture.bCtrl = CtrlObj->AsBool();
				OutUserDefinedGesture.bAlt = AltObj->AsBool();
				OutUserDefinedGesture.bShift = ShiftObj->AsBool();

				bResult = true;
			}
		}
	}

	return bResult;
}

void FUserDefinedGestures::SetUserDefinedGesture( const FUICommandInfo& CommandInfo )
{
	// Find or create the command context json value
	const FName BindingContext = CommandInfo.GetBindingContext();
	const FName CommandName = CommandInfo.GetCommandName();

	TSharedPtr<FJsonValue> ContextObj = Gestures->Values.FindRef( BindingContext.ToString() );
	if( !ContextObj.IsValid() )
	{
		ContextObj = Gestures->Values.Add( BindingContext.ToString(), MakeShareable( new FJsonValueObject( MakeShareable( new FJsonObject ) ) ) );
	}

	const TSharedPtr<const FInputGesture> InputGesture = CommandInfo.GetActiveGesture();

	FInputGesture GestureToSave;

	// Save an empty invalid gesture if one was not set
	// This is an indication that the user doesn't want this bound and not to use the default gesture
	if( InputGesture.IsValid() )
	{
		GestureToSave = *InputGesture;
	}

	{
		TSharedPtr<FJsonValueObject> GestureValueObj = MakeShareable( new FJsonValueObject( MakeShareable( new FJsonObject ) ) );
		TSharedPtr<FJsonObject> GestureObj = GestureValueObj->AsObject();

		// Set the gesture values for the command
		GestureObj->Values.Add( TEXT("Key"), MakeShareable( new FJsonValueString( GestureToSave.Key.ToString() ) ) );
		GestureObj->Values.Add( TEXT("Control"),  MakeShareable( new FJsonValueBoolean( GestureToSave.bCtrl ) ) );
		GestureObj->Values.Add( TEXT("Alt"),  MakeShareable( new FJsonValueBoolean( GestureToSave.bAlt ) ) );
		GestureObj->Values.Add( TEXT("Shift"),  MakeShareable( new FJsonValueBoolean( GestureToSave.bShift ) ) );

		ContextObj->AsObject()->Values.Add( CommandName.ToString(), GestureValueObj );
	}

}

void FUserDefinedGestures::RemoveAll()
{
	Gestures = MakeShareable(new FJsonObject);
}


/**
 * Gets the user defined gesture (if any) from the provided command name
 * 
 * @param InBindingContext	The context in which the command is active
 * @param InCommandName		The name of the command to get the gesture from
 */
bool FInputBindingManager::GetUserDefinedGesture( const FName InBindingContext, const FName InCommandName, FInputGesture& OutUserDefinedGesture )
{
	if( !UserDefinedGestures.IsValid() )
	{
		UserDefinedGestures = MakeShareable( new FUserDefinedGestures );
		UserDefinedGestures->LoadGestures();
	}

	return UserDefinedGestures->GetUserDefinedGesture( InBindingContext, InCommandName, OutUserDefinedGesture );
}

void FInputBindingManager::CheckForDuplicateDefaultGestures( const FBindingContext& InBindingContext, TSharedPtr<FUICommandInfo> InCommandInfo ) const
{
	const bool bCheckDefault = true;
	TSharedPtr<FUICommandInfo> ExistingInfo = GetCommandInfoFromInputGesture( InBindingContext.GetContextName(), InCommandInfo->DefaultGesture, bCheckDefault );
	if( ExistingInfo.IsValid() )
	{
		if( ExistingInfo->CommandName != InCommandInfo->CommandName )
		{
			// Two different commands with the same name in the same context or parent context
			UE_LOG(LogSlate, Fatal, TEXT("The command '%s.%s' has the same default gesture as '%s.%s' [%s]"), 
				*InCommandInfo->BindingContext.ToString(),
				*InCommandInfo->CommandName.ToString(), 
				*ExistingInfo->BindingContext.ToString(),
				*ExistingInfo->CommandName.ToString(), 
				*InCommandInfo->DefaultGesture.GetInputText().ToString() );
		}
	}
}


void FInputBindingManager::NotifyActiveGestureChanged( const FUICommandInfo& CommandInfo )
{
	FContextEntry& ContextEntry = ContextMap.FindChecked( CommandInfo.GetBindingContext() );

	// Slow but doesn't happen frequently
	for( FGestureMap::TIterator It( ContextEntry.GestureToCommandInfoMap ); It; ++It )
	{
		// Remove the currently active gesture from the map if one exists
		if( It.Value() == CommandInfo.GetCommandName() )
		{
			It.RemoveCurrent();
			// There should only be one active gesture
			break;
		}
	}

	if( CommandInfo.GetActiveGesture()->IsValidGesture() )
	{
		checkSlow( !ContextEntry.GestureToCommandInfoMap.Contains( *CommandInfo.GetActiveGesture() ) )
		ContextEntry.GestureToCommandInfoMap.Add( *CommandInfo.GetActiveGesture(), CommandInfo.GetCommandName() );
	}
	

	// The user defined gestures should have already been created
	check( UserDefinedGestures.IsValid() );

	UserDefinedGestures->SetUserDefinedGesture( CommandInfo );

	// Broadcast the gesture event when a new one is added
	OnUserDefinedGestureChanged.Broadcast(CommandInfo);
}

void FInputBindingManager::SaveInputBindings()
{
	if( UserDefinedGestures.IsValid() )
	{
		UserDefinedGestures->SaveGestures();
	}
}

void FInputBindingManager::RemoveUserDefinedGestures()
{
	if( UserDefinedGestures.IsValid() )
	{
		UserDefinedGestures->RemoveAll();
		UserDefinedGestures->SaveGestures();
	}
}

void FInputBindingManager::GetCommandInfosFromContext( const FName InBindingContext, TArray< TSharedPtr<FUICommandInfo> >& OutCommandInfos ) const
{
	ContextMap.FindRef( InBindingContext ).CommandInfoMap.GenerateValueArray( OutCommandInfos );
}

void FInputBindingManager::CreateInputCommand( const TSharedRef<FBindingContext>& InBindingContext, TSharedRef<FUICommandInfo> InCommandInfo )
{
	check( InCommandInfo->BindingContext == InBindingContext->GetContextName() );

	// The command name should be valid
	check( InCommandInfo->CommandName != NAME_None );

	// Should not have already created a gesture for this command
	check( !InCommandInfo->ActiveGesture->IsValidGesture() );
	
	const FName ContextName = InBindingContext->GetContextName();

	FContextEntry& ContextEntry = ContextMap.FindOrAdd( ContextName );
	
	// Our parent context must exist.
	check( InBindingContext->GetContextParent() == NAME_None || ContextMap.Find( InBindingContext->GetContextParent() ) != NULL );

	FCommandInfoMap& CommandInfoMap = ContextEntry.CommandInfoMap;

	if( !ContextEntry.BindingContext.IsValid() )
	{
		ContextEntry.BindingContext = InBindingContext;
	}

	if( InBindingContext->GetContextParent() != NAME_None  )
	{
		check( InBindingContext->GetContextName() != InBindingContext->GetContextParent() );
		// Set a mapping from the parent of the current context to the current context
		ParentToChildMap.AddUnique( InBindingContext->GetContextParent(), InBindingContext->GetContextName() );
	}

	if( InCommandInfo->DefaultGesture.IsValidGesture() )
	{
		CheckForDuplicateDefaultGestures( *InBindingContext, InCommandInfo );
	}
	
	{
		TSharedPtr<FUICommandInfo> ExistingInfo = CommandInfoMap.FindRef( InCommandInfo->CommandName );
		ensureMsgf( !ExistingInfo.IsValid(), TEXT("A command with name %s already exists in context %s"), *InCommandInfo->CommandName.ToString(), *InBindingContext->GetContextName().ToString() );
	}

	// Add the command info to the list of known infos.  It can only exist once.
	CommandInfoMap.Add( InCommandInfo->CommandName, InCommandInfo );

	// See if there are user defined gestures for this command
	FInputGesture UserDefinedGesture;
	bool bFoundUserDefinedGesture = GetUserDefinedGesture( ContextName, InCommandInfo->CommandName, UserDefinedGesture );

	
	if( !bFoundUserDefinedGesture && InCommandInfo->DefaultGesture.IsValidGesture() )
	{
		// Find any existing command with the same gesture 
		// This is for inconsistency between default and user defined gesture.  We need to make sure that if default gestures are changed to a gesture that a user set to a different command, that the default gesture doesn't replace
		// the existing commands gesture. Note: Duplicate default gestures are found above in CheckForDuplicateDefaultGestures
		FName ExisingCommand = ContextEntry.GestureToCommandInfoMap.FindRef( InCommandInfo->DefaultGesture );

		if( ExisingCommand == NAME_None )
		{
			// No existing command has a user defined gesture and no user defined gesture is available for this command 
			TSharedRef<FInputGesture> NewGesture = MakeShareable( new FInputGesture( InCommandInfo->DefaultGesture ) );
			InCommandInfo->ActiveGesture = NewGesture;
		}

	}
	else if( bFoundUserDefinedGesture )
	{
		// Find any existing command with the same gesture 
		// This is for inconsistency between default and user defined gesture.  We need to make sure that if default gestures are changed to a gesture that a user set to a different command, that the default gesture doesn't replace
		// the existing commands gesture.
		FName ExisingCommandName = ContextEntry.GestureToCommandInfoMap.FindRef( UserDefinedGesture );

		if( ExisingCommandName != NAME_None )
		{
			// Get the command with using the same gesture
			TSharedPtr<FUICommandInfo> ExistingInfo = CommandInfoMap.FindRef( ExisingCommandName );
			if( *ExistingInfo->ActiveGesture != ExistingInfo->DefaultGesture )
			{
				// two user defined gestures are the same within a context.  If the keybinding editor was used this wont happen so this must have been directly a modified user setting file
				UE_LOG(LogSlate, Error, TEXT("Duplicate user defined gestures found: [%s,%s].  Gesture for %s being removed"), *InCommandInfo->GetLabel().ToString(), *ExistingInfo->GetLabel().ToString(), *ExistingInfo->GetLabel().ToString() );
			}
			ContextEntry.GestureToCommandInfoMap.Remove( *ExistingInfo->ActiveGesture );
			// Remove the existing gesture. 
			ExistingInfo->ActiveGesture = MakeShareable( new FInputGesture() );
		
		}

		TSharedRef<FInputGesture> NewGesture = MakeShareable( new FInputGesture( UserDefinedGesture ) );
		// Set the active gesture on the command info
		InCommandInfo->ActiveGesture = NewGesture;
	}

	// If the active gesture is valid, map the gesture to the map for fast lookup when processing bindings
	if( InCommandInfo->ActiveGesture->IsValidGesture() )
	{
		checkSlow( !ContextEntry.GestureToCommandInfoMap.Contains( *InCommandInfo->GetActiveGesture() ) );
		ContextEntry.GestureToCommandInfoMap.Add( *InCommandInfo->GetActiveGesture(), InCommandInfo->GetCommandName() );
	}
}

const TSharedPtr<FUICommandInfo> FInputBindingManager::FindCommandInContext( const FName InBindingContext, const FInputGesture& InGesture, bool bCheckDefault ) const
{
	const FContextEntry& ContextEntry = ContextMap.FindRef( InBindingContext );
	
	TSharedPtr<FUICommandInfo> FoundCommand = NULL;

	if( bCheckDefault )
	{
		const FCommandInfoMap& InfoMap = ContextEntry.CommandInfoMap;
		for( FCommandInfoMap::TConstIterator It(InfoMap); It && !FoundCommand.IsValid(); ++It )
		{
			const FUICommandInfo& CommandInfo = *It.Value();
			if( CommandInfo.DefaultGesture == InGesture )
			{
				FoundCommand = It.Value();
			}	
		}
	}
	else
	{
		// faster lookup for active gestures
		FName CommandName = ContextEntry.GestureToCommandInfoMap.FindRef( InGesture );
		if( CommandName != NAME_None )
		{
			FoundCommand = ContextEntry.CommandInfoMap.FindChecked( CommandName );
		}
	}

	return FoundCommand;
}

const TSharedPtr<FUICommandInfo> FInputBindingManager::FindCommandInContext( const FName InBindingContext, const FName CommandName ) const 
{
	const FContextEntry& ContextEntry = ContextMap.FindRef( InBindingContext );
	
	return ContextEntry.CommandInfoMap.FindRef( CommandName );
}


void FInputBindingManager::GetAllChildContexts( const FName InBindingContext, TArray<FName>& AllChildren ) const
{
	AllChildren.Add( InBindingContext );

	TArray<FName> TempChildren;
	ParentToChildMap.MultiFind( InBindingContext, TempChildren );
	for( int32 ChildIndex = 0; ChildIndex < TempChildren.Num(); ++ChildIndex )
	{
		GetAllChildContexts( TempChildren[ChildIndex], AllChildren );
	}
}

const TSharedPtr<FUICommandInfo> FInputBindingManager::GetCommandInfoFromInputGesture( const FName InBindingContext, const FInputGesture& InGesture, bool bCheckDefault ) const
{
	TSharedPtr<FUICommandInfo> FoundCommand = NULL;

	FName CurrentContext = InBindingContext;
	while( CurrentContext != NAME_None && !FoundCommand.IsValid() )
	{
		const FContextEntry& ContextEntry = ContextMap.FindRef( CurrentContext );

		FoundCommand = FindCommandInContext( CurrentContext, InGesture, bCheckDefault );

		CurrentContext = ContextEntry.BindingContext->GetContextParent();
	}
	
	if( !FoundCommand.IsValid() )
	{
		TArray<FName> Children;
		GetAllChildContexts( InBindingContext, Children );

		for( int32 ContextIndex = 0; ContextIndex < Children.Num() && !FoundCommand.IsValid(); ++ContextIndex )
		{
			FoundCommand = FindCommandInContext( Children[ContextIndex], InGesture, bCheckDefault );
		}
	}
	

	return FoundCommand;
}

/**
 * Returns a list of all known input contexts
 *
 * @param OutInputContexts	The generated list of contexts
 * @return A list of all known input contexts                   
 */
void FInputBindingManager::GetKnownInputContexts( TArray< TSharedPtr<FBindingContext> >& OutInputContexts ) const
{
	for( TMap< FName, FContextEntry >::TConstIterator It(ContextMap); It; ++It )
	{
		OutInputContexts.Add( It.Value().BindingContext );
	}
}

TSharedPtr<FBindingContext> FInputBindingManager::GetContextByName( const FName& InContextName )
{
	FContextEntry* FindResult = ContextMap.Find( InContextName );
	if ( FindResult == NULL )
	{
		return TSharedPtr<FBindingContext>();
	}
	else
	{
		return FindResult->BindingContext;
	}
}

void FInputBindingManager::RemoveContextByName( const FName& InContextName )
{
	ContextMap.Remove(InContextName);
}



/**
 * Maps a command info to a series of delegates that are executed by a multibox or mouse/keyboard input
 *
 * @param InUICommandInfo	The command info to map
 * @param ExecuteAction		The delegate to call when the command should be executed
 */
void FUICommandList::MapAction( const TSharedPtr< const FUICommandInfo > InUICommandInfo, FExecuteAction ExecuteAction )
{
	MapAction( InUICommandInfo, ExecuteAction, FCanExecuteAction(), FIsActionChecked(), FIsActionButtonVisible() );
}


/**
 * Maps a command info to a series of delegates that are executed by a multibox or mouse/keyboard input
 *
 * @param InUICommandInfo	The command info to map
 * @param ExecuteAction		The delegate to call when the command should be executed
 * @param CanExecuteAction	The delegate to call to see if the command can be executed
 */
void FUICommandList::MapAction( const TSharedPtr< const FUICommandInfo > InUICommandInfo, FExecuteAction ExecuteAction, FCanExecuteAction CanExecuteAction )
{
	MapAction( InUICommandInfo, ExecuteAction, CanExecuteAction, FIsActionChecked(), FIsActionButtonVisible() );
}

/**
 * Maps a command info to a series of delegates that are executed by a multibox or mouse/keyboard input
 *
 * @param InUICommandInfo	The command info to map
 * @param ExecuteAction		The delegate to call when the command should be executed
 * @param CanExecuteAction	The delegate to call to see if the command can be executed
 * @param IsCheckedDelegate	The delegate to call to see if the command should appear checked when visualized in a multibox
 */
void FUICommandList::MapAction( const TSharedPtr< const FUICommandInfo > InUICommandInfo, FExecuteAction ExecuteAction, FCanExecuteAction CanExecuteAction, FIsActionChecked IsCheckedDelegate )
{
	MapAction( InUICommandInfo, ExecuteAction, CanExecuteAction, IsCheckedDelegate, FIsActionButtonVisible() );
}

void FUICommandList::MapAction( const TSharedPtr< const FUICommandInfo > InUICommandInfo, FExecuteAction ExecuteAction, FCanExecuteAction CanExecuteAction, FIsActionChecked IsCheckedDelegate, FIsActionButtonVisible IsVisibleDelegate )
{
	FUIAction Action;
	Action.ExecuteAction = ExecuteAction;
	Action.CanExecuteAction = CanExecuteAction;
	Action.IsCheckedDelegate = IsCheckedDelegate;
	Action.IsActionVisibleDelegate = IsVisibleDelegate;

	MapAction( InUICommandInfo, Action );

}

void FUICommandList::MapAction( const TSharedPtr< const FUICommandInfo > InUICommandInfo, const FUIAction& InUIAction )
{
	check( InUICommandInfo.IsValid() );

	// Check against already mapped actions
	checkSlow( !UICommandBindingMap.Contains( InUICommandInfo ) );

	ContextsInList.Add( InUICommandInfo->GetBindingContext() );
	UICommandBindingMap.Add( InUICommandInfo, InUIAction );
}


void FUICommandList::Append( const TSharedRef<FUICommandList>& InCommandsToAppend )
{
	check( &InCommandsToAppend.Get() != this );

	// Clear out any invalid parents or children
	CleanupPointerArray(ParentUICommandLists);
	CleanupPointerArray(ChildUICommandLists);

	// Add the new parent. Add this to the parent's child list.
	ParentUICommandLists.AddUnique( InCommandsToAppend );
	InCommandsToAppend->ChildUICommandLists.AddUnique( this->AsShared() );
}

/**
 * Executes the action associated with the provided command info
 * Note: It is assumed at this point that CanExecuteAction was already checked
 *
 * @param InUICommandInfo	The command info execute
 */
bool FUICommandList::ExecuteAction( const TSharedRef< const FUICommandInfo > InUICommandInfo ) const
{
	const FUIAction* Action = GetActionForCommand(InUICommandInfo);

	if( Action )
	{
		FSlateApplication::Get().OnLogSlateEvent(EEventLog::UICommand, InUICommandInfo->GetLabel());
		Action->Execute();
		return true;
	}

	return false;
}

/**
 * Calls the CanExecuteAction associated with the provided command info to see if ExecuteAction can be called
 *
 * @param InUICommandInfo	The command info execute
 */
bool FUICommandList::CanExecuteAction( const TSharedRef< const FUICommandInfo > InUICommandInfo ) const
{
	const FUIAction* Action = GetActionForCommand(InUICommandInfo);

	if( Action )
	{
		return Action->CanExecute();
	}

	// If there is no action then assume its possible to execute (Some menus with only children that do nothing themselves will have no delegates bound )
	return true;
}


bool FUICommandList::TryExecuteAction( const TSharedRef< const FUICommandInfo > InUICommandInfo ) const
{
	if( !CanExecuteAction( InUICommandInfo ) )
	{
		return false;
	}

	return ExecuteAction( InUICommandInfo );
}


/**
 * Calls the IsVisible delegate associated with the provided command info to see if the command should be visible in a toolbar
 *
 * @param InUICommandInfo	The command info execute
 */
EVisibility FUICommandList::GetVisibility( const TSharedRef< const FUICommandInfo > InUICommandInfo ) const
{
	const FUIAction* Action = GetActionForCommand(InUICommandInfo);

	if (Action)
	{
		return Action->IsVisible();
	}

	// If there is no action then assume it is visible
	return EVisibility::Visible;
}

/**
 * Calls the IsChecked delegate to see if the visualization of this command in a multibox should appear checked
 *
 * @param InUICommandInfo	The command info execute
 */
bool FUICommandList::IsChecked( const TSharedRef< const FUICommandInfo > InUICommandInfo ) const
{
	const FUIAction* Action = GetActionForCommand(InUICommandInfo);

	if( Action )
	{
		return Action->IsChecked();
	}

	return false;
}

/**
 * Processes any user interface actions which are activated by the specified keyboard event
 *
 * @param InBindingContext	The context in which the actions are valid
 * @param InKeyboardEvent	The keyboard event to check
 *
 * @return true if an action was processed
 */
bool FUICommandList::ProcessCommandBindings( const FKeyboardEvent& InKeyboardEvent ) const 
{
	return ConditionalProcessCommandBindings( InKeyboardEvent.GetKey(), InKeyboardEvent.IsControlDown(), InKeyboardEvent.IsAltDown(), InKeyboardEvent.IsShiftDown(), InKeyboardEvent.IsRepeat() );
}

/**
 * Processes any user interface actions which are activated by the specified mouse event
 *
 * @param InBindingContext	The context in which the actions are valid
 * @param InKeyboardEvent	The mouse event to check
 *
 * @return true if an action was processed
 */
bool FUICommandList::ProcessCommandBindings( const FPointerEvent& InMouseEvent ) const
{
	return ConditionalProcessCommandBindings( InMouseEvent.GetEffectingButton(), InMouseEvent.IsControlDown(), InMouseEvent.IsAltDown(), InMouseEvent.IsShiftDown(), InMouseEvent.IsRepeat() );
}

/**
 * Helper function to execute an interface action delegate or exec command if valid
 *
 * @param InBindingContext The binding context where commands are currently valid
 * @param Key		The current key that is pressed
 * @param bCtrl		True if control is pressed
 * @param bAlt		True if alt is pressed
 * @param bShift	True if shift is pressed
 * @param bRepeat	True if command is repeating (held)
 * @return True if a command was executed, False otherwise
 */
bool FUICommandList::ConditionalProcessCommandBindings( const FKey Key, bool bCtrl, bool bAlt, bool bShift, bool bRepeat ) const
{
	if ( !bRepeat && !FSlateApplication::Get().IsDragDropping() )
	{
		FInputGesture CheckGesture( Key );
		CheckGesture.bCtrl = bCtrl;
		CheckGesture.bAlt = bAlt;
		CheckGesture.bShift = bShift;

		if( CheckGesture.IsValidGesture() )
		{
			TSet<FName> AllContextsToCheck;
			GatherContextsForList(AllContextsToCheck);

			for( TSet<FName>::TConstIterator It(AllContextsToCheck); It; ++It )
			{
				FName Context = *It;
	
				// Only active gestures process commands
				const bool bCheckDefault = false;

				// Check to see if there is any command in the context activated by the gesture
				TSharedPtr<FUICommandInfo> Command = FInputBindingManager::Get().FindCommandInContext( Context, CheckGesture, bCheckDefault );

				if( Command.IsValid() && ensure( *Command->GetActiveGesture() == CheckGesture ) )
				{
					// Find the bound action for this command
					const FUIAction* Action = GetActionForCommand(Command);

					// If there is no Action mapped to this command list, continue to the next context
					if( Action )
					{
						if(Action->CanExecute())
						{
							// If the action was found and can be executed, do so now
							Action->Execute();
							return true;
						}
						else
						{
							// An action wasn't bound to the command but a gesture was found or an action was found but cant be executed
							return false;
						}
					}
				}
			}
		}
	}

	// No action was processed
	return false;
}

const FUIAction* FUICommandList::GetActionForCommand(TSharedPtr<const FUICommandInfo> Command) const
{
	// Make sure the command is valid
	if ( !ensure(Command.IsValid()) )
	{
		return NULL;
	}

	// Check in my own binding map. This should not be prevented by CanProduceActionForCommand.
	// Any action directly requested from a command list should be returned if it actually exists in the list.
	const FUIAction* Action = UICommandBindingMap.Find( Command );

	if ( !Action )
	{
		// We did not find the action in our own list. Recursively attempt to find the command in children and parents.
		const bool bIncludeChildren = true;
		const bool bIncludeParents = true;
		TSet<TSharedRef<const FUICommandList>> VisitedLists;
		Action = GetActionForCommandRecursively(Command.ToSharedRef(), bIncludeChildren, bIncludeParents, VisitedLists);
	}
	
	return Action;
}

const FUIAction* FUICommandList::GetActionForCommandRecursively(const TSharedRef<const FUICommandInfo>& Command, bool bIncludeChildren, bool bIncludeParents, TSet<TSharedRef<const FUICommandList>>& InOutVisitedLists) const
{
	// Detect cycles in the graph
	{
		const TSharedRef<const FUICommandList>& ListAsShared = AsShared();
		if ( InOutVisitedLists.Contains(ListAsShared) )
		{
			// This node was already visited. End recursion.
			return NULL;
		}

		InOutVisitedLists.Add( ListAsShared );
	}

	const FUIAction* Action = NULL;

	// Make sure I am capable of processing this command
	bool bCapableOfCommand = true;
	if ( CanProduceActionForCommand.IsBound() )
	{
		bCapableOfCommand = CanProduceActionForCommand.Execute(Command);
	}

	if ( bCapableOfCommand )
	{
		// Check in my own binding map
		Action = UICommandBindingMap.Find( Command );
	
		// If the action was not found, check in my children binding maps
		if ( !Action && bIncludeChildren )
		{
			for ( auto ChildIt = ChildUICommandLists.CreateConstIterator(); ChildIt; ++ChildIt )
			{
				TWeakPtr<FUICommandList> Child = *ChildIt;
				if ( Child.IsValid() )
				{
					const bool bShouldIncludeChildrenOfChild = true;
					const bool bShouldIncludeParentsOfChild = false;
					Action = Child.Pin()->GetActionForCommandRecursively(Command, bShouldIncludeChildrenOfChild, bShouldIncludeParentsOfChild, InOutVisitedLists);
					if ( Action )
					{
						break;
					}
				}
			}
		}
	}

	// If the action was not found, check in my parent binding maps
	if ( !Action && bIncludeParents )
	{
		for ( auto ParentIt = ParentUICommandLists.CreateConstIterator(); ParentIt; ++ParentIt )
		{
			TWeakPtr<FUICommandList> Parent = *ParentIt;
			if ( Parent.IsValid() )
			{
				const bool bShouldIncludeChildrenOfParent = false;
				const bool bShouldIncludeParentsOfParent = true;
				Action = Parent.Pin()->GetActionForCommandRecursively(Command, bShouldIncludeChildrenOfParent, bShouldIncludeParentsOfParent, InOutVisitedLists);
				if ( Action )
				{
					break;
				}
			}
		}
	}

	return Action;
}

void FUICommandList::GatherContextsForList(TSet<FName>& OutAllContexts) const
{
	TSet<TSharedRef<const FUICommandList>> VisitedLists;
	GatherContextsForListRecursively(OutAllContexts, VisitedLists);
}

void FUICommandList::GatherContextsForListRecursively(TSet<FName>& OutAllContexts, TSet<TSharedRef<const FUICommandList>>& InOutVisitedLists) const
{
	// Detect cycles in the graph
	{
		const TSharedRef<const FUICommandList>& ListAsShared = AsShared();
		if ( InOutVisitedLists.Contains(ListAsShared) )
		{
			// This node was already visited. End recursion.
			return;
		}

		InOutVisitedLists.Add( ListAsShared );
	}

	// Include all contexts on this list
	OutAllContexts.Append(ContextsInList);

	// Include all the parent contexts
	for ( auto ParentIt = ParentUICommandLists.CreateConstIterator(); ParentIt; ++ParentIt )
	{
		TWeakPtr<FUICommandList> Parent = *ParentIt;
		if ( Parent.IsValid() )
		{
			Parent.Pin()->GatherContextsForListRecursively(OutAllContexts, InOutVisitedLists);
		}
	}
}
