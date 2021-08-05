//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "dynamic_hooking.h"
#include "jumper.h"

#include <common/memory.h>

namespace micro_profiler
{
	class function_patch : noncopyable
	{
	public:
		template <typename T>
		function_patch(executable_memory_allocator &allocator, byte_range body, T *interceptor);

	private:
		std::shared_ptr<void> _trampoline;
		jumper _jumper;
	};



	template <typename T>
	inline function_patch::function_patch(executable_memory_allocator &allocator, byte_range body, T *interceptor)
		: _trampoline(allocator.allocate(c_trampoline_size)), _jumper(body.begin(), _trampoline.get())
	{
		initialize_trampoline(_trampoline.get(), _jumper.entry(), body.begin(), interceptor);
		_jumper.activate(true);
	}
}
