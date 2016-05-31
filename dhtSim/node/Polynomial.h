#ifndef __POLYNOMIAL_H__
#define __POLYNOMIAL_H__

#include <algorithm>
#include <utility>
#include <vector>
#include <iostream>

template <typename T, typename EvalType = T>
class Polynomial {
public:
	Polynomial();
	Polynomial(const T* coeffs, size_t numCoeffs);
	Polynomial(const std::vector<T>& coeffs);
	Polynomial(const Polynomial<T, EvalType>& poly);
	Polynomial(Polynomial<T, EvalType>&& poly);
	Polynomial(std::initializer_list<T> l);

	Polynomial<T, EvalType> operator=(const Polynomial<T, EvalType>& poly);

	EvalType	eval(const EvalType& x) const;
	int			degree() const;

	Polynomial<T, EvalType> operator-() const;

	Polynomial<T, EvalType> operator+(const Polynomial<T, EvalType>& b) const;
	Polynomial<T, EvalType> operator-(const Polynomial<T, EvalType>& b) const;
	Polynomial<T, EvalType> operator*(const Polynomial<T, EvalType>& b) const;
	Polynomial<T, EvalType> operator/(const Polynomial<T, EvalType>& b) const;

	Polynomial<T, EvalType> operator*(const T& c) const;
	Polynomial<T, EvalType> operator/(const T& c) const;

	bool operator<(const Polynomial<T, EvalType>& b) const;
	bool operator>(const Polynomial<T, EvalType>& b) const;
	bool operator<=(const Polynomial<T, EvalType>& b) const;
	bool operator>=(const Polynomial<T, EvalType>& b) const;
	bool operator==(const Polynomial<T, EvalType>& b) const;
	bool operator!=(const Polynomial<T, EvalType>& b) const;

	template <typename T, typename EvalType>
	friend std::ostream& operator<<(std::ostream& ostr, const Polynomial<T, EvalType>& poly);
private:
	void removeLeadingZeroes();

	std::vector<T>	m_vCoeffs;
};

template <typename T, typename EvalType>
Polynomial<T, EvalType>::Polynomial()
{
}

template <typename T, typename EvalType>
Polynomial<T, EvalType>::Polynomial(const T* coeffs, size_t numCoeffs)
	: m_vCoeffs(coeffs, coeffs + numCoeffs)
{
	removeLeadingZeroes();
}

template <typename T, typename EvalType>
Polynomial<T, EvalType>::Polynomial(const std::vector<T>& coeffs)
	: m_vCoeffs(coeffs)
{
	removeLeadingZeroes();
}

template <typename T, typename EvalType>
Polynomial<T, EvalType>::Polynomial(const Polynomial<T, EvalType>& poly)
	: m_vCoeffs(poly.m_vCoeffs)
{
	removeLeadingZeroes();
}

template <typename T, typename EvalType>
Polynomial<T, EvalType>::Polynomial(Polynomial<T, EvalType>&& poly)
	: m_vCoeffs(std::move(poly.m_vCoeffs))
{
	removeLeadingZeroes();
}

template <typename T, typename EvalType>
Polynomial<T, EvalType>::Polynomial(std::initializer_list<T> l)
	: m_vCoeffs(l)
{
	removeLeadingZeroes();
}

template <typename T, typename EvalType>
Polynomial<T, EvalType> Polynomial<T, EvalType>::operator=(const Polynomial<T, EvalType>& poly)
{
	m_vCoeffs = poly.m_vCoeffs;
	removeLeadingZeroes();
	return *this;
}

template <typename T, typename EvalType>
EvalType Polynomial<T, EvalType>::eval(const EvalType& x) const
{
	if (m_vCoeffs.size() < 1) {
		return T();
	}

	EvalType result = *m_vCoeffs.rbegin();
	for (auto it = m_vCoeffs.rbegin() + 1; it != m_vCoeffs.rend(); ++it) {
		result = result * x + *it;
	}

	return result;
}

template <typename T, typename EvalType>
Polynomial<T, EvalType> Polynomial<T, EvalType>::operator+(const Polynomial<T, EvalType>& b) const
{
	size_t newPolyDegree = m_vCoeffs.size() > b.m_vCoeffs.size() ? m_vCoeffs.size() : b.m_vCoeffs.size();
	std::vector<T> newCoeffs(newPolyDegree, T());

	for (size_t i = 0; i < newPolyDegree; ++i) {
		if (i < m_vCoeffs.size()) {
			newCoeffs[i] += m_vCoeffs[i];
		}
		if (i < b.m_vCoeffs.size()) {
			newCoeffs[i] += b.m_vCoeffs[i];
		}
	}

	return Polynomial<T, EvalType>(newCoeffs);
}

template <typename T, typename EvalType>
Polynomial<T, EvalType> Polynomial<T, EvalType>::operator-(const Polynomial<T, EvalType>& b) const
{
	return *this + (-b);
}

template <typename T, typename EvalType>
Polynomial<T, EvalType> Polynomial<T, EvalType>::operator-() const
{
	Polynomial<T, EvalType> poly(*this);

	for (auto it = poly.m_vCoeffs.begin(); it != poly.m_vCoeffs.end(); ++it) {
		*it = -*it;
	}

	return poly;
}

// I know FFT - O(n log n) is more efficient than this - O(n^2),
// but I'm trading speed for simplicity. also I don't expect
// large degree polynomials or much polynomial computation to
// occur at all.
template <typename T, typename EvalType>
Polynomial<T, EvalType> Polynomial<T, EvalType>::operator*(const Polynomial<T, EvalType>& b) const
{
	size_t newPolyDegree = m_vCoeffs.size() + b.m_vCoeffs.size() - 1;
	std::vector<T> newCoeffs(newPolyDegree, T());

	for (size_t i = 0; i < m_vCoeffs.size(); ++i) {
		for (size_t j = 0; j < b.m_vCoeffs.size(); ++j) {
			newCoeffs[i + j] += m_vCoeffs[i] * b.m_vCoeffs[j];
		}
	}

	return Polynomial<T, EvalType>(newCoeffs);
}

template <typename T, typename EvalType>
Polynomial<T, EvalType> Polynomial<T, EvalType>::operator/(const Polynomial<T, EvalType>& b) const
{
	if (m_vCoeffs.size() < b.m_vCoeffs.size()) {
		return Polynomial<T, EvalType>{T()};
	}

	// perform polynomial long division

	size_t newPolyDegree = m_vCoeffs.size() - b.m_vCoeffs.size() + 1;
	std::vector<T> newCoeffs(newPolyDegree, T());

	// we use the std::transform function to perform operations
	// on the entire polynomial with a single line of code in an
	// efficient manner, and std::transform doesn't work with
	// overlapping but unequal input and output ranges. therefore
	// we need an extra result vector to store the result of step 3.
	std::vector<T>	resultVecs[2];
	resultVecs[0].assign(m_vCoeffs.begin() + newPolyDegree - 1, m_vCoeffs.end());
	resultVecs[1].assign(b.m_vCoeffs.size(), 0);

	// but we switch back and forth between which vector
	// contains the temporary result of the subtraction
	// in step 3 and which contains the current remainder
	// of the last iteration. this way we avoid
	// an extra vector copy.
	std::vector<T>* currentRemainder = &resultVecs[0];
	std::vector<T>* currentResult = &resultVecs[1];

	std::vector<T>	currDivisorMultiple;
	currDivisorMultiple.assign(b.m_vCoeffs.size(), 0);

	for (size_t i = 0; i < newPolyDegree; ++i) {
		// 1. divide the leading coefficients
		T div = *currentRemainder->rbegin() / *b.m_vCoeffs.rbegin();
		newCoeffs[newCoeffs.size() - 1 - i] = div;

		// 2. multiply out the divisor by the result
		std::transform(b.m_vCoeffs.begin(), b.m_vCoeffs.end(),
			currDivisorMultiple.begin(), [div](T a) { return a * div; });

		// 3. subtract the result to get the next remainder
		std::transform(currentRemainder->begin(), currentRemainder->end() - 1,
			currDivisorMultiple.begin(), currentResult->begin() + 1, std::minus<T>());

		// 4. drop down the next term
		if (i + 1 < newPolyDegree) {
			*currentResult->begin() = m_vCoeffs[m_vCoeffs.size() - 1 - b.m_vCoeffs.size() - i];
		}

		// 5. swap the two vectors for the next iteration
		std::vector<T>* tmpSwap = currentRemainder;
		currentRemainder = currentResult;
		currentResult = tmpSwap;
	}

	return Polynomial<T, EvalType>(newCoeffs);
}

template <typename T, typename EvalType>
Polynomial<T, EvalType> Polynomial<T, EvalType>::operator*(const T& c) const
{
	Polynomial<T, EvalType> poly = *this;
	for (auto it = poly.m_vCoeffs.begin(); it != poly.m_vCoeffs.end(); ++it) {
		*it *= c;
	}

	return poly;
}

template <typename T, typename EvalType>
Polynomial<T, EvalType> Polynomial<T, EvalType>::operator/(const T& c) const
{
	Polynomial<T, EvalType> poly = *this;
	for (auto it = poly.m_vCoeffs.begin(); it != poly.m_vCoeffs.end(); ++it) {
		*it /= c;
	}

	return poly;
}

template <typename T, typename EvalType>
Polynomial<T, EvalType> operator*(const T& c, const Polynomial<T, EvalType>& b)
{
	return b * c;
}

template <typename T, typename EvalType>
bool Polynomial<T, EvalType>::operator<(const Polynomial<T, EvalType>& b) const
{
	Polynomial<T, EvalType> difference = *this - b;
	if (*difference.m_vCoeffs.rbegin() < 0) {
		return true;
	} else {
		return false;
	}
}

template <typename T, typename EvalType>
bool Polynomial<T, EvalType>::operator>(const Polynomial<T, EvalType>& b) const
{
	return !(*this <= b);
}

template <typename T, typename EvalType>
bool Polynomial<T, EvalType>::operator<=(const Polynomial<T, EvalType>& b) const
{
	return *this == b || *this < b;
}

template <typename T, typename EvalType>
bool Polynomial<T, EvalType>::operator>=(const Polynomial<T, EvalType>& b) const
{
	return !(*this < b);
}

template <typename T, typename EvalType>
bool Polynomial<T, EvalType>::operator==(const Polynomial<T, EvalType>& b) const
{
	return m_vCoeffs == b.m_vCoeffs;
}

template <typename T, typename EvalType>
bool Polynomial<T, EvalType>::operator!=(const Polynomial<T, EvalType>& b) const
{
	return !(*this == b);
}

template <typename T, typename EvalType>
void Polynomial<T, EvalType>::removeLeadingZeroes()
{
	while (m_vCoeffs.size() > 0 && *m_vCoeffs.rbegin() == 0) {
		m_vCoeffs.pop_back();
	}
}

template <typename T, typename EvalType>
int Polynomial<T, EvalType>::degree() const
{
	return m_vCoeffs.size() - 1;
}

template <typename T, typename EvalType>
std::ostream& operator<<(std::ostream& ostr, const Polynomial<T, EvalType>& poly)
{
	size_t currPower = poly.m_vCoeffs.size() - 1;
	for (auto it = poly.m_vCoeffs.rbegin(); it != poly.m_vCoeffs.rend(); ++it, --currPower) {
		// don't output zero coefficients
		if (*it) {
			if (currPower < poly.m_vCoeffs.size() - 1) {
				if (*it >= T()) {
					ostr << "+ ";
				}
			}

			if (*it >= T()) {
				ostr << *it;
			} else if (*it < T()) {
				ostr << "- " << -*it;
			}

			if (currPower > 0) {
				ostr << " x^" << currPower;
			}

			ostr << " ";
		}
	}

	return ostr;
}

#endif