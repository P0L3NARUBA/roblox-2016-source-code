#include "stdafx.h"

#include "v8datamodel/ServerScriptService.h"
#include "v8datamodel/Folder.h"
#include "script/script.h"
#include "script/CoreScript.h"
#include "script/ModuleScript.h"
#include "network/Players.h"

using namespace RBX;

const char* const RBX::sServerScriptService = "ServerScriptService";

REFLECTION_BEGIN();
Reflection::PropDescriptor<ServerScriptService, bool> ServerScriptService::desc_loadStringEnabled("LoadStringEnabled", category_Behavior, &ServerScriptService::getLoadStringEnabled, &ServerScriptService::setLoadStringEnabled, Reflection::PropertyDescriptor::PUBLIC_SERIALIZED);
REFLECTION_END();

ServerScriptService::ServerScriptService(void)
	: loadStringEnabled(false)
{
	setName(sServerScriptService);
}

void ServerScriptService::setLoadStringEnabled(bool value)
{
	bool changed = value != loadStringEnabled;
    loadStringEnabled = value;
	if (changed && Network::Players::isCloudEdit(this))
	{
		raisePropertyChanged(desc_loadStringEnabled);
	}
}

bool ServerScriptService::askAddChild(const Instance* instance) const
{
	if (Instance::fastDynamicCast<Folder>(instance) != nullptr)
		return true;

	return ((Instance::fastDynamicCast<Script>(instance) != nullptr) &&
        (Instance::fastDynamicCast<LocalScript>(instance) == nullptr)) ||
        (Instance::fastDynamicCast<ModuleScript>(instance) != nullptr);
}

bool ServerScriptService::scriptShouldRun(BaseScript* script)
{	
	bool isAncestor = isAncestorOf(script);
	RBXASSERT(isAncestor);

	if(!isAncestor)
		return false;

	if (!RBX::Network::Players::backendProcessing(script))
		return false;

	if (script->fastDynamicCast<Script>() && !script->fastDynamicCast<LocalScript>())
		return true;

	return false;
}