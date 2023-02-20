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

#include <common/auto_increment.h>
#include <common/file_id.h>
#include <common/module.h>
#include <common/primitives.h>
#include <common/protocol.h>
#include <memory>
#include <mt/mutex.h>
#include <sdb/table.h>

namespace micro_profiler
{
	struct image_info;

	class module_tracker : module::events
	{
	public:
		typedef image_info metadata_t;
		typedef std::shared_ptr<const metadata_t> metadata_ptr;
		struct events;

		struct mapping : module::mapping
		{
			id_t id;
			id_t module_id;
		};

		struct module_info
		{
			id_t id;
			file_id file;
			std::string path;
			std::uint32_t hash;
		};

	public:
		module_tracker(module &module_helper);

		void get_changes(loaded_modules &loaded_modules_, unloaded_modules &unloaded_modules_);
		std::shared_ptr<module::mapping> lock_mapping(unsigned int module_id, std::uint32_t &hash);
		metadata_ptr get_metadata(unsigned int module_id) const;
		std::shared_ptr<void> notify(events &events_);

	private:
		typedef containers::unordered_map<unsigned int /*module_id*/, module_info> modules_registry_t;

	private:
		static std::uint32_t calculate_hash(const std::string &path_);

		// module::events methods
		virtual void mapped(const module::mapping &mapping_) override;
		virtual void unmapped(void *base) override;

	private:
		mutable mt::mutex _mtx;
		sdb::table< mapping, auto_increment_constructor<mapping> > _mappings;
		sdb::table< module_info, auto_increment_constructor<module_info> > _modules;
		id_t _last_reported_mapping_id;
		unloaded_modules _uqueue;
		module &_module_helper;
		const file_id _this_module_file;
		const std::shared_ptr<void> _module_notifier;
	};

	struct module_tracker::events
	{
		virtual void mapped(id_t mapping_id, id_t module_id, const module::mapping &m) = 0;
		virtual void unmapped(id_t mapping_id) = 0;
	};
}
