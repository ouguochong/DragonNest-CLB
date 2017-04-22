#include "stage_base.hpp"
#include "dragonnest.hpp"

#include "config.hpp"
#include "opcodes.hpp"

namespace game
{
	namespace stage
	{
		stage_base::stage_base(dragonnest* client) 
			: client(client)
		{
			tcp_socket = std::make_unique<network::tcp_socket>();
		}

		stage_base::~stage_base()
		{

		}
		
		bool stage_base::connect(std::string const& ip, unsigned short tcp_port)
		{
			return this->tcp_socket.get()->try_connect(ip, tcp_port);
		}
		
		bool stage_base::handle_connect()
		{
			return this->on_enter_stage();
		}
		
		bool stage_base::handle_read()
		{
			std::vector<network::io_packet> packets;

			if (!this->recv_packets(packets))
			{
				return false;
			}

			for (network::io_packet& packet : packets)
			{
				if (!this->forward_packet(packet))
				{
					return false;
				}
			}
			
			return true;
		}
		
		SOCKET stage_base::get_desc() const
		{
			return this->tcp_socket.get()->get_desc();
		}

		
		in_addr stage_base::get_local_ip() const
		{
			char local_host_name[80];
			memset(local_host_name, 0, sizeof(local_host_name));

			if (gethostname(local_host_name, sizeof(local_host_name)) != SOCKET_ERROR)
			{
				hostent* host_entry = gethostbyname(local_host_name);

				if (host_entry)
				{
					in_addr addr;
					memset(&addr, 0, sizeof(in_addr));
					memcpy(&addr, host_entry->h_addr_list[0], sizeof(in_addr));
			
					return addr;
				}	
			}
    
			return in_addr();
		}

		bool stage_base::send_packet(network::io_packet& packet)
		{
			return this->tcp_socket.get()->send_packet(packet);
		}

		bool stage_base::recv_packets(std::vector<network::io_packet>& packets)
		{
			return this->tcp_socket.get()->recv_packets(packets);
		}

		bool stage_base::forward_packet(network::io_packet& packet)
		{
			unsigned short header = packet.read2();

			switch (header)
			{
			case opcode::in::system::village_info:
				return this->on_village_info(packet);
					
			case opcode::in::system::game_info:
				return this->on_game_info(packet);

			default:
				break;
			}
			
			return this->handle_packet(header, packet);
		}
			
		bool stage_base::on_village_info(network::io_packet& packet)
		{
			this->client->session().session_id() = packet.read4();

			std::string channel_ip = packet.read_string(32);
			unsigned short channel_port = static_cast<unsigned short>(packet.read4());

			this->client->session().account_database_id() = packet.read4();
			this->client->session().certifying_key() = packet.read8();

			return this->client->migrate(server_type::village, channel_ip, channel_port);
		}
			
		bool stage_base::on_game_info(network::io_packet& packet)
		{
			packet.read4();											// GameTaskType (GameTaskType::eType)
			packet.read1();											// cReqGameIDType
			unsigned int transfer_ip = packet.read4();				// nGameServerIP
			unsigned short rudp_port = packet.read2();				// nGameServerPort
			unsigned short tcp_port = packet.read2();				// nGameServerTcpPort
			packet.read4();											// nRet

			this->client->session().account_database_id() = packet.read4();
			this->client->session().certifying_key() = packet.read8();
           
			char ip_string[16];
			memset(ip_string, 0, sizeof(ip_string));
			
			unsigned char* ip_buffer = reinterpret_cast<unsigned char*>(&transfer_ip);
			sprintf(ip_string, "%i.%i.%i.%i", ip_buffer[0], ip_buffer[1], ip_buffer[2], ip_buffer[3]);
	
			return this->client->migrate(server_type::field, std::string(ip_string), tcp_port, rudp_port);
		}
	}
}