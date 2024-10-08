#include <frontend/primitives.h>

#include "comparisons.h"
#include "primitive_helpers.h"

#include <ut/assert.h>
#include <ut/test.h>

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( PrimitivesTests )
			test( AddingNonReentrantCallAddsAllTimes )
			{
				// INIT
				call_statistics data[] = {
					make_call_statistics(1, 1, 0, 101, 19210, 0, 1199321, 12391, 1003),
					make_call_statistics(2, 1, 0, 203, 11023, 0, 1100321, 32391, 1703),
					make_call_statistics(3, 1, 0, 305, 91000, 0, 2101021, 22009, 1913),
					make_call_statistics(4, 1, 0, 207, 11000, 0, 7101021, 22109, 1912),
				};
				auto lookup = [&] (id_t id) {	return id ? &(data[id - 1]) : nullptr;	};

				// ACT
				add(data[0], data[2], lookup);
				add(data[1], data[2], lookup);

				// ASSERT
				assert_equal_pred(make_call_statistics(1, 1, 0, 101, 110210, 0, 3300342, 34400, 1913), data[0], eq());
				assert_equal_pred(make_call_statistics(2, 1, 0, 203, 102023, 0, 3201342, 54400, 1913), data[1], eq());

				// ACT
				add(data[0], data[3], lookup);
				add(data[1], data[3], lookup);

				// ASSERT
				assert_equal_pred(make_call_statistics(1, 1, 0, 101, 121210, 0, 10401363, 56509, 1913), data[0], eq());
				assert_equal_pred(make_call_statistics(2, 1, 0, 203, 113023, 0, 10302363, 76509, 1913), data[1], eq());
			}


			test( AddingReentrantCallAddsAllButInclusiveTimes )
			{
				// INIT
				call_statistics data[] = {
					make_call_statistics(1, 1, 0, 101, 19210, 0, 1199321, 12391, 1003),
					make_call_statistics(2, 1, 0, 203, 11023, 0, 1100321, 32391, 1703),
					make_call_statistics(3, 1, 0, 305, 0, 0, 0, 0, 0),
					make_call_statistics(4, 1, 0, 207, 0, 0, 0, 0, 0),
					make_call_statistics(5, 1, 4, 207, 0, 0, 0, 0, 0),
					make_call_statistics(6, 1, 3, 305, 91000, 0, 2101021, 22009, 1913),
					make_call_statistics(7, 1, 5, 207, 11000, 0, 7101021, 22109, 1912),
				};
				auto lookup = [&] (id_t id) {	return id ? &(data[id - 1]) : nullptr;	};

				// ACT
				add(data[0], data[5], lookup);
				add(data[1], data[6], lookup);

				// ASSERT
				assert_equal_pred(make_call_statistics(1, 1, 0, 101, 110210, 0, 1199321, 34400, 1003), data[0], eq());
				assert_equal_pred(make_call_statistics(2, 1, 0, 203, 22023, 0, 1100321, 54500, 1703), data[1], eq());
			}
		end_test_suite
	}
}
