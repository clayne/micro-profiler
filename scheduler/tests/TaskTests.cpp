#include <scheduler/task.h>

#include "mocks.h"

#include <queue>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace scheduler
{
	namespace tests
	{
		using namespace micro_profiler::tests;

		begin_test_suite( TaskTests )
			mocks::queue queues[2];

			test( RunningATaskSchedulesOneOnTheQueuePassed )
			{
				// INIT
				vector<int> called(2);

				// INIT / ACT
				auto t1 = task<int>::run([&] {	return called[0]++, 18;	}, queues[0]);
				auto t2 = task<int>::run([&] {	return called[1]++, 181;	}, queues[1]);

				// ASSERT
				assert_equal(1u, queues[0].tasks.size());
				assert_equal(1u, queues[1].tasks.size());

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + 1 + 0, called);
				assert_equal(0u, queues[0].tasks.size());
				assert_equal(1u, queues[1].tasks.size());

				// ACT
				queues[1].run_one();

				// ASSERT
				assert_equal(plural + 1 + 1, called);
				assert_equal(0u, queues[0].tasks.size());
				assert_equal(0u, queues[1].tasks.size());
			}


			test( ContinuationsAreScheduledWhenAntecedentIsComplete )
			{
				// INIT
				auto t1 = task<int>::run([&] {	return 18;	}, queues[0]);
				auto t2 = task<int>::run([&] {	return 1;	}, queues[1]);

				// ACT
				t1.continue_with([&] (const async_result<int> &) {	return string();	}, queues[0]);
				t1.continue_with([&] (const async_result<int> &) {	return 17;	}, queues[1]);
				t2.continue_with([&] (const async_result<int> &) {	return string();	}, queues[1]);
				t2.continue_with([&] (const async_result<int> &) {	return 17.19;	}, queues[1]);
				t2.continue_with([&] (const async_result<int> &) {	return plural + 1 + 19;	}, queues[0]);

				// ASSERT
				assert_equal(1u, queues[0].tasks.size());
				assert_equal(1u, queues[1].tasks.size());

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(1u, queues[0].tasks.size());
				assert_equal(2u, queues[1].tasks.size());

				// ACT
				queues[1].run_one();

				// ASSERT
				assert_equal(2u, queues[0].tasks.size());
				assert_equal(3u, queues[1].tasks.size());
			}


			test( ContinuationCallbacksAreCalledInRespectiveQueue )
			{
				// INIT
				vector<int> called(5);
				auto t1 = task<int>::run([&] {	return 18;	}, queues[0]);
				auto t2 = task<int>::run([&] {	return 1;	}, queues[1]);

				t1.continue_with([&] (const async_result<int> &) {	return called[0]++, string();	}, queues[0]);
				t1.continue_with([&] (const async_result<int> &) {	return called[1]++, 17;	}, queues[1]);
				t2.continue_with([&] (const async_result<int> &) {	return called[2]++, string();	}, queues[1]);
				t2.continue_with([&] (const async_result<int> &) {	return called[3]++, 17.19;	}, queues[1]);
				t2.continue_with([&] (const async_result<int> &) {	return called[4]++, plural + 1 + 19;	}, queues[0]);

				// ACT
				queues[1].run_one();
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + 0 + 0 + 0 + 0 + 0, called);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + 0 + 0 + 0 + 0 + 1, called);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + 1 + 0 + 0 + 0 + 1, called);
				assert_is_empty(queues[0].tasks);

				// ACT
				queues[1].run_one();
				queues[1].run_one();

				// ASSERT
				assert_equal(plural + 1 + 0 + 1 + 1 + 1, called);

				// ACT
				queues[1].run_one();

				// ASSERT
				assert_equal(plural + 1 + 1 + 1 + 1 + 1, called);
				assert_is_empty(queues[0].tasks);
				assert_is_empty(queues[1].tasks);
			}


			test( ChainedContinuationsAreCalledSequentially )
			{
				// INIT
				vector<int> called(4);

				// INIT / ACT
				task<int>::run([&] {	return called[0]++, 18;	}, queues[0])
					.continue_with([&] (const async_result<int> &) {	return called[1]++, string("lorem ipsum");	}, queues[1])
					.continue_with([&] (const async_result<string> &) {	return called[2]++, 1.1;	}, queues[0])
					.continue_with([&] (const async_result<double> &) {	return called[3]++, 0;	}, queues[0]);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + 1 + 0 + 0 + 0, called);
				assert_is_empty(queues[0].tasks);
				assert_equal(1u, queues[1].tasks.size());

				// ACT
				queues[1].run_one();

				// ASSERT
				assert_equal(plural + 1 + 1 + 0 + 0, called);
				assert_equal(1u, queues[0].tasks.size());
				assert_is_empty(queues[1].tasks);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + 1 + 1 + 1 + 0, called);
				assert_equal(1u, queues[0].tasks.size());
				assert_is_empty(queues[1].tasks);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + 1 + 1 + 1 + 1, called);
				assert_is_empty(queues[0].tasks);
				assert_is_empty(queues[1].tasks);
			}


			test( ResultOfRootAntecedantIsDeliveredToContinuations )
			{
				// INIT
				vector< pair<int, int> > obtained1;
				auto s1 = task<int>::run([&] {	return 18;	}, queues[0]);

				// INIT / ACT
				s1.continue_with([&] (const async_result<int> &a) {	return obtained1.push_back(make_pair(0, *a)), 1;	}, queues[0]);
				s1.continue_with([&] (const async_result<int> &a) {	return obtained1.push_back(make_pair(1, *a)), 1;	}, queues[0]);

				// ACT
				queues[0].run_one();
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + make_pair(0, 18), obtained1);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + make_pair(0, 18) + make_pair(1, 18), obtained1);

				// INIT
				vector< pair<int, string> > obtained2;
				auto s2 = task<string>::run([&] {	return string("lorem");	}, queues[0]);

				// INIT / ACT
				s2.continue_with([&] (const async_result<string> &a) {	return obtained2.push_back(make_pair(0, *a)), 1;	}, queues[0]);
				s2.continue_with([&] (const async_result<string> &a) {	return obtained2.push_back(make_pair(1, *a)), 1;	}, queues[0]);
				s2.continue_with([&] (const async_result<string> &a) {	return obtained2.push_back(make_pair(2, *a)), 1;	}, queues[0]);

				// ACT
				queues[0].run_one();
				queues[0].run_one();
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + make_pair(0, string("lorem")) + make_pair(1, string("lorem")), obtained2);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + make_pair(0, string("lorem")) + make_pair(1, string("lorem")) + make_pair(2, string("lorem")), obtained2);
			}


			test( ResultOfIntermediateAntecedantIsDeliveredToContinuations )
			{
				// INIT
				vector< pair<int, int> > obtained1;
				auto s1 = task<int>::run([] {	return 18;	}, queues[0])
					.continue_with([] (const async_result<int> &) {	return 13;	}, queues[0]);

				// INIT / ACT
				s1.continue_with([&] (const async_result<int> &a) {	return obtained1.push_back(make_pair(0, *a)), 1;	}, queues[0]);
				s1.continue_with([&] (const async_result<int> &a) {	return obtained1.push_back(make_pair(1, *a)), 1;	}, queues[0]);

				// ACT
				queues[0].run_one();
				queues[0].run_one();
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + make_pair(0, 13), obtained1);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + make_pair(0, 13) + make_pair(1, 13), obtained1);

				// INIT
				vector< pair<int, string> > obtained2;
				auto s2 = task<int>::run([&] {	return 1;	}, queues[0])
					.continue_with([] (const async_result<int> &) {	return string("ipsum");	}, queues[0]);

				// INIT / ACT
				s2.continue_with([&] (const async_result<string> &a) {	return obtained2.push_back(make_pair(0, *a)), 1;	}, queues[0]);
				s2.continue_with([&] (const async_result<string> &a) {	return obtained2.push_back(make_pair(1, *a)), 1;	}, queues[0]);
				s2.continue_with([&] (const async_result<string> &a) {	return obtained2.push_back(make_pair(2, *a)), 1;	}, queues[0]);

				// ACT
				queues[0].run_one();
				queues[0].run_one();
				queues[0].run_one();
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + make_pair(0, string("ipsum")) + make_pair(1, string("ipsum")), obtained2);

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(plural + make_pair(0, string("ipsum")) + make_pair(1, string("ipsum")) + make_pair(2, string("ipsum")), obtained2);
			}


			test( VoidContinuationsMixWellWithNonVoid )
			{
				// INIT / ACT
				auto step = 0;
				task<void>::run([&] {	step = 17;	}, queues[0])
					.continue_with([&] (const async_result<void> &r) {	return *r, step = 11, 17;	}, queues[1])
					.continue_with([&] (const async_result<int> &r) {	*r, step = 3;	}, queues[0])
					.continue_with([&] (const async_result<void> &r) {	*r, step = 5;	}, queues[0]);

				// ASSERT
				assert_equal(1u, queues[0].tasks.size());
				assert_equal(0u, queues[1].tasks.size());

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(17, step);
				assert_equal(0u, queues[0].tasks.size());
				assert_equal(1u, queues[1].tasks.size());

				// ACT
				queues[1].run_one();

				// ASSERT
				assert_equal(11, step);
				assert_equal(1u, queues[0].tasks.size());
				assert_equal(0u, queues[1].tasks.size());

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(3, step);
				assert_equal(1u, queues[0].tasks.size());
				assert_equal(0u, queues[1].tasks.size());

				// ACT
				queues[0].run_one();

				// ASSERT
				assert_equal(5, step);
				assert_equal(0u, queues[0].tasks.size());
				assert_equal(0u, queues[1].tasks.size());
			}

		end_test_suite
	}
}
