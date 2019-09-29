#include "../src/constraints.h"
#include "../src/inputpayload.h"
#include <z3.h>
#include <assert.h>
#include <unordered_map>

// This class is a solver for a PathConstraint object
class PathConstraintZ3Solver
{
public:
	PathConstraintZ3Solver();
	~PathConstraintZ3Solver();
	void init(const PathConstraint* pathConstraint);
	void pushState();
	void popState();
	void addConstraint(const int constraintIndex);

	// Solves the current solver's conditions, fills in the new input payload and returns true if it could be solved, false otherwise
	bool solve(InputPayload& outInputSolved);

private:
	// Populates all variables declarations inside this function
	void addAllVariablesDeclarations(const PathConstraint* pathConstraint);

	// Add all internal Z3 constraints from each individual branch
	void addAllInternalZ3Constraints(const PathConstraint* pathConstraint);

	// This functions gets a declaration/app from an AST and adds the internal declaration and symbol names to the internal data structures
	void addDeclFuncAndSymbolFromAst(Z3_ast astRes);

	const PathConstraint* m_pathConstraint = nullptr;// The path constraint the we need to iterate and solve
	Z3_config m_config;
	Z3_context m_context; // The Z3 context 
	Z3_solver m_solver;		// The solver object

	// Common sorts we reuse
	Z3_sort m_sortBV1, m_sortBV8;

	// Common constants of BV1 sort
	Z3_ast m_constBV1_0, m_constBV1_1;


	// Current declarations and their names (byte indices variables + jump symbols variables)
	std::vector<Z3_symbol> 				m_currentDeclSymbols; 
	std::vector<Z3_func_decl> 			m_currentDeclFuncs;

	// The sorts list created internally
	std::vector<Z3_sort> 				m_sorts;

	// the Z3_ast declaration/app for each jump symb - used to assert on them (i.e. impose conditions)
	std::vector<Z3_ast>					m_jumpSymbolDeclAST; 

	// Same as above but for byte indices variables
	std::unordered_map<int, Z3_ast> 	m_byteVariablesDeclAST;

	// Same for constants
	std::vector<Z3_ast>					m_constants;

};

