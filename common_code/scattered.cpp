#include "scattered.h"
#include <assert.h>
#include <Windows.h>
#ifdef _DEBUG
#include <WinDNS.h>
#include <DbgHelp.h>
#include <Psapi.h>
#include <boost/lexical_cast.hpp>
#pragma comment( lib, "Dbghelp.lib" )
#pragma comment( lib, "Psapi.lib" )
#endif
#pragma comment( lib, "Winmm.lib" )

typedef LONG (__stdcall * NT_SET_TIMER_RESOLUTION)
	(
	IN ULONG DesiredTime,
	IN BOOLEAN SetResolution,
	OUT PULONG ActualTime
	);

typedef LONG (__stdcall * NT_QUERY_TIMER_RESOLUTION)
	(
	OUT PULONG MaximumTime,
	OUT PULONG MinimumTime,
	OUT PULONG CurrentTime
	);

struct pc_cycle
{
	pc_cycle()
	{
		LARGE_INTEGER frep;
		if (!QueryPerformanceFrequency(&frep))
		{
			_sCycle = 0;
			_msCycle = 0;
			_usCycle = 0;
			assert(false);
			return;
		}
		_sCycle = 1.0/(double)frep.QuadPart;
		_msCycle = 1000.0/(double)frep.QuadPart;
		_usCycle = 1000000.0/(double)frep.QuadPart;
	}

	double _sCycle;
	double _msCycle;
	double _usCycle;
} _pcCycle;

void enable_high_resolution()
{
	HMODULE hNtDll = LoadLibrary(TEXT("NtDll.dll"));
	if (hNtDll)
	{
		NT_QUERY_TIMER_RESOLUTION _NtQueryTimerResolution = (NT_QUERY_TIMER_RESOLUTION)GetProcAddress(hNtDll, "NtQueryTimerResolution");
		NT_SET_TIMER_RESOLUTION _NtSetTimerResolution = (NT_SET_TIMER_RESOLUTION)GetProcAddress(hNtDll, "NtSetTimerResolution");
		if (_NtQueryTimerResolution && _NtSetTimerResolution)
		{
			ULONG MaximumTime = 0;
			ULONG MinimumTime = 0;
			ULONG CurrentTime = 0;
			if (!_NtQueryTimerResolution(&MaximumTime, &MinimumTime, &CurrentTime))
			{
				ULONG ActualTime = 0;
				if (!_NtSetTimerResolution(MinimumTime, TRUE, &ActualTime))
				{
					FreeLibrary(hNtDll);
					return;
				}
			}
		}
		FreeLibrary(hNtDll);
	}
	timeBeginPeriod(1);
}

void enable_realtime_priority()
{
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
}

void set_priority(int p)
{
	SetPriorityClass(GetCurrentProcess(), p);
}

long long get_tick_us()
{
	LARGE_INTEGER quadPart;
	QueryPerformanceCounter(&quadPart);
	return (long long)((double)quadPart.QuadPart*_pcCycle._usCycle);
}

long long get_tick_ms()
{
	LARGE_INTEGER quadPart;
	QueryPerformanceCounter(&quadPart);
	return (long long)((double)quadPart.QuadPart*_pcCycle._msCycle);
}

int get_tick_s()
{
	LARGE_INTEGER quadPart;
	QueryPerformanceCounter(&quadPart);
	return (int)((double)quadPart.QuadPart*_pcCycle._sCycle);
}
//////////////////////////////////////////////////////////////////////////

void* get_sp()
{
	void* bp;
	void* sp;
	void* ip;

	get_bp_sp_ip(&bp, &sp, &ip);
	return sp;
}

//////////////////////////////////////////////////////////////////////////

void passing_test::operator=(passing_test&& s)
{
	assert(s._count);
	_count = s._count;
	_count->_moveCount++;
	s._count.reset();
	if (_count->_cb)
	{
		_count->_cb(_count);
	}
}

void passing_test::operator=(const passing_test& s)
{
	assert(s._count);
	_count = s._count;
	_count->_copyCount++;
	if (_count->_cb)
	{
		_count->_cb(_count);
	}
}

passing_test::passing_test(passing_test&& s)
{
	*this = std::move(s);
}

passing_test::passing_test(const passing_test& s)
{
	*this = s;
}

passing_test::passing_test(int id, const std::function<void(std::shared_ptr<count>)>& cb)
{
	_count = std::shared_ptr<count>(new count);
	_count->_copyCount = 0;
	_count->_moveCount = 0;
	_count->_id = id;
	_count->_cb = cb;
}

passing_test::passing_test(int id)
{
	_count = std::shared_ptr<count>(new count);
	_count->_copyCount = 0;
	_count->_moveCount = 0;
	_count->_id = id;
}

passing_test::~passing_test()
{

}
//////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

bool _loadAllModules()
{
	HANDLE hProcess = ::GetCurrentProcess();
	static const int maxHandles = 4096;
	HMODULE aryHandles[maxHandles] = { 0 };

	unsigned bytes = 0;

	BOOL result = ::EnumProcessModules(
		hProcess, aryHandles, sizeof(aryHandles), (LPDWORD)&bytes);

	if (FALSE == result)
	{
		return false;
	}

	const int iCount = bytes / sizeof(HMODULE);

	for (int i = 0; i < iCount; ++i)
	{
		char moduleName[256] = { 0 };
		char imageName[256] = { 0 };
		MODULEINFO Info;

		::GetModuleInformation(hProcess, aryHandles[i], &Info, sizeof(Info));
		::GetModuleFileNameExA(hProcess, aryHandles[i], imageName, 256);
		::GetModuleBaseNameA(hProcess, aryHandles[i], moduleName, 256);

		::SymLoadModule64(hProcess, aryHandles[i], imageName, moduleName, (DWORD64)Info.lpBaseOfDll, (DWORD)Info.SizeOfImage);
	}

	return true;
}

bool _initialize()
{
	// ÉèÖÃ·ûºÅÒýÇæ
	DWORD symOpts = ::SymGetOptions();
	symOpts |= SYMOPT_LOAD_LINES;
	symOpts |= SYMOPT_DEBUG;
	::SymSetOptions(symOpts);

	if (FALSE == ::SymInitialize(::GetCurrentProcess(), NULL, TRUE))
	{
		return false;
	}

	// 	if (!_loadAllModules())
	// 	{
	// 		return false;
	// 	}
	return true;
}

size_t _stackwalk(QWORD *pTrace, size_t maxDepth, CONTEXT *pContext)
{
	STACKFRAME64 stackFrame64;
	HANDLE hProcess = ::GetCurrentProcess();
	HANDLE hThread = ::GetCurrentThread();

	size_t depth = 0;

	::ZeroMemory(&stackFrame64, sizeof(stackFrame64));

	__try
	{
#ifdef _WIN64
		stackFrame64.AddrPC.Offset = pContext->Rip;
		stackFrame64.AddrPC.Mode = AddrModeFlat;
		stackFrame64.AddrStack.Offset = pContext->Rsp;
		stackFrame64.AddrStack.Mode = AddrModeFlat;
		stackFrame64.AddrFrame.Offset = pContext->Rbp;
		stackFrame64.AddrFrame.Mode = AddrModeFlat;
#else
		stackFrame64.AddrPC.Offset = pContext->Eip;
		stackFrame64.AddrPC.Mode = AddrModeFlat;
		stackFrame64.AddrStack.Offset = pContext->Esp;
		stackFrame64.AddrStack.Mode = AddrModeFlat;
		stackFrame64.AddrFrame.Offset = pContext->Ebp;
		stackFrame64.AddrFrame.Mode = AddrModeFlat;
#endif

		bool successed = true;

		while (successed && (depth < maxDepth))
		{
			successed = ::StackWalk64(
#ifdef _WIN64
				IMAGE_FILE_MACHINE_AMD64,
#else
				IMAGE_FILE_MACHINE_I386,
#endif
				hProcess,
				hThread,
				&stackFrame64,
				pContext,
				NULL,
				SymFunctionTableAccess64,
				SymGetModuleBase64,
				NULL
				) != FALSE;

			pTrace[depth] = stackFrame64.AddrPC.Offset;

			if (!successed)
			{
				break;
			}

			if (stackFrame64.AddrFrame.Offset == 0)
			{
				break;
			}
			++depth;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
	return depth;
}

size_t check_file_name(char* name)
{
	size_t length = strlen(name);
	if (length)
	{
		name[length++] = '.';
		size_t l = strlen(name + length);
		length += l ? l : -1;
	}
	return length;
}

list<stack_line_info> get_stack_list(size_t maxDepth, bool module, bool symbolName)
{
	assert(maxDepth <= 32);

	QWORD trace[33];
	CONTEXT context;

	::ZeroMemory(&context, sizeof(context));
	context.ContextFlags = CONTEXT_FULL;
#ifdef _WIN64
	get_bp_sp_ip((void**)&context.Rbp, (void**)&context.Rsp, (void**)&context.Rip);
#else
	get_bp_sp_ip((void**)&context.Ebp, (void**)&context.Esp, (void**)&context.Eip);
#endif
	size_t depths = _stackwalk(trace, maxDepth+1, &context);

	list<stack_line_info> imageList;
	HANDLE hProcess = ::GetCurrentProcess();
	for (size_t i = 1; i < depths; i++)
	{
		stack_line_info stackResult;

		{
			DWORD symbolDisplacement = 0;
			IMAGEHLP_LINE64 imageHelpLine;
			imageHelpLine.SizeOfStruct = sizeof(imageHelpLine);

			if (::SymGetLineFromAddr64(hProcess, trace[i], &symbolDisplacement, &imageHelpLine))
			{
				stackResult.file = string(imageHelpLine.FileName, check_file_name(imageHelpLine.FileName));
				memset(imageHelpLine.FileName, 0, stackResult.file.size() + 1);
				stackResult.line = (int)imageHelpLine.LineNumber;
			}
			else
			{
				stackResult.line = -1;
			}
		}
		if (symbolName)
		{
			static const int maxNameLength = 1024;
			char symbolBf[sizeof(IMAGEHLP_SYMBOL64)+maxNameLength] = { 0 };
			PIMAGEHLP_SYMBOL64 symbol;
			DWORD64 symbolDisplacement64 = 0;

			symbol = (PIMAGEHLP_SYMBOL64)symbolBf;
			symbol->SizeOfStruct = sizeof(symbolBf);
			symbol->MaxNameLength = maxNameLength;

			if (::SymGetSymFromAddr64(
				hProcess,
				trace[i],
				&symbolDisplacement64,
				symbol)
				)
			{
				stackResult.symbolName = symbol->Name;
			}
			else
			{
				stackResult.symbolName = "unknow...";
			}
		}
		if (module)
		{
			IMAGEHLP_MODULE64 imageHelpModule;
			imageHelpModule.SizeOfStruct = sizeof(imageHelpModule);

			if (::SymGetModuleInfo64(hProcess, trace[i], &imageHelpModule))
			{
				stackResult.module = string(imageHelpModule.ImageName, check_file_name(imageHelpModule.ImageName));
				memset(imageHelpModule.ImageName, 0, stackResult.module.size() + 1);
			}
		}
		imageList.push_back(std::move(stackResult));
	}
	return std::move(imageList);
}

struct init_mod
{
	init_mod()
	{
		bool ok = _initialize();
		assert(ok);
	}
} _init;
#endif