#include "field_stage.hpp"

#include "opcodes.hpp"
#include "output.hpp"

namespace game
{
	namespace stage
	{
		field_stage::field_stage(dragonnest* client, game::session& session, unsigned short rudp_port)
			: stage_base(client), rudp_port(rudp_port), session(session)
		{
			rudp_socket = std::make_unique<network::rudp_socket>(this);
		}

		field_stage::~field_stage()
		{
			printf("[field_stage] leaving stage\n");
		}
		
		bool field_stage::connect(std::string const& ip, unsigned short tcp_port)
		{
			if (this->rudp_socket.get()->try_connect(ip.c_str(), this->rudp_port))
			{
				if (!this->send_connect_request())//|| !this->send_ready_to_receive())
					return false;

				//return true;
				return this->tcp_socket.get()->try_connect(ip, tcp_port);
			}

			return false;
		}
		
		bool field_stage::send_udp_packet(network::io_packet& packet)
		{
			return this->rudp_socket.get()->send_packet(packet);
		}

		bool field_stage::on_enter_stage()
		{
			printf("[field_stage] entering stage\n");
			return true;
			return this->send_connect_game_request();
		}

		bool field_stage::handle_packet(unsigned short header, network::io_packet& packet)
		{
			switch (header)
			{
			case 0x1702:
			default:
				printf("[field_stage] unhandled packet with header: %04X\n", header);
			}

			return true;
		}
		
		bool field_stage::send_connect_request()
		{
			in_addr local_addr = this->get_local_ip();
			in_addr temp_addr = local_addr;

			local_addr.S_un.S_un_b.s_b1 = temp_addr.S_un.S_un_b.s_b4;
			local_addr.S_un.S_un_b.s_b2 = temp_addr.S_un.S_un_b.s_b3;
			local_addr.S_un.S_un_b.s_b3 = temp_addr.S_un.S_un_b.s_b2;
			local_addr.S_un.S_un_b.s_b4 = temp_addr.S_un.S_un_b.s_b1;

			network::io_packet packet(opcode::out::system::connect_request);
			packet.write4(this->session.session_id());						// nSessionID
			packet.write4(*reinterpret_cast<unsigned int*>(&local_addr));	// nAddrIP
			packet.write2(0x0000);											// nPort

			return this->send_udp_packet(packet);
		}
		
		bool field_stage::send_ready_to_receive()
		{
			network::io_packet packet(opcode::out::system::connect_request);
			packet.write4(this->session.session_id());						// nSessionID
			packet.write_wide_string(inet_ntoa(this->get_local_ip()), 32);	// wszVirtualIp
			
			return this->send_udp_packet(packet);
		}

		bool field_stage::send_connect_game_request()
		{
			network::io_packet packet(opcode::out::system::connect_game);
			packet.write4(this->session.session_id());						// nSessionID
			packet.write4(this->session.account_database_id());				// nAccountDBID
			packet.write8(this->session.certifying_key());					// biCertifyingKey

			return this->send_packet(packet);
		}
	}
}