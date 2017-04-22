#pragma once

#include "generic.hpp"
#include "io_packet.hpp"

#include "rudp_definitions.hpp"
#include "rudp_connection.hpp"

#include <map>
#include <vector>

namespace game
{
	struct _addr_less_ : std::binary_function<sockaddr_in*, sockaddr_in, bool>
	{
		bool operator() (const sockaddr_in* a, const sockaddr_in* b) const 
		{
			if (a->sin_port < b->sin_port)
				return true;

			if (a->sin_port == b->sin_port && a->sin_addr.S_un.S_addr < b->sin_addr.S_un.S_addr)
				return true;

			return false;
		}
	};

	namespace network
	{
		class rudp_socket_frame
		{
			friend class rudp_connection;
			
		public:
			rudp_socket_frame();
			~rudp_socket_frame();

		private:
			static unsigned int __stdcall start_worker_thread(void* param);

			void worker_thread();

		public:
			bool open(unsigned short port);
			void close();
			
			int connect(const char* ip, int port);
			void disconnect(int net_id);
			void disconnect_async(int net_id);

			int send_data(int net_id, void* data, int length, int priority = _RELIABLE);
			
			unsigned int get_thread_id();
			char* get_public_ip();
			char* get_private_ip();
			unsigned int get_current_tick();
	
		protected:			
			virtual bool accept(int net_id, sockaddr_in* addr, void* buffer, int length) = 0;
			virtual void recv_data(int net_id, void* data, int length) { }
			virtual void disconnected(int net_id, bool force, bool unreachable) { }
			virtual void time_event() { }
			
			int send_to(void* data, int length, sockaddr_in* addr);
			int check_packet(void* data, int length, void* output);

		private:
			void get_host_addr();

			rudp_connection* connect(unsigned int ip, int port);
			void disconnect_ptr(rudp_connection* connection, bool force, bool unreachable = false);
			
			void recv_data(void* data, int length, sockaddr_in* addr);

			void check_unreachable(unsigned int current_tick);
			void check_recv_tick(unsigned int current_tick);

			void flush_ack();
			void ping_check();

			bool is_alive();
			
			void enter();
			void leave();
			
		private:
			SOCKET sock;
			bool alive;

			char public_ip[32];
			char private_ip[32];
			unsigned short port;

			CRITICAL_SECTION critical_section;

			HANDLE thread_handle;
			unsigned int thread_id;

			unsigned int last_check_time[3];
			unsigned int current_tick;

			int net_id_count;
			std::map<sockaddr_in*, rudp_connection*, _addr_less_> connect_ref;
			std::map<int, rudp_connection*> connect_list;
			std::vector<std::pair<bool, int>> disconnect_list;
		};
	}
}