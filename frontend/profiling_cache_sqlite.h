//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "profiling_cache.h"

#include <mt/mutex.h>
#include <scheduler/task_node.h>

namespace micro_profiler
{
	class profiling_cache_sqlite : public profiling_cache, public profiling_cache_tasks
	{
	public:
		profiling_cache_sqlite(const std::string &preferences_db);

		static void create_database(const std::string &preferences_db);

		virtual scheduler::task<id_t> persisted_module_id(id_t module_id) override;

		virtual std::shared_ptr<module_info_metadata> load_metadata(unsigned int hash, id_t associated_module_id) override;
		virtual void store_metadata(const module_info_metadata &metadata, id_t associated_module_id) override;

		virtual std::vector<tables::cached_patch> load_default_patches(id_t cached_module_id) override;
		virtual void update_default_patches(id_t cached_module_id, std::vector<unsigned int> add_rva,
			std::vector<unsigned int> remove_rva) override;

	private:
		std::shared_ptr< scheduler::task_node<id_t> > get_cached_module_id_task(id_t module_id);

	private:
		mt::mutex _mtx;
		const std::string _preferences_db;
		std::unordered_map< id_t, std::shared_ptr< scheduler::task_node<id_t> > > _module_mapping_tasks;
	};
}
