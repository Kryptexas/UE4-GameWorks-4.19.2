#pragma once


class FDDCScopeStatHelper
{
private:
	FName TransactionGuid;
	double StartTime;
	bool bHasParent;
public:
	FDDCScopeStatHelper(const TCHAR* CacheKey, const FName& FunctionName);

	~FDDCScopeStatHelper();

	void AddTag(const FName& Tag, const FString& Value);

	bool HasParent() const { return bHasParent;  }
};

