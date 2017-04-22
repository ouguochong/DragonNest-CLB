#include "rudp_socket_frame.hpp"

#include "main_form.hpp"

#include <process.h>
#pragma comment(lib, "winmm")

#include <iprtrmib.h>

#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi")

#define CXIP_A(IP)	((IP & 0xFF000000) >> 24)
#define CXIP_B(IP)	((IP & 0x00FF0000) >> 16)
#define CXIP_C(IP)	((IP & 0x0000FF00) >> 8)
#define CXIP_D(IP)	(IP & 0x000000FF)

namespace game
{
	namespace network
	{
		static unsigned long get_time_tick()
		{
			static int pivot = timeGetTime();
			return timeGetTime() - pivot;
		}

		const unsigned int header_length = 3;

		rudp_socket_frame::rudp_socket_frame()
		{
			this->sock = INVALID_SOCKET;
			this->alive = true;
			
			memset(this->public_ip, 0, sizeof(this->public_ip));
			memset(this->private_ip, 0, sizeof(this->private_ip));
			this->port = 0;
			
			InitializeCriticalSection(&this->critical_section);

			this->thread_handle = NULL;
			this->thread_id = 0;
			
			memset(this->last_check_time, 0, sizeof(this->last_check_time));
			this->current_tick = 0;

			this->net_id_count = 1;

			get_time_tick();
		}

		rudp_socket_frame::~rudp_socket_frame()
		{
			this->close();
		}

		unsigned int __stdcall rudp_socket_frame::start_worker_thread(void* param)
		{
			reinterpret_cast<rudp_socket_frame*>(param)->worker_thread();
			return 0;
		}
		
		void rudp_socket_frame::worker_thread()
		{
			while (this->alive)
			{
				timeval timeout = { 0, _SELECT_TIMEOUT_SRV };

				fd_set fdset;
				FD_ZERO(&fdset);
				FD_SET(this->sock, &fdset);

				this->current_tick = get_time_tick();
				
				if (select(FD_SETSIZE, &fdset, NULL, NULL, &timeout) != SOCKET_ERROR)
				{
					unsigned int busy_time = this->current_tick + _SELECT_BUSYTIME;

					if (fdset.fd_count > 0)
					{
						unsigned long total_bytes = 0;
						
						do
						{
							ioctlsocket(this->sock, FIONREAD, &total_bytes);
							
							for (unsigned int total_read = 0, bytes_read = 0; total_read < total_bytes && this->alive; total_read += bytes_read)
							{
								unsigned char buffer[4096];

								sockaddr_in addr;
								int addr_length = sizeof(sockaddr_in);
								
								bytes_read = recvfrom(this->sock, reinterpret_cast<char*>(buffer), sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&addr), &addr_length);

								if (bytes_read == SOCKET_ERROR)
								{
									if (GetLastError() == WSAECONNRESET)
									{
										this->enter();

										std::map<sockaddr_in*, rudp_connection*, _addr_less_>::iterator iter = this->connect_ref.find(&addr);

										if (iter != this->connect_ref.end() && (*iter).first->sin_addr.S_un.S_addr == addr.sin_addr.S_un.S_addr && (*iter).first->sin_port == addr.sin_port)
											this->disconnect_ptr((*iter).second, false);

										this->leave();
									}

									break;
								}
								else
								{
									if (bytes_read == 0)
										break;
									
									this->recv_data(buffer, bytes_read, &addr);
								}
							}
						}
						while (total_bytes != 0 && get_time_tick() < busy_time);
					}
				}
				
				this->time_event();

				unsigned int current_time = this->current_tick;
	
				if (this->alive && current_time - this->last_check_time[2] >= _IDLE_PROCESS)
				{
					this->last_check_time[2] = current_time;
					this->enter();

					if (current_time - this->last_check_time[0] > _CHECK_UNREACHABLE)
					{
						this->last_check_time[0] = current_time;
						this->check_unreachable(current_time);
					}

					this->flush_ack();
					this->leave();

					static unsigned long pre_tick = 0;

					if (pre_tick == 0)
					{
						pre_tick = current_time;
					}
					else if (pre_tick + _PINGSENDTICK < current_time)
					{
						this->ping_check();
						pre_tick = current_time;
					}
				}

				if (this->alive)
				{
					this->enter();

					if (this->disconnect_list.size() > 0)
					{
						for (unsigned int i = 0; i < this->disconnect_list.size(); i++)
						{
							std::map<int, rudp_connection*>::iterator iter = this->connect_list.find(this->disconnect_list[i].second);
							
							if (iter != this->connect_list.end())
								this->disconnect_ptr((*iter).second, true, this->disconnect_list[i].first);
						}

						this->disconnect_list.clear();
					}

					this->leave();
				}
			}
		}

		bool rudp_socket_frame::open(unsigned short port)
		{
			this->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

			if (this->sock == INVALID_SOCKET)
				return false;
			
			sockaddr_in addr;
			memset(&addr, 0, sizeof(sockaddr_in));
			
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

			if (bind(this->sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
				return false;

			unsigned long non_blocking = 0;
			ioctlsocket(this->sock, FIONBIO, &non_blocking);

			unsigned int send_length = 51200;

			if (setsockopt(this->sock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&send_length), sizeof(send_length)) == SOCKET_ERROR)
				return false;

			this->thread_handle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, rudp_socket_frame::start_worker_thread, reinterpret_cast<void*>(this), 0, &this->thread_id));
			this->port = port;
			return true;
		}

		void rudp_socket_frame::close()
		{
			this->alive = false;
			
			if (WaitForSingleObject(this->thread_handle, 2000) != WAIT_TIMEOUT)
			{
				if (this->thread_handle) 
				{
					CloseHandle(this->thread_handle);
					this->thread_handle = NULL;
				}
			}
			
			if (this->sock != INVALID_SOCKET)
			{
				closesocket(this->sock);
				this->sock = INVALID_SOCKET;
			}

			this->enter();

			for (std::map<int, rudp_connection*>::iterator iter = this->connect_list.begin(); iter != this->connect_list.end(); iter++)
				delete (*iter).second;

			this->connect_list.clear();
			this->connect_ref.clear();

			this->leave();
		}
		
		int rudp_socket_frame::connect(const char* ip, int port)
		{
			rudp_connection* connection = this->connect(inet_addr(ip), port);
			return connection->get_id();
		}

		void rudp_socket_frame::disconnect(int net_id)
		{
			this->enter();

			std::map<int, rudp_connection*>::iterator iter = this->connect_list.find(net_id);

			if (iter != this->connect_list.end())
			{
				unsigned int i = 0;

				for (i = 0; i < this->disconnect_list.size(); i++)
					if (this->disconnect_list[i].second == net_id)
						break;

				if (i == this->disconnect_list.size())
				{
					RELIABLE_UDP_HEADER ack;
					ack.combo = _PACKET_HEADER(7, 0);
					ack.crc = 0;
					ack.crc = rudp_connection::get_crc(reinterpret_cast<unsigned char*>(&ack), sizeof(ack));
					
					this->send_to(&ack, sizeof(ack), (*iter).second->get_addr());
					this->send_to(&ack, sizeof(ack), (*iter).second->get_addr());

					this->disconnect_list.push_back(std::make_pair(false, net_id));
				}
			}

			this->leave();
		}

		void rudp_socket_frame::disconnect_async(int net_id)
		{
			std::map<int, rudp_connection*>::iterator iter = this->connect_list.find(net_id);

			if (iter != this->connect_list.end())
			{
				unsigned int i = 0;

				for (i = 0; i < this->disconnect_list.size(); i++)
					if (this->disconnect_list[i].second == net_id)
						break;

				if (i == this->disconnect_list.size())
				{
					RELIABLE_UDP_HEADER ack;
					ack.combo = _PACKET_HEADER(7, 0);
					ack.crc = 0;
					ack.crc = rudp_connection::get_crc(reinterpret_cast<unsigned char*>(&ack), sizeof(ack));
					
					this->send_to(&ack, sizeof(ack), (*iter).second->get_addr());
					this->send_to(&ack, sizeof(ack), (*iter).second->get_addr());

					this->disconnect_list.push_back(std::make_pair(true, net_id));
				}
			}
		}
		
		int rudp_socket_frame::send_data(int net_id, void* data, int length, int priority)
		{
			int ret = -1;
			this->enter();
			
			std::map<int, rudp_connection*>::iterator iter = this->connect_list.find(net_id);

			if (iter != this->connect_list.end())
				ret = (*iter).second->send_data(data, length, priority, this);

			this->leave();
			return ret;
		}
		
		unsigned int rudp_socket_frame::get_thread_id() 
		{ 
			return this->thread_id; 
		}
		
		char* rudp_socket_frame::get_public_ip() 
		{ 
			return this->public_ip; 
		}

		char* rudp_socket_frame::get_private_ip() 
		{ 
			return this->private_ip; 
		}

		unsigned int rudp_socket_frame::get_current_tick() 
		{
			return this->current_tick; 
		}

		int rudp_socket_frame::send_to(void* data, int length, sockaddr_in* addr)
		{
			return sendto(this->sock, reinterpret_cast<char*>(data), length, 0, reinterpret_cast<sockaddr*>(addr), sizeof(sockaddr_in));
		}
		
		int rudp_socket_frame::check_packet(void* data, int length, void* output)
		{
			RELIABLE_UDP_HEADER* header = reinterpret_cast<RELIABLE_UDP_HEADER*>(data);

			if (length <= sizeof(RELIABLE_UDP_HEADER))
				return 0;

			unsigned char crc = header->crc;

			header->crc = 0;
			header->crc = crc;

			if (crc != rudp_connection::get_crc(reinterpret_cast<unsigned char*>(data), length))	
				return 0;

			memcpy(output, &header[1], length - sizeof(RELIABLE_UDP_HEADER));
			return length - sizeof(RELIABLE_UDP_HEADER);
		}
		
		void rudp_socket_frame::get_host_addr()
		{
			unsigned int public_ip = 0;
			unsigned int public_ip_mask = 0;

			unsigned int private_ip = 0;
			unsigned int private_ip_mask = 0;

			MIB_IPADDRTABLE* ip_addr_table = reinterpret_cast<MIB_IPADDRTABLE*>(malloc(sizeof(MIB_IPADDRTABLE)));

			if (!ip_addr_table)
				return;

			unsigned long size = 0;

			if (GetIpAddrTable(ip_addr_table, &size, 0) == ERROR_INSUFFICIENT_BUFFER)
			{
				free(ip_addr_table);

				ip_addr_table = reinterpret_cast<MIB_IPADDRTABLE*>(malloc(size));

				if (!ip_addr_table) 
					return;
			}

			if (GetIpAddrTable(ip_addr_table, &size, 0) != NO_ERROR)
				return;

			for (unsigned int i = 0; i < ip_addr_table->dwNumEntries; i++)
			{
				unsigned int current_ip = ntohl(ip_addr_table->table[i].dwAddr);
				bool is_private = false;

				if (CXIP_A(current_ip) == 127)
				{
					continue;
				}
				else if (CXIP_A(current_ip) == 10)
				{
					is_private = true;
				}
				else if (CXIP_A(current_ip) == 172)
				{
					if (CXIP_B(current_ip) >= 16 && CXIP_B(current_ip) <= 31) 
						is_private = true;
				}
				else if (CXIP_A(current_ip) == 192)
				{
					if (CXIP_B(current_ip) == 168) 
						is_private = true;
				}

				if (is_private)
				{
					if (!private_ip || private_ip > current_ip)
					{
						private_ip = current_ip;
						private_ip_mask = ntohl(ip_addr_table->table[i].dwMask);
					}
				}
				else
				{
					if (!public_ip)
					{
						public_ip = current_ip;
						public_ip_mask = ntohl(ip_addr_table->table[i].dwMask);
					}
				}

				if (private_ip && public_ip) 
					break;
			}

			bool ip_adjust = false;

			if (private_ip && !public_ip)
			{
				ip_adjust = true;
				
				for (unsigned int i = 0; i < ip_addr_table->dwNumEntries; i++)
				{
					unsigned int current_ip = ntohl(ip_addr_table->table[i].dwAddr);
					bool is_private = false;

					if (CXIP_A(current_ip) == 127)
					{
						continue;
					}
					else if (CXIP_A(current_ip) == 10)
					{
						is_private = true;
					}
					else if (CXIP_A(current_ip) == 172)
					{
						if (CXIP_B(current_ip) >= 16 && CXIP_B(current_ip) <= 31) 
							is_private = true;
					}
					else if (CXIP_A(current_ip) == 192)
					{
						if (CXIP_B(current_ip) == 168) 
							is_private = true;
					}
				
					if (is_private && private_ip != current_ip)
					{
						public_ip = current_ip;
						public_ip_mask = ntohl(ip_addr_table->table[i].dwMask);
						break;
					}
				}
			}

			if (!public_ip)
			{
				public_ip = private_ip;
				public_ip_mask = private_ip_mask;
			}
			else
			{
				if (ip_adjust && private_ip > public_ip)
				{
					unsigned int temp_ip = private_ip;
					unsigned int temp_ip_mask = private_ip_mask;
					
					private_ip = public_ip;
					private_ip_mask = public_ip_mask;
					
					public_ip = temp_ip;
					public_ip_mask = temp_ip_mask;
				}
			}

			free(ip_addr_table);
				
			if (!private_ip && !public_ip)
				return;

			unsigned int n_public_ip = htonl(public_ip);
			strcpy_s(this->public_ip, inet_ntoa(*reinterpret_cast<in_addr*>(&n_public_ip)));
			
			unsigned int n_private_ip = htonl(private_ip);
			strcpy_s(this->private_ip, inet_ntoa(*reinterpret_cast<in_addr*>(&n_private_ip)));

			printf("public [ip: %s]\n", this->public_ip);
			printf("private [ip: %s]\n", this->private_ip);
		}

		rudp_connection* rudp_socket_frame::connect(unsigned int ip, int port)
		{
			this->enter();
			
			rudp_connection* connection = new rudp_connection(ip, port, this->net_id_count++);
			
			if (this->net_id_count == 0) 
				this->net_id_count++;

			this->connect_list[connection->get_id()] = connection;
			this->connect_ref[connection->get_addr()] = connection;

			this->leave();
			return connection;
		}
		
		void rudp_socket_frame::disconnect_ptr(rudp_connection* connection, bool force, bool unreachable)
		{
			std::map<int, rudp_connection*>::iterator iter = this->connect_list.find(connection->get_id());

			if (iter != this->connect_list.end())
				this->connect_list.erase(iter);

			std::map<sockaddr_in*, rudp_connection*, _addr_less_>::iterator ref_iter = this->connect_ref.find(connection->get_addr());

			if (ref_iter != this->connect_ref.end())
				this->connect_ref.erase(ref_iter);
	
			this->leave();
			this->disconnected(connection->get_id(), force, unreachable);
			this->enter();

			delete connection;
		}

		void rudp_socket_frame::recv_data(void* data, int length, sockaddr_in* addr)
		{
			rudp_connection* connection = NULL;
			RELIABLE_UDP_HEADER* header = reinterpret_cast<RELIABLE_UDP_HEADER*>(data);

			this->enter();

			std::map<sockaddr_in*, rudp_connection*, _addr_less_>::iterator iter = this->connect_ref.find(addr);

			if (iter != this->connect_ref.end() && (*iter).first->sin_addr.S_un.S_addr == addr->sin_addr.S_un.S_addr && (*iter).first->sin_port == addr->sin_port)
				connection = (*iter).second;

			this->leave();

			if (connection == NULL)
			{
				if (!this->accept(this->net_id_count, addr, data, length))		
					return;

				connection = this->connect(addr->sin_addr.S_un.S_addr, addr->sin_port);
			}

			connection->recv_data(data, length, this);
		}

		void rudp_socket_frame::check_unreachable(unsigned int current_tick)
		{
			for (std::map<int, rudp_connection*>::iterator iter = this->connect_list.begin(); iter != this->connect_list.end(); iter++)
				if (!(*iter).second->check_unreachable(current_tick, this))
					this->disconnect_async((*iter).second->get_id());
		}

		void rudp_socket_frame::check_recv_tick(unsigned int current_tick)
		{
			for (std::map<int, rudp_connection*>::iterator iter = this->connect_list.begin(); iter != this->connect_list.end(); iter++)
				if (!(*iter).second->check_recv_tick(current_tick))
					this->disconnect_async((*iter).second->get_id());
		}

		void rudp_socket_frame::flush_ack()
		{
			for (std::map<int, rudp_connection*>::iterator iter = this->connect_list.begin(); iter != this->connect_list.end(); iter++)
				(*iter).second->flush_ack(this);
		}

		void rudp_socket_frame::ping_check()
		{
			if (!this->connect_list.empty())
			{
				for (std::map<int, rudp_connection*>::iterator iter = this->connect_list.begin(); iter != this->connect_list.end(); iter++)
					(*iter).second->ping_check(this);
			}
		}
		
		bool rudp_socket_frame::is_alive()
		{
			return this->alive;
		}

		void rudp_socket_frame::enter()
		{
			EnterCriticalSection(&this->critical_section);
		}

		void rudp_socket_frame::leave()
		{
			LeaveCriticalSection(&this->critical_section);
		}
	}
}