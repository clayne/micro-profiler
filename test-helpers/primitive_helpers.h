#pragma once

#include <common/primitives.h>

namespace micro_profiler
{
	namespace tests
	{
		template <typename AddressT>
		inline std::pair<AddressT, function_statistics> make_statistics_base(AddressT address,
			count_t times_called, unsigned int max_reentrance, timestamp_t inclusive_time, timestamp_t exclusive_time,
			timestamp_t max_call_time)
		{
			return std::make_pair(address, function_statistics(times_called, max_reentrance, inclusive_time, exclusive_time,
				max_call_time));
		}

		template <typename AddressT>
		inline std::pair< AddressT, typename statistic_types_t<AddressT>::function_detailed > make_statistics(AddressT address,
			count_t times_called, unsigned int max_reentrance, timestamp_t inclusive_time, timestamp_t exclusive_time,
			timestamp_t max_call_time)
		{
			std::pair< AddressT, typename statistic_types_t<AddressT>::function_detailed > result;

			result.first = address;
			static_cast<function_statistics &>(result.second) = function_statistics(times_called, max_reentrance,
				inclusive_time, exclusive_time, max_call_time);
			return result;
		}

		template <typename AddressT>
		inline std::pair< AddressT, typename statistic_types_t<AddressT>::function_detailed > make_statistics(AddressT address,
			count_t times_called, unsigned int max_reentrance, timestamp_t inclusive_time, timestamp_t exclusive_time,
			timestamp_t max_call_time, std::vector< std::pair<AddressT, typename statistic_types_t<AddressT>::function> > callees,
			std::vector< std::pair<AddressT, count_t> > callers)
		{
			auto r = make_statistics(address, times_called, max_reentrance, inclusive_time, exclusive_time, max_call_time);

			r.second.callees = typename statistic_types_t<AddressT>::map(callees.begin(), callees.end());
			r.second.callers = typename statistic_types_t<AddressT>::map_callers(callers.begin(), callers.end());
			return r;
		}

		template <typename AddressT>
		inline std::pair< AddressT, typename statistic_types_t<AddressT>::function_detailed > make_statistics(AddressT address,
			count_t times_called, unsigned int max_reentrance, timestamp_t inclusive_time, timestamp_t exclusive_time,
			timestamp_t max_call_time, const std::pair<AddressT, function_statistics> &callee1)
		{
			std::pair< AddressT, typename statistic_types_t<AddressT>::function_detailed > result = make_statistics(address, times_called,
				max_reentrance, inclusive_time, exclusive_time, max_call_time);

			result.second.callees.insert(callee1);
			return result;
		}

		template <typename AddressT>
		inline std::pair< AddressT, typename statistic_types_t<AddressT>::function_detailed > make_statistics(AddressT address,
			count_t times_called, unsigned int max_reentrance, timestamp_t inclusive_time, timestamp_t exclusive_time,
			timestamp_t max_call_time, const std::pair<AddressT, function_statistics> &callee1,
			const std::pair<AddressT, function_statistics> &callee2)
		{
			std::pair< AddressT, typename statistic_types_t<AddressT>::function_detailed > result = make_statistics(address, times_called,
				max_reentrance, inclusive_time, exclusive_time, max_call_time);

			result.second.callees.insert(callee1);
			result.second.callees.insert(callee2);
			return result;
		}

		template <typename AddressT>
		inline std::pair< AddressT, typename statistic_types_t<AddressT>::function_detailed > make_statistics(AddressT address,
			count_t times_called, unsigned int max_reentrance, timestamp_t inclusive_time, timestamp_t exclusive_time,
			timestamp_t max_call_time, const std::pair<AddressT, function_statistics> &callee1,
			const std::pair<AddressT, function_statistics> &callee2,
			const std::pair<AddressT, function_statistics> &callee3)
		{
			std::pair< AddressT, typename statistic_types_t<AddressT>::function_detailed > result = make_statistics(address, times_called,
				max_reentrance, inclusive_time, exclusive_time, max_call_time);

			result.second.callees.insert(callee1);
			result.second.callees.insert(callee2);
			result.second.callees.insert(callee3);
			return result;
		}

		template <typename AddressT>
		inline std::pair< AddressT, typename statistic_types_t<AddressT>::function_detailed > make_statistics(AddressT address,
			count_t times_called, unsigned int max_reentrance, timestamp_t inclusive_time, timestamp_t exclusive_time,
			timestamp_t max_call_time, const std::pair<AddressT, function_statistics> &callee1,
			const std::pair<AddressT, function_statistics> &callee2,
			const std::pair<AddressT, function_statistics> &callee3,
			const std::pair<AddressT, function_statistics> &callee4)
		{
			std::pair< AddressT, typename statistic_types_t<AddressT>::function_detailed > result = make_statistics(address, times_called,
				max_reentrance, inclusive_time, exclusive_time, max_call_time);

			result.second.callees.insert(callee1);
			result.second.callees.insert(callee2);
			result.second.callees.insert(callee3);
			result.second.callees.insert(callee4);
			return result;
		}
	}
}
