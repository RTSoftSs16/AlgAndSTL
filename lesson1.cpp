#include <algorithm>
#include <chrono>
#include <memory>
#include <vector>
#include <iostream>
#include <random>

using namespace std::chrono;

using std::vector;

const int TestSeconds = 2;

typedef unsigned int uint;

typedef std::linear_congruential_engine<uint, 48271, 0, 2147483647> minstd_rand;

void initialize(vector<uint> &v, minstd_rand &rand)
{
	for (uint &elem : v)
	{
		elem = rand() % v.size();
	}
}

// find values from s2 which are not in s1 and copy to dif, return the number of values found
size_t naiveSearchForNew(vector<uint> &dif, const vector<uint> &s1, const vector<uint> &s2)
{
	for (uint e2 : s2)
	{
		if (std::find(s1.cbegin(), s1.cend(), e2) == s1.cend())
		{
			dif.push_back(e2);
		}
	} // for is2
	return dif.size();
}

// find values from s2 which are not in s1 and copy to dif, return the number of values found
size_t fasterSearchForNew(vector<uint> &dif, const vector<uint> &_s1, const vector<uint> &_s2)
{
	size_t n2 = _s2.size();
	if (n2 == 0)
	{
		return 0;
	}
	vector<uint> s2(_s2);
	std::sort(s2.begin(), s2.end());
	size_t n1 = _s1.size();
	if (n1 == 0)
	{
		dif = s2;
		return n2;
	}
	vector<uint> s1(_s1);
	std::sort(s1.begin(), s1.end());
	size_t i1 = 0, i2 = 0;
	uint cur1 = s1[0];
	uint cur2 = s2[0];
	bool endOf1 = false;
	bool endOf2 = false;
	bool move1, move2;
	do
	{
		if (cur1 == cur2)
		{
			move1 = !endOf1;
			move2 = !endOf2;
		}
		else
		{
			move1 = false;
			move2 = false;
			if (cur1 < cur2)
			{
				if (!endOf1)
				{
					move1 = true;
				}
				else if (!endOf2) // endOf1 && !endOf2: can copy the rest actually
				{
					dif.push_back(cur2);
					move2 = true;
				}
			}
			else // cur1 > cur2
			{
				if (!endOf2)
				{
					if ((i1 == 0) || (s1[i1 - 1] < cur2))
					{
						dif.push_back(cur2);
					}
					move2 = true;
				}
			}
		}
		if (move1)
		{
			if (++i1 < n1)
			{
				cur1 = s1[i1];
			}
			else
			{
				endOf1 = true;
			}
		}
		if (move2)
		{
			if (++i2 < n2)
			{
				cur2 = s2[i2];
			}
			else
			{
				endOf2 = true;
			}
		}
	}
	while (move1 || move2);
	return dif.size();
}

static void test(int n, minstd_rand &rand)
{
	vector<uint> s1(n);

	initialize(s1, rand);

	vector<uint> s2(s1);

	for (int j = 2; j < n; j += n / 3)
	{
		s2[j] = (s2[j] < std::numeric_limits<uint>::max()) ? (s2[j] + 1) : 0;
	}

	vector<uint> dif;
	dif.reserve(n / 2);

	size_t difCount;
	int counter = 0;
	double timePassed;

	int checkDifCount = naiveSearchForNew(dif, s1, s2);

	steady_clock::time_point start = steady_clock::now();
	do
	{
		dif.clear();
//		difCount = naiveSearchForNew(dif, s1, s2);
		difCount = fasterSearchForNew(dif, s1, s2);
		if (difCount != checkDifCount)
		{
			std::cerr << "check failed!" << std::endl;
			std::terminate();
		}
		timePassed = duration_cast<duration<double>>(steady_clock::now() - start).count();
		++counter;
	}
	while (timePassed < TestSeconds);

	std::cout << "One search time (n=" << n << "): " << timePassed / counter << " sec" << std::endl;
}

void lesson1()
{
	minstd_rand rand;
/*
	rand.seed(123);
	vector<uint> s1 = { 30, 3, 3 };
	vector<uint> s2 = { 2, 3, 6, 7 };
	vector<uint> dif;
	size_t n = fasterSearchForNew(dif, s1, s2);
	std::cout << n << "\n";
*/
	const int sizes[] = { 10, 20, 50, 100, 200, 500, 1000, 2000, 3000, 5000, 10000, 15000, 20000, 25000, 50000 };
	for (int i : sizes)
	{
		rand.seed(123);
		test(i, rand);
	}
}
