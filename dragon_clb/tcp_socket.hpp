#pragma once

#include "generic.hpp"
#include "io_packet.hpp"

namespace game
{
	namespace network
	{
		class tcp_socket
		{
		public:
			tcp_socket();
			~tcp_socket();

			bool try_connect(std::string const& ip, unsigned short port);
			bool try_disconnect();

			bool recv_packets(std::vector<io_packet>& packets);
			bool send_packet(io_packet& packet);

			SOCKET get_desc() const;

		protected:
			bool raw_recv(unsigned char* buffer, std::size_t size);
			bool raw_send(unsigned char const* buffer, std::size_t size);

		private:
			SOCKET sock;

			unsigned int recv_offset;
			unsigned char recv_buffer[1024 * 16];
		};
	}
}