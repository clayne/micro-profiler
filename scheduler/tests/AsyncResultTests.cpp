#include <scheduler/task.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace scheduler
{
	namespace tests
	{
		namespace
		{
			struct no_default_constructor
			{
				no_default_constructor(int value_)
					: value(value_)
				{	}

				int value;
			};
		}

		begin_test_suite( ValuedAsyncResultTests )
			test( DestroyingUnsetResultIsOK )
			{
				{
				// INIT / ACT
					async_result<int> r1;
					async_result<string> r2;

				// ASSERT (no crash at destroy)
				}
			}


			test( ProvidedValueIsAvailableOnDereference )
			{
				// INIT / ACT
				async_result<int> r1, r2;
				async_result<string> r3, r4;

				// ACT
				r1.set(150);
				r2.set(13);
				r3.set("lorem");
				r4.set("ipsum");

				// ASSERT
				assert_equal(150, *r1);
				assert_equal(13, *r2);
				assert_equal("lorem", *r3);
				assert_equal("ipsum", *r4);
			}


			test( NoDefaultInstanceIsCreatedByAsyncResult )
			{
				// INIT
				async_result<no_default_constructor> r;

				// ACT
				r.set(no_default_constructor(11911));

				// ASSERT
				assert_equal(11911, (*r).value);
			}


			test( ValueHeldIsDestroyedOnAsyncResultDestruction )
			{
				// INIT
				const auto f = make_shared<bool>();
				unique_ptr< async_result< shared_ptr<void> > > r(new async_result< shared_ptr<void> >());

				// ACT
				r->set(f);

				// ASSERT
				assert_is_true(f.use_count() > 1);

				// ACT
				r.reset();

				// ASSERT
				assert_equal(1, f.use_count());
			}


			test( ProvidedExceptionIsThrownOnDereference )
			{
				// INIT
				async_result<int> r1, r2, r3;
				auto e2 = make_shared<int>(1192);
				auto e3 = make_shared<string>("abc");

				r1.fail(make_exception_ptr(1928));
				r2.fail(make_exception_ptr(e2));
				r3.fail(make_exception_ptr(e3));

				// ACT
				try
				{
					*r1;
					assert_is_false(true);
				}
				catch (int value)
				{

				// ASSERT
					assert_equal(1928, value);
				}

				// ACT
				try
				{
					*r2;
					assert_is_false(true);
				}
				catch (shared_ptr<int> value)
				{

				// ASSERT
					assert_equal(e2, value);
				}

				// ACT
				try
				{
					*r3;
					assert_is_false(true);
				}
				catch (shared_ptr<string> value)
				{

				// ASSERT
					assert_equal(e3, value);
				}
			}


			test( ExceptionHeldIsDestroyedOnAsyncResultDestruction )
			{
				// INIT
				const auto f = make_shared<bool>();
				unique_ptr< async_result<int> > r(new async_result<int>());

				// ACT
				r->fail(make_exception_ptr(f));

				// ASSERT
				assert_is_true(f.use_count() > 1);

				// ACT
				r.reset();

				// ASSERT
				assert_equal(1, f.use_count());
			}


			test( SettingNonEmptyResultFails )
			{
				// INIT
				async_result<int> with_value, with_exception;

				with_value.set(12345);
				with_exception.fail(make_exception_ptr(111));

				// ACT / ASSERT
				assert_throws(with_value.set(1), logic_error);
				assert_throws(with_exception.set(2), logic_error);
				assert_throws(with_value.fail(make_exception_ptr("bbb")), logic_error);
				assert_throws(with_exception.fail(make_exception_ptr("ccc")), logic_error);
			}
		end_test_suite


		begin_test_suite( VoidAsyncResultTests )
			test( DestroyingUnsetResultIsOK )
			{
				{
				// INIT / ACT
					async_result<void> r;

				// ASSERT (no crash at destroy)
				}
			}


			test( ProvidedValueIsAvailableOnDereference )
			{
				// INIT / ACT
				async_result<void> r;

				// ACT
				r.set();

				// ACT / ASSERT (no exception)
				*r;
			}


			test( ProvidedExceptionIsThrownOnDereference )
			{
				// INIT
				async_result<void> r1, r2, r3;
				auto e2 = make_shared<int>(1192);
				auto e3 = make_shared<string>("abc");

				r1.fail(make_exception_ptr(1928));
				r2.fail(make_exception_ptr(e2));
				r3.fail(make_exception_ptr(e3));

				// ACT
				try
				{
					*r1;
					assert_is_false(true);
				}
				catch (int value)
				{

				// ASSERT
					assert_equal(1928, value);
				}

				// ACT
				try
				{
					*r2;
					assert_is_false(true);
				}
				catch (shared_ptr<int> value)
				{

				// ASSERT
					assert_equal(e2, value);
				}

				// ACT
				try
				{
					*r3;
					assert_is_false(true);
				}
				catch (shared_ptr<string> value)
				{

				// ASSERT
					assert_equal(e3, value);
				}
			}


			test( ExceptionHeldIsDestroyedOnAsyncResultDestruction )
			{
				// INIT
				const auto f = make_shared<bool>();
				unique_ptr< async_result<int> > r(new async_result<int>());

				// ACT
				r->fail(make_exception_ptr(f));

				// ASSERT
				assert_is_true(f.use_count() > 1);

				// ACT
				r.reset();

				// ASSERT
				assert_equal(1, f.use_count());
			}


			test( SettingNonEmptyResultFails )
			{
				// INIT
				async_result<void> with_value, with_exception;

				with_value.set();
				with_exception.fail(make_exception_ptr(111));

				// ACT / ASSERT
				assert_throws(with_value.set(), logic_error);
				assert_throws(with_exception.set(), logic_error);
				assert_throws(with_value.fail(make_exception_ptr("bbb")), logic_error);
				assert_throws(with_exception.fail(make_exception_ptr("ccc")), logic_error);
			}
		end_test_suite
	}
}
