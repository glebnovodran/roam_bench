#ifndef ROAM_QJS
#	define ROAM_QJS 0
#endif

enum class RoamProgKind {
	NATIVE,
	PINT,
	PLOP,
	QJS
};

// Native
void roam_ctrl_native(SmpChar* pChar);

// Pint
void init_roam_pint();

void reset_roam_pint();

bool chr_exec_init_pint_func(ScnObj* pObj, void* pWkMem);

bool chr_exec_reset_pint_func(ScnObj* pObj, void* pWkMem);

void roam_ctrl_pint(SmpChar* pChar);

// QJS

void init_roam_qjs();

void reset_roam_qjs();

void roam_ctrl_qjs(SmpChar* pChar);
