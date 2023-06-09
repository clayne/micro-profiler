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

#include "server_endpoint_sockets.h"

#include <arpa/inet.h>
#include <common/noncopyable.h>
#include <fcntl.h>
#include <logger/log.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/types.h>

#pragma warning(disable: 4127)
#pragma warning(disable: 4389)

#define PREAMBLE "IPC socket server: "

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace ipc
	{
		namespace sockets
		{
			namespace
			{
				void setup_socket(const socket_handle &s)
				{
					linger l = {};

					l.l_onoff = 1;
					if (::setsockopt(s, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(l)) < 0)
						throw initialization_failed("setsockopt(..., SO_LINGER, ...) failed");
				}

				template <typename SocketT, typename T>
				int send_scalar(const SocketT &s, T value)
				{
					byte_representation<T> data;

					data.value = value;
					data.reorder();
					return ::send(s, data.bytes, sizeof(data.bytes), MSG_NOSIGNAL);
				}

				template <typename T>
				int recv_scalar(const socket_handle &s, T &value, int options)
				{
					byte_representation<T> data;
					int result = ::recv(s, data.bytes, sizeof(data.bytes), options);

					data.reorder();
					value = data.value;
					return result;
				}

				socket_handler::status dummy(socket_handler &, const socket_handle &)
				{	return socket_handler::proceed;	}
			}

			socket_handler::socket_handler(unsigned id_, socket_handle &s, const socket_handle &aux_socket,
					const handler_t &initial_handler)
				: id(id_), handler(initial_handler), _socket(s), _aux_socket(aux_socket)
			{	}

			socket_handler::~socket_handler() throw()
			{
				handler = handler_t(); // first - release the underlying session...
				_socket.reset(); // ... then - the socket
			}

			template <typename ContainerT>
			void socket_handler::run(ContainerT &handlers)
			try
			{
				LOG(PREAMBLE "processing thread started.");
				for (;;)
				{
					fd_set fds;

					FD_ZERO(&fds);
					for (auto i = handlers.begin(); i != handlers.end(); ++i)
						FD_SET((*i)->_socket, &fds);
					if (::select(FD_SETSIZE, &fds, NULL, NULL, NULL) < 0)
						return;
					for (auto i = handlers.begin(); i != handlers.end(); )
					{
						switch(FD_ISSET((*i)->_socket, &fds) ? (*i)->handler(**i, (*i)->_socket) : proceed)
						{
						case proceed:
							++i;
							break;

						case remove_this:
							i = handlers.erase(i);
							break;

						case exit:
							LOG(PREAMBLE "processing thread ended...") % A(handlers.size());
							return;
						}
					}
				}
			}
			catch (exception &e)
			{
				LOGE(PREAMBLE "processing failed.") % A(e.what());
			}
			catch (...)
			{
				LOGE(PREAMBLE "processing failed (unknown reason).");
			}

			void socket_handler::disconnect() throw()
			{	send_scalar(_aux_socket, id);	}

			void socket_handler::message(const_byte_range payload)
			{
				unsigned int size = static_cast<unsigned int>(payload.length());

				send_scalar(_socket, size);
				::send(_socket, reinterpret_cast<const char *>(payload.begin()), size, MSG_NOSIGNAL);
			}


			server::server(const char *endpoint_id, const shared_ptr<ipc::server> &factory)
				: _factory(factory), _aux_socket(0), _next_id(100)
			{
				socket_handle server_socket(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
				host_port hp(endpoint_id);
				sockaddr_in service = make_sockaddr_in(hp.host.c_str(), hp.port);

				setup_socket(server_socket);
				if (::bind(server_socket, (sockaddr *)&service, sizeof(service)))
					throw initialization_failed("bind() failed");
				if (::listen(server_socket, max_backlog))
					throw initialization_failed("listen() failed");

				_aux_socket.reset(connect_aux(hp.port));
				_handlers.push_back(socket_handler::ptr_t(new socket_handler(_next_id++, server_socket,
					_aux_socket, bind(&server::accept_preinit, this, _2))));
				_server_thread.reset(new mt::thread(bind(&socket_handler::run<handlers_t>, ref(_handlers))));
			}

			server::~server()
			{
				send_scalar(_aux_socket, 0u);
				_server_thread->join();
			}

			socket_handler::status server::handle_preinit(socket_handler &h, const socket_handle &s)
			{
				unsigned int magic;

				if (sizeof(magic) == recv_scalar(s, magic, MSG_PEEK) && magic == init_magic)
				{
					recv_scalar(s, magic, MSG_WAITALL);
					for (auto i = _handlers.begin(); i != _handlers.end(); ++i)
					{
						socket_handler::handler_t &handler = (*i)->handler;

						if (i == _handlers.begin()) // acceptor, guaranteed to be the first
							handler = bind(&server::accept_regular, this, _2);
						else if (i->get() == &h) // aux handler
							handler = bind(&server::handle_aux, this, _2);
						else // regular session handler
							handler = bind(&server::handle_session, this, _2, _factory->create_session(**i));
					}
				}
				return socket_handler::proceed;
			}

			socket_handler::status server::accept_preinit(const socket_handle &s)
			{
				socket_handle new_connection(::accept(s, NULL, NULL));

				setup_socket(new_connection);
				_handlers.push_back(socket_handler::ptr_t(new socket_handler(_next_id++, new_connection,
					_aux_socket, bind(&server::handle_preinit, this, _1, _2))));
				return socket_handler::proceed;
			}

			socket_handler::status server::accept_regular(const socket_handle &s)
			{
				socket_handle new_connection(::accept(s, NULL, NULL));

				setup_socket(new_connection);

				socket_handler::ptr_t h(new socket_handler(_next_id++, new_connection, _aux_socket, &dummy));

				h->handler = bind(&server::handle_session, this, _2, _factory->create_session(*h));
				_handlers.push_back(h);
				return socket_handler::proceed;
			}

			socket_handler::status server::handle_session(const socket_handle &s, const channel_ptr_t &inbound)
			{
				unsigned int size;

				if (recv_scalar(s, size, MSG_WAITALL) > 0)
				{
					_buffer.resize(size);
					::recv(s, reinterpret_cast<char *>(&_buffer[0]), size, MSG_WAITALL);
					inbound->message(const_byte_range(&_buffer[0], _buffer.size()));
					return socket_handler::proceed;
				}
				else
				{
					inbound->disconnect();
				}
				return socket_handler::remove_this;
			}

			socket_handler::status server::handle_aux(const socket_handle &s)
			{
				unsigned id;

				if (recv_scalar(s, id, MSG_WAITALL) > 0)
					for (auto i = _handlers.begin(); i != _handlers.end(); ++i)
						if ((*i)->id == id)
						{
							_handlers.erase(i);
							return socket_handler::proceed;
						}
				return socket_handler::exit;
			}

			int server::connect_aux(unsigned short port)
			{
				sockaddr_in service = make_sockaddr_in(c_localhost, port);
				int s = static_cast<int>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
				u_long arg = 1 /* nonblocking */;

				if (-1 == s)
					throw initialization_failed("aux socket creation failed");
				if (::connect(s, (sockaddr *)&service, sizeof(service)))
					throw (::close(s), initialization_failed("aux socket connection failed"));
				::ioctl(s, O_NONBLOCK, &arg);
				send_scalar(s, init_magic);
				return s;
			}


			shared_ptr<void> run_server(const char *endpoint_id, const shared_ptr<ipc::server> &factory)
			{	return shared_ptr<void>(new server(endpoint_id, factory));}
		}
	}
}
