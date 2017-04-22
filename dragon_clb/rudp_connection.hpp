#pragma once

#include "generic.hpp"

#include "rudp_definitions.hpp"

#include <list>
#include <vector>

namespace game
{
	namespace network
	{
		class rudp_socket_frame;

		class rudp_connection
		{
		public:
			rudp_connection(unsigned int ip, int port, int net_id);
			~rudp_connection();

			bool check_unreachable(unsigned int time_tick, rudp_socket_frame* sock);
			bool check_recv_tick(unsigned int current_tick);

			void flush_ack(rudp_socket_frame* sock);
			void ping_check(rudp_socket_frame* sock);
			
			int send_data(void* data, int length, int priority, rudp_socket_frame* sock);
			void recv_data(void* data, int length, rudp_socket_frame* sock);

			int get_id() const;
			sockaddr_in* get_addr();

			int get_send_queue_size();
			
			static unsigned char get_crc(const unsigned char* data, int length);

		private:
			void send_ack(int flags, int ack, sockaddr_in* addr, rudp_socket_frame* sock);
			
			bool resend_queue(unsigned short type, unsigned short* ack_list, int ack_num, rudp_socket_frame* sock);
			bool remove_queue(unsigned short type, unsigned short* ack_list, int ack_num);

			PACKET_QUEUE* create_queue(rudp_socket_frame* sock, void* buffer, int length, int type = 0);

			void release_queue(PACKET_QUEUE* queue);

			static int mem_level(int size);

		private:
			sockaddr_in target_addr;
			int net_id;

			unsigned char unreachable_count;
			unsigned int rto_tick_counter;

			unsigned short ack_list[3][16];
			int ack_list_count[3];

			std::list<PACKET_QUEUE*> send_queue;
			std::list<PACKET_QUEUE*> recv_queue;

			std::vector<PACKET_QUEUE*> empty_queue[7]; // 0~31, 32~63, 64~127, 128~255, 256~511, 512~1023, 1024~2047
			std::vector<char*> alloced_queue;

			std::list<unsigned short> ack_queue;

			unsigned int rto_tick;

			unsigned short seq_count;
			unsigned short ack_count;
			
			unsigned short recv_seq_count;
			unsigned short recv_ack_count;

			unsigned int recv_tick;

			unsigned int last_check_time;
			unsigned int send_bytes;
			unsigned int count;
		};
	}
}