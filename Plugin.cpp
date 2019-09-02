#define DLLFUNC extern "C" __declspec(dllexport)

#include "common/IPrefix.h"
#include "f4se/PluginAPI.h"
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/GameRTTI.h"

class MyAnimationGraphManagerHolder : public IAnimationGraphManagerHolder {
public:
	virtual void SendEvent(const BSFixedString* eventName) = 0;
};

BOOL ConsolePrint(HANDLE hOut, const char *format, ...) {
	va_list args;
	va_start(args, format);

	size_t len = _vscprintf(format, args);
	char *buffer = new char[len + 1];

	DWORD bytes_written;
	vsprintf_s(buffer, len + 1, format, args);
	BOOL bRet = WriteFile(hOut, buffer, len, &bytes_written, NULL);

	delete[] buffer;
	va_end(args);

	return bRet;
}

int nullableStringLength(const char *str) {
	int len = str ? strlen(str) : -1;
	return len;
}

void DumpNpc(HANDLE hOut, TESNPC *ent) {
	ConsolePrint(hOut, "npc (%p):\n", ent);
	if (ent) {
		const char *name = ent->fullName.name.c_str();
		ConsolePrint(hOut, "Full Name: %p[%u] %s\n", name, nullableStringLength(name), name);
	}
	else {
		ConsolePrint(hOut, "Null Entity\n");
	}
}

void DumpForm(HANDLE hOut, TESForm *ent) {
	ConsolePrint(hOut, "Form (%p):\n", ent);
	if (ent) {
		UInt8 formType = ent->formType;
		UInt32 formID = ent->formID;
		UInt32 flags = ent->flags;
		const char *name = ent->GetFullName();
		const char *edid = ent->GetEditorID();

		ConsolePrint(hOut, "Type: %u, ID: %x, Flags: %x\n", UInt32(formType), formID, flags);
		ConsolePrint(hOut, "Name: %p[%u] %s\n", name, nullableStringLength(name), name);
		ConsolePrint(hOut, "EDID: %p[%u] %s\n", edid, nullableStringLength(edid), edid);

		TESNPC *npc = DYNAMIC_CAST(ent, TESForm, TESNPC);
		if (npc) {
			DumpNpc(hOut, npc);
		}
	}
	else {
		ConsolePrint(hOut, "Null Entity\n");
	}
}

void HackGraph(HANDLE hOut, IAnimationGraphManagerHolder *animGraphHolder) {
	MyAnimationGraphManagerHolder *graph = (MyAnimationGraphManagerHolder*)animGraphHolder;
	ConsolePrint(hOut, "Graph: %p, %p\n", graph, &MyAnimationGraphManagerHolder::SendEvent);
}

void DumpRef(HANDLE hOut, TESObjectREFR *ent) {
	ConsolePrint(hOut, "Refr (%p):\n", ent);
	if (ent) {
		const char *refn = CALL_MEMBER_FN(ent, GetReferenceName)();
		ConsolePrint(hOut, "RefN: %p[%u] %s\n", refn, nullableStringLength(refn), refn);
		DumpForm(hOut, ent);

		IAnimationGraphManagerHolder *animGraphHolder = &ent->animGraphHolder;
		HackGraph(hOut, animGraphHolder);
	}
	else {
		ConsolePrint(hOut, "Null Entity\n");
	}
}

void ServiceThread() {
	const static DWORD buffer_length = 128;
	char buffer[buffer_length];
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD bytes_read;

	ConsolePrint(hStdOut, "Ready, press enter to search!\n");
	while (1) {
		BOOL bRet = ReadFile(hStdIn, buffer, buffer_length - 1, &bytes_read, NULL);
		if (!bRet) {
			ConsolePrint(hStdOut, "I/O error?\n");
		}
		else {
			buffer[bytes_read] = 0;
			TESForm *searchRet = LookupFormByID(0xdc939);
			if (!searchRet) {
				ConsolePrint(hStdOut, "Lookup Failed!\n");
			}
			else {
				TESObjectREFR *ref = DYNAMIC_CAST(searchRet, TESForm, TESObjectREFR);
				if (!ref) {
					ConsolePrint(hStdOut, "Bad cast?\n");
				}
				else {
					DumpRef(hStdOut, ref);
				}
			}
		}
	}
}

DWORD WINAPI ServiceThreadWrapper(LPVOID) {
	ServiceThread();
	return 0;
}

DLLFUNC bool F4SEPlugin_Query(const F4SEInterface * f4se, PluginInfo * info) {
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "Terabyte Test Plugin Charlie";
	info->version = 1;
	return true;
}

DLLFUNC bool F4SEPlugin_Load(const F4SEInterface * f4se) {
	DWORD thread_id;
	AllocConsole();
	HANDLE hThread = CreateThread(NULL, 0, ServiceThreadWrapper, NULL, 0, &thread_id);
	return hThread != 0;
}