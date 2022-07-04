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

namespace micro_profiler
{
	typedef views::table<long_address_t> address_table;
	typedef std::shared_ptr<const address_table> address_table_cptr;
	typedef views::table<id_t> selector_table;
	typedef std::shared_ptr<const selector_table> selector_table_cptr;
	typedef std::shared_ptr<selector_table> selector_table_ptr;

	struct derived_statistics
	{
		static address_table_cptr addresses(selector_table_cptr selection_, calls_statistics_table_cptr hierarchy);
		static calls_statistics_table_cptr callers(address_table_cptr callees, calls_statistics_table_cptr hierarchy);
		static calls_statistics_table_cptr callees(address_table_cptr callers, calls_statistics_table_cptr hierarchy);
	};
}