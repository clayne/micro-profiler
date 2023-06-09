//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include <frontend/headers_model.h>

#include <common/configuration.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		const char c_order_by[] = "OrderBy";
		const char c_order_direction[] = "OrderDirection";
		const char c_width[] = "Width";
//		const char c_caption[] = "Caption";

		template <typename T>
		bool load_int(const hive &configuration, const char *name, T &value)
		{
			int stored;

			return configuration.load(name, stored) ? value = static_cast<T>(stored), true : false;
		}

		bool load_int(const hive &configuration, const char *name, bool &value)
		{
			int stored;

			return configuration.load(name, stored) ? value = !!stored, true : false;
		}
	}

	void headers_model::store(hive &configuration) const
	{
		configuration.store(c_order_by, _sort_column != npos() ? static_cast<int>(_sort_column) : -1);
		configuration.store(c_order_direction, _sort_ascending ? 1 : 0);
		for (auto i = _columns.begin(); i != _columns.end(); ++i)
		{
			auto cc = configuration.create(i->id.c_str());

			cc->store(c_width, i->width);
//			cc->store(c_caption, unicode(i->caption).c_str());
		}
	}

	void headers_model::update(const hive &configuration)
	{
//		string tmp;

		load_int(configuration, c_order_by, _sort_column);
		_sort_column = _sort_column < static_cast<index_type>(_columns.size()) ? _sort_column : npos();
		load_int(configuration, c_order_direction, _sort_ascending);
		for (auto i = _columns.begin(); i != _columns.end(); ++i)
			if (auto cc = configuration.open(i->id.c_str()))
			{
				load_int(*cc, c_width, i->width);
//				if (cc->load(c_caption, tmp))
//					i->caption = unicode(tmp);
			}
	}
	
	headers_model::index_type headers_model::get_count() const throw()
	{	return static_cast<index_type>(_columns.size());	}

	void headers_model::get_value(index_type index, short int &width) const throw()
	{	width = _columns[index].width;	}

	pair<headers_model::index_type, bool> headers_model::get_sort_order() const throw()
	{	return make_pair(_sort_column, _sort_ascending);	}

	void headers_model::set_width(index_type item, short int width)
	{
		_columns[item].width = width;
		invalidate(item);
	}

	agge::full_alignment headers_model::get_alignment(index_type index) const
	{	return agge::full_alignment::create(_columns[index].alignment, agge::align_center);	}

	agge::full_alignment headers_model::get_header_alignment(index_type index) const
	{	return agge::full_alignment::create(_columns[index].alignment, agge::align_near);	}

	void headers_model::get_caption(index_type item, agge::richtext_t &caption) const
	{	caption << _columns[item].caption;	}

	void headers_model::activate_column(index_type column_)
	{
		const auto &activated = _columns[column_];

		if (activated.compare)
		{
			_sort_ascending = _sort_column == column_ ? !_sort_ascending : activated.ascending;
			_sort_column = column_;
			sort_order_changed(_sort_column, _sort_ascending);
		}
	}
}
