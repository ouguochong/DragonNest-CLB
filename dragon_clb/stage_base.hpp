#pragma once

#include "generic.hpp"
#include "io_packet.hpp"

#include "tcp_socket.hpp"

namespace game
{
	class dragonnest;

	namespace stage
	{
		class stage_base
		{
		public:
			stage_base(dragonnest* client);
			virtual ~stage_base();

			virtual bool connect(std::string const& ip, unsigned short tcp_port);
			
			bool handle_connect();
			bool handle_read();

			SOCKET get_desc() const;
			in_addr get_local_ip() const;

		protected:
			bool send_packet(network::io_packet& packet);
			bool recv_packets(std::vector<network::io_packet>& packets);
			
			virtual bool on_enter_stage() = 0;
			virtual bool handle_packet(unsigned short header, network::io_packet& packet) = 0;
			
			bool forward_packet(network::io_packet& packet);
			
		private:
			bool on_village_info(network::io_packet& packet);
			bool on_game_info(network::io_packet& packet);
			
		protected:
			std::unique_ptr<network::tcp_socket> tcp_socket;

		private:
			dragonnest* client;
		};
	}
}