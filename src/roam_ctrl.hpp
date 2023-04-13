enum class RoamProgKind {
	NATIVE,
	PINT,
	PLOP
};

void roam_ctrl_native(SmpChar* pChar);

void init_roam_pint();

void reset_roam_pint();

bool chr_exec_init_pint_func(ScnObj* pObj, void* pWkMem);

bool chr_exec_reset_pint_func(ScnObj* pObj, void* pWkMem);

void roam_ctrl_pint(SmpChar* pChar);