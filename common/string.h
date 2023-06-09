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

#include "noncopyable.h"
#include "types.h"

#include <string>

namespace micro_profiler
{
	class locale_lock : noncopyable
	{
	public:
		locale_lock(const char *locale_);
		~locale_lock();

	private:
		std::string _previous;
		bool _locked_ok;
	};

	void unicode(std::string &destination, const char *ansi_value);
	void unicode(std::string &destination, const std::string &ansi_value);
	void unicode(std::string &destination, const wchar_t *value);
	void unicode(std::string &destination, const std::wstring &value);
	std::string unicode(const std::wstring &value);
	std::wstring unicode(const std::string &value);

	std::string to_string(const guid_t &id);
	guid_t from_string(const std::string &text);

	template <typename CharT>
	inline bool replace(std::basic_string<CharT> &text, const std::basic_string<CharT> &what,
		const std::basic_string<CharT> &replacement)
	{
		typename std::basic_string<CharT>::size_type pos = text.find(what);

		if (pos != std::basic_string<CharT>::npos)
		{
			text.replace(pos, what.size(), replacement);
			return true;
		}
		return false;
	}

	inline void unicode(std::string &destination, const std::string &ansi_value)
	{	unicode(destination, ansi_value.c_str());	}

	inline void unicode(std::string &destination, const std::wstring &value)
	{	unicode(destination, value.c_str());	}

	inline std::string unicode(const std::wstring &value)
	{
		std::string result;

		unicode(result, value);
		return result;
	}
}
