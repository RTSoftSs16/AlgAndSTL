#define USE_AVX
//#define USE_AVX512 // VS 2015 doesn't support AVX-512

#include <memory>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <cmath>

#ifdef USE_AVX
#include <immintrin.h>
#include <malloc.h>
#endif

using namespace std::chrono;

const int TestSeconds = 2;

bool useAVX = false;
bool HW_AVX512F = false;

void initMatrix(double *a, int numRows, int numColumns, double startValue, double step)
{
	int idx = 0;
	for (int i = 0; i < numRows; ++i)
	{
		for (int j = 0; j < numColumns; ++j)
		{
			a[idx++] = startValue;
			startValue += step;
		}
	}
}

void checkMatrix(const double *a, int numRows, int numColumns, double value)
{
	int idx = 0;
	for (int i = 0; i < numRows; ++i)
	{
		for (int j = 0; j < numColumns; ++j)
		{
			if (abs(a[idx++] - value) > 1E-9)
			{
				std::cerr << "checkMatrix failed!" << std::endl;
				std::terminate();
			}
		}
	}
}

void addMatricesByRows(double *sum, const double *a, const double *b,
	int numRows, int numColumns) // sum[,] = a[,] + b[,]
{
	int idx = 0;
	for (int i = 0; i < numRows; ++i)
	{
		for (int j = 0; j < numColumns; ++j)
		{
			sum[idx] = a[idx] + b[idx];
			++idx;
		}
	}
}

void addMatricesByColumns(double *sum, const double *a, const double *b,
	int numRows, int numColumns) // sum[,] = a[,] + b[,]
{
	for (int j = 0; j < numColumns; ++j)
	{
		int idx = j;
		for (int i = 0; i < numRows; ++i)
		{
			sum[idx] = a[idx] + b[idx];
			idx += numColumns;
		}
	}
}

#ifdef USE_AVX
// http://stackoverflow.com/questions/27204712/entrywise-addition-of-two-double-arrays-using-avx
void addMatricesAVX(double* sum, const double* a, const double* b, int numRows, int numColumns)
{
	int size = numRows * numColumns;
	int i = 0;

#ifdef USE_AVX512
	if (HW_AVX512F)
	{
		// Note we are doing as many blocks of 8 as we can.  If the size is not divisible by 8
		// then we will have some left over that will then be performed serially.
		// AVX-512 loop
		for (int count = (size & ~0x7); i < count; i += 8)
		{
			const __m512d kA8 = _mm512_load_pd(&a[i]);
			const __m512d kB8 = _mm512_load_pd(&b[i]);

			const __m512d kRes = _mm512_add_pd(kA8, kB8);
			_mm512_stream_pd(&sum[i], kRes);
		}
	}
#endif

	// AVX loop
	for (int count = (size & ~0x3); i < count; i += 4)
	{
		const __m256d kA4 = _mm256_load_pd(&a[i]);
		const __m256d kB4 = _mm256_load_pd(&b[i]);

		const __m256d kRes = _mm256_add_pd(kA4, kB4);
		_mm256_stream_pd(&sum[i], kRes);
	}

/* don't mix AVX/SSE2 (performance penalty on switching)
	// SSE2 loop
	for (int count = (size & ~0x1); i < count; i += 2)
	{
		const __m128d kA2 = _mm_load_pd(&a[i]);
		const __m128d kB2 = _mm_load_pd(&b[i]);

		const __m128d kRes = _mm_add_pd(kA2, kB2);
		_mm_stream_pd(&sum[i], kRes);
	}
*/

	// the rest
	for (; i < size; i++)
	{
		sum[i] = a[i] + b[i];
	}
}
#endif

int runTest(double *sum, const double *a, const double *b, int numRows, int numColumns, int numSeries,
	void addFunc(double *, const double *, const double *, int numRows, int numColumns))
{
	int counter = 0;
	double timePassed;
	steady_clock::time_point start = steady_clock::now();
	do
	{
		for (int i = 0; i < numSeries; ++i)
		{
			addFunc(sum, a, b, numRows, numColumns);
		}
		timePassed = duration_cast<duration<double>>(steady_clock::now() - start).count();
		counter += numSeries;
	}
	while (timePassed < TestSeconds);
	return (int)(counter / timePassed);
}

int testAddMatricesByRows(double *sum, const double *a, const double *b, int numRows, int numColumns, int numSeries)
{
	return runTest(sum, a, b, numRows, numColumns, numSeries, addMatricesByRows);
}

int testAddMatricesByColumns(double *sum, const double *a, const double *b, int numRows, int numColumns, int numSeries)
{
	return runTest(sum, a, b, numRows, numColumns, numSeries, addMatricesByColumns);
}

#ifdef USE_AVX
int testAddMatricesAVX(double *sum, const double *a, const double *b, int numRows, int numColumns, int numSeries)
{
	return runTest(sum, a, b, numRows, numColumns, numSeries, addMatricesAVX);
}
#endif

inline double* allocateDoubleVector(int count) // allocates 64-byte aligned array (AVX need 32-byte aligned data)
{
	return static_cast<double*>(_aligned_malloc(count * sizeof(double), 64));
}

inline void freeDoubleVector(double* v)
{
	_aligned_free(v);
}

void testMatrix(int numRows, int numColumns, int numSeries)
{
	const double firstValue = 0.0;
	const double step = 0.5;
	double lastValue = firstValue + (numRows * numColumns - 1) * step;

	int numElements = numRows * numColumns;
	std::cout << "Matrices: " << numRows << 'x' << numColumns << " ("
		<< numElements * sizeof(double) << " bytes)" << std::endl;

	// create and initialize source matrices
	double *a = allocateDoubleVector(numRows * numColumns);
	double *b = allocateDoubleVector(numRows * numColumns);
	initMatrix(a, numRows, numColumns, firstValue, step);
	initMatrix(b, numRows, numColumns, lastValue, -step);

	// create sum matrix
	double *sum = allocateDoubleVector(numRows * numColumns);

	// check addMatricesByRows is Ok
	initMatrix(sum, numRows, numColumns, 0.0, 0.0); // zero sum
	addMatricesByRows(sum, a, b, numRows, numColumns);
	checkMatrix(sum, numRows, numColumns, firstValue + lastValue);

#ifdef USE_AVX
	if (useAVX)
	{
		// check addMatricesAVX is Ok
		initMatrix(sum, numRows, numColumns, 0.0, 0.0);
		addMatricesAVX(sum, a, b, numRows, numColumns);
		checkMatrix(sum, numRows, numColumns, firstValue + lastValue);
	}
#endif

	// check addMatricesByColumns is Ok
	initMatrix(sum, numRows, numColumns, 0.0, 0.0);
	addMatricesByColumns(sum, a, b, numRows, numColumns);
	checkMatrix(sum, numRows, numColumns, firstValue + lastValue);

	// run tests

#ifdef USE_AVX
	int avx;
	if (useAVX)
	{
		avx = testAddMatricesAVX(sum, a, b, numRows, numColumns, numSeries);
		std::cout << "Number of matrix additions (AVX) per second: " << avx << std::endl;
		std::cout << "Number of individual additions (AVX) per second: "
			<< (int64_t)avx * numElements / 1000000 << " (x 1E6)" << std::endl;
	}
#endif
	int byRows = testAddMatricesByRows(sum, a, b, numRows, numColumns, numSeries);
	std::cout << "Number of matrix additions (row major) per second: " << byRows << std::endl;
	std::cout << "Number of individual additions (row major) per second: "
		<< (int64_t)byRows * numElements / 1000000 << " (x 1E6)" << std::endl;

	int byColumns = testAddMatricesByColumns(sum, a, b, numRows, numColumns, numSeries);
	std::cout << "Number of matrix additions (column major) per second: " << byColumns << std::endl;
	std::cout << "Number of individual additions (column major) per second: "
		<< (int64_t)byColumns * numElements / 1000000 << " (x 1E6)" << std::endl;

	freeDoubleVector(a);
	freeDoubleVector(b);
	freeDoubleVector(sum);

#ifdef USE_AVX
	if (useAVX)
	{
		std::cout << "Ratio AVX / row major: " << std::fixed << std::setprecision(1)
			<< (double)avx / byRows << std::endl;
	}
#endif
	std::cout << "Ratio row major / column major: " << std::fixed << std::setprecision(1)
		<< (double)byRows / byColumns << std::endl;

	std::cout << std::endl;
}

// http://stackoverflow.com/questions/6121792/how-to-check-if-a-cpu-supports-the-sse3-instruction-set
bool checkAVX(bool &HW_AVX, bool &HW_AVX512F)
{
	int info[4];

	__cpuid(info, 0);
	int nIds = info[0];
	if (nIds == 0)
	{
		return false;
	}

	bool result = false;

	__cpuid(info, 1);
	bool osUsesXSAVE_XRSTORE = (info[2] & ((int)1 << 27)) != 0;

	if (osUsesXSAVE_XRSTORE)
	{
		HW_AVX = ((info[2] & ((int)1 << 28)) != 0);
		HW_AVX512F = (info[1] & ((int)1 << 16)) != 0;
		if (HW_AVX)
		{
			unsigned long long xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
			result = ((xcrFeatureMask & 0x6) == 0x6);
		}
	}

	return result;
}

void testMatrix()
{
#ifdef _DEBUG
	std::cout << "Warning: debug build. Benchmark results are not sensible (use Release please)" << std::endl;
#endif
#ifdef USE_AVX
	bool HW_AVX;
	useAVX = checkAVX(HW_AVX, HW_AVX512F);
	if (HW_AVX)
	{
		std::cout << "CPU supports AVX-" << (HW_AVX512F ? "512" : "256") << std::endl;
		if (useAVX)
		{
#ifndef USE_AVX512
			HW_AVX512F = false;
#endif
			std::cout << "Using AVX-" << (HW_AVX512F ? "512" : "256") << std::endl;
		}
	}
	else
	{
		std::cout << "CPU doesn't support AVX" << std::endl;
	}
#endif
	testMatrix(2, 2, 200);
	testMatrix(4, 4, 150);
	testMatrix(8, 8, 100);
	testMatrix(10, 10, 50);
	testMatrix(20, 20, 25);
	testMatrix(25, 35, 20);
	testMatrix(50, 70, 10);
	testMatrix(75, 105, 10);
	testMatrix(100, 140, 5);
	testMatrix(200, 280, 1);
	testMatrix(300, 420, 1);
	testMatrix(400, 560, 1);
	testMatrix(500, 700, 1);
	testMatrix(750, 1050, 1);
	testMatrix(1000, 1400, 1);
	testMatrix(2000, 2800, 1);
}
