#pragma once

#include "EnumOnlyHeader.generated.h"

UENUM()
enum ESomeEnum
{
	One,
	Two,
	Three
};

UENUM()
namespace ESomeNamespacedEnum
{
	enum Type
	{
		Four,
		Five,
		Six
	};
}
