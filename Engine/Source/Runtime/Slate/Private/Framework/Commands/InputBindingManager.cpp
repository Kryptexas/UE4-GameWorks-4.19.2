// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	InputBindingManager.cpp: Implements the FInputBindingManager class.
=============================================================================*/

#include "Slate.h"
#include "RemoteConfigIni.h"


/* FUserDefinedGestures helper class
 *****************************************************************************/

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


/* FInputBindingManager structors
 *****************************************************************************/

FInputBindingManager& FInputBindingManager::Get()
{
	static FInputBindingManager* Instance= NULL;
	if( Instance == NULL )
	{
		Instance = new FInputBindingManager();
	}

	return *Instance;
}


/* FInputBindingManager interface
 *****************************************************************************/

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
