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

#include "db.h"

#include <common/image_info.h>
#include <common/module.h>
#include <common/protocol.h>

namespace micro_profiler
{
	namespace tables
	{
		template <typename T>
		struct record : identity, T
		{	};

		template <typename BaseT>
		struct table : BaseT
		{
			typedef BaseT base_t;

			mutable wpl::signal<void ()> invalidate;
		};


		struct statistics : calls_statistics_table
		{
			std::function<void ()> request_update;
		};


		typedef record<thread_info> thread;
		typedef views::table<thread> threads;


		typedef record<mapped_module_ex> module_mapping;
		typedef views::table<module_mapping> module_mappings;


		struct modules : table< containers::unordered_map<unsigned int /*persistent_id*/, module_info_metadata> >
		{
			typedef std::shared_ptr<void> handle_t;

			typedef std::function<void (const module_info_metadata &metadata)> metadata_ready_cb;
			std::function<void (handle_t &request, unsigned int persistent_id, const metadata_ready_cb &ready)>
				request_presence;

		private:
			using table< containers::unordered_map<unsigned int /*persistent_id*/, module_info_metadata> >::invalidate;
		};


		struct patch
		{
			patch()
				: id(0u)
			{	state.requested = state.error = state.active = 0;	}

			unsigned int id;
			struct
			{
				unsigned int requested : 1,
					error : 1,
					active : 1;
			} state;
		};

		typedef containers::unordered_map<unsigned int /*rva*/, patch> image_patches;

		struct patches : table< containers::unordered_map<unsigned int /*persistent_id*/, image_patches> >
		{
			std::function<void (unsigned int persistent_id, range<const unsigned int, size_t> rva)> apply;
			std::function<void (unsigned int persistent_id, range<const unsigned int, size_t> rva)> revert;
		};
	}
}
