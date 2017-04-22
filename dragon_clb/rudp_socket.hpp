#pragma once

#include "generic.hpp"
#include "io_packet.hpp"

#include "rudp_socket_frame.hpp"
#include "rudp_crypto.hpp"

namespace game
{
	namespace stage
	{
		class field_stage;
	}

	namespace network
	{
		class rudp_socket : public rudp_socket_frame, public crypto::rudp_crypto
		{
		public:
			rudp_socket(stage::field_stage* stage);
			~rudp_socket();

			bool try_connect(const char* ip, int port);
			bool try_connect(unsigned int ip, int port);

			void disconnect();
			void disconnected(int net_id, bool force, bool unreachable);
			
			bool send_packet(io_packet& packet);
			int send_packet(int main_header, int sub_header, void* data, int size, int priority, unsigned char seq_level = 0);

			void recv_packet(void* data, int size, ADDR* addr);
			//bool recv_packets(std::vector<io_packet>& packets);

			void set_receiver(void* receiver);

			void detect_udp_addr(const char* ip, int port);
			void get_addr(unsigned int* ip, unsigned short* port);

		protected:
			bool accept(int net_id, sockaddr_in* addr);
			void recv_data(int net_id, void* data, int length);

		private:
			int send_to(void* data, int length, int priority);
			bool accept(int net_id, sockaddr_in* addr, void* buffer, int length);

		private:
			//struct _TEMP_UDPPACKET
			//{
			//	USHORT nSize;
			//	_ADDR addr;
			//	BYTE cSeq;
			//	DNGAME_PACKET packet;
			//};

			ADDR direct_addr;
			ADDR detect_addr;

			int net_id;

			unsigned int udp_ip;
			unsigned short udp_port;

			void* receiver; // CSeqReceiver
			bool idle;
			
			stage::field_stage* stage;
		};
	}
}