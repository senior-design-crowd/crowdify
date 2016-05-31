#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <dbghelp.h>
#include <eh.h>
#include <Psapi.h>

#else

#include <errno.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <signal.h>

struct sigaction oldSIGSEGV, oldSIGCHLD;

#endif

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

ofstream* pFp;

#include "nodeMessages.h"

#include "PlatformDependent.h"

namespace PlatformDependent {
	int rootRank;
}

#ifdef _WIN32

typedef unsigned int exception_code_t;

const char* opDescription(const ULONG opcode)
{
	switch (opcode) {
	case 0: return "read";
	case 1: return "write";
	case 8: return "user-mode data execution prevention (DEP) violation";
	default: return "unknown";
	}
}

const char* seDescription(const exception_code_t& code)
{
	switch (code) {
	case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
	case EXCEPTION_BREAKPOINT:               return "EXCEPTION_BREAKPOINT";
	case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT";
	case EXCEPTION_FLT_DENORMAL_OPERAND:     return "EXCEPTION_FLT_DENORMAL_OPERAND";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
	case EXCEPTION_FLT_INEXACT_RESULT:       return "EXCEPTION_FLT_INEXACT_RESULT";
	case EXCEPTION_FLT_INVALID_OPERATION:    return "EXCEPTION_FLT_INVALID_OPERATION";
	case EXCEPTION_FLT_OVERFLOW:             return "EXCEPTION_FLT_OVERFLOW";
	case EXCEPTION_FLT_STACK_CHECK:          return "EXCEPTION_FLT_STACK_CHECK";
	case EXCEPTION_FLT_UNDERFLOW:            return "EXCEPTION_FLT_UNDERFLOW";
	case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION";
	case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO";
	case EXCEPTION_INT_OVERFLOW:             return "EXCEPTION_INT_OVERFLOW";
	case EXCEPTION_INVALID_DISPOSITION:      return "EXCEPTION_INVALID_DISPOSITION";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
	case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION";
	case EXCEPTION_SINGLE_STEP:              return "EXCEPTION_SINGLE_STEP";
	case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
	default: return "UNKNOWN EXCEPTION";
	}
}

std::string exceptionInfo(struct _EXCEPTION_POINTERS* ep, bool has_exception_code = false, exception_code_t code = 0)
{
	HMODULE hm;
	::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, static_cast<LPCTSTR>(ep->ExceptionRecord->ExceptionAddress), &hm);
	MODULEINFO mi;
	::GetModuleInformation(::GetCurrentProcess(), hm, &mi, sizeof(mi));
	char fn[MAX_PATH];
	::GetModuleFileNameExA(::GetCurrentProcess(), hm, fn, MAX_PATH);

	std::ostringstream oss;
	oss << "SE " << (has_exception_code ? seDescription(code) : "") << " at address 0x" << std::hex << ep->ExceptionRecord->ExceptionAddress << std::dec
		<< " inside " << fn << " loaded at base address 0x" << std::hex << mi.lpBaseOfDll << "\n";

	if (has_exception_code && (
		code == EXCEPTION_ACCESS_VIOLATION ||
		code == EXCEPTION_IN_PAGE_ERROR)) {
		oss << "Invalid operation: " << opDescription(ep->ExceptionRecord->ExceptionInformation[0]) << " at address 0x" << std::hex << ep->ExceptionRecord->ExceptionInformation[1] << std::dec << "\n";
	}

	if (has_exception_code && code == EXCEPTION_IN_PAGE_ERROR) {
		oss << "Underlying NTSTATUS code that resulted in the exception " << ep->ExceptionRecord->ExceptionInformation[2] << "\n";
	}

	return oss.str();
}

void CreateMinidump(struct _EXCEPTION_POINTERS* apExceptionInfo)
{
	typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

	HMODULE mhLib = ::LoadLibrary("dbghelp.dll");
	MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress(mhLib, "MiniDumpWriteDump");

	HANDLE  hFile = ::CreateFile("node_core.dmp", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);

	_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
	ExInfo.ThreadId = ::GetCurrentThreadId();
	ExInfo.ExceptionPointers = apExceptionInfo;
	ExInfo.ClientPointers = FALSE;

	pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithFullMemory/*MiniDumpNormal*/, &ExInfo, NULL, NULL);
	::CloseHandle(hFile);
}

int YourReportHook(int reportType, char *message, int *returnValue)	
{
	std::string str = message;
	int			lineNum = -5;
	bool		before = false;

	MPI_Send((void*)str.c_str(), str.length(), MPI_CHAR, PlatformDependent::rootRank, NodeToRootMessageTags::VECTOR_OPERATION, MPI_COMM_WORLD);
	MPI_Send((void*)&lineNum, 1, MPI_INT, PlatformDependent::rootRank, NodeToRootMessageTags::VECTOR_OPERATION, MPI_COMM_WORLD);
	MPI_Send((void*)&before, sizeof(bool), MPI_CHAR, PlatformDependent::rootRank, NodeToRootMessageTags::VECTOR_OPERATION, MPI_COMM_WORLD);

	return 0;
}

LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
	string exceptInfo = exceptionInfo(ExceptionInfo);
	CreateMinidump(ExceptionInfo);

	int tmp;
	YourReportHook(0, (char*)exceptInfo.c_str(), &tmp);

	return EXCEPTION_CONTINUE_SEARCH;
}
#else
void signalHandler(int sig, siginfo_t* sigInfo, void* ucontext)
{
  ofstream& fp = *pFp;
  struct sigaction* oldSigHandler;
  stringstream ss;
  
  fp << "In sig handler." << endl;
  fp.flush();
  
  if(sig == SIGSEGV) {
    oldSigHandler = &oldSIGSEGV;
    ss << "Caught SIGSEGV with code: ";
    
    if(sigInfo->si_code == SEGV_MAPERR) {
      ss << "Address not mapped to object." << endl;
    } else if(sigInfo->si_code == SEGV_ACCERR) {
      ss << "Invalid permissions for mapped object." << endl;
    } else {
      ss << "Unknown." << endl;
    }
  } else if(sig == SIGCHLD) {
    oldSigHandler = &oldSIGCHLD;
    ss << "Caught SIGCHLD for child process " << sigInfo->si_pid << "." << endl;
    
    if(sigInfo->si_code == CLD_EXITED) {
      ss << "Child process exited with code " << sigInfo->si_status << "." << endl;
    } else if(sigInfo->si_code == CLD_KILLED) {
      ss << "Child process was killed with signal " << sigInfo->si_status << "." << endl;
    } else if(sigInfo->si_code == CLD_DUMPED) {
      ss << "Child process was terminated abnormally with signal " << sigInfo->si_status << "." << endl;
    } else if(sigInfo->si_code == CLD_TRAPPED) {
      ss << "Child process hit breakpoint with signal " << sigInfo->si_status << "." << endl;
    } else if(sigInfo->si_code == CLD_STOPPED) {
      ss << "Child process was stopped with signal " << sigInfo->si_status << "." << endl;
    } else if(sigInfo->si_code == CLD_CONTINUED) {
      ss << "Child process was continued with signal " << sigInfo->si_status << "." << endl;
    }
  }
  
  fp << "Formed message." << endl << endl;
  fp << ss.str();
  fp.flush();
  
  string str = ss.str();
  int lineNum = -5;
  bool before = false;
  
  MPI_Send((void*)str.c_str(), str.length(), MPI_CHAR, PlatformDependent::rootRank, NodeToRootMessageTags::VECTOR_OPERATION, MPI_COMM_WORLD);
  MPI_Send((void*)&lineNum, 1, MPI_INT, PlatformDependent::rootRank, NodeToRootMessageTags::VECTOR_OPERATION, MPI_COMM_WORLD);
  MPI_Send((void*)&before, sizeof(bool), MPI_CHAR, PlatformDependent::rootRank, NodeToRootMessageTags::VECTOR_OPERATION, MPI_COMM_WORLD);
  
  fp << "Sent MPI message to " << PlatformDependent::rootRank << "." << endl;
  fp.flush();
  
  struct sigaction sigact;
  memset((void*)&sigact, 0, sizeof(struct sigaction));
  
  sigact.sa_sigaction = signalHandler;
  sigact.sa_flags = SA_SIGINFO;
  
  if(oldSigHandler->sa_flags & SA_SIGINFO) {
    fp << "Calling advanced sig handler: 0x" << hex << oldSigHandler->sa_sigaction << endl;
    fp.flush();
    
    if((unsigned)oldSigHandler->sa_sigaction > 1) {
      (*oldSigHandler->sa_sigaction)(sig, sigInfo, ucontext);
    } else {
      fp << "Calling default sig handler." << endl;
      fp.flush();
      signal(sig, SIG_DFL);
      raise(sig);
      sigaction(SIGSEGV, &sigact, NULL);
    }
  } else {
    fp << "Calling sig handler: 0x" << hex << oldSigHandler->sa_handler << endl;
    fp.flush();
    
    if((unsigned)oldSigHandler->sa_handler > 1) {
      (*oldSigHandler->sa_handler)(sig);
    } else {
      fp << "Calling default sig handler." << endl;
      fp.flush();
      signal(sig, SIG_DFL);
      raise(sig);
      sigaction(SIGSEGV, &sigact, NULL);
    }
  }
}
#endif

bool BeingDebugged()
{
#ifdef _WIN32
	return IsDebuggerPresent() == TRUE;
#else
	return ptrace(PTRACE_TRACEME, 0, NULL, 0) == -1;
#endif
}

void SetExceptionHooks(int rootRank, ofstream* a_pFp)
{
	pFp = a_pFp;
	ofstream& fp = *pFp;
	
	PlatformDependent::rootRank = rootRank;

#ifdef _WIN32

	_CrtSetReportHook(YourReportHook);
	AddVectoredExceptionHandler(1, VectoredHandler);
#else
	fp << "Setting signal handlers." << endl;
	
	struct sigaction sigact;
	
	memset((void*)&sigact, 0, sizeof(struct sigaction));
	memset((void*)&oldSIGSEGV, 0, sizeof(struct sigaction));
	memset((void*)&oldSIGCHLD, 0, sizeof(struct sigaction));
	
	sigact.sa_sigaction = signalHandler;
	sigact.sa_flags = SA_SIGINFO;
	
	// oldSIGSEGV, oldSIGCHLD
	fp << "Setting SIGSEGV handler." << endl;
	if(sigaction(SIGSEGV, &sigact, &oldSIGSEGV) < 0) {
	  fprintf(stderr, "Failed to register SIGSEGV handler.\n");
	}
	fp << "Setting SIGCHLD handler." << endl;
	if(sigaction(SIGCHLD, &sigact, &oldSIGCHLD) < 0) {
	  fprintf(stderr, "Failed to register SIGCHLD handler.\n");
	}
#endif

}

bool RunProc(const char* processPath, const char* cmdLine, ofstream& fp)
{
	bool success = true;

#ifdef _WIN32
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	BOOL ret = CreateProcess(NULL, const_cast<char*>(cmdLine), NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi);
	fp << "\tProcess created, return code: " << ret << endl;

	if (!ret) {
		success = false;

		char* pBuffer = 0;
		DWORD err = GetLastError();

		DWORD msgRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			err,
			0,
			(char*)&pBuffer,
			0,
			NULL);

		if (msgRet && pBuffer) {
			fp << "\tError (" << err << "): " << pBuffer << endl;
			LocalFree(pBuffer);
		} else {
			fp << "\tError (" << err << ") that couldnt be processed." << endl;
		}
	}
#else
	switch(fork())
	{
	case 0:
		if (execlp(processPath, cmdLine) == -1) {
			fp << "\tFailed to call execute process, errorno: " << errno << endl;
		}
		break;
	case -1:
		success = false;
		fp << "\tFailed to fork process, errorno: " << errno << endl;
		break;
	default:
		break;
	}
#endif

	return success;
}