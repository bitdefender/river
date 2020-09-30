BITS 32

/*
 * this code is the equivalent of rep modifier
 * the code between `init` label and `jmp wrapout`
 * should work fine if ecx value is not 0, otherwise it's infinite loop
 *
 * the code between wrapin and wrapout is what we call a `farloop`
 * loop operand is signed imm8 so if the instrumented code that
 * we `rep` is larger than 127 bytes, loop cannot jump enough. So we use
 * loop to a jmp instruction that points to the actual target
 *
 * `lea ecx, ecx + 1` is needed in case ecx value is zero. In order for
 * loop to have valid behavior, ecx must be one before reaching the loop
 * instruction, thus triggering the loop not to take the natural path,
 * but to exit.
 */

	jmp wrapin
init:
	jmp codein
loop:
	loop init
	jmp codeout

codein:
	// actual code
	jmp loop
codeout:
	jmp wrapout

farloop:
	jmp init
wrapin:
	lea ecx, [ecx + 1]
	loop farloop
wrapout:
