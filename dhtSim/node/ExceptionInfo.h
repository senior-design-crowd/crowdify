#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

class InfoFromSE {
public:
	typedef unsigned int exception_code_t;

	static const char* opDescription(const ULONG opcode);
	static const char* seDescription(const exception_code_t& code);
	static std::string information(struct _EXCEPTION_POINTERS* ep, bool has_exception_code = false, exception_code_t code = 0);
	static void CreateMinidump(struct _EXCEPTION_POINTERS* apExceptionInfo);
};