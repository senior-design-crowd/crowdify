#ifndef __HASH_EVAL_H__
#define __HASH_EVAL_H__

#include <cstddef>

class HashEval
{
public:
	// default hash length of 32 bytes = 256 bits
	HashEval(int hashLen = 32);

	bool GetHashPoint(const unsigned char* buf, size_t len, double* r_x, double* r_y);
private:
	size_t	m_hashLen;
	double	m_maxNumerator;
};

#endif