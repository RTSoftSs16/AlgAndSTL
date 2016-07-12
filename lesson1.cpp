#include <algorithm>
#include <chrono>
#include <memory>
#include <iostream>
#include <random>

using namespace std::chrono;

const int TestSeconds = 2;

typedef unsigned int uint;

typedef std::linear_congruential_engine<uint, 48271, 0, 2147483647> minstd_rand;

void initialize(uint *s, int n, minstd_rand &rand)
{
	for (int i = 0; i < n; ++i)
	{
		s[i] = rand();
	}
}

// find values from s2 which are not in s1 and copy to dif, return the number of values found
int naiveSearchForNew(uint *dif, const uint *s1, const uint *s2, int n)
{
	int difCount = 0;
	for (int is2 = 0; is2 < n; ++is2)
	{
		bool found = false;
		uint findWhat = s2[is2];
		for (int is1 = 0; is1 < n; ++is1)
		{
			if (s1[is1] == findWhat)
			{
				found = true;
				break;
			}
		} // for is1
		if (!found)
		{
			dif[difCount++] = findWhat;
		}
	} // for is2
	return difCount;
}

static void test(int n, minstd_rand &rand)
{
	std::unique_ptr<uint[]> ps1(new uint[n]);
	std::unique_ptr<uint[]> ps2(new uint[n]);
	std::unique_ptr<uint[]> pdif(new uint[n]);
	uint *s1 = ps1.get();
	uint *s2 = ps2.get();
	uint *dif = pdif.get();

	initialize(s1, n, rand);
	std::copy(s1, s1 + n, s2); // VS 2015 generates error C4996 if _SCL_SECURE_NO_WARNINGS not defined
	int checkDifCount = 0;
	for (int j = 2; j < n; j += n / 3)
	{
		s2[j] = (s2[j] < std::numeric_limits<uint>::max()) ? (s2[j] + 1) : 0;
		++checkDifCount;
	}

	int difCount;
	int counter = 0;
	double timePassed;
	steady_clock::time_point start = steady_clock::now();
	do
	{
		difCount = naiveSearchForNew(dif, s1, s2, n);
		if (difCount != checkDifCount)
		{
			std::cerr << "check failed!" << std::endl;
			std::terminate();
		}
		timePassed = duration_cast<duration<double>>(steady_clock::now() - start).count();
		++counter;
	}
	while (timePassed < TestSeconds);
/*
	std::cout << "Values from S2 not in S1: (";
	for (int i = 0; i < difCount; ++i)
	{
		if (i > 0)
		{
			std::cout << " ";
		}
		std::cout << dif[i];
	}
	std::cout << ")" << std::endl;
*/
	std::cout << "Number of searches per second (n=" << n << "): " << counter << std::endl;
}

void lesson1()
{
	minstd_rand rand;
	for (int i = 10; i <= 10000; i *= 10)
	{
		rand.seed(123);
		test(i, rand);
	}
}
