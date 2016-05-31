#include <cstddef>
#include <limits.h>
#include "HashEval.h"

HashEval::HashEval(int hashLen)
	: m_hashLen(hashLen)
{
	// must be able to split the hash into
	// integer-sized chunks and the number of
	// ints in the hash should be even so it
	// can be split into one half for the x coordinate
	// and another half for the y coordinate
	if (m_hashLen % 8 != 0) {
		throw;
	}

	double maxInt = (double)UINT_MAX;
	m_maxNumerator = maxInt;

	int numIntChunksPerCoord = m_hashLen / 8;

	for (int i = 0; i < numIntChunksPerCoord - 1; ++i) {
		m_maxNumerator = maxInt + m_maxNumerator * 256.0;
	}
}

bool HashEval::GetHashPoint(const unsigned char* buf, size_t len, double* r_x, double* r_y)
{
	if (len != m_hashLen) {
		return false;
	}

	double* coords[] = { r_x, r_y };
	const unsigned int* hashInts = reinterpret_cast<const unsigned int*>(buf);

	int numIntChunksPerCoord = m_hashLen / 8;

	for (int i = 0, hashIntIndex = 0; i < 2; ++i) {
		*coords[i] = hashInts[hashIntIndex];
		++hashIntIndex;

		for (int j = 0; j < numIntChunksPerCoord - 1; ++j, ++hashIntIndex) {
			*coords[i] = hashInts[hashIntIndex] + *coords[i] * 256.0;
		}

		*coords[i] /= m_maxNumerator;
	}

	return true;
}