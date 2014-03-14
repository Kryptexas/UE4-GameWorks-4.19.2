// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISlateStyle.h"

/**
 * A slate style chunk that contains a collection of named properties that guide the appearance of Slate.
 * At the moment, basically FEditorStyle.
 */
class SLATE_API FSlateStyleSet : public ISlateStyle
{
public:
	/**
	 * Construct a style chunk.
	 * @param InStyleSetName The name used to identity this style set
	 */
	FSlateStyleSet(const FName& InStyleSetName)
		: StyleSetName( InStyleSetName )
		, DefaultBrush( new FSlateImageBrush( FPaths::EngineContentDir() / TEXT("Slate/Checkerboard.png"), FVector2D(16,16), FLinearColor::White, ESlateBrushTileType::Both ) )
	{
		// Add a mapping so that this resource will be discovered by GetStyleResources.
		Set( TEXT( "Default" ), GetDefaultBrush() );
	}

	/** Destructor. */
	virtual ~FSlateStyleSet()
	{
		// Delete all allocated brush resources.
		for( TMap< FName, FSlateBrush* >::TIterator It( BrushResources ) ; It ; ++ It )
		{
			delete It.Value();
		}
	}

	virtual const FName& GetStyleSetName() const OVERRIDE
	{
		return StyleSetName;
	}

	virtual void GetResources( TArray< const FSlateBrush* >& OutResources ) const OVERRIDE
	{
		// Collection for this style's brush resources.
		TArray< const FSlateBrush* > SlateBrushResources;
		for( TMap< FName, FSlateBrush* >::TConstIterator It( BrushResources ) ; It ; ++ It )
		{
			SlateBrushResources.Add( It.Value() );
		}

		//Gather resources from our definitions
		for( TMap< FName, TSharedRef< struct FSlateWidgetStyle > >::TConstIterator It( WidgetStyleValues ) ; It ; ++ It )
		{
			It.Value()->GetResources( SlateBrushResources );
		}

		// Append this style's resources to OutResources.
		OutResources.Append( SlateBrushResources );
	}

	virtual void SetContentRoot( const FString& InContentRootDir )
	{
		ContentRootDir = InContentRootDir;
	}

	virtual FString RootToContentDir( const ANSICHAR* RelativePath, const TCHAR* Extension )
	{
		return ( ContentRootDir / RelativePath ) + Extension;
	}

	virtual FString RootToContentDir( const WIDECHAR* RelativePath, const TCHAR* Extension )
	{
		return ( ContentRootDir / RelativePath ) + Extension;
	}

	virtual FString RootToContentDir( const FString& RelativePath, const TCHAR* Extension )
	{
		return ( ContentRootDir / RelativePath ) + Extension;
	}

	virtual FString RootToContentDir( const ANSICHAR* RelativePath )
	{
		return ( ContentRootDir / RelativePath );
	}

	virtual FString RootToContentDir( const WIDECHAR* RelativePath )
	{
		return ( ContentRootDir / RelativePath );
	}

	virtual FString RootToContentDir( const FString& RelativePath )
	{
		return ( ContentRootDir / RelativePath );
	}

	virtual void SetCoreContentRoot(const FString& InCoreContentRootDir)
	{
		CoreContentRootDir = InCoreContentRootDir;
	}

	virtual FString RootToCoreContentDir(const ANSICHAR* RelativePath, const TCHAR* Extension)
	{
		return (CoreContentRootDir / RelativePath) + Extension;
	}

	virtual FString RootToCoreContentDir(const WIDECHAR* RelativePath, const TCHAR* Extension)
	{
		return (CoreContentRootDir / RelativePath) + Extension;
	}

	virtual FString RootToCoreContentDir(const FString& RelativePath, const TCHAR* Extension)
	{
		return (CoreContentRootDir / RelativePath) + Extension;
	}

	virtual FString RootToCoreContentDir(const ANSICHAR* RelativePath)
	{
		return (CoreContentRootDir / RelativePath);
	}

	virtual FString RootToCoreContentDir(const WIDECHAR* RelativePath)
	{
		return (CoreContentRootDir / RelativePath);
	}

	virtual FString RootToCoreContentDir(const FString& RelativePath)
	{
		return (CoreContentRootDir / RelativePath);
	}
	virtual float GetFloat( const FName PropertyName, const ANSICHAR* Specifier = NULL ) const OVERRIDE
	{
		const float* Result = FloatValues.Find( Join( PropertyName, Specifier ) );

		return Result ? *Result : FStyleDefaults::GetFloat();
	}

	virtual FVector2D GetVector( const FName PropertyName, const ANSICHAR* Specifier = NULL ) const OVERRIDE
	{
		const FVector2D* const Result = Vector2DValues.Find( Join( PropertyName, Specifier ) );

		return Result ? *Result : FStyleDefaults::GetVector2D();
	}

	virtual const FLinearColor& GetColor( const FName PropertyName, const ANSICHAR* Specifier = NULL ) const OVERRIDE
	{
		const FLinearColor* Result = ColorValues.Find( Join( PropertyName, Specifier ) );

		return Result ? *Result : FStyleDefaults::GetColor();
	}

	virtual const FSlateColor GetSlateColor( const FName PropertyName, const ANSICHAR* Specifier = NULL ) const OVERRIDE
	{
		static FSlateColor UseForegroundStatic = FSlateColor::UseForeground();

		const FName LookupName( Join( PropertyName, Specifier ) );
		const FSlateColor* Result = SlateColorValues.Find( LookupName );

		if ( Result == NULL )
		{
			const FLinearColor* LinearColorLookup = ColorValues.Find( LookupName );
			if ( LinearColorLookup == NULL )
			{
				return UseForegroundStatic;
			}

			return *LinearColorLookup;
		}

		return *Result;
	}

	virtual const FMargin& GetMargin( const FName PropertyName, const ANSICHAR* Specifier = NULL ) const OVERRIDE
	{
		const FMargin* const Result = MarginValues.Find( Join( PropertyName, Specifier ) );

		return Result ? *Result : FStyleDefaults::GetMargin();
	}

	virtual const FSlateBrush* GetBrush( const FName PropertyName, const ANSICHAR* Specifier = NULL ) const OVERRIDE
	{
		const FName StyleName = Join( PropertyName, Specifier );
		const FSlateBrush* Result = BrushResources.FindRef( StyleName );

		if ( Result == NULL )
		{
			TWeakPtr< FSlateDynamicImageBrush > WeakImageBrush = DynamicBrushes.FindRef( StyleName );
			TSharedPtr< FSlateDynamicImageBrush > ImageBrush = WeakImageBrush.Pin();

			if ( ImageBrush.IsValid() )
			{
				Result = ImageBrush.Get();
			}
		}

		return Result ? Result : GetDefaultBrush();
	}

	const TSharedPtr< FSlateDynamicImageBrush > GetDynamicImageBrush( const FName BrushTemplate, const FName TextureName, const ANSICHAR* Specifier = NULL )
	{
		return GetDynamicImageBrush( BrushTemplate, Specifier, NULL, TextureName );
	}

	const TSharedPtr< FSlateDynamicImageBrush > GetDynamicImageBrush( const FName BrushTemplate, const ANSICHAR* Specifier, UTexture2D* TextureResource, const FName TextureName )
	{
		return GetDynamicImageBrush( Join( BrushTemplate, Specifier ), TextureResource, TextureName );
	}

	const TSharedPtr< FSlateDynamicImageBrush > GetDynamicImageBrush( const FName BrushTemplate, UTexture2D* TextureResource, const FName TextureName )
	{
		//create a resource name
		FName ResourceName;
		ResourceName = TextureName == NAME_None ? BrushTemplate : FName( *( BrushTemplate.ToString() + TextureName.ToString() ) );

		//see if we already have that brush
		TWeakPtr< FSlateDynamicImageBrush > WeakImageBrush = DynamicBrushes.FindRef( ResourceName );

		//if we don't have the image brush, then make it
		TSharedPtr< FSlateDynamicImageBrush > ReturnBrush = WeakImageBrush.Pin();

		if ( !ReturnBrush.IsValid() )
		{
			const FSlateBrush* Result = BrushResources.FindRef( Join( BrushTemplate, NULL ) );

			if( Result == NULL )
			{
				Result = GetDefaultBrush();
			}

			//create the new brush
			ReturnBrush = MakeShareable( new FSlateDynamicImageBrush( TextureResource,Result->ImageSize, ResourceName ) );

			//add it to the dynamic brush list
			DynamicBrushes.Add( ResourceName, ReturnBrush );
		}

		return ReturnBrush;
	}

	virtual FSlateBrush* GetDefaultBrush() const OVERRIDE
	{
		return DefaultBrush;
	}

	virtual const FSlateBrush* GetOptionalBrush( const FName PropertyName, const ANSICHAR* Specifier = NULL ) const OVERRIDE
	{
		const FName StyleName = Join( PropertyName, Specifier );
		const FSlateBrush* Result = BrushResources.FindRef( StyleName );

		if ( Result == NULL )
		{
			TWeakPtr< FSlateDynamicImageBrush > WeakImageBrush = DynamicBrushes.FindRef( StyleName );
			TSharedPtr< FSlateDynamicImageBrush > ImageBrush = WeakImageBrush.Pin();

			if ( ImageBrush.IsValid() )
			{
				Result = ImageBrush.Get();
			}
		}

		return Result ? Result : FStyleDefaults::GetNoBrush();
	}

	virtual const FSlateSound& GetSound( const FName PropertyName, const ANSICHAR* Specifier = NULL ) const OVERRIDE
	{
		const FSlateSound* Result = Sounds.Find( Join( PropertyName, Specifier ) );

		return Result ? *Result : FStyleDefaults::GetSound();
	}

	virtual FSlateFontInfo GetFontStyle( const FName PropertyName, const ANSICHAR* Specifier = NULL ) const OVERRIDE
	{
		const FSlateFontInfo* Result = FontInfoResources.Find( Join( PropertyName, Specifier ) );

		return Result ? *Result : FStyleDefaults::GetFontInfo();
	}


public:

	template< typename DefinitionType >            
	FORCENOINLINE void Set( const FName PropertyName, const DefinitionType& InStyleDefintion )
	{
		const TSharedRef< struct FSlateWidgetStyle >* DefinitionPtr = WidgetStyleValues.Find( PropertyName );

		if ( DefinitionPtr == NULL )
		{
			WidgetStyleValues.Add( PropertyName, MakeShareable( new DefinitionType( InStyleDefintion ) ) );
		}
		else
		{
			WidgetStyleValues.Add( PropertyName, MakeShareable( new DefinitionType( InStyleDefintion ) ) );
		}
	}

	/**
	 * Set float properties
	 * @param PropertyName - Name of the property to set.
	 * @param InFloat - The value to set.
	 */
	FORCENOINLINE void Set( const FName PropertyName, const float InFloat )
	{
		FloatValues.Add( PropertyName, InFloat );
	}

	/**
	 * Add a FVector2D property to this style's collection.
	 * @param PropertyName - Name of the property to set.
	 * @param InVector - The value to set.
	 */
	FORCENOINLINE void Set( const FName PropertyName, const FVector2D InVector )
	{
		Vector2DValues.Add( PropertyName, InVector );
	}

	/**
	 * Set FLinearColor property.
	 * @param PropertyName - Name of the property to set.
	 * @param InColor - The value to set.
	 */
	FORCENOINLINE void Set( const FName PropertyName, const FLinearColor& InColor )
	{
		ColorValues.Add( PropertyName, InColor );
	}

	FORCENOINLINE void Set( const FName PropertyName, const FColor& InColor )
	{
		ColorValues.Add( PropertyName, InColor );
	}

	/**
	 * Add a FSlateLinearColor property to this style's collection.
	 * @param PropertyName - Name of the property to add.
	 * @param InColor - The value to add.
	 */
	FORCENOINLINE void Set( const FName PropertyName, const FSlateColor& InColor )
	{
		SlateColorValues.Add( PropertyName, InColor );
	}

	/**
	 * Add a FMargin property to this style's collection.
	 * @param PropertyName - Name of the property to add.
	 * @param InMargin - The value to add.
	 */
	FORCENOINLINE void Set( const FName PropertyName, const FMargin& InMargin )
	{
		MarginValues.Add( PropertyName, InMargin );
	}

	/**
	 * Add a FSlateBrush property to this style's collection
	 * @param PropertyName - Name of the property to set.
	 * @param InBrush - The brush to set.
	 */
	FORCENOINLINE void Set( const FName PropertyName, FSlateBrush* InBrush )
	{
		BrushResources.Add( PropertyName, InBrush );
	}

	FORCENOINLINE void Set( const FName PropertyName, FSlateNoResource* InBrush )
	{
		BrushResources.Add( PropertyName, InBrush );
	}

	FORCENOINLINE void Set( const FName PropertyName, FSlateBoxBrush* InBrush )
	{
		BrushResources.Add( PropertyName, InBrush );
	}

	FORCENOINLINE void Set( const FName PropertyName, FSlateBorderBrush* InBrush )
	{
		BrushResources.Add( PropertyName, InBrush );
	}

	FORCENOINLINE void Set( const FName PropertyName, FSlateImageBrush* InBrush )
	{
		BrushResources.Add( PropertyName, InBrush );
	}

	FORCENOINLINE void Set( const FName PropertyName, FSlateDynamicImageBrush* InBrush )
	{
		BrushResources.Add( PropertyName, InBrush );
	}

	FORCENOINLINE void Set( const FName PropertyName, FSlateColorBrush* InBrush )
	{
		BrushResources.Add( PropertyName, InBrush );
	}

	/**
	 * Set FSlateSound properties
	 *
	 * @param PropertyName  Name of the property to set
	 * @param InSound		Sound to set
	 */
	FORCENOINLINE void Set( FName PropertyName, const FSlateSound& InSound )
	{
		Sounds.Add( PropertyName, InSound );
	}
		
	/**
	 * Set FSlateFontInfo properties
	 *
	 * @param PropertyName  Name of the property to set
	 * @param InFontStyle   The value to set
	 */
	FORCENOINLINE void Set( FName PropertyName, const FSlateFontInfo& InFontInfo )
	{
		FontInfoResources.Add( PropertyName, InFontInfo );
	}


protected:

	virtual const FSlateWidgetStyle* GetWidgetStyleInternal( const FName DesiredTypeName, const FName StyleName ) const OVERRIDE
	{
		const TSharedRef< struct FSlateWidgetStyle >* StylePtr = WidgetStyleValues.Find( StyleName );

		if ( StylePtr == NULL )
		{
			Log( ISlateStyle::Warning, FText::Format( NSLOCTEXT("SlateStyleSet", "UnknownWidgetStyle", "Unable to find Slate Widget Style '{0}'. Using {1} defaults instead."), FText::FromName( StyleName ), FText::FromName( DesiredTypeName ) ) );
			return NULL;
		}

		const TSharedRef< struct FSlateWidgetStyle > Style = *StylePtr;

		if ( Style->GetTypeName() != DesiredTypeName )
		{
			Log( ISlateStyle::Error, FText::Format( NSLOCTEXT("SlateStyleSet", "WrongWidgetStyleType", "The Slate Widget Style '{0}' is not of the desired type. Desired: '{1}', Actual: '{2}'"), FText::FromName( StyleName ), FText::FromName( DesiredTypeName ), FText::FromName( Style->GetTypeName() ) ) );
			return NULL;
		}

		return &Style.Get();
	}

	virtual void Log( ISlateStyle::EStyleMessageSeverity Severity, const FText& Message ) const OVERRIDE
	{
		if ( Severity == ISlateStyle::Error )
		{
			UE_LOG( LogSlateStyle, Error, TEXT("%s"), *Message.ToString() );
		}
		else if ( Severity == ISlateStyle::PerformanceWarning )
		{
			UE_LOG( LogSlateStyle, Warning, TEXT("%s"), *Message.ToString() );
		}
		else if ( Severity == ISlateStyle::Warning )
		{
			UE_LOG( LogSlateStyle, Warning, TEXT("%s"), *Message.ToString() );
		}
		else if ( Severity == ISlateStyle::Info )
		{
			UE_LOG( LogSlateStyle, Log, TEXT("%s"), *Message.ToString() );
		}
		else
		{
			UE_LOG( LogSlateStyle, Fatal, TEXT("%s"), *Message.ToString() );
		}
	}

	virtual void LogUnusedBrushResources()
	{
		TArray<FString> Filenames;
		IFileManager::Get().FindFilesRecursive( Filenames, *ContentRootDir, *FString("*.png"), true, false, false );

		for ( FString& FilePath : Filenames )
		{
			bool IsUsed = false;
			for ( auto& Entry : BrushResources )
			{
				if ( IsBrushFromFile( FilePath, Entry.Value ) )
				{
					IsUsed = true;
					break;
				}
			}

			if ( !IsUsed )
			{
				for ( auto& Entry : WidgetStyleValues )
				{
					TArray< const FSlateBrush* > WidgetBrushes;
					Entry.Value->GetResources( WidgetBrushes );

					for ( const FSlateBrush* Brush : WidgetBrushes )
					{
						if ( IsBrushFromFile( FilePath, Brush ) )
						{
							IsUsed = true;
							break;
						}
					}

					if ( IsUsed )
					{
						break;
					}
				}
			}

			if ( !IsUsed )
			{
				Log( ISlateStyle::Warning, FText::FromString( FilePath ) );
			}
		}
	}

protected:

	bool IsBrushFromFile( const FString& FilePath, const FSlateBrush* Brush )
	{
		FString Path = Brush->GetResourceName().ToString();
		FPaths::MakeStandardFilename( Path );
		if ( Path.Compare( FilePath, ESearchCase::IgnoreCase ) == 0 )
		{
			return true;
		}

		return false;
	}

	/** The name used to identity this style set */
	FName StyleSetName;

	/** This dir is Engine/Editor/Slate folder **/
	FString ContentRootDir;

	/** This dir is Engine/Slate folder to share the items **/
	FString CoreContentRootDir;

	TMap< FName, TSharedRef< struct FSlateWidgetStyle > > WidgetStyleValues;

	/** float property storage. */
	TMap< FName, float > FloatValues;

	/** FVector2D property storage. */
	TMap< FName, FVector2D > Vector2DValues;

	/** Color property storage. */
	TMap< FName, FLinearColor > ColorValues;
	
	/** FSlateColor property storage. */
	TMap< FName, FSlateColor > SlateColorValues;

	/** FMargin property storage. */
	TMap< FName, FMargin > MarginValues;

	/* FSlateBrush property storage */
	FSlateBrush* DefaultBrush;
	TMap< FName, FSlateBrush* > BrushResources;

	/** SlateSound property storage */
	TMap< FName, FSlateSound > Sounds;
	
	/** FSlateFontInfo property storage. */
	TMap< FName, FSlateFontInfo > FontInfoResources;

	/** A list of dynamic brushes */
	TMap< FName, TWeakPtr< FSlateDynamicImageBrush > > DynamicBrushes;
};


