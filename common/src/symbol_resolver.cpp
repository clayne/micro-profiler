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

#include <common/symbol_resolver.h>

#include <map>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace
	{
		class ii_symbol_resolver : public symbol_resolver
		{
		public:
			virtual const string &symbol_name_by_va(long_address_t address) const
			{
				cached_names_map::const_iterator i = find_symbol_by_va(address);

				return i != _mapped_symbols.end() ? i->second.name : _empty;
			}

			virtual pair<string, unsigned> symbol_fileline_by_va(long_address_t address) const
			{
				cached_names_map::const_iterator i = find_symbol_by_va(address);

				if (i == _mapped_symbols.end())
					return pair<string, unsigned>();
				files_map::const_iterator j = _files.find(i->second.file_id);

				return make_pair(j->second, i->second.line);
			}

			virtual void add_image(const char *image, long_address_t load_address)
			{
				shared_ptr< image_info<symbol_info> > ii = load_image_info(image);
				unsigned file_id_base = _files.size();

				ii->enumerate_files([&] (const pair<unsigned, string> &file) {
					_files.insert(make_pair(file_id_base + file.first, file.second));
				});
				ii->enumerate_functions([&] (symbol_info symbol) {
					symbol.file_id += file_id_base;
					_mapped_symbols.insert(make_pair(load_address + symbol.rva, symbol));
				});
			}

		private:
			typedef map<unsigned int, string> files_map;
			typedef map<long_address_t, symbol_info> cached_names_map;

		private:
			cached_names_map::const_iterator find_symbol_by_va(long_address_t address) const
			{
				cached_names_map::const_iterator i = _mapped_symbols.lower_bound(address);

				if (i->first > address && i != _mapped_symbols.begin())
					--i;
				return address < i->second.size + i->first ? i : _mapped_symbols.end();
			}

		private:
			string _empty;
			files_map _files;
			cached_names_map _mapped_symbols;
		};
	}

	symbol_info_mapped::symbol_info_mapped(const char *name_, byte_range body_)
		: name(name_), body(body_)
	{	}


	offset_image_info::offset_image_info(const std::shared_ptr< image_info<symbol_info> > &underlying, size_t base)
		: _underlying(underlying), _base((byte *)base)
	{	}

	void offset_image_info::enumerate_functions(const symbol_callback_t &callback) const
	{
		struct local
		{
			static void offset_symbol(const symbol_callback_t &callback, const symbol_info &si, byte *base)
			{
				symbol_info_mapped offset_si(si.name.c_str(), byte_range(base + si.rva, si.size));

				callback(offset_si);
			}
		};
		_underlying->enumerate_functions(bind(&local::offset_symbol, callback, _1, _base));
	}


	shared_ptr<symbol_resolver> symbol_resolver::create()
	{	return shared_ptr<ii_symbol_resolver>(new ii_symbol_resolver());	}
}
