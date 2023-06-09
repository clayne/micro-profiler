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

#include "constructors.h"
#include "primitives.h"

#include <common/auto_increment.h>
#include <common/hash.h>
#include <tuple>
#include <sdb/integrated_index.h>

namespace sdb
{
	template <typename Table1T, typename Table2T>
	class joined_record;

	template <typename T>
	class hash< std::vector<T> >
	{
	public:
		std::size_t operator ()(const std::vector<T> &key) const
		{
			std::size_t v = h((T()));

			for (auto i = key.begin(); i != key.end(); ++i)
				v = h(v, h(*i));
			return v;
		}

	private:
		micro_profiler::knuth_hash h;
	};
}

namespace micro_profiler
{
	typedef std::tuple<id_t /*thread_id*/, id_t /*parent_id*/, long_address_t> call_node_key;
	typedef sdb::table< call_statistics, auto_increment_constructor<call_statistics> > calls_statistics_table;
	typedef std::shared_ptr<const calls_statistics_table> calls_statistics_table_cptr;
	typedef std::vector<long_address_t> callstack_key;

	namespace keyer
	{
		template <typename K1, typename K2>
		struct combine2
		{
			template <typename R>
			std::tuple<
				typename sdb::result<K1, R>::type,
				typename sdb::result<K2, R>::type
			> operator ()(const R &record) const
			{	return std::make_tuple(keyer1(record), keyer2(record));	}

			template <typename IndexT, typename R, typename KeyT>
			void operator ()(IndexT &index, R &record, const KeyT &key) const
			{	keyer1(index, record, std::get<0>(key)), keyer2(index, record, std::get<1>(key));	}

			K1 keyer1;
			K2 keyer2;
		};

		template <typename K1, typename K2, typename K3>
		struct combine3
		{
			template <typename R>
			std::tuple<
				typename sdb::result<K1, R>::type,
				typename sdb::result<K2, R>::type,
				typename sdb::result<K3, R>::type
			> operator ()(const R &record) const
			{	return std::make_tuple(keyer1(record), keyer2(record), keyer3(record));	}

			template <typename IndexT, typename R, typename KeyT>
			void operator ()(IndexT &index, R &record, const KeyT &key) const
			{	keyer1(index, record, std::get<0>(key)), keyer2(index, record, std::get<1>(key)), keyer3(index, record, std::get<2>(key));	}

			K1 keyer1;
			K2 keyer2;
			K3 keyer3;
		};

		struct id
		{
			template <typename T>
			id_t operator ()(const T &record) const
			{	return record.id;	}

			template <typename Table1T, typename Table2T>
			id_t operator ()(const sdb::joined_record<Table1T, Table2T> &record) const
			{	return (*this)(record.left());	}
		};

		struct external_id : id
		{
			using id::operator ();

			template <typename IndexT, typename T, typename K>
			void operator ()(IndexT &, T &record, const K &key) const
			{	record.id = key;	}
		};

		struct parent_id
		{
			id_t operator ()(const call_statistics &record) const
			{	return record.parent_id;	}

			template <typename IndexT>
			void operator ()(IndexT &, call_statistics &record, id_t key) const
			{	record.parent_id = key;	}
		};

		struct module_id
		{
			template <typename T>
			id_t operator ()(const T &record) const
			{	return record.module_id;	}

			template <typename IndexT, typename T>
			void operator ()(IndexT &, T &record, id_t key) const
			{	record.module_id = key;	}
		};

		struct rva
		{
			template <typename T>
			unsigned int operator ()(const T &record) const
			{	return record.rva;	}

			template <typename IndexT, typename T>
			void operator ()(IndexT &, T &record, unsigned int key) const
			{	record.rva = key;	}
		};

		struct thread_id
		{
			id_t operator ()(const call_statistics &record) const
			{	return record.thread_id;	}

			template <typename IndexT>
			void operator ()(IndexT &, call_statistics &record, id_t key) const
			{	record.thread_id = key;	}
		};

		struct base
		{
			template <typename T>
			long_address_t operator ()(const T &record) const
			{	return record.base;	}
		};

		struct address
		{
			long_address_t operator ()(const call_statistics &record) const
			{	return record.address;	}

			template <typename IndexT>
			void operator ()(IndexT &, call_statistics &record, long_address_t key) const
			{	record.address = key;	}
		};

		struct parent_address
		{
			parent_address(const calls_statistics_table &hierarchy)
				: _by_id(sdb::unique_index<id>(hierarchy))
			{	}

			long_address_t operator ()(const call_statistics& record) const
			{
				if (auto parent = _by_id.find(record.parent_id))
					return parent->address;
				return long_address_t();
			}

		private:
			const sdb::immutable_unique_index<calls_statistics_table, id> &_by_id;
		};

		template <typename TableT>
		class callstack
		{
		public:
			callstack(const TableT &underlying)
				: _by_id(sdb::unique_index<id>(underlying))
			{	}

			template <typename T>
			const callstack_key &operator ()(const T &record) const
			{
				_key_buffer.clear();
				key_append(record);
				return _key_buffer;
			}

			template <typename IndexT, typename T>
			void operator ()(const IndexT &index, T &record, callstack_key key) const
			{
				record.thread_id = 0;
				record.address = key.back();
				key.pop_back();
				record.parent_id = key.empty() ? id_t() : index[key].id;
			}

		private:
			template <typename T>
			inline void key_append(const T &record) const
			{
				if (const auto parent_id = record.parent_id)
					key_append(_by_id[parent_id]);
				_key_buffer.push_back(record.address);
			}

		private:
			const sdb::immutable_unique_index<TableT, id> &_by_id;
			mutable callstack_key _key_buffer;
		};


		typedef combine3<thread_id, parent_id, address> callnode;
		typedef combine2<module_id, rva> symbol_id;
	}
}
