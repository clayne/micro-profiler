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

#include "binding.h"
#include "format.h"
#include "misc.h"
#include "types.h"

#include <cstdint>

namespace micro_profiler
{
	namespace sql
	{
		template <typename T>
		struct record_reader
		{
			template <typename U>
			void operator ()(U)
			{	}

			template <typename U>
			void operator ()(int U::*field, const char *)
			{	record.*field = sqlite3_column_int(&statement, index++);	}

			template <typename U>
			void operator ()(unsigned int U::*field, const char *)
			{	record.*field = static_cast<unsigned int>(sqlite3_column_int(&statement, index++));	}

			template <typename U>
			void operator ()(std::int64_t U::*field, const char *)
			{	record.*field = sqlite3_column_int64(&statement, index++);	}

			template <typename U>
			void operator ()(std::uint64_t U::*field, const char *)
			{	record.*field = static_cast<std::uint64_t>(sqlite3_column_int64(&statement, index++));	}

			template <typename U>
			void operator ()(double U::*field, const char *)
			{	record.*field = sqlite3_column_double(&statement, index++);	}

			template <typename U>
			void operator ()(std::string U::*field, const char *)
			{	record.*field = (const char *)sqlite3_column_text(&statement, index++);	}

			template <typename U, typename F>
			void operator ()(const primary_key<U, F> &field, const char *name)
			{	(*this)(field.field, name);	}

			T &record;
			sqlite3_stmt &statement;
			int index;
		};

		template <typename T>
		class reader
		{
		public:
			reader(statement_ptr &&statement);

			bool operator ()(T& value);

		private:
			statement_ptr _statement;
		};

		template <typename T>
		class select_builder
		{
		public:
			select_builder(const char *table_name);

			template <typename U>
			void operator ()(U);

			template <typename F>
			void operator ()(F field, const char *name);

			reader<T> create_reader(sqlite3 &database) const;

			template <typename W>
			reader<T> create_reader(sqlite3 &database, const W &where) const;

		private:
			int _index;
			std::string _expression_text;
		};



		template <typename T>
		inline reader<T>::reader(statement_ptr &&statement)
			: _statement(std::move(statement))
		{	}

		template <typename T>
		inline bool reader<T>::operator ()(T& record)
		{
			switch (sqlite3_step(_statement.get()))
			{
			case SQLITE_DONE:
				return false;

			case SQLITE_ROW:
				record_reader<T> rr = {	record, *_statement, 0	};

				describe<T>(rr);
				return true;
			}
			throw 0;
		}


		template <typename T>
		inline select_builder<T>::select_builder(const char *table_name)
			: _index(0), _expression_text("SELECT")
		{
			describe<T>(*this);
			_expression_text += " FROM ";
			_expression_text += table_name;
		}

		template <typename T>
		template <typename U>
		inline void select_builder<T>::operator ()(U)
		{	}

		template <typename T>
		template <typename F>
		inline void select_builder<T>::operator ()(F /*field*/, const char *name)
		{
			_expression_text += _index++ ? ',' : ' ';
			_expression_text += name;
		}

		template <typename T>
		inline reader<T> select_builder<T>::create_reader(sqlite3 &database) const
		{	return reader<T>(create_statement(database, _expression_text.c_str()));	}

		template <typename T>
		template <typename W>
		inline reader<T> select_builder<T>::create_reader(sqlite3 &database, const W &where) const
		{
			auto expression_text = _expression_text;

			expression_text += " WHERE ";
			format_expression(expression_text, where);

			auto s = create_statement(database, expression_text.c_str());

			bind_parameters(*s, where);
			return reader<T>(std::move(s));
		}
	}
}