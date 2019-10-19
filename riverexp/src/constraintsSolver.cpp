#include "constraintsSolver.h"
#include "../src/utils.h"
#include <assert.h>

// Byte variable indices will be named as "@index" (e.g. @123, @1), while the jump address symbol variable as $hexaaddr (e.g. $ff030201)

PathConstraintZ3Solver::PathConstraintZ3Solver()
{
	m_config 	= Z3_mk_config(); // TODO: should we init this each time ? profile how expensive it is
	m_context 	= Z3_mk_context(m_config);
	m_solver 	= Z3_mk_solver(m_context);

	Z3_solver_inc_ref(m_context, m_solver);

	m_currentDeclFuncs.clear();
	m_currentDeclSymbols.clear();
	m_jumpSymbolDeclAST.clear();
	m_sorts.clear();
	m_constants.clear();

	// Create bitvector sorts on 1 and 8 bits, then constants 0 and 1 for 1 bit sort
	m_sortBV1 = Z3_mk_bv_sort(m_context, 1);
	m_sortBV8 = Z3_mk_bv_sort(m_context, 8);
	m_constBV1_0 = Z3_mk_numeral(m_context, "0", m_sortBV1);
	Z3_inc_ref(m_context, m_constBV1_0);
	m_constBV1_1 = Z3_mk_numeral(m_context, "1", m_sortBV1);
	Z3_inc_ref(m_context, m_constBV1_1);

	m_sorts.push_back(m_sortBV1);
	m_sorts.push_back(m_sortBV8);
	m_constants.push_back(m_constBV1_0);
	m_constants.push_back(m_constBV1_1);

	//addDeclFuncAndSymbolFromAst(m_constBV1_0);
	//addDeclFuncAndSymbolFromAst(m_constBV1_1);
}

PathConstraintZ3Solver::~PathConstraintZ3Solver()
{
	Z3_solver_dec_ref(m_context, m_solver);

	for (const Z3_ast& ast : m_constants)
	{
		Z3_dec_ref(m_context, ast);
	}

	for (const Z3_ast& ast : m_jumpSymbolDeclAST)
	{
		Z3_dec_ref(m_context, ast);
	}

	for (const auto& it : m_byteVariablesDeclAST)
	{
		Z3_dec_ref(m_context, it.second);
	}
}

void PathConstraintZ3Solver::init(const PathConstraint* pathConstraint)
{
	m_pathConstraint = pathConstraint;
	addAllVariablesDeclarations(m_pathConstraint);
	addAllInternalZ3Constraints(m_pathConstraint);
}

void PathConstraintZ3Solver::pushState()
{
	Z3_solver_push(m_context, m_solver);
}

void PathConstraintZ3Solver::popState()
{
	Z3_solver_pop(m_context, m_solver, 1);
}

void PathConstraintZ3Solver::addConstraint(const int constraintIndex)
{
	assert(m_pathConstraint->constraints.size() > constraintIndex);
	const Test& constraint = m_pathConstraint->constraints[constraintIndex];

	// Add the needed assert for the variable 
	bool needJump = constraint.was_taken;
	if (constraint.isInverted)
		needJump = !needJump;
	Z3_ast eqJumpAssert = Z3_mk_eq(m_context, m_jumpSymbolDeclAST[constraintIndex], needJump ? m_constBV1_1 : m_constBV1_0);
	//Z3_inc_ref(m_context, eqJumpAssert); // Is needed ?
	Z3_solver_assert(m_context, m_solver, eqJumpAssert);
}

// Solves the current solver's conditions, fills in the new input payload and returns true if it could be solved, false otherwise
bool PathConstraintZ3Solver::solve(InputPayload& outInputSolved)
{
	// printf("solver string\n%s\n", (char*)Z3_solver_to_string(m_context, m_solver));
	Z3_lbool result = Z3_solver_check(m_context, m_solver);
	if (result != Z3_L_TRUE)
		return false; // Model can't be solved 
	// Get the model and its constants
	Z3_model model = Z3_solver_get_model(m_context, m_solver);

 	//printf("model obtained:\n%s\n", (char*)Z3_model_to_string(m_context, model));

    if (model) 
    {
    	Z3_model_inc_ref(m_context, model);
    	// Read variable values representing byte indices from the model
		const int num_constants = Z3_model_get_num_consts(m_context, model);
    	for (int i = 0; i < num_constants; i++) 
    	{
        	Z3_symbol name;
        	Z3_func_decl cnst = Z3_model_get_const_decl(m_context, model, i);
        	name = Z3_get_decl_name(m_context, cnst);
        	Z3_string varName = Z3_get_symbol_string(m_context, name);
        	if (varName)
        	{
        		if (varName[0] != '$') // Ignore jump conditions ?
        		{
        			// Get the byte index 
        			const int byte_index = atoi(varName+1);
        			// Create constant ast and evaluate it inside model and context
        			Z3_ast a,v;
        			a = Z3_mk_app(m_context, cnst, 0, 0);
			        v = a;
			        const bool ok = Z3_model_eval(m_context, model, a, 1, &v);
			        if (Z3_get_ast_kind(m_context, v) == Z3_NUMERAL_AST) // Is it a numeric value ?
			        {
			        	int value = -1;
			        	if (Z3_get_numeral_int(m_context, v, &value))	// Succeded to get the value ?
			        	{
		        			outInputSolved.setByte(byte_index, (char)value);
			        	}
			        }
        		}
        	}
    	}
    	Z3_model_dec_ref(m_context, model);
    	return true;
    }
    return false;
}

void PathConstraintZ3Solver::addDeclFuncAndSymbolFromAst(Z3_ast astRes)
{
	// Get the func decl and symbol from the constant variable just created (representing a byte index variable in the input payload)
	Z3_ast_kind astKind = Z3_get_ast_kind(m_context, astRes);
	assert(astKind == Z3_APP_AST);
		
	Z3_func_decl decl = Z3_get_app_decl(m_context, (Z3_app)astRes);
	Z3_symbol symbol = Z3_get_decl_name(m_context, decl);
	//Z3_string symbolStr = Z3_get_symbol_string(m_context, symbol);
	//printf("%s\n", symbolStr);

	m_currentDeclFuncs.push_back(decl);
	m_currentDeclSymbols.push_back(symbol);
}

// Populates all variables declarations inside this function
void PathConstraintZ3Solver::addAllVariablesDeclarations(const PathConstraint* pathConstraint)
{
	// For each different byte index used in the pathConstraint, add a declaration on 8 bit sort here
	char tempBuff[128];
	for (const int byteIndex : pathConstraint->variables)
	{
		//snprintf(tempBuff, 128, "%d", byteIndex);
		tempBuff[0] = '@';
		Utils::my_itoa(byteIndex, tempBuff+1, 10);
		Z3_symbol s_i = Z3_mk_string_symbol(m_context, tempBuff);
		Z3_ast astRes = Z3_mk_const(m_context, s_i, m_sortBV8);

		Z3_inc_ref(m_context, astRes);

		addDeclFuncAndSymbolFromAst(astRes);
		m_byteVariablesDeclAST.insert(std::make_pair(byteIndex, astRes));
	}

	// Do the same for the jump symbols variables
	for (const Test& test : pathConstraint->constraints)
	{
		// Add variable decl of the specified constraint
		snprintf(tempBuff, 128, "$%s", test.test_address.c_str());
		Z3_symbol s_i = Z3_mk_string_symbol(m_context, tempBuff);
		Z3_ast jumpSymbolAst = Z3_mk_const(m_context, s_i, m_sortBV1);
		Z3_inc_ref(m_context, jumpSymbolAst);

		addDeclFuncAndSymbolFromAst(jumpSymbolAst);
		m_jumpSymbolDeclAST.push_back(jumpSymbolAst);
	}
}

// Add all internal Z3 constraints from each individual branch
void PathConstraintZ3Solver::addAllInternalZ3Constraints(const PathConstraint* pathConstraint)
{
	for (const Test& test : pathConstraint->constraints)
	{
		//Test test_copy = test;
		//test_copy.Z3_code = "(assert (let ((a!1 (bvor ((_ extract 7 7) @0) (bvnot ((_ extract 7 7) (bvadd #xa7 @1)))))) (let ((a!2 (bvnot (bvor (ite (= @0 #x59) #b1 #b0) (bvnot a!1))))) (= a!2 $f629ee26))))";

		// TODO: serialize all Z3_code in binary to make them faster for Z3 solver to inject them 
		Z3_ast_vector assertsFromStringCode = Z3_parse_smtlib2_string(m_context, test.Z3_code.c_str(), 0, 0, 0, 
																		m_currentDeclFuncs.size(), m_currentDeclSymbols.data(), m_currentDeclFuncs.data());
		unsigned int numAssertsInStringCode = Z3_ast_vector_size(m_context, assertsFromStringCode);
		for (int i = 0; i < numAssertsInStringCode; i++)
		{
			Z3_ast ast = Z3_ast_vector_get(m_context, assertsFromStringCode, i);
			//Z3_inc_ref(m_context, ast);  // Is needed ?
			Z3_solver_assert(m_context, m_solver, ast);
		}
	}
} 
