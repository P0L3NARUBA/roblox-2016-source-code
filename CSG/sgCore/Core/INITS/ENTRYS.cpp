#include "../sg.h"

int  nMEM_TRACE_LEVEL = 2;             
BOOL bINHOUSE_VERSION = FALSE;         
//---------------------------------------------------------------

BOOL (*pExtStartupInits)(void) = nullptr;

void (*pExtFreeAllMem)(void) = nullptr;

void (*pExtSyntaxInit)(void) = nullptr;

void (*pExtActionsInit)(void) = nullptr;

void (*pExtRegObjects)(void) = nullptr;

void (*pExtRegCommonMethods)(void) = nullptr;

void (*pExtInitObjMetods)(void) = nullptr;

void (*pExtConfigPars)(void) = nullptr;

void (*pExtSysAttrs)(void) = nullptr;


BOOL (*pExtBeforeRunCmdLoop)(void) = nullptr;

BOOL (*pExtBeforeNew)(void) = nullptr;
BOOL (*pExtAfterNew)(void) = nullptr;
BOOL (*pExtReLoadCSG)(hOBJ hobj, hCSG *hcsg) = nullptr;

