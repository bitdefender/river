#ifdef	__ASSEMBLER__

# define GET_PC_THUNK(reg) __x86.get_pc_thunk.reg

/* ELF uses byte-counts for .align, most others use log2 of count of bytes.  */
#define ALIGNARG(log2) 1<<log2
#define ASM_SIZE_DIRECTIVE(name) .size name,.-name;
#define JUMPTARGET(sym)	sym
# define C_LABEL(name)	name##:
# define C_SYMBOL_NAME(name) name

# define cfi_startproc			.cfi_startproc
# define cfi_endproc			.cfi_endproc
# define cfi_adjust_cfa_offset(off)	.cfi_adjust_cfa_offset off
# define cfi_def_cfa_register(reg)	.cfi_def_cfa_register reg
# define cfi_def_cfa(reg, off)		.cfi_def_cfa reg, off


#ifdef	PROF
/* The mcount code relies on a normal frame pointer being on the stack
   to locate our caller, so push one just for its benefit.  */
#define CALL_MCOUNT \
  pushl %ebp; cfi_adjust_cfa_offset (4); movl %esp, %ebp; \
  cfi_def_cfa_register (ebp); call JUMPTARGET(mcount); \
  popl %ebp; cfi_def_cfa (esp, 4);
#else
#define CALL_MCOUNT		/* Do nothing.  */
#endif

#define	ENTRY(name)							      \
  .globl C_SYMBOL_NAME(name);						      \
  .type C_SYMBOL_NAME(name),@function;					      \
  .align ALIGNARG(4);							      \
  C_LABEL(name)								      \
  cfi_startproc;							      \
  CALL_MCOUNT

#undef	END
#define END(name)							      \
  cfi_endproc;								      \
  ASM_SIZE_DIRECTIVE(name)


# define SETUP_PIC_REG(reg) \
  .ifndef GET_PC_THUNK(reg);						      \
  .section .gnu.linkonce.t.GET_PC_THUNK(reg),"ax",@progbits;		      \
  .globl GET_PC_THUNK(reg);						      \
  .hidden GET_PC_THUNK(reg);						      \
  .p2align 4;								      \
  .type GET_PC_THUNK(reg),@function;					      \
GET_PC_THUNK(reg):							      \
  movl (%esp), %e##reg;							      \
  ret;									      \
  .size GET_PC_THUNK(reg), . - GET_PC_THUNK(reg);			      \
  .previous;								      \
  .endif;								      \
  call GET_PC_THUNK(reg)

# define LOAD_PIC_REG(reg) \
  SETUP_PIC_REG(reg); addl $_GLOBAL_OFFSET_TABLE_, %e##reg

# define LOAD_GOT_AND_RTLD_GLOBAL_RO \
  LOAD_PIC_REG(dx); \
  mov _rtld_global_ro@GOT(%edx), %ecx


.globl patch__rtld_global_ro
.text
ENTRY(patch__rtld_global_ro_stub)
	.type patch__rtld_global_ro_stub, @gnu_indirect_function
	call patch__rtld_global_ro
	ret
END(patch__rtld_global_ro_stub)


.globl get_rtld_global_ro_addr
.section .text
get_rtld_global_ro_addr:
LOAD_GOT_AND_RTLD_GLOBAL_RO
movl %ecx, %eax
ret
#endif
