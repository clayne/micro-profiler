//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

// Important! Compilation of this file implies NO SSE/SSE2 usage! (/arch:xxx for MSVC compiler)
// Please, check Code Generation settings for this file!

#include <collector/calls_collector_thread.h>

#include <collector/system.h>

namespace micro_profiler
{
	calls_collector_thread::calls_collector_thread(size_t trace_limit)
		: _active_trace(&_traces[0]), _inactive_trace(&_traces[1]), _trace_limit(sizeof(call_record) * trace_limit),
			_proceed_collection(false, true)			
	{
		return_entry re = { };
		_return_stack.push_back(re);
	}

	void calls_collector_thread::on_enter(const void *callee, timestamp_t timestamp, const void **stack_ptr) throw()
	{
		if (_return_stack.back().stack_ptr != stack_ptr)
		{
			// Regular nesting...
			_return_stack.push_back();

			return_entry &e = _return_stack.back();

			e.stack_ptr = stack_ptr;
			e.return_address = *stack_ptr;
		}
		else
		{
			// Tail-call optimization...
			track(0, timestamp);
		}
		track(callee, timestamp);
	}

	const void *calls_collector_thread::on_exit(timestamp_t timestamp) throw()
	{
		const void *return_address = _return_stack.back().return_address;

		_return_stack.pop_back();
		track(0, timestamp);
		return return_address;
	}

	void calls_collector_thread::track(const void *callee, timestamp_t timestamp) throw()
	{
		for (trace_t *trace = 0; ; atomic_store(_active_trace, trace), _proceed_collection.wait())
		{
			do
				trace = atomic_compare_exchange(_active_trace, trace, _active_trace);
			while (!trace);

			if (trace->byte_size() < _trace_limit)
			{
				const call_record record = { timestamp, callee };

				trace->push_back(record);
				atomic_store(_active_trace, trace);
				break;
			}
		}
	}

	void calls_collector_thread::read_collected(const reader_t &reader)
	{
		trace_t *trace;
		trace_t * const active_trace = &_traces[1] == _inactive_trace ? &_traces[0] : &_traces[1];

		do
			trace = atomic_compare_exchange(_active_trace, _inactive_trace, active_trace);
		while (trace != active_trace);

		_inactive_trace = trace;

		if (_inactive_trace->byte_size() >= _trace_limit)
			_proceed_collection.raise();

		if (_inactive_trace->size())
			reader(_inactive_trace->data(), _inactive_trace->size());
		_inactive_trace->clear();
	}
}
