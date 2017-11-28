#include "Z3SymbolicExecutor.h"

#include "../CommonCrossPlatform/Common.h"

Z3SymbolicExecutor::Z3SymbolicCpuFlag::Z3SymbolicCpuFlag() {
	Unset();
}

void Z3SymbolicExecutor::Z3SymbolicCpuFlag::SetParent(Z3SymbolicExecutor *p) {
	parent = p;
}

void Z3SymbolicExecutor::Z3SymbolicCpuFlag::SetValue(Z3_ast val) {
	value = val;
}

void Z3SymbolicExecutor::Z3SymbolicCpuFlag::Unset() {
	value = nullptr;
}

Z3_ast Z3SymbolicExecutor::Z3SymbolicCpuFlag::GetValue() {
	if (value == (Z3_ast)&lazyMarker) {
		value = Eval();
	}

	return value;
}


// Zero flag - simple compare with zero
Z3_ast Z3FlagZF::Eval() {
	printf("<sym> lazyZF %p\n", source);

	return Z3_mk_ite(parent->context,
		Z3_mk_eq(
			parent->context,
			source,
			Z3_mk_int(parent->context, 0, Z3_get_sort(parent->context, source))
		),
		parent->oneFlag,
		parent->zeroFlag
	);
}

void Z3FlagZF::SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op) {
	source = src;
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagZF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
}

void Z3FlagZF::LoadState(stk::LargeStack &stack) {
	source = (Z3_ast)stack.Pop();
}


// Sign flag - extract msb
Z3_ast Z3FlagSF::Eval() {
	printf("<sym> lazySF %p\n", source);

	return Z3_mk_extract(
		parent->context,
		31,
		31,
		source
	);
}

void Z3FlagSF::SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op) {
	source = src;
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagSF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
}

void Z3FlagSF::LoadState(stk::LargeStack &stack) {
	source = (Z3_ast)stack.Pop();
}

// Sign flag - extract msb
Z3_ast Z3FlagPF::Eval() {
	Z3_ast b4 = Z3_mk_bvxor(
		parent->context,
		source,
		Z3_mk_bvashr(
			parent->context,
			source,
			Z3_mk_int(
				parent->context,
				4,
				parent->dwordSort
			)
		)
	);

	Z3_ast b2 = Z3_mk_bvxor(
		parent->context,
		b4,
		Z3_mk_bvashr(
			parent->context,
			b4,
			Z3_mk_int(
				parent->context,
				2,
				parent->dwordSort
			)
		)
	);

	Z3_ast b1 = Z3_mk_bvxor(
		parent->context,
		b2,
		Z3_mk_bvashr(
			parent->context,
			b2,
			Z3_mk_int(
				parent->context,
				1,
				parent->dwordSort
			)
		)
	);

	printf("<sym> lazyPF %p\n", source);

	return Z3_mk_extract(
		parent->context,
		0,
		0,
		b1
	);
}

void Z3FlagPF::SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op) {
	source = src;
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagPF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
}

void Z3FlagPF::LoadState(stk::LargeStack &stack) {
	source = (Z3_ast)stack.Pop();
}

// Carry flag
// ADD: c = (a & b) | ((a | b) & ~r)
// SUB: c = ~((a & ~b) | ((a | ~b) & ~r))
// ADC: ?
// SBB: ?
// MUL: ?
// IMUL: ?
// ??
Z3_ast Z3FlagCF::Eval() {
	switch (func) {
		case Z3_FLAG_OP_ADD:
		case Z3_FLAG_OP_SUB:
			break;
		default :
			DEBUG_BREAK;
	}

	Z3_ast nr = Z3_mk_bvnot(
		parent->context,
		Z3_mk_extract(
			parent->context,
			31,
			31,
			source
		)
	);

	Z3_ast a = Z3_mk_extract(
		parent->context,
		31,
		31,
		p[0]
	);

	Z3_ast b = Z3_mk_extract(
		parent->context,
		31,
		31,
		p[0]
	);

	if (func == Z3_FLAG_OP_SUB) {
		b = Z3_mk_bvnot(
			parent->context,
			b
		);
	}

	Z3_ast c = Z3_mk_bvor(
		parent->context,
		Z3_mk_bvand(
			parent->context,
			a,
			b
		),
		Z3_mk_bvand(
			parent->context,
			nr,
			Z3_mk_bvor(
				parent->context,
				a,
				b
			)
		)
	);

	if (func == Z3_FLAG_OP_SUB) {
		c = Z3_mk_bvnot(
			parent->context,
			c
		);
	}

	printf("<sym> lazyCF %p <= %p, %p\n", source, p[0], p[1]);

	return c;
}

void Z3FlagCF::SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op) {
	source = src;
	p[0] = o1;
	p[1] = o2;
	func = op;
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagCF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
	stack.Push((DWORD)p[0]);
	stack.Push((DWORD)p[1]);
	stack.Push((DWORD)func);
}

void Z3FlagCF::LoadState(stk::LargeStack &stack) {
	func = (unsigned int)stack.Pop();
	p[1] = (Z3_ast)stack.Pop();
	p[0] = (Z3_ast)stack.Pop();
	source = (Z3_ast)stack.Pop();
}



// Overflow flag
// ADD: c = (a ^ r) & (b ^ r)
// SUB: c = (a ^ r) & (~b ^ r)
// ADC: ?
// SBB: ?
// MUL: ?
// IMUL: ?
// ??
Z3_ast Z3FlagOF::Eval() {
	switch (func) {
		case Z3_FLAG_OP_ADD:
		case Z3_FLAG_OP_SUB:
		case Z3_FLAG_OP_CMP:
			break;
		default:
			DEBUG_BREAK;
	}

	Z3_ast r = Z3_mk_extract(
		parent->context,
		31,
		31,
		source
	);

	Z3_ast a = Z3_mk_extract(
		parent->context,
		31,
		31,
		p[0]
	);

	Z3_ast b = Z3_mk_extract(
		parent->context,
		31,
		31,
		p[0]
	);

	if ((func == Z3_FLAG_OP_SUB) || (func == Z3_FLAG_OP_CMP)) {
		b = Z3_mk_bvnot(
			parent->context,
			b
		);
	}

	printf("<sym> lazyOF %p <= %p, %p\n", source, p[0], p[1]);

	return Z3_mk_bvand(
		parent->context,
		Z3_mk_bvxor(
			parent->context,
			a,
			r
		),
		Z3_mk_bvxor(
			parent->context,
			b,
			r
		)
	);
}

void Z3FlagOF::SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op) {
	source = src;
	p[0] = o1;
	p[1] = o2;
	func = op;
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagOF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
	stack.Push((DWORD)p[0]);
	stack.Push((DWORD)p[1]);
	stack.Push((DWORD)func);
}

void Z3FlagOF::LoadState(stk::LargeStack &stack) {
	func = (unsigned int)stack.Pop();
	p[1] = (Z3_ast)stack.Pop();
	p[0] = (Z3_ast)stack.Pop();
	source = (Z3_ast)stack.Pop();
}

// Adjust flag
Z3_ast Z3FlagAF::Eval() {
	DEBUG_BREAK;
}

void Z3FlagAF::SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op) {
	source = src;
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagAF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
}

void Z3FlagAF::LoadState(stk::LargeStack &stack) {
	source = (Z3_ast)stack.Pop();
}

// Direction flag
Z3_ast Z3FlagDF::Eval() {
	DEBUG_BREAK;
}

void Z3FlagDF::SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op) {
	source = src;
	value = (Z3_ast)&lazyMarker;
}

void Z3FlagDF::SaveState(stk::LargeStack &stack) {
	stack.Push((DWORD)source);
}

void Z3FlagDF::LoadState(stk::LargeStack &stack) {
	source = (Z3_ast)stack.Pop();
}
