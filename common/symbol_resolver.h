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

#pragma once

#include "range.h"

#include <functional>
#include <memory>
#include <string>

namespace micro_profiler
{
	struct module_info_basic;
	struct module_info_metadata;

	struct symbol_info
	{
		std::string name;
		unsigned int rva, size;
		unsigned int id;
		unsigned int file_id, line;
	};

	struct symbol_info_mapped
	{
		symbol_info_mapped(const char *name_, byte_range body_);

		std::string name;
		byte_range body;
	};

	template <typename SymbolT>
	struct image_info
	{
		typedef std::function<void (const SymbolT &symbol)> symbol_callback_t;
		typedef std::function<void (const std::pair<unsigned /*file_id*/, std::string /*path*/> &file)> file_callback_t;

		virtual ~image_info() {	}
		virtual void enumerate_functions(const symbol_callback_t &callback) const = 0;
		virtual void enumerate_files(const file_callback_t &/*callback*/) const {	}
	};

	std::shared_ptr< image_info<symbol_info> > load_image_info(const char *image_path);

	class offset_image_info : public image_info<symbol_info_mapped>
	{
	public:
		offset_image_info(const std::shared_ptr< image_info<symbol_info> > &underlying, size_t base);

		virtual void enumerate_functions(const symbol_callback_t &callback) const;

	private:
		std::shared_ptr< image_info<symbol_info> > _underlying;
		byte *_base;
	};

	struct symbol_resolver
	{
		typedef std::pair<std::string, unsigned> fileline_t;

		virtual ~symbol_resolver()	{	}
		virtual const std::string &symbol_name_by_va(long_address_t address) const = 0;
		virtual bool symbol_fileline_by_va(long_address_t address, fileline_t &fileline) const = 0;
		virtual void add_metadata(const module_info_basic &basic, const module_info_metadata &metadata) = 0;

		static std::shared_ptr<symbol_resolver> create();
	};
}
