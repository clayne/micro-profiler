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

#include <common/memory.h>
#include <common/range.h>
#include <vector>

namespace micro_profiler
{
#pragma pack(push, 1)
	template <size_t size = sizeof(void*)>
	struct revert_entry : range<byte, byte>
	{
		typedef byte buffer_t[size];

		revert_entry(byte *ptr, byte length);

		void restore() const;

		buffer_t original;
	};
#pragma pack(pop)

	typedef std::vector< revert_entry<> > revert_buffer;



	template <size_t size>
	inline revert_entry<size>::revert_entry(byte *ptr, byte length_)
		: range<byte, byte>(ptr, length_)
	{	mem_copy(original, begin(), length());	}

	template <size_t size>
	inline void revert_entry<size>::restore() const
	{	mem_copy(begin(), original, length());	}
}
