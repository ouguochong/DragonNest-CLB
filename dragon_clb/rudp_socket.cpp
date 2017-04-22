#include "rudp_socket.hpp"
#include "field_stage.hpp"

namespace game
{
	namespace network
	{
		rudp_socket::rudp_socket(stage::field_stage* stage)
			: stage(stage)
		{
			memset(&this->direct_addr, 0, sizeof(ADDR));
			memset(&this->detect_addr, 0, sizeof(ADDR));

			this->net_id = 0;
			
			this->udp_ip = 0;
			this->udp_port = 0;

			this->receiver = NULL;
			this->idle = false;
		}

		rudp_socket::~rudp_socket()
		{
			if (this->net_id != NULL)
				this->disconnect();

			rudp_socket_frame::close();
			this->idle = false;
		}

		bool rudp_socket::try_connect(const char* ip, int port)
		{
			return (this->idle = this->try_connect(inet_addr(ip), port));
		}

		bool rudp_socket::try_connect(unsigned int ip, int port)
		{
			if (this->net_id > 0)
				return true;

			sockaddr_in addr;
			addr.sin_addr.S_un.S_addr = ip;

			memcpy(this->direct_addr.ip, &ip, 4);
			this->direct_addr.port = port;

			if (!rudp_socket_frame::open(0))
				return false;

			this->net_id = rudp_socket_frame::connect(inet_ntoa(addr.sin_addr), port);
			this->idle = true;
			return true;
		}
		
		void rudp_socket::disconnect()
		{
			if (this->net_id != 0)
			{
				rudp_socket_frame::disconnect(this->net_id);
				rudp_socket::close();
			}
		}

		void rudp_socket::disconnected(int net_id, bool force, bool unreachable)
		{
			if (this->net_id != 0)
			{
				rudp_socket::close();
				this->net_id = 0;
			}
		}

		bool rudp_socket::send_packet(io_packet& packet)
		{
			unsigned short opcodes = packet.get_header();
			unsigned char* data = packet.get_data() + 2;
			unsigned int size = packet.get_size() - 2;

			int length = this->send_packet(opcodes & 0x00FF, (opcodes & 0xFF00) >> 8, data, size, _RELIABLE);

			return (length > 0);
		}

		int rudp_socket::send_packet(int main_header, int sub_header, void* data, int size, int priority, unsigned char seq_level)
		{
			DNGAME_PACKET packet;
			memset(&packet, 0, sizeof(DNGAME_PACKET));

			int length = 0;

			length = this->encode_game_packet(&packet, main_header, sub_header, data, size, seq_level);
			length = this->send_to(&packet, length, priority);
			return length;
		}

		void rudp_socket::recv_packet(void* data, int size, ADDR* addr)
		{
			if (size <= 0)
				return;

			for (int i = 0, packet_size = 0; i < size; i += packet_size)
			{
				DNGAME_PACKET* packet = reinterpret_cast<DNGAME_PACKET*>(reinterpret_cast<char*>(data) + i);
				packet_size = this->calc_game_packet_size(packet, size - i);

				if (packet_size + i > size)
					break;
				
				if (this->decode_game_packet(packet))
				{			
					/*
					if (m_pReceiver)
						m_pReceiver->RecvData(p);
					*/

					if (this->stage)
					{
						io_packet final_packet(reinterpret_cast<unsigned char*>(packet->data), packet->datasize);
						
						if (!this->stage->handle_packet((packet->sub_header << 8) | packet->header, final_packet))
							return;
					}					
				}
			}
		}

		void rudp_socket::set_receiver(void* receiver)
		{
			this->receiver = receiver;
		}

		void rudp_socket::detect_udp_addr(const char* ip, int port)
		{
			if (port <= 0)
				return;

			if (this->detect_addr.port > 0)
				return;

			unsigned int addr_ip = _inet_addr(ip);
			memcpy(this->detect_addr.ip, &addr_ip, 4);
			this->detect_addr.port = port;

			if (this->detect_addr.port > 0)
			{
				for (int i = 0; this->udp_port == 0 && i < 4000; i += (4000 / 10))
				{
					SOCKADDR_IN addr;
					addr.sin_family = AF_INET;
					addr.sin_port = ntohs(this->detect_addr.port);
					addr.sin_addr.S_un.S_addr = _inet_addr(this->detect_addr.ip);

					rudp_socket_frame::send_to("ping", 4, &addr);
					Sleep(4000 / 10);
				}
			}
		}
		
		void rudp_socket::get_addr(unsigned int* ip, unsigned short* port)
		{
			*ip = this->udp_ip;
			*port = this->udp_port;
		}
		
		bool rudp_socket::accept(int net_id, sockaddr_in* addr)
		{
			/* disable access from the outside */
			return false;
		}
		
		void rudp_socket::recv_data(int net_id, void* data, int length)
		{
			return this->recv_packet(data, length, &this->direct_addr);
		}
		
		int rudp_socket::send_to(void* data, int length, int priority)
		{
			return rudp_socket_frame::send_data(this->net_id, data, length, priority);
		}
		
		bool rudp_socket::accept(int net_id, sockaddr_in* addr, void* buffer, int length)
		{
			if (this->detect_addr.port != 0 && addr->sin_port == ntohs(this->detect_addr.port) && addr->sin_addr.S_un.S_addr == _inet_addr(this->detect_addr.ip))
			{
				typedef struct _RETURN_UDP_ADDR
				{
					unsigned short port[2];
					unsigned long addr[2];
				} RETURN_UDP_ADDR;
				
				RETURN_UDP_ADDR* packet = reinterpret_cast<RETURN_UDP_ADDR*>(buffer);

				if (length == sizeof(*packet) && packet->port[0] == packet->port[1] && packet->addr[0] == packet->addr[1] && this->udp_port == 0)
				{
					this->udp_ip = packet->addr[0];
					this->udp_port = packet->port[0];
				}

				return true;
			}

			return false;
		}
	}
}