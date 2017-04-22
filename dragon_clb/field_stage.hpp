#pragma once

#include "generic.hpp"
#include "stage_base.hpp"

#include "rudp_socket.hpp"

#include "dragonnest.hpp"

#include "session.hpp"

namespace game
{
	namespace stage
	{
		class field_stage : public stage_base
		{
			friend class network::rudp_socket;

		public:
			field_stage(dragonnest* client, session& session, unsigned short rudp_port);
			~field_stage();
			
			bool connect(std::string const& ip, unsigned short tcp_port);

		protected:
			bool send_udp_packet(network::io_packet& packet);

			bool on_enter_stage();
			bool handle_packet(unsigned short header, network::io_packet& packet);

		private:
			bool send_connect_request();
			bool send_ready_to_receive();

			bool send_connect_game_request();

		private:
			unsigned short rudp_port;
			std::unique_ptr<network::rudp_socket> rudp_socket;

			session session;
		};
	}
}