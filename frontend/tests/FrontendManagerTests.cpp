#include <frontend/frontend_manager.h>

#include "helpers.h"
#include "mocks.h"
#include "mock_channel.h"

#include <algorithm>
#include <common/serialization.h>
#include <frontend/frontend.h>
#include <frontend/frontend_ui.h>
#include <frontend/profiling_cache_sqlite.h>
#include <strmd/serializer.h>
#include <test-helpers/file_helpers.h>
#include <test-helpers/helpers.h>
#include <test-helpers/mock_queue.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			const auto null_ui_factory = [] (shared_ptr<profiling_session>) {	return shared_ptr<micro_profiler::frontend_ui>();	};

			shared_ptr<profiling_session> create_ui_context(string executable)
			{
				auto ctx = make_shared<profiling_session>();

				ctx->process_info.executable = executable;
				ctx->process_info.ticks_per_second = 1;
				return ctx;
			}

			template <typename CommandDataT>
			void message(ipc::channel &channel, messages_id c, const CommandDataT &data)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter, packer> archive(b);

				archive(c);
				archive(data);
				channel.message(const_byte_range(&b.buffer[0], static_cast<unsigned>(b.buffer.size())));
			}

			initialization_data make_initialization_data(const string &executable, timestamp_t ticks_per_second)
			{
				initialization_data idata = {	executable, ticks_per_second };
				return idata;
			}

			template <typename T, size_t n>
			void reset_all(T (&a)[n])
			{	fill_n(a, n, T());	}
		}

		namespace mocks
		{
			class frontend_ui : public micro_profiler::frontend_ui
			{		
			public:
				frontend_ui(shared_ptr<profiling_session> ui_context_)
					: ui_context(ui_context_), close_on_destroy(true)
				{	}

				~frontend_ui()
				{
					if (close_on_destroy)
						closed(); // this is here only to make frontend_manager's life harder - it should ignore this signal.
				}

				void emulate_close()
				{	closed();	}

			public:
				shared_ptr<profiling_session> ui_context;
				bool close_on_destroy;

			private:
				virtual void activate() {	}
			};
		}


		begin_test_suite( FrontendManagerTests )

			mocks::outbound_channel outbound;
			shared_ptr<tables::statistics> statistics;
			shared_ptr<tables::modules> modules;
			shared_ptr<tables::module_mappings> mappings;
			temporary_directory dir;
			mocks::queue worker, apartment;
			function<frontend *(ipc::channel &outbound)> new_frontend;

			init( Init )
			{
				statistics = make_shared<tables::statistics>();
				modules = make_shared<tables::modules>();
				mappings = make_shared<tables::module_mappings>();
				new_frontend = [this] (ipc::channel &outbound) {
					return new frontend(outbound, make_shared<profiling_cache_sqlite>(dir.track_file("pfc.db")), worker, apartment);
				};
			}


			test( OpeningFrontendChannelIncrementsInstanceCount )
			{
				// INIT
				frontend_manager m(new_frontend, null_ui_factory);

				// ACT / ASSERT (must not throw)
				ipc::channel_ptr_t c1 = m.create_session(outbound);

				// ACT / ASSERT
				assert_equal(1u, m.instances_count());

				// ACT
				ipc::channel_ptr_t c2 = m.create_session(outbound);

				// ACT / ASSERT
				assert_equal(2u, m.instances_count());
			}


			test( ClosingFrontendChannelsDecrementsInstanceCount )
			{
				// INIT
				frontend_manager m(new_frontend, null_ui_factory);
				ipc::channel_ptr_t c1 = m.create_session(outbound);
				ipc::channel_ptr_t c2 = m.create_session(outbound);

				// ACT
				c2.reset();

				// ACT / ASSERT
				assert_equal(1u, m.instances_count());
			}


			vector< shared_ptr<mocks::frontend_ui> > _ui_creation_log;

			function<shared_ptr<frontend_ui> (shared_ptr<profiling_session> ui_context)> logging_ui_factory()
			{
				return [this] (shared_ptr<profiling_session> ui_context) -> shared_ptr<frontend_ui> {
					shared_ptr<mocks::frontend_ui> ui(new mocks::frontend_ui(ui_context));

					_ui_creation_log.push_back(ui);
					return ui;
				};
			}

			test( FrontendInitializaionLeadsToUICreation )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());
				ipc::channel_ptr_t c1 = m.create_session(outbound);
				ipc::channel_ptr_t c2 = m.create_session(outbound);

				// ACT
				message(*c1, init, make_initialization_data("c:\\test\\some.exe", 12332));

				// ASSERT
				assert_equal(1u, _ui_creation_log.size());

				assert_equal("c:\\test\\some.exe", _ui_creation_log[0]->ui_context->process_info.executable);
				assert_equal(12332, _ui_creation_log[0]->ui_context->process_info.ticks_per_second);

				// ACT
				message(*c2, init, make_initialization_data("kernel.exe", 12333));

				// ASSERT
				assert_equal(2u, _ui_creation_log.size());

				assert_equal("kernel.exe", _ui_creation_log[1]->ui_context->process_info.executable);
				assert_equal(12333, _ui_creation_log[1]->ui_context->process_info.ticks_per_second);
			}


			obsolete_test( FrontendManagerIsHeldByFrontendsAlive )


			struct mock_ui : frontend_ui
			{
				mock_ui(shared_ptr<profiling_session> ctx)
					: hold(micro_profiler::statistics(ctx))
				{	}

				virtual void activate() override
				{	}

				shared_ptr<tables::statistics> hold;
			};

			test( ThereAreNoInstancesAfterCloseAll )
			{
				// INIT
				frontend_manager m(new_frontend, [] (shared_ptr<profiling_session> ctx) {
					return make_shared<mock_ui>(ctx);
				});
				auto ctx1 = create_ui_context("somefile.exe");
				auto ctx2 = create_ui_context("somefile.exe");

				m.load_session(ctx1);
				m.load_session(ctx2);

				// ACT
				m.close_all();

				// ASSERT
				assert_equal(0u, m.instances_count());
				assert_null(m.get_active());
				assert_equal(1, ctx1.use_count());
				assert_equal(1, ctx2.use_count());
			}


			test( UIEventsAreIgnoredAfterCloseAll )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());
				ipc::channel_ptr_t c1 = m.create_session(outbound);
				ipc::channel_ptr_t c2 = m.create_session(outbound);

				message(*c1, init, make_initialization_data("", 10));
				message(*c2, init, make_initialization_data("", 15));

				// ACT
				m.close_all();

				// ASSERT
				assert_null(m.get_active());

				// ACT
				_ui_creation_log[0]->activated();

				// ASSERT
				assert_null(m.get_active());

			}


			test( FrontendsAreDisconnectedWhenManagerIsReleased )
			{
				// INIT
				mocks::outbound_channel outbound_channels[2];
				shared_ptr<frontend_manager> m(new frontend_manager(new_frontend, null_ui_factory));
				ipc::channel_ptr_t c1 = m->create_session(outbound_channels[0]);
				ipc::channel_ptr_t c2 = m->create_session(outbound_channels[1]);

				// ACT
				m.reset();

				// ASSERT
				assert_is_true(outbound_channels[0].disconnected);
				assert_is_true(outbound_channels[1].disconnected);
			}


			test( FrontendForClosedUIIsDisconnected )
			{
				// INIT
				mocks::outbound_channel outbound_channels[3];
				frontend_manager m(new_frontend, logging_ui_factory());
				ipc::channel_ptr_t c[] = {
					m.create_session(outbound_channels[0]),
					m.create_session(outbound_channels[1]),
					m.create_session(outbound_channels[2]),
				};

				message(*c[0], init, make_initialization_data("", 1));
				message(*c[1], init, make_initialization_data("", 1));
				message(*c[2], init, make_initialization_data("", 1));

				// ACT
				_ui_creation_log[0]->emulate_close();
				_ui_creation_log[2]->emulate_close();

				// ASSERT
				assert_is_true(outbound_channels[0].disconnected);
				assert_is_true(outbound_channels[2].disconnected);

				// ACT
				_ui_creation_log[1]->emulate_close();

				// ASSERT
				assert_is_true(outbound_channels[1].disconnected);
			}


			vector< weak_ptr<mocks::frontend_ui> > _ui_creation_log_w;

			function<shared_ptr<frontend_ui> (shared_ptr<profiling_session> ui_context)> logging_ui_factory_weak()
			{
				return [this] (shared_ptr<profiling_session> ui_context) -> shared_ptr<frontend_ui> {
					shared_ptr<mocks::frontend_ui> ui(new mocks::frontend_ui(ui_context));

					_ui_creation_log_w.push_back(ui);
					return ui;
				};
			}

			test( FrontendUIIsHeldAfterCreation )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory_weak());
				ipc::channel_ptr_t c = m.create_session(outbound);

				// ACT
				message(*c, init, initialization_data());

				// ASSERT
				assert_equal(1u, _ui_creation_log_w.size());
				assert_is_false(_ui_creation_log_w[0].expired());
			}


			test( UIIsHeldEvenAfterFrontendReleased )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory_weak());
				ipc::channel_ptr_t c[] = {
					m.create_session(outbound), m.create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());

				// ACT
				c[0].reset();

				// ASSERT
				assert_equal(2u, _ui_creation_log_w.size());
				assert_is_false(_ui_creation_log_w[0].expired());
				assert_is_false(_ui_creation_log_w[1].expired());
			}


			test( InstancesAreManagedByOpenedUIsAfterFrontendsReleased )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());
				ipc::channel_ptr_t c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());
				message(*c[2], init, initialization_data());

				// ACT
				reset_all(c);

				// ASSERT
				assert_equal(3u, m.instances_count());

				// ACT
				_ui_creation_log[1]->emulate_close();

				// ACT / ASSERT
				assert_equal(2u, m.instances_count());

				// ACT
				_ui_creation_log[2]->emulate_close();
				_ui_creation_log[0]->emulate_close();

				// ACT / ASSERT
				assert_equal(0u, m.instances_count());
			}


			test( InstanceLingersWhenUIIsClosedButFrontendIsStillReferenced )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory_weak());
				ipc::channel_ptr_t c = m.create_session(outbound);

				message(*c, init, initialization_data());

				// ACT
				_ui_creation_log_w[0].lock()->emulate_close();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
				assert_equal(1u, m.instances_count());
			}


			test( InstanceLingersWhenUIIsClosedButFrontendIsStillReferencedNoExternalUIReference )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory_weak());
				ipc::channel_ptr_t c = m.create_session(outbound);

				message(*c, init, initialization_data());

				// ACT (must not crash - make sure even doubled close doesn't blow things up)
				m.get_instance(0)->ui->closed();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
				assert_equal(1u, m.instances_count());
			}


			test( UIIsDestroyedOnManagerDestructionWhenFrontendIsHeld )
			{
				// INIT
				shared_ptr<frontend_manager> m(new frontend_manager(new_frontend, logging_ui_factory_weak()));
				ipc::channel_ptr_t c = m->create_session(outbound);

				message(*c, init, initialization_data());

				// ACT
				m.reset();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
			}


			test( UIIsDestroyedOnManagerDestructionWhenFrontendIsReleased )
			{
				// INIT
				shared_ptr<frontend_manager> m(new frontend_manager(new_frontend, logging_ui_factory_weak()));
				ipc::channel_ptr_t c = m->create_session(outbound);

				message(*c, init, initialization_data());
				c.reset();

				// ACT
				m.reset();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
			}


			test( OnlyExternallyHeldUISurvivesManagerDestruction )
			{
				// INIT
				shared_ptr<frontend_manager> m(new frontend_manager(new_frontend, logging_ui_factory_weak()));
				ipc::channel_ptr_t c[] = {
					m->create_session(outbound), m->create_session(outbound), m->create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());
				message(*c[2], init, initialization_data());
				reset_all(c);

				auto ui = _ui_creation_log_w[2].lock();

				// ACT
				m.reset();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
				assert_is_true(_ui_creation_log_w[1].expired());
				assert_is_false(_ui_creation_log_w[2].expired());
			}


			obsolete_test( ManagerIsHeldByOpenedUI )


			test( NoInstanceIsReturnedForEmptyManager )
			{
				// INIT
				frontend_manager m(new_frontend, null_ui_factory);

				// ACT / ASSERT
				assert_null(m.get_instance(0));
				assert_null(m.get_instance(10));
			}


			test( InstancesAreReturned )
			{
				// INIT
				frontend_manager m(new_frontend, null_ui_factory);

				// ACT
				ipc::channel_ptr_t c1 = m.create_session(outbound);
				ipc::channel_ptr_t c2 = m.create_session(outbound);
				ipc::channel_ptr_t c3 = m.create_session(outbound);

				// ACT / ASSERT
				assert_not_null(m.get_instance(0));
				assert_is_empty(m.get_instance(0)->title);
				assert_not_null(m.get_instance(1));
				assert_not_null(m.get_instance(2));
				assert_null(m.get_instance(3));
			}


			test( InstanceContainsExecutableNameModelAndUIAfterInitialization )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());
				ipc::channel_ptr_t c = m.create_session(outbound);

				// ACT
				message(*c, init, make_initialization_data("c:\\dev\\micro-profiler", 1));

				// ACT / ASSERT
				assert_not_null(m.get_instance(0));
				assert_equal("c:\\dev\\micro-profiler", m.get_instance(0)->title);
				assert_equal(_ui_creation_log[0], m.get_instance(0)->ui);
			}


			test( ClosingAllInstancesDisconnectsAllFrontendsAndDestroysAllUI )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory_weak());
				mocks::outbound_channel outbound_channels[3];
				ipc::channel_ptr_t c[] = {
					m.create_session(outbound_channels[0]), m.create_session(outbound_channels[1]), m.create_session(outbound_channels[2]),
				};

				message(*c[0], init, make_initialization_data("", 1));
				message(*c[1], init, make_initialization_data("", 1));
				message(*c[2], init, make_initialization_data("", 1));
				
				// ACT
				m.close_all();

				// ASSERT
				assert_is_true(outbound_channels[0].disconnected);
				assert_is_true(_ui_creation_log_w[0].expired());
				assert_is_true(outbound_channels[1].disconnected);
				assert_is_true(_ui_creation_log_w[1].expired());
				assert_is_true(outbound_channels[2].disconnected);
				assert_is_true(_ui_creation_log_w[2].expired());

				// ACT
				reset_all(c);

				// ASSERT
				assert_equal(0u, m.instances_count());
			}


			test( NoActiveInstanceInEmptyManager )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());

				// ACT / ASSERT
				assert_null(m.get_active());
			}


			test( NoInstanceConsideredActiveIfNoUICreated )
			{
				// INIT
				frontend_manager m(new_frontend, null_ui_factory);
				ipc::channel_ptr_t c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				// ACT
				message(*c[0], init, initialization_data());

				// ASSERT
				assert_null(m.get_active());
			}


			test( LastInitializedInstanceConsideredActive )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());
				ipc::channel_ptr_t c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				// ACT
				message(*c[0], init, initialization_data());

				// ASSERT
				assert_not_null(m.get_active());
				assert_equal(m.get_instance(0), m.get_active());

				// ACT
				message(*c[1], init, initialization_data());

				// ASSERT
				assert_equal(m.get_instance(1), m.get_active());
			}


			test( ActiveInstanceIsResetOnFrontendReleaseEvenIfUIDidNotSignalledOfClose )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());
				ipc::channel_ptr_t c = m.create_session(outbound);

				message(*c, init, initialization_data()); // make it active
				_ui_creation_log[0]->close_on_destroy = false;

				// ACT
				m.close_all();
				c.reset();

				// ASSERT
				assert_null(m.get_active());
			}


			test( ActiveInstanceIsNotResetOnFrontendReleaseWhenOtherAliveInstanceIsActive )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());
				ipc::channel_ptr_t c1 = m.create_session(outbound);

				message(*c1, init, initialization_data());
				_ui_creation_log[0]->close_on_destroy = false;

				// ACT
				ipc::channel_ptr_t c2 = m.create_session(outbound);
				message(*c2, init, initialization_data());
				c1.reset();

				// ASSERT
				assert_not_null(m.get_active());
			}


			test( UIActivationSwitchesActiveInstanceInManager )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());
				ipc::channel_ptr_t c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());
				message(*c[2], init, initialization_data());

				// ACT
				_ui_creation_log[1]->activated();

				// ASSERT
				assert_not_null(m.get_active());
				assert_equal(m.get_instance(1), m.get_active());

				// ACT
				_ui_creation_log[0]->activated();

				// ASSERT
				assert_equal(m.get_instance(0), m.get_active());

				// ACT
				_ui_creation_log[2]->activated();

				// ASSERT
				assert_equal(m.get_instance(2), m.get_active());
			}


			test( NoInstanceIsActiveAfterCloseAll )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory_weak());
				ipc::channel_ptr_t c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());
				_ui_creation_log_w[0].lock()->activated();

				// ACT
				m.close_all();

				// ACT / ASSERT
				assert_null(m.get_active());
			}


			test( NoInstanceIsActiveAfterActiveInstanceIsClosed )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());
				ipc::channel_ptr_t c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());
				_ui_creation_log[0]->activated();

				// ACT
				_ui_creation_log[0]->emulate_close();

				// ACT / ASSERT
				assert_null(m.get_active());
			}


			test( ActiveInstancetIsIntactAfterInactiveInstanceIsClosed )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());
				ipc::channel_ptr_t c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());
				_ui_creation_log[0]->activated();

				// ACT
				_ui_creation_log[1]->emulate_close();

				// ACT / ASSERT
				assert_equal(m.get_instance(0), m.get_active());
			}


			test( CreatingInstanceFromModelAddsNewInstanceToTheListAndConstructsUI )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());
				auto ctx1 = create_ui_context("somefile.exe");
				auto ctx2 = create_ui_context("/usr/bin/grep");

				// ACT
				m.load_session(ctx1);

				// ASSERT
				assert_equal(1u, m.instances_count());
				assert_equal("somefile.exe", m.get_instance(0)->title);
				assert_equal(_ui_creation_log[0], m.get_instance(0)->ui);

				// ACT
				m.load_session(ctx2);

				// ASSERT
				assert_equal(2u, m.instances_count());
				assert_equal("/usr/bin/grep", m.get_instance(1)->title);
				assert_equal(_ui_creation_log[1], m.get_instance(1)->ui);
			}


			test( NoAttemptToDisconnectFrontendIsMadeIfNoFrontendExistsOnCloseAll )
			{
				// INIT
				frontend_manager m(new_frontend, logging_ui_factory());

				// ACT
				m.load_session(create_ui_context("somefile.exe"));

				// ACT / ASSERT (does not crash)
				m.close_all();
				m.close_all();
			}

		end_test_suite
	}
}
