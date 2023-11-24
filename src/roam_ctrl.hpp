#ifndef ROAM_QJS
#	define ROAM_QJS 0
#endif

#ifndef ROAM_LUA
#	define ROAM_LUA 0
#endif

#ifndef ROAM_WRENCH
#	define ROAM_WRENCH 0
#endif
enum class RoamProgKind {
	NATIVE,
	PINT,
	PLOP,
	QJS,
	LUA,
	WRENCH,
	MINION
};

bool is_mood_enabled();

bool is_mood_visible();

bool is_roamctrl_disabled();

double calc_mood_arg(double nowTime);

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

// Lua

void init_roam_lua();

void reset_roam_lua();

void roam_ctrl_lua(SmpChar* pChar);

// wrench

void init_roam_wrench();

void reset_roam_wrench();

void roam_ctrl_wrench(SmpChar* pChar);

// minion

void init_roam_minion();

void reset_roam_minion();

bool chr_exec_init_minion_func(ScnObj* pObj, void* pWkMem);

bool chr_exec_reset_minion_func(ScnObj* pObj, void* pWkMem);

void roam_ctrl_minion(SmpChar* pChar);
