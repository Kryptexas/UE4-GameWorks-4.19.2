// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

class FNameValidatorInterface;

//////////////////////////////////////////////////////////////////////////
// FNameValidtorFactory
class UNREALED_API FNameValidatorFactory
{
public:
	static TSharedPtr<class INameValidatorInterface> MakeValidator(class UEdGraphNode* Node);
};

enum EValidatorResult
{
	/** Name is valid for this object */
	Ok,
	/** The name is already in use and invalid */
	AlreadyInUse,
	/** The entered name is blank */
	EmptyName,
	/* The entered name matches the current name */
	ExistingName,
	/* The entered name is too long */
	TooLong
};


//////////////////////////////////////////////////////////////////////////
// FNameValidatorInterface

class UNREALED_API INameValidatorInterface
{
public:
	/** @return true if FName is valid, false otherwise */
	virtual EValidatorResult IsValid (const FName& Name, bool bOriginal = false) = 0;

	/** @return true if FString is valid, false otherwise */
	virtual EValidatorResult IsValid (const FString& Name, bool bOriginal = false) = 0;

	static FString GetErrorString(const FString& Name, EValidatorResult ErrorCode);

	/** @return Ok if was valid and doesn't require anything, AlreadyInUse if was already in use and is replaced with new one */
	EValidatorResult FindValidString(FString& InOutName);

	/** 
	 * Helper method to see if object exists with this name in the blueprint. Useful for 
	 * testing for name conflicts with objects create with Blueprint as their outer
	 *
	 * @param Blueprint	The blueprint to check
	 * @param Name		The name to check for conflicts
	 *
	 * @return True if name is not used by object in the blueprint, False otherwise
	 */
	static bool BlueprintObjectNameIsUnique(class UBlueprint* Blueprint, const FName& Name);
};

/////////////////////////////////////////////////////
// FKismetNameValidator
class UNREALED_API FKismetNameValidator : public INameValidatorInterface
{
public:
	FKismetNameValidator(const class UBlueprint* Blueprint, FName InExistingName = NAME_None);
	~FKismetNameValidator() {}

	// Begin FNameValidatorInterface
	virtual EValidatorResult IsValid( const FString& Name, bool bOriginal = false) OVERRIDE;
	virtual EValidatorResult IsValid( const FName& Name, bool bOriginal = false) OVERRIDE;
	// End FNameValidatorInterface
private:
	/** Name set to validate */
	TArray<FName> Names;
	/** The blueprint to check for validity within */
	const UBlueprint* BlueprintObject;
	/** The current name of the object being validated */
	FName ExistingName;
};

/////////////////////////////////////////////////////
// FStringSetNameValidator

class UNREALED_API FStringSetNameValidator : public INameValidatorInterface
{
public:
	// Begin FNameValidatorInterface
	virtual EValidatorResult IsValid(const FString& Name, bool bOriginal) OVERRIDE;
	virtual EValidatorResult IsValid(const FName& Name, bool bOriginal) OVERRIDE;
	// End FNameValidatorInterface

protected:
	// This class is a base class for anything that just needs to validate a string is unique
	FStringSetNameValidator(const FString& InExistingName)
		: ExistingName(InExistingName)
	{
	}

protected:
	/* Name set to validate */
	TSet<FString> Names;
	FString ExistingName;
};

/////////////////////////////////////////////////////
// FAnimStateTransitionNodeSharedRulesNameValidator

class UNREALED_API FAnimStateTransitionNodeSharedRulesNameValidator : public FStringSetNameValidator
{
public:
	FAnimStateTransitionNodeSharedRulesNameValidator(class UAnimStateTransitionNode* InStateTransitionNode);
};

/////////////////////////////////////////////////////
// FAnimStateTransitionNodeSharedCrossfadeNameValidator

class UNREALED_API FAnimStateTransitionNodeSharedCrossfadeNameValidator : public FStringSetNameValidator
{
public:
	FAnimStateTransitionNodeSharedCrossfadeNameValidator(class UAnimStateTransitionNode* InStateTransitionNode);
};

/////////////////////////////////////////////////////
// FDummyNameValidator

// Always returns the same value
class UNREALED_API FDummyNameValidator : public INameValidatorInterface
{
public:
	FDummyNameValidator(EValidatorResult InReturnValue) : ReturnValue(InReturnValue) {}

	// Begin FNameValidatorInterface
	virtual EValidatorResult IsValid(const FString& Name, bool bOriginal) OVERRIDE { return ReturnValue; }
	virtual EValidatorResult IsValid(const FName& Name, bool bOriginal) OVERRIDE { return ReturnValue; }
	// End FNameValidatorInterface

private:
	EValidatorResult ReturnValue;
};