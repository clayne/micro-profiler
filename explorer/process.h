//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include <common/types.h>
#include <cstdint>
#include <tasker/private_queue.h>
#include <sdb/table.h>

namespace micro_profiler
{
	struct process_info
	{
		enum architectures {	x86, x64,	};
		typedef std::int64_t process_time_t;

		id_t pid, parent_pid;
		std::string path;
		architectures architecture;
		process_time_t started_at;
		std::shared_ptr<void> handle;
		mt::milliseconds cpu_time;
		float cpu_usage;

		unsigned int cycle;
	};

	namespace tables
	{
		struct process_constructor
		{
			process_info operator ()() const
			{
				process_info value = {};
				return value;
			}
		};

		typedef sdb::table<process_info, process_constructor> processes;
	}

	class process_explorer : public tables::processes
	{
	public:
		process_explorer(mt::milliseconds update_interval, tasker::queue &apartment_queue,
			const std::function<mt::milliseconds ()> &clock);

	private:
		void update();

	private:
		tasker::private_queue _apartment;
		const std::function<mt::milliseconds ()> _clock;
		const mt::milliseconds _update_interval;
		mt::milliseconds _last_update;
		unsigned int _cycle;
	};
}
