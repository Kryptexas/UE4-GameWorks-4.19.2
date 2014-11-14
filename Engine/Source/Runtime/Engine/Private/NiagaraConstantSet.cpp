#include "EnginePrivate.h"
#include "Niagara/NiagaraConstantSet.h"

template<>
struct TStructOpsTypeTraits<FNiagaraConstantMap> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithSerializer = true
	};
};
IMPLEMENT_STRUCT(NiagaraConstantMap);
