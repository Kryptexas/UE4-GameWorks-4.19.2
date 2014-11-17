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

UENUM()
enum class ECppEnum : uint8
{
	Seven,
	Eight,
	Nine
};
