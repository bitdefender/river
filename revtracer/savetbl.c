#include "translatetbl.h"

unsigned int __stdcall FuncNoSave(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pSave, unsigned int *dwSaveWr) {
	return *dwSaveWr = 0;
}


unsigned int __stdcall HandlePush(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pSave, unsigned int *dwSaveWr) {
	/*mov edx, [saveLog]

	mov ecx, [r_sp]
	sub ecx, 4
	mov eax, [ecx]
	mov[edx], eax
	add edx, 4
	mov[r_sp], ecx

	mov[saveLog], edx*/
	return 0;
}

unsigned int __stdcall HandlePop(struct _exec_env *pEnv, struct _cb_info *pCB, unsigned int *dwFlags, char *pI, char *pSave, unsigned int *dwSaveWr) {
	return 0;
}


OpCopy SaveTable00[] = {
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave
};

OpCopy SaveTable0F[] = {
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,

	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave,
	FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave, FuncNoSave
};