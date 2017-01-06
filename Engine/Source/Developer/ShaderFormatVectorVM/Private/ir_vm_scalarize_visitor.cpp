// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ShaderFormatVectorVM.h"
#include "CoreMinimal.h"
#include "hlslcc.h"
#include "hlslcc_private.h"
#include "VectorVMBackend.h"

PRAGMA_DISABLE_SHADOW_VARIABLE_WARNINGS
#include "glsl_parser_extras.h"
PRAGMA_POP

#include "hash_table.h"
#include "ir_rvalue_visitor.h"
#include "PackUniformBuffers.h"
#include "IRDump.h"
#include "OptValueNumbering.h"
#include "ir_optimization.h"
#include "ir_expression_flattening.h"
#include "ir.h"



/** splits all assignments into a separate assignment for each of it's components. Eventually down to the individual scalars. */
class ir_scalarize_visitor2 : public ir_hierarchical_visitor
{
	_mesa_glsl_parse_state *parse_state;

	unsigned dest_component;

	ir_rvalue* curr_rval;

	bool is_struct;

	bool has_split;

	ir_scalarize_visitor2(_mesa_glsl_parse_state *in_state)
		: parse_state(in_state)
		, dest_component(0)
		, curr_rval(nullptr)
		, is_struct(false)
		, has_split(false)
	{
	}

	~ir_scalarize_visitor2()
	{
	}

	virtual ir_visitor_status visit(ir_constant *ir)
	{
		check(ir->type->is_numeric());

		if (dest_component >= 0 && dest_component < ir->type->components())
		{
			switch (ir->type->base_type)
			{
			case GLSL_TYPE_FLOAT:
				ir->value.f[0] = ir->value.f[dest_component];
				ir->type = glsl_type::float_type;
				break;
			case GLSL_TYPE_INT:
				ir->value.i[0] = ir->value.i[dest_component];
				ir->type = glsl_type::int_type;
				break;
			case GLSL_TYPE_UINT:
				ir->value.u[0] = ir->value.u[dest_component];
				ir->type = glsl_type::uint_type;
				break;
			case GLSL_TYPE_BOOL:
				ir->value.b[0] = ir->value.b[dest_component];
				ir->type = glsl_type::bool_type;
				break;
			}
		}
		return visit_continue;
	}

	virtual ir_visitor_status visit_enter(ir_call* call)
	{
		//We don't touch special calls, they're handled elsewhere.
		EVectorVMOp special_op = get_special_vm_opcode(call->callee);
		if (special_op != EVectorVMOp::done && special_op != EVectorVMOp::random && special_op != EVectorVMOp::select && special_op != EVectorVMOp::noise && special_op != EVectorVMOp::fmod)
			return visit_continue_with_parent;

		unsigned max_components = 0;
		foreach_iter(exec_list_iterator, param_iter, call->actual_parameters)
		{
			ir_rvalue* param = (ir_rvalue *)param_iter.get();
			check(param->type->base_type != GLSL_TYPE_STRUCT);
			max_components = FMath::Max(max_components, param->type->components());
		}

		if (max_components == 1)
			return visit_continue_with_parent;

		if (special_op == EVectorVMOp::noise)
		{
			//We scalarize noise slightly differently.
			//Maybe we'll refactor this to some alternate mode for scalarization rather than specific to noise.

			//Find the sig taking the right number of args.
			ir_function* func = const_cast<ir_function*>(call->callee->function());//I'm not modifying anything here, I just don't have time to go in and write a const iterator.
			ir_function_signature* correct_sig = nullptr;
			foreach_iter(exec_list_iterator, iter, *func)
			{
				ir_function_signature* sig = (ir_function_signature *)iter.get();
				unsigned num_args = 0;
				bool all_scalar = true;
				foreach_iter(exec_list_iterator, param_iter, sig->parameters)
				{
					++num_args;
					all_scalar &= ((ir_rvalue*)param_iter.get())->type->is_scalar();
				}
				if (num_args == max_components && all_scalar)
				{
					correct_sig = sig;
					break;
				}
			}

			if (!correct_sig)
			{
				_mesa_glsl_error(parse_state, "could not find a correct signature for %s", func->name);
				return visit_stop;
			}

			//For noise we want one call, but to split up the params to be individual components.
			ir_variable* old_dest = call->return_deref->var;
			void* perm_mem_ctx = ralloc_parent(call);

			dest_component = 0;
			exec_list new_params;
			for (unsigned i = 0; i < max_components; ++i)
			{
				dest_component = i;
				curr_rval = nullptr;
				foreach_iter(exec_list_iterator, param_iter, call->actual_parameters)
				{
					ir_rvalue* param = (ir_rvalue*)param_iter.get();
					check(curr_rval == nullptr);
					param->accept(this);
					check(curr_rval);
					curr_rval->remove();
					new_params.push_tail(curr_rval);
				}
			}

			call->actual_parameters.make_empty();
			call->actual_parameters.append_list(&new_params);
			call->callee = correct_sig;
		}
		else
		{
			//TODO: All this should work but in all my uses, the global funciton has been stripped as it's not technically in use in the IR at this point.
			//TODO: I need to mark any of my intrinsics that may be scalarized as non-strippable.

			//find the function signature for this function that takes only scalars.
			ir_function* func = const_cast<ir_function*>(call->callee->function());//I'm not modifying anything here, I just don't have time to go in and write a const iterator.
			ir_function_signature* scalar_sig = nullptr;
			foreach_iter(exec_list_iterator, iter, *func)
			{
				ir_function_signature* sig = (ir_function_signature *)iter.get();
				bool all_scalar = true;
				foreach_iter(exec_list_iterator, param_iter, sig->parameters)
				{
					ir_variable *param = (ir_variable *)param_iter.get();
					if (!param->type->is_scalar())
					{
						all_scalar = false;
						break;
					}
				}
				if (all_scalar)
				{
					scalar_sig = sig;
				}
			}

			if (!scalar_sig)
			{
				_mesa_glsl_error(parse_state, "could not find a scalar signature for function %s", func->name);
				return visit_stop;
			}

			//Clone the call max_component times and visit each param to scalarize them to the right component.
			ir_variable* old_dest = call->return_deref->var;
			void* perm_mem_ctx = ralloc_parent(call);
			for (unsigned i = 0; i < max_components; ++i)
			{
				dest_component = i;

				ir_variable* new_dest = new(perm_mem_ctx) ir_variable(old_dest->type->get_base_type(), old_dest->name, ir_var_temporary);
				call->insert_before(new_dest);

				exec_list new_params;
				foreach_iter(exec_list_iterator, param_iter, call->actual_parameters)
				{
					ir_rvalue* param = (ir_rvalue*)param_iter.get();
					param = param->clone(perm_mem_ctx, nullptr);
					curr_rval = nullptr;
					param->accept(this);
					new_params.push_tail(curr_rval ? curr_rval : param);
				}

				call->insert_before(new(perm_mem_ctx)ir_call(scalar_sig, new(perm_mem_ctx) ir_dereference_variable(new_dest), &new_params));

				ir_dereference* deref = call->return_deref->clone(perm_mem_ctx, nullptr);
				unsigned write_mask = 1 << dest_component;
				call->insert_before(new(perm_mem_ctx)ir_assignment(deref, new(perm_mem_ctx) ir_dereference_variable(new_dest), nullptr, write_mask));
			}

			call->remove();
		}

		return visit_continue_with_parent;
	}

	virtual ir_visitor_status visit_enter(ir_assignment* assign)
	{
		if (assign->condition)
		{
			_mesa_glsl_error(parse_state, "conditional assignment in instruction stream");
			return visit_stop;
		}

		void* perm_mem_ctx = ralloc_parent(assign);

		const glsl_type* type = assign->lhs->type;

		//Already scalar or writes to just one component. Bail.
		unsigned num_components_written = 0;
		unsigned write_mask = assign->write_mask;
		while (write_mask > 0)
		{
			num_components_written += write_mask & 0x1 ? 1 : 0;
			write_mask = write_mask >> 1;
		}

		if (type->is_scalar() || num_components_written == 1)
		{
			return visit_continue_with_parent;
		}

		has_split = true;

		is_struct = type->base_type == GLSL_TYPE_STRUCT;
		check(is_struct || type->is_matrix() || type->is_vector());

		unsigned num_components = is_struct ? type->length : type->components();

		write_mask = assign->write_mask == 0 ? 0xFFFFFFFF : assign->write_mask;
		ir_assignment* comp_assign = NULL;

		for (unsigned comp_idx = 0; comp_idx < num_components; ++comp_idx)
		{
			if (is_struct || (write_mask & 0x1))
			{
				if (comp_assign)
				{
					assign->insert_before(comp_assign);
					comp_assign = NULL;
				}

				comp_assign = assign->clone(perm_mem_ctx, NULL);
				check(comp_assign);
				dest_component = comp_idx;

				if (is_struct)
				{
					comp_assign->write_mask = 0;
					comp_assign->set_lhs(new(perm_mem_ctx)ir_dereference_record(comp_assign->lhs, comp_assign->lhs->type->fields.structure[dest_component].name));
				}
				else
				{
					comp_assign->write_mask = 1 << dest_component;
				}

				comp_assign->write_mask = is_struct ? 0 : 1 << dest_component;

				curr_rval = nullptr;
				comp_assign->rhs->accept(this);
				if (curr_rval)
				{
					comp_assign->rhs->replace_with(curr_rval);
					comp_assign->rhs = curr_rval;
				}
			}
			write_mask = write_mask >> 1;
		}

		if (comp_assign)
		{
			assign->replace_with(comp_assign);
		}

		return visit_continue_with_parent;
	}

	virtual ir_visitor_status visit_enter(ir_swizzle* swiz)
	{
		check(!is_struct);//We shouldn't be hitting a swizzle if we're splitting a struct assignment.

		if (swiz->mask.num_components > 1)
		{
			check(dest_component < swiz->mask.num_components);

			unsigned src_comp = 0;
			switch (dest_component)
			{
			case 0: src_comp = swiz->mask.x; break;
			case 1: src_comp = swiz->mask.y; break;
			case 2: src_comp = swiz->mask.z; break;
			case 3: src_comp = swiz->mask.w; break;
			default:check(0);
				break;
			}

			//Handle case where we're swizzling a matrix array access.
			//Convert directly into a matrix swizzle
			if (ir_dereference_array* array_deref = swiz->val->as_dereference_array())
			{
				void* perm_mem_ctx = ralloc_parent(array_deref);
				const glsl_type* array_type = array_deref->array->type;
				const glsl_type* type = array_deref->type;

				//Only supporting array derefs for matrices atm.
				check(array_type->is_matrix());

				//Only support constant matrix indices. Not immediately clear how I'd do non-const access.
				ir_constant* index = array_deref->array_index->as_constant();
				check(index->type == glsl_type::uint_type && index->type->is_scalar());

				src_comp = index->value.u[0] * array_type->vector_elements + src_comp;
				ir_dereference_variable* matrix_deref = new(perm_mem_ctx)ir_dereference_variable(array_deref->variable_referenced());
				swiz->val->replace_with(matrix_deref);
				swiz->val = matrix_deref;
			}

			swiz->mask.num_components = 1;
			swiz->mask.x = src_comp;
			swiz->mask.has_duplicates = false;
			swiz->type = swiz->type->get_base_type();
		}

		return visit_continue_with_parent;
	}

	virtual ir_visitor_status visit_enter(ir_dereference_array* array_deref)
	{
		void* perm_mem_ctx = ralloc_parent(array_deref);
		const glsl_type* array_type = array_deref->array->type;
		const glsl_type* type = array_deref->type;

		//Only supporting array derefs for matrices atm.
		check(array_type->is_matrix());

		//Only support constant matrix indices. Not immediately clear how I'd do non-const access.
		ir_constant* index = array_deref->array_index->as_constant();
		check(index->type == glsl_type::uint_type && index->type->is_scalar());

		unsigned swiz_comp = index->value.u[0] * array_type->vector_elements + dest_component;
		ir_swizzle* swiz = new(perm_mem_ctx) ir_swizzle(array_deref, type->is_scalar() ? 0 : dest_component, 0, 0, 0, 1);
		curr_rval = swiz;

		return visit_continue_with_parent;
	}

	virtual ir_visitor_status visit_enter(ir_dereference_record* deref)
	{
		void* perm_mem_ctx = ralloc_parent(deref);
		const glsl_type* type = deref->type;

		if (type->base_type == GLSL_TYPE_STRUCT)
		{
			check(dest_component < type->length);

			ir_dereference_record* rec = new(perm_mem_ctx) ir_dereference_record(deref, type->fields.structure[dest_component].name);
			curr_rval = rec;
		}
		else
		{
			check(deref->type->is_numeric());
			check(type->components() >= dest_component || type->is_scalar());

			ir_swizzle* swiz = new(perm_mem_ctx) ir_swizzle(deref, type->is_scalar() ? 0 : dest_component, 0, 0, 0, 1);
			curr_rval = swiz;
		}

		return visit_continue_with_parent;
	}

	virtual ir_visitor_status visit(ir_dereference_variable *deref)
	{
		void* perm_mem_ctx = ralloc_parent(deref);
		ir_variable* var = deref->variable_referenced();
		const glsl_type* type = var->type;
		if (type->base_type == GLSL_TYPE_STRUCT)
		{
			check(dest_component < type->length);

			ir_dereference_record* rec = new(perm_mem_ctx) ir_dereference_record(var, type->fields.structure[dest_component].name);
			curr_rval = rec;
		}
		else
		{
			if (!type->is_scalar())
			{
				check(var->type->is_numeric());
				check(type->components() > dest_component);

				ir_swizzle* swiz = new(perm_mem_ctx) ir_swizzle(deref, dest_component, 0, 0, 0, 1);
				curr_rval = swiz;
			}
		}

		return visit_continue_with_parent;
	}

	virtual ir_visitor_status visit_enter(ir_expression *expr)
	{
		ir_rvalue* old_rval = curr_rval;

		if (is_struct)
		{
			check(dest_component < expr->type->length);
			expr->type = expr->type->fields.structure[dest_component].type;
		}
		else
		{
			expr->type = expr->type->get_base_type();
		}

		unsigned int num_operands = expr->get_num_operands();

		//visit all operands and replace any rvalues with duplicates that only access the dest_component of that var.
		for (unsigned op = 0; op < num_operands; ++op)
		{
			curr_rval = NULL;
			expr->operands[op]->accept(this);

			if (curr_rval)
			{
				if (op + 1 < num_operands)
					expr->operands[op]->insert_before(curr_rval);
				else
					expr->operands[op]->replace_with(curr_rval);

				expr->operands[op] = curr_rval;
			}
		}

		curr_rval = old_rval;
		return visit_continue_with_parent;
	}

public:
	static void run(exec_list *ir, _mesa_glsl_parse_state *state)
	{
		bool progress = false;
		do
		{
			ir_scalarize_visitor2 scalarize_visitor(state);
			visit_list_elements(&scalarize_visitor, ir);
			progress = scalarize_visitor.has_split;
		} while (progress);

		do
		{
			progress = do_dead_code(ir, false);
			progress = do_dead_code_local(ir) || progress;
		} while (progress);
	}
};


void vm_scalarize_ops(exec_list* ir, _mesa_glsl_parse_state* state)
{
	//Generate the noise signatures. Maybe should move noise handling to another visitor.		

	//Find the noise func.
	ir_function* noise_func = nullptr;
	foreach_iter(exec_list_iterator, iter, *ir)
	{
		if (ir_function* noise = ((ir_instruction*)iter.get())->as_function())
		{
			if (strcmp(noise->name, "noise") == 0)
			{
				noise_func = noise;
				break;
			}			
		}
	}
	if (noise_func)
	{
		ir_function_signature* noise1 = nullptr;

		//Noise1 sig may already exist so find it.
		foreach_iter(exec_list_iterator, iter, *noise_func)
		{
			ir_function_signature* sig = (ir_function_signature *)iter.get();

			bool scalar = true;
			unsigned num_params = 0;
			foreach_iter(exec_list_iterator, iterInner, sig->parameters)
			{
				scalar &= ((ir_rvalue*)iterInner.get())->type->is_scalar();
				++num_params;
			}
			if (num_params == 1 && scalar)
			{
				noise1 = sig;
			}
		}

		if (!noise1)
		{
			ir_function_signature* new_sig = new(state)ir_function_signature(glsl_type::float_type);
			new_sig->is_builtin = true;
			new_sig->has_output_parameters = false;
			new_sig->parameters.push_tail(new(state)ir_variable(glsl_type::float_type, "x", ir_var_in));
			noise_func->add_signature(new_sig);
		}

		ir_function_signature* new_sig = new(state)ir_function_signature(glsl_type::float_type);
		new_sig->is_builtin = true;
		new_sig->has_output_parameters = false;
		new_sig->parameters.push_tail(new(state)ir_variable(glsl_type::float_type, "x", ir_var_in));
		new_sig->parameters.push_tail(new(state)ir_variable(glsl_type::float_type, "y", ir_var_in));
		noise_func->add_signature(new_sig);

		new_sig = new(state)ir_function_signature(glsl_type::float_type);
		new_sig->is_builtin = true;
		new_sig->has_output_parameters = false;
		new_sig->parameters.push_tail(new(state)ir_variable(glsl_type::float_type, "x", ir_var_in));
		new_sig->parameters.push_tail(new(state)ir_variable(glsl_type::float_type, "y", ir_var_in));
		new_sig->parameters.push_tail(new(state)ir_variable(glsl_type::float_type, "z", ir_var_in));
		noise_func->add_signature(new_sig);

		//ir->push_tail(noise_func);
	}

	ir_scalarize_visitor2::run(ir, state);
}