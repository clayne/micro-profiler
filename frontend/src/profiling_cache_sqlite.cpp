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

#include <frontend/profiling_cache_sqlite.h>

#include <frontend/profiling_preferences_db.h>
#include <logger/log.h>
#include <sqlite++/database.h>
#include <tasker/task.h>

#define PREAMBLE "Profiling Cache (sqlite): "

using namespace tasker;
using namespace std;

namespace micro_profiler
{
	profiling_cache_sqlite::profiling_cache_sqlite(const string &preferences_db, queue &worker)
		: _preferences_db(preferences_db), _worker(worker)
	{	}

	void profiling_cache_sqlite::create_database(const string &preferences_db)
	try
	{
		sql::transaction t(sql::create_connection(preferences_db.c_str()));

		t.create_table<tables::module>();
		t.create_table<tables::symbol_info>();
		t.create_table<tables::source_file>();
		t.create_table<tables::cached_patch>();
		t.commit();
		LOG(PREAMBLE "database initialized...");
	}
	catch (...)
	{
	}

	tasker::task<id_t> profiling_cache_sqlite::persisted_module_id(unsigned int hash)
	{	return tasker::task<id_t>(get_cached_module_id_task(hash));	}

	shared_ptr<module_info_metadata> profiling_cache_sqlite::load_metadata(unsigned int hash)
	{
		sql::transaction tx(sql::create_connection(_preferences_db.c_str()));
		auto r_modules = tx.select<tables::module>(sql::c(&tables::module::hash) == sql::p(hash));

		for (auto m = make_shared<tables::module>(); r_modules(*m); )
		{
			auto r_symbols = tx.select<tables::symbol_info>(sql::c(&tables::symbol_info::module_id) == sql::p(m->id));
			auto r_sources = tx.select<tables::source_file>(sql::c(&tables::source_file::module_id) == sql::p(m->id));

			for (tables::symbol_info item; r_symbols(item); )
				m->symbols.push_back(item);
			for (tables::source_file item; r_sources(item); )
				m->source_files[item.id] = item.path;
			return m;
		}
		return nullptr;
	}

	void profiling_cache_sqlite::store_metadata(const module_info_metadata &metadata)
	{
		sql::transaction t(sql::create_connection(_preferences_db.c_str()), sql::transaction::immediate);
		tables::module mm;
		tables::symbol_info s;
		tables::source_file sf;
		auto w_modules = t.insert<tables::module>();
		auto w_symbols = t.insert<tables::symbol_info>();
		auto w_sources = t.insert<tables::source_file>();

		static_cast<module_info_metadata &>(mm) = metadata;
		w_modules(mm);
		sf.module_id = s.module_id = mm.id;
		for (auto i = metadata.symbols.begin(); i != metadata.symbols.end(); ++i)
			static_cast<symbol_info &>(s) = *i, w_symbols(s);
		for (auto i = metadata.source_files.begin(); i != metadata.source_files.end(); ++i)
			sf.id = i->first, sf.path = i->second, w_sources(sf);
		t.commit();

		mt::lock_guard<mt::mutex> l(_mtx);
		const auto i = _module_mapping_tasks.find(mm.hash);
		const auto task = i == _module_mapping_tasks.end()
			? _module_mapping_tasks.insert(make_pair(mm.hash, make_shared< task_node<id_t> >())).first->second
			: i->second;
		try
		{	task->set(move(mm.id)); }
		catch (...)
		{	}
	}

	vector<tables::cached_patch> profiling_cache_sqlite::load_default_patches(id_t cached_module_id)
	{
		typedef tuple<tables::cached_patch, tables::symbol_info> item_type;

		sql::transaction tx(sql::create_connection(_preferences_db.c_str()));
		auto r = tx.select<item_type>(
			sql::c<0>(&tables::cached_patch::rva) == sql::c<1>(&tables::symbol_info::rva)
				&& sql::c<0>(&tables::cached_patch::module_id) == sql::p(cached_module_id)
				&& sql::c<1>(&tables::symbol_info::module_id) == sql::p(cached_module_id)
		);
		vector<tables::cached_patch> result;

		for (item_type item; r(item); )
		{
			result.push_back(get<0>(item));
			result.back().size = get<1>(item).size;
		}
		return move(result);
	}

	void profiling_cache_sqlite::update_default_patches(id_t cached_module_id, vector<unsigned int> add_rva,
		vector<unsigned int> remove_rva)
	{
		sql::transaction tx(sql::create_connection(_preferences_db.c_str()), sql::transaction::immediate);
		tables::cached_patch item = {	0, cached_module_id,	};
		auto w = tx.insert<tables::cached_patch>();
		unsigned rva;
		auto del = tx.remove<tables::cached_patch>(sql::c(&tables::cached_patch::module_id) == sql::p(cached_module_id)
			&& sql::c(&tables::cached_patch::rva) == sql::p(rva));

		for (auto i = begin(add_rva); i != end(add_rva); ++i)
			item.rva = *i, w(item);
		for (auto i = begin(remove_rva); i != end(remove_rva); ++i)
			rva = *i, del.reset(), del.execute();
		tx.commit();
	}

	shared_ptr< tasker::task_node<id_t> > profiling_cache_sqlite::get_cached_module_id_task(unsigned int hash)
	{
		mt::lock_guard<mt::mutex> l(_mtx);
		const auto i = _module_mapping_tasks.find(hash);

		if (i != _module_mapping_tasks.end())
			return i->second;

		const auto preferences_db = _preferences_db;
		const auto task = make_shared< task_node<id_t> >();

		_module_mapping_tasks.insert(make_pair(hash, task));
		_worker.schedule([hash, preferences_db, task] {	find_module(task, preferences_db, hash);	});
		return task;
	}

	void profiling_cache_sqlite::find_module(shared_ptr< tasker::task_node<id_t> > task, const string &preferences_db,
		unsigned int hash)
	{
		sql::transaction tx(sql::create_connection(preferences_db.c_str()));
		auto r_modules = tx.select<tables::module>(sql::c(&tables::module::hash) == sql::p(hash));

		for (tables::module m; r_modules(m); )
		{
			try
			{	task->set(move(m.id));	}
			catch (...)
			{	}
			return;
		}
	}
}
