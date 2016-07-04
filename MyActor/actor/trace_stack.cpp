#include "trace_stack.h"

#if (defined ENABLE_DUMP_STACK || defined PRINT_ACTOR_STACK)
#ifdef WIN32
#include <WinDNS.h>
#include <DbgHelp.h>
#include <Psapi.h>
#include <fstream>
#include <direct.h>
#ifdef _MSC_VER
#pragma comment( lib, "Dbghelp.lib" )
#pragma comment( lib, "Psapi.lib" )
#endif
#elif __linux__
#include <execinfo.h>
#if (__i386__ || __x86_64__)
#include <dlfcn.h>
#endif
#endif
#endif

#if (defined ENABLE_DUMP_STACK || defined PRINT_ACTOR_STACK)
#ifdef WIN32
bool _loadAllModules()
{
	HANDLE hProcess = GetCurrentProcess();
	static const int maxHandles = 4096;
	HMODULE aryHandles[maxHandles] = { 0 };

	unsigned bytes = 0;

	BOOL result = EnumProcessModules(
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

		GetModuleInformation(hProcess, aryHandles[i], &Info, sizeof(Info));
		GetModuleFileNameExA(hProcess, aryHandles[i], imageName, 256);
		GetModuleBaseNameA(hProcess, aryHandles[i], moduleName, 256);

		SymLoadModule64(hProcess, aryHandles[i], imageName, moduleName, (DWORD64)Info.lpBaseOfDll, (DWORD)Info.SizeOfImage);
	}

	return true;
}

bool _initialize()
{
	// ÉèÖÃ·ûºÅÒýÇæ
	DWORD symOpts = SymGetOptions();
	symOpts |= SYMOPT_LOAD_LINES;
	symOpts |= SYMOPT_DEBUG;
	SymSetOptions(symOpts);

	if (FALSE == SymInitialize(GetCurrentProcess(), NULL, TRUE))
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
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();

	size_t depth = 0;

	ZeroMemory(&stackFrame64, sizeof(stackFrame64));
#ifdef _MSC_VER
	__try
#endif
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
			successed = StackWalk64(
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
#ifdef _MSC_VER
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
#endif
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

extern "C" void __fastcall get_bp_sp_ip(void** pbp, void** psp, void** pip);

std::list<stack_line_info> get_stack_list(size_t maxDepth, size_t offset, bool module, bool symbolName)
{
	void* bp = NULL;
	void* sp = NULL;
	void* ip = NULL;
	get_bp_sp_ip((void**)&bp, (void**)&sp, (void**)&ip);
	return get_stack_list(bp, sp, ip, maxDepth, 2+offset, module, symbolName);
}

std::list<stack_line_info> get_stack_list(void* bp, void* sp, void* ip, size_t maxDepth, size_t offset, bool module, bool symbolName)
{
	assert(maxDepth <= 32);

	QWORD trace[33];
	CONTEXT context;

	ZeroMemory(&context, sizeof(context));
	context.ContextFlags = CONTEXT_FULL;
#ifdef _WIN64
	context.Rbp = (DWORD64)bp;
	context.Rsp = (DWORD64)sp;
	context.Rip = (DWORD64)ip;
#else
	context.Ebp = (DWORD)bp;
	context.Esp = (DWORD)sp;
	context.Eip = (DWORD)ip;
#endif
	size_t depths = _stackwalk(trace, maxDepth + 1, &context);

	std::list<stack_line_info> imageList;
	HANDLE hProcess = GetCurrentProcess();
	for (size_t i = offset; i < depths; i++)
	{
		stack_line_info stackResult;

		{
			DWORD symbolDisplacement = 0;
			IMAGEHLP_LINE64 imageHelpLine;
			imageHelpLine.SizeOfStruct = sizeof(imageHelpLine);

			if (SymGetLineFromAddr64(hProcess, trace[i], &symbolDisplacement, &imageHelpLine))
			{
				stackResult.file = std::string(imageHelpLine.FileName, check_file_name(imageHelpLine.FileName));
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

			if (SymGetSymFromAddr64(
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

			if (SymGetModuleInfo64(hProcess, trace[i], &imageHelpModule))
			{
				stackResult.module = std::string(imageHelpModule.ImageName, check_file_name(imageHelpModule.ImageName));
				memset(imageHelpModule.ImageName, 0, stackResult.module.size() + 1);
			}
		}
		imageList.push_back(std::move(stackResult));
	}
	return imageList;
}

#ifdef PRINT_ACTOR_STACK
run_thread* _thread = NULL;
boost::asio::io_service* _stackLogIos = NULL;
boost::asio::io_service::work* _stackLogWork = NULL;
std::ofstream _stackLogFile;

void stack_overflow_format(int size, const std::list<stack_line_info>& createStack)
{
	stack_overflow_format(size, std::list<stack_line_info>(createStack));
}

void stack_overflow_format(int size, std::list<stack_line_info>&& createStack)
{
	if (_stackLogIos)
	{
		_stackLogIos->post(std::bind([size](std::list<stack_line_info>& createStack)
		{
			_stackLogFile << "---------------------------";
			_stackLogFile << get_time_string_ms() << "---------------------------\r\n\r\n";
			_stackLogFile << "overflow size: " << size << "\r\n\r\n";
			for (stack_line_info& ele : createStack)
			{
				_stackLogFile << "file: " << ele.file << "\r\n";
				_stackLogFile << "line: " << ele.line << "\r\n";
				_stackLogFile << "module: " << ele.module << "\r\n";
				_stackLogFile << "symbolName: " << ele.symbolName << "\r\n\r\n";
			}
		}, std::move(createStack)));
	}
}

#endif

void install_check_stack()
{
	bool ok = _initialize();
	assert(ok);
#ifdef PRINT_ACTOR_STACK
	auto stk = get_stack_list(1, 0, true, true);
	std::string moduleName;
	if (!stk.empty())
	{
		moduleName = &stk.front().module[stk.front().module.find_last_of('\\') + 1];
	}
	_mkdir((moduleName + " stack_log\\").c_str());
	_stackLogFile.open(moduleName + " stack_log\\" + get_time_string_file_s() + ".log");
	if (_stackLogFile.good())
	{
		_stackLogIos = new boost::asio::io_service;
		_stackLogWork = new boost::asio::io_service::work(*_stackLogIos);
		_thread = new run_thread([&]
		{
			boost::system::error_code ec;
			_stackLogIos->run(ec);
		});
	}
	else
	{
		_stackLogIos = NULL;
		_stackLogWork = NULL;
		_thread = NULL;
	}
#endif
}

void uninstall_check_stack()
{
#ifdef PRINT_ACTOR_STACK
	if (_stackLogIos)
	{
		delete _stackLogWork;
		_thread->join();
		delete _thread;
		delete _stackLogIos;
		_stackLogFile.close();
	}
#endif
}

#elif __linux__
//////////////////////////////////////////////////////////////////////////

void cut_file_line(const char* buff, int& length, int& line)
{
	length = 0;
	line = -1;
	int i = 0;
	int lastColon = -1;
	while (buff[i])
	{
		if (':' == buff[i])
		{
			length = i;
			lastColon = i;
		}
		i++;
	}
	if (-1 != lastColon)
	{
		line = atoi(buff + lastColon + 1);
	}
}

#define ADDR2LINE "addr2line -f -e "

std::list<stack_line_info> get_stack_list(void** traceback, size_t size, bool module, bool symbolName)
{
	std::list<stack_line_info> imageList;
	char cmdHead[256] = ADDR2LINE;
	char *prog = cmdHead + (sizeof(ADDR2LINE)-1);
	const int moduleSize = readlink("/proc/self/exe", prog, sizeof(cmdHead)-(prog - cmdHead) - 1);
	if (moduleSize > 0)
	{
		const int prefixSize = (sizeof(ADDR2LINE)-1) + moduleSize;
		std::string cmd = cmdHead;
		for (size_t i = 0; i < size; i++)
		{
			char tmp[24];
			snprintf(tmp, sizeof(tmp), " %p", traceback[i]);
			cmd.append(tmp);
		}
		FILE *fp = popen(cmd.c_str(), "r");
		if (fp)
		{
			char buff[1024];
			while (true)
			{
				stack_line_info stackResult;
				if (fgets(buff, sizeof(buff), fp))
				{
					if (symbolName)
					{
						stackResult.symbolName = buff;
						if (!stackResult.symbolName.empty())
						{
							stackResult.symbolName.resize(stackResult.symbolName.size()-1);
						}
					}
					if (module)
					{
						stackResult.module = std::string(cmdHead + (sizeof(ADDR2LINE)-1), moduleSize);
					}
					if (fgets(buff, sizeof(buff), fp))
					{
						int length, line;
						cut_file_line(buff, length, line);
						stackResult.line = line;
						stackResult.file = std::string(buff, length);
						imageList.push_back(std::move(stackResult));
					}
					else
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			pclose(fp);
		}
	}
	return imageList;
}

std::list<stack_line_info> get_stack_list(size_t maxDepth, size_t offset, bool module, bool symbolName)
{
	assert(maxDepth && maxDepth <= 32);
	void* traceback[32];
	int depth = backtrace(traceback, maxDepth);
	if (depth > offset)
	{
		return get_stack_list(&traceback[offset], depth - offset, module, symbolName);
	}
	return std::list<stack_line_info>();
}

std::list<stack_line_info> get_stack_list(void* reg_bp, void* reg_sp, void* reg_ip, size_t maxDepth, size_t offset, bool module, bool symbolName)
{
	assert(maxDepth && maxDepth <= 32);
	std::vector<void*> traceback;
#if (__i386__ || __x86_64__)
	void* ip = reg_ip;
	void** bp = (void**)reg_bp;
	while (bp && ip && traceback.size() < 32)
	{
		Dl_info dlinfo;
		if (!dladdr(ip, &dlinfo))
		{
			break;
		}
		traceback.push_back(ip);
		ip = bp[1];
		bp = (void**)bp[0];
	}
#elif (_ARM32 || _ARM64)
	//FIXME
	traceback.push_back(reg_ip);
#endif
	if (traceback.size() > offset)
	{
		return get_stack_list(&traceback[offset], traceback.size() - offset, module, symbolName);
	}
	return std::list<stack_line_info>();
}

void install_check_stack()
{
}

void uninstall_check_stack()
{
}

#endif
#else
void install_check_stack(){}
void uninstall_check_stack(){}
#endif