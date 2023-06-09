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

#include <common/smart_ptr.h>
#include <common/types.h>

namespace micro_profiler
{
	template <typename T, typename U>
	inline std::shared_ptr<T> make_bound(const std::shared_ptr<U> &underlying)
	{
		const auto p = make_shared_copy(std::make_pair(underlying, T(*underlying)));
		return make_shared_aspect(p, &p->second);
	}

	template <typename T>
	inline T initialize()
	{
		T value = {	};
		return value;
	}

	template <typename T, typename F1>
	inline T initialize(const F1 &field1)
	{
		T value = {	field1,	};
		return value;
	}

	template <typename T, typename F1, typename F2>
	inline T initialize(const F1 &field1, const F2 &field2)
	{
		T value = {	field1, field2,	};
		return value;
	}

	template <typename T, typename F1, typename F2, typename F3>
	inline T initialize(const F1 &field1, const F2 &field2, const F3 &field3)
	{
		T value = {	field1, field2, field3,	};
		return value;
	}

	template <typename T, typename F1, typename F2, typename F3, typename F4>
	inline T initialize(const F1 &field1, const F2 &field2, const F3 &field3, const F4 &field4)
	{
		T value = {	field1, field2, field3, field4,	};
		return value;
	}

	template <typename T, typename F1, typename F2, typename F3, typename F4, typename F5>
	inline T initialize(const F1 &field1, const F2 &field2, const F3 &field3, const F4 &field4, const F5 &field5)
	{
		T value = {	field1, field2, field3, field4, field5,	};
		return value;
	}

	template <typename T, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6, typename F7>
	inline T initialize(const F1 &field1, const F2 &field2, const F3 &field3, const F4 &field4, const F5 &field5, const F6 &field6, const F7 &field7)
	{
		T value = {	field1, field2, field3, field4, field5, field6, field7,	};
		return value;
	}
}
