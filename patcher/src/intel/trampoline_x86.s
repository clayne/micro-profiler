#	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
#
#	Permission is hereby granted, free of charge, to any person obtaining a copy
#	of this software and associated documentation files (the "Software"), to deal
#	in the Software without restriction, including without limitation the rights
#	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#	copies of the Software, and to permit persons to whom the Software is
#	furnished to do so, subject to the following conditions:
#
#	The above copyright notice and this permission notice shall be included in
#	all copies or substantial portions of the Software.
#
#	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#	THE SOFTWARE.

.text
	.globl micro_profiler_trampoline_proto, micro_profiler_trampoline_proto_end
	.globl _micro_profiler_trampoline_proto, _micro_profiler_trampoline_proto_end

	micro_profiler_trampoline_proto:	# argument passing: RDI, RSI, RDX, RCX, R8, and R9, <stack>
	_micro_profiler_trampoline_proto:
		push	%ecx
		push	%edx
		rdtsc
		mov	$0x31415901, %ecx # 1st argument, interceptor
		push	$0x31415902 # 4th argument, callee
		push	%edx
		push	%eax # 3rd argument, timestamp
		lea	0x14(%esp), %edx # 2nd argument, stack_ptr
		call	on_enter + 0x31415983 # on_enter address (displacement)
	on_enter:
		pop	%edx
		pop	%ecx

		lea	0x04(%esp), %esp
		call	trampoline_proto_end
		lea	-0x04(%esp), %esp

		push	%eax
		rdtsc
		mov	$0x31415901, %ecx # 1st argument, interceptor
		push	%edx
		push	%eax # 3rd argument, timestamp
		lea	0x0C(%esp), %edx # 2nd argument, stack_ptr
		call	on_exit + 0x31415984 # on_exit address (displacement)
	on_exit:
		mov	%eax, 0x04(%esp) # restore return address
		pop	%eax
		ret
	trampoline_proto_end:
	micro_profiler_trampoline_proto_end:
	_micro_profiler_trampoline_proto_end:
