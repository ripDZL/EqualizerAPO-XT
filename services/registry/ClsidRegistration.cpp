/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
*/

#include "stdafx.h"
#include "services/registry/ClsidRegistration.h"
#include "services/registry/RegistryError.h"
#include "services/registry/RegistryTransaction.h"

namespace
{
const wchar_t* const clsidRoot = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\";

void writeClsidTree(IRegistry& registry, const std::wstring& clsidString,
	const std::wstring& className, const std::wstring& dllPath)
{
	const std::wstring classKey = clsidRoot + clsidString;
	const std::wstring serverKey = classKey + L"\\InprocServer32";

	registry.createKey(classKey);
	registry.writeValue(classKey, L"", className);
	registry.createKey(serverKey);
	registry.writeValue(serverKey, L"", dllPath);
	registry.writeValue(serverKey, L"ThreadingModel", L"Both");
}

[[noreturn]] void rollbackAndReport(RegistryTransaction& transaction,
const RegistryError& registrationError)
{
	transaction.rollback();
	if (!transaction.rollbackFailures().empty())
	{
		throw RegistryError(registrationError.getMessage()
			+ L"; CLSID rollback failed: " + transaction.rollbackFailures().front());
	}
	throw RegistryError(registrationError.getMessage());
}
}

namespace ClsidRegistration
{
void registerClsidTree(IRegistry& registry, const std::wstring& clsidString,
	const std::wstring& className, const std::wstring& dllPath)
{
	RegistryTransaction transaction(registry);
	try
	{
		writeClsidTree(transaction, clsidString, className, dllPath);
		transaction.commit();
	}
	catch (const RegistryError& error)
	{
		rollbackAndReport(transaction, error);
	}
}

void registerClsidTrees(IRegistry& registry, const std::vector<ClsidTree>& trees)
{
	RegistryTransaction transaction(registry);
	try
	{
		for (const ClsidTree& tree : trees)
			writeClsidTree(transaction, tree.clsidString, tree.className, tree.dllPath);
		transaction.commit();
	}
	catch (const RegistryError& error)
	{
		rollbackAndReport(transaction, error);
	}
}

void unregisterClsidTree(IRegistry& registry, const std::wstring& clsidString)
{
	const std::wstring classKey = clsidRoot + clsidString;

	registry.deleteKey(classKey + L"\\InprocServer32");
	registry.deleteKey(classKey);
}
}
