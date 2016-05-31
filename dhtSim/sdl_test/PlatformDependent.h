#ifndef __PLATFORM_DEPENDENT_H__
#define __PLATFORM_DEPENDENT_H__

#include <fstream>

bool BeingDebugged();
void SetExceptionHooks(int rootRank, std::ofstream* a_pFp);
bool RunProc(const char* processPath, const char* cmdLine, std::ofstream& fp);

#endif