/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.

	The COM class-registration tree, extracted from the APO DLL's
	DllRegisterServer (audit #250 A3/F002): the CLSID writes were the last
	registry mutations in the machine-changing path that no test could
	reach, because they lived inside a fixed-signature STDAPI export.
	DllRegisterServer keeps its structure (RegisterAPO ordering, the
	catch-and-unregister rollback); the key/value writes go through the
	port here so a fake registry can pin the exact tree - the spelling
	HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID (unified by F022), the
	InprocServer32 default and the Both threading model.

	Deliberately GUID-agnostic (takes the formatted CLSID string) so this
	unit compiles without the APO's ATL headers.
*/

#pragma once

#include <string>
#include <vector>

#include "services/registry/IRegistry.h"

namespace ClsidRegistration
{
struct ClsidTree
{
	std::wstring clsidString;
	std::wstring className;
	std::wstring dllPath;
};

// Writes HKLM\SOFTWARE\Classes\CLSID\<clsid> with the class display name
// and its InprocServer32 (path + ThreadingModel Both). On a registry failure,
// it asks RegistryTransaction to restore the pre-call registry state; an
// undo failure is included in the propagated error for the caller to report.
void registerClsidTree(IRegistry& registry, const std::wstring& clsidString,
	const std::wstring& className, const std::wstring& dllPath);

// Registers each tree as one transaction. A failure asks RegistryTransaction
// to restore every completed and partial tree to its pre-call state.
void registerClsidTrees(IRegistry& registry, const std::vector<ClsidTree>& trees);

// Deletes the InprocServer32 subkey, then the class key.
void unregisterClsidTree(IRegistry& registry, const std::wstring& clsidString);
}
