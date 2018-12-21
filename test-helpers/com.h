#pragma once

#include <memory>

namespace micro_profiler
{
	namespace tests
	{
		class com_event
		{
		public:
			com_event();

			void set();
			void wait();

		private:
			std::shared_ptr<void> _handle;
		};
	}
}