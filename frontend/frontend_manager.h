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

#include <common/noncopyable.h>
#include <common/types.h>
#include <ipc/endpoint.h>
#include <memory>
#include <list>
#include <string>
#include <wpl/base/signals.h>

namespace micro_profiler
{
	class functions_list;
	struct symbol_resolver;

	class frontend : public ipc::channel, noncopyable
	{
	public:
		frontend(ipc::channel &outbound);
		~frontend();

		void disconnect_session() throw();

	public:
		std::function<void(const std::wstring &process_name, const std::shared_ptr<functions_list> &model)> initialized;
		std::function<void()> released;

	private:
		// ipc::channel methods
		virtual void disconnect() throw();
		virtual void message(const_byte_range payload);

	private:
		ipc::channel &_outbound;
		std::shared_ptr<symbol_resolver> _resolver;
		std::shared_ptr<functions_list> _model;
	};

	struct frontend_ui
	{
		typedef std::shared_ptr<frontend_ui> ptr;

		virtual void activate() = 0;

		wpl::signal<void()> activated;
		wpl::signal<void()> closed;
	};

	class frontend_manager : public ipc::session_factory
	{
	public:
		struct instance
		{
			std::wstring executable;
			std::shared_ptr<functions_list> model;
			frontend_ui::ptr ui;
		};

		typedef std::function<frontend_ui::ptr(const std::shared_ptr<functions_list> &model,
			const std::wstring &executable)> frontend_ui_factory;
		typedef std::shared_ptr<frontend_manager> ptr;

	public:
		static std::shared_ptr<frontend_manager> create(const frontend_ui_factory &ui_factory);

		void close_all() throw();

		size_t instances_count() const throw();
		const instance *get_instance(unsigned index) const throw();
		const instance *get_active() const throw();
		void create_instance(const std::wstring &executable, const std::shared_ptr<functions_list> &model);

		// ipc::session_factory methods
		virtual std::shared_ptr<ipc::channel> create_session(ipc::channel &outbound);

	private:
		struct instance_impl : instance
		{
			instance_impl(micro_profiler::frontend *frontend_);

			micro_profiler::frontend *frontend;
			wpl::slot_connection ui_activated_connection;
			wpl::slot_connection ui_closed_connection;
		};

		typedef std::list<instance_impl> instance_container;

	private:
		frontend_manager(const frontend_ui_factory &ui_factory);

		void on_frontend_released(instance_container::iterator i) throw();
		void on_ready_for_ui(instance_container::iterator i, const std::wstring &executable,
			const std::shared_ptr<functions_list> &model);

		void on_ui_activated(instance_container::iterator i);
		void on_ui_closed(instance_container::iterator i) throw();

		void addref() throw();
		void release() throw();
		static void destroy(frontend_manager *p);

	private:
		unsigned _references;
		frontend_ui_factory _ui_factory;
		instance_container _instances;
		const instance_impl *_active_instance;
	};
}
