#include "UnrealCodeAnalyzerTestsPCH.h"
#include "Inheritance.h"

class FTestDerivedClass : public FTestBaseClass
{ };

static void Function_Inheritance()
{
	FTestDerivedClass TestDerivedClass;
}
