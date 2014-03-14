// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* Smart COM object pointer.
*/
template<typename T>
class FCOMPtr
{
public:
	typedef T PointerType;

public:
	FCOMPtr() :	RawPointer(nullptr)
	{
	}

	FCOMPtr(PointerType* const Source) : RawPointer(Source)
	{
		if(RawPointer)
		{
			RawPointer->AddRef();
		}
	}

	FCOMPtr(const FCOMPtr<PointerType>& Source) : RawPointer(Source.RawPointer)
	{
		if(RawPointer)
		{
			RawPointer->AddRef();
		}
	}

	FCOMPtr(FCOMPtr<PointerType>&& Source) : RawPointer(Source.RawPointer)
	{	
		Source.RawPointer = nullptr;
	}	

	FCOMPtr<PointerType>& operator=(PointerType* const Source) 
	{
		if(RawPointer != Source)
		{
			if(RawPointer)
			{
				RawPointer->Release();
			}
			RawPointer = Source;
			if(RawPointer)
			{
				RawPointer->AddRef();
			}
		}
		return *this;
	}

	FCOMPtr<PointerType>& operator=(const FCOMPtr<PointerType>& Source) 
	{
		if(RawPointer != Source.RawPointer)
		{
			if(RawPointer)
			{
				RawPointer->Release();
			}
			RawPointer = Source.RawPointer;
			if(RawPointer)
			{
				RawPointer->AddRef();
			}
		}
		return *this;
	}

	FCOMPtr<PointerType>& operator=(FCOMPtr<PointerType>&& Source) 
	{			
		if(RawPointer != Source.RawPointer)
		{
			if(RawPointer)
			{
				RawPointer->Release();
			}
			RawPointer = Source.RawPointer;
			Source.RawPointer = nullptr;
		}
		return *this;
	}

	~FCOMPtr() 
	{
		if(RawPointer)
		{
			RawPointer->Release();
		}
	}

	PointerType** operator&()
	{
		return &(RawPointer);
	}

	PointerType* operator->() const 
	{
		check(RawPointer != nullptr);
		return RawPointer;
	}
	
	bool operator==(PointerType* const InRawPointer) const
	{
		return RawPointer == InRawPointer;
	}

	bool operator!=(PointerType* const InRawPointer) const
	{
		return RawPointer != InRawPointer;
	}

	operator PointerType*() const
	{
		return RawPointer;
	}

	void Reset()
	{
		if(RawPointer)
		{
			RawPointer->Release();
			RawPointer = nullptr;
		}
	}

	// Set the pointer without adding a reference.
	void Attach(PointerType* InRawPointer)
	{
		if(RawPointer)
		{
			RawPointer->Release();
		}
		RawPointer = InRawPointer;
	}

	// Reset the pointer without releasing a reference.
	void Detach()
	{
		RawPointer = nullptr;
	}

	HRESULT FromQueryInterface(REFIID riid, IUnknown* Unknown)
	{
		if(RawPointer)
		{
			RawPointer->Release();
			RawPointer = nullptr;
		}
		return Unknown->QueryInterface(riid, reinterpret_cast<void**>(&(RawPointer)));
	}

private:
	PointerType* RawPointer;
};