// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __SourceCodeNavigation_h__
#define __SourceCodeNavigation_h__

#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogSelectionDetails, Log, All);


/**
 * Source code navigation functionality
 */
class FSourceCodeNavigation
{

public:
	/** Holds useful information about a function's symbols */
	struct FFunctionSymbolInfo
	{
		/** Function symbol string */
		FString SymbolName;

		/** The name of the UObject class this function is in, if any */
		FString ClassName;

		/** The name of the module the function is in (or empty string if not known) */
		FString ModuleName;
	};


	/** Allows function symbols to be organized by class */
	struct FEditCodeMenuClass 
	{
		/** Name of the class */
		FString Name;

		/** Module name this class resides in */
		FString ModuleName;

		/** All of the functions in this class */
		TArray< FFunctionSymbolInfo > Functions;

		/** True if the list of functions is a complete list, or false if we still do not have all data digested yet (or
		    the user only asked for a list of classes, and we've determined the class may have relevant functions.) */
		bool bIsCompleteList;

		/** Referenced object (e.g., a blueprint for a kismet graph symbol) */
		TWeakObjectPtr<UObject> ReferencedObject;
	};

	/**
	 * Asynchronously locates the source file and line for a specific function in a specific module and navigates an external editing to that source line
	 *
	 * @param	VSAccessor		The VSAccessor module.  Needed to talk to external editing tools.  This needs to be passed in because module access is currently not thread safe.
	 * @param	FunctionSymbolName	The function to navigate tool (e.g. "MyClass::MyFunction")
	 * @param	FunctionModuleName	The module to search for this function's symbols (e.g. "GameName-Win64-Debug")
	 * @param	bIgnoreLineNumber	True if we should just open the file and not go to a specific line number
	 */
	static void NavigateToFunctionSourceAsync( const FString& FunctionSymbolName, const FString& FunctionModuleName, const bool bIgnoreLineNumber );

	/** Gather modes for GatherFunctionsForActors() */
	struct EGatherMode
	{
		enum Type
		{
			/** Gather classes and all functions in those classes */
			ClassesAndFunctions,

			/** Only gather the class names (much faster!) */
			ClassesOnly
		};
	};

	/**
	 * Finds all of the functions in classes for the specified list of actors
	 *
	 * @param	Actors			List of actors to gather functions for
	 * @param	GatherMode		Whether to gather all classes and functions, or only classes
	 * @param	Classes	(Out)	Sorted list of function symbols organized by class for actor classes
	 */
	static void GatherFunctionsForActors( TArray< AActor* >& Actors, const EGatherMode::Type GatherMode, TArray< FEditCodeMenuClass >& Classes );

	/** Delegate that's triggered when any symbol query has completed */
	DECLARE_MULTICAST_DELEGATE( FOnSymbolQueryFinished );

	/** Call this to access the multi-cast delegate that you can register a callback with */
	static FOnSymbolQueryFinished& AccessOnSymbolQueryFinished();

	/** Returns the name of the suggested IDE, based on platform */
	UNREALED_API static FText GetSuggestedSourceCodeIDE(bool bShortIDEName = false);

	/** Returns the url to the location where the suggested IDE can be downloaded */
	UNREALED_API static FString GetSuggestedSourceCodeIDEDownloadURL();

	/** Returns true if the compiler for the current platform is available for use */
	UNREALED_API static bool IsCompilerAvailable();

	/** Finds the base directory for a given module name. Does not rely on symbols; finds matching .build.cs files. */
	UNREALED_API static bool FindModulePath( const FString& ModuleName, FString &OutModulePath );

	/** Finds the path to a given class. Does not rely on symbols; finds matching .build.cs files. */
	UNREALED_API static bool FindClassHeaderPath( UField *Field, FString &OutClassHeaderPath );

	/** Opens a single source file */
	UNREALED_API static bool OpenSourceFile( const FString& AbsoluteSourcePath, int32 LineNumber = 0, int32 ColumnNumber = 0 );

	/** Opens a multiple source files */
	UNREALED_API static bool OpenSourceFiles(const TArray<FString>& AbsoluteSourcePaths);

	UNREALED_API static bool OpenModuleSolution();

	/** Delegate that's triggered when any symbol query has completed */
	DECLARE_MULTICAST_DELEGATE( FOnCompilerNotFound );

	/** Call this to access the multi-cast delegate that you can register a callback with */
	UNREALED_API static FOnCompilerNotFound& AccessOnCompilerNotFound();

private:
	/** Recursively finds files which mark the root of a directory tree. Stops recursing once a file is found. */
	static void FindRootFilesRecursive(TArray<FString> &FileNames, const FString &BaseDirectory, const FString &Wildcard);
};



#endif		// __SourceCodeNavigation_h__