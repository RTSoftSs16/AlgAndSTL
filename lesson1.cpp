#include <algorithm>
#include <chrono>
#include <iterator>
#include <iostream>
#include <memory>
#include <random>
#include <unordered_set>
#include <vector>

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
	}
	return dif.size();
}

// find values from s2 which are not in s1 and copy to dif, return the number of values found
size_t sortAndCompareSearchForNew(vector<uint> &dif, const vector<uint> &_s1, const vector<uint> &_s2)
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
				else if (!endOf2) // endOf1 && !endOf2: can copy the rest of s2 and stop
				{
					std::copy(s2.begin() + i2, s2.end(), std::back_inserter(dif));
					break;
				}
			}
			else // cur1 > cur2
			{
				if (endOf2)
				{
					break;
				}
				if ((i1 == 0) || (s1[i1 - 1] < cur2))
				{
					dif.push_back(cur2);
				}
				move2 = true;
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

// find values from s2 which are not in s1 and copy to dif, return the number of values found
size_t hashTableSearchForNew(vector<uint> &dif, const vector<uint> &s1, const vector<uint> &s2)
{
	std::unordered_set<uint> set1(s1.cbegin(), s1.cend());
/*	for (uint elem : s2)
	{
		if (set1.find(elem) == set1.cend())
		{
			dif.push_back(elem);
		}
	}*/
	std::copy_if(s2.cbegin(), s2.cend(), std::back_inserter(dif),
		[&set1](uint s2elem) { return (set1.find(s2elem) == set1.cend()); });
	return dif.size();
}

// returns actual time passed and searchFunc run counter
std::pair<double, size_t> test(
	vector<uint> &dif, const vector<uint> &s1, const vector<uint> &s2, size_t checkDifCount,
	size_t searchFunc(vector<uint> &, const vector<uint> &, const vector<uint> &))
{
	size_t difCount;
	size_t counter = 0;
	double timePassed;
	steady_clock::time_point start = steady_clock::now();
	do
	{
		dif.clear();
		difCount = searchFunc(dif, s1, s2);
		if (difCount != checkDifCount)
		{
			std::cerr << "check failed!" << std::endl;
			std::terminate();
		}
		timePassed = duration_cast<duration<double>>(steady_clock::now() - start).count();
		++counter;
	}
	while (timePassed < TestSeconds);
	return std::pair<double, size_t>(timePassed, counter);
}

static void test(size_t n, minstd_rand &rand)
{
	std::cout << "N=" << n << std::endl;

	vector<uint> s1(n);

	initialize(s1, rand);

	vector<uint> s2(s1);

	// make some differences
	for (int j = 2; j < n; j += n / 3)
	{
		s2[j] = (s2[j] < std::numeric_limits<uint>::max()) ? (s2[j] + 1) : 0;
	}

	vector<uint> dif;
	dif.reserve(n); // no reallocation will occur

	size_t checkDifCount1 = naiveSearchForNew(dif, s1, s2);
	dif.clear();
	size_t checkDifCount2 = hashTableSearchForNew(dif, s1, s2);

	std::cout << "Do naive search..." << std::endl;
	std::pair<double, size_t> naive = test(dif, s1, s2, checkDifCount2, naiveSearchForNew);

	std::cout << "Do sort and compare search..." << std::endl;
	std::pair<double, size_t> sortAndCompare = test(dif, s1, s2, checkDifCount1, sortAndCompareSearchForNew);

	std::cout << "Do hash table search..." << std::endl;
	std::pair<double, size_t> hashTable = test(dif, s1, s2, checkDifCount1, hashTableSearchForNew);

	double naiveSearchTime = naive.first / naive.second;
	double sortAndCompareTime = sortAndCompare.first / sortAndCompare.second;
	double hashTableTime = hashTable.first / hashTable.second;
	std::cout << "Naive search time (one run): " << naiveSearchTime << " sec" << std::endl;
	std::cout << "Sort and compare time (one run): " << sortAndCompareTime << " sec" << std::endl;
	std::cout << "Hash table search time (one run): " << hashTableTime << " sec" << std::endl;
	const double MinTime = 1E-12;
	if (sortAndCompareTime > MinTime) // protecting from divide by zero, overflow, etc.
	{
		std::cout << "Naive to sort and compare search time: " << naiveSearchTime / sortAndCompareTime << std::endl;
	}
	if (hashTableTime > MinTime)
	{
		std::cout << "Naive to hash table search time: " << naiveSearchTime / hashTableTime << std::endl;
		std::cout << "Sort and compare to hash table search time: " << sortAndCompareTime / hashTableTime << std::endl;
	}
}

void lesson1()
{
	minstd_rand rand;
/*
	rand.seed(123);
	vector<uint> s1 = { 10, 3, 3 };
	vector<uint> s2 = { 9, 7, 6, 3, 2 };
	vector<uint> dif;
	size_t n = sortAndCompareSearchForNew(dif, s1, s2);
	std::cout << n << "\n";
*/
	const int sizes[] = { 10, 20, 50, 100, 200, 500, 1000, 2000, 3000, 5000, 10000, 15000, 20000, 25000, 50000, 75000, 100000 };
	for (int i : sizes)
	{
		rand.seed(123); // be determenistic
		test(i, rand);
	}
}
