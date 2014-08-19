
/**  List of built-in constants
*	The values of these enums correspond directly to indices for the compiler and interpreter
*/
namespace NiagaraConstants
{
	enum {
		DeltaSeconds = 0,
		EmitterPosition,
		EmitterAge,
		ActorXAxis,
		ActorYAxis,
		ActorZAxis,
		NumBuiltinConstants
	} ConstantDef;

	FName ConstantNames[] = {
		"DeltaTime",
		"Emitter position",
		"Emitter age",
		"Emitter X Axis",
		"Emitter Y Axis",
		"Emitter Z Axis",
		""
	};
}

