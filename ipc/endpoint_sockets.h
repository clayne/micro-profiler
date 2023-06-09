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

#include "endpoint.h"

#include <algorithm>
#include <functional>

namespace micro_profiler
{
	namespace ipc
	{
		namespace sockets
		{
			channel_ptr_t connect_client(const char *destination_endpoint_id, channel &inbound);
			std::shared_ptr<void> run_server(const char *endpoint_id, const std::shared_ptr<server> &factory);


			template <typename T>
			union byte_representation
			{
				char bytes[sizeof(T)];
				T value;

				void reorder();
			};



			template <typename T>
			inline void byte_representation<T>::reorder()
			{
				byte_representation<unsigned> order;

				order.value = 0xFF;
				if (order.bytes[0])
				{
					for (auto i = 0u; i < sizeof(T) / 2; ++i)
						std::swap(bytes[i], bytes[sizeof(T) - 1 - i]);
				}
			}
		}
	}
}
