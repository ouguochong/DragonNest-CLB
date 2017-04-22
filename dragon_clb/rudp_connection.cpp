#include "rudp_connection.hpp"
#include "rudp_socket_frame.hpp"

namespace game
{
	namespace network
	{
		rudp_connection::rudp_connection(unsigned int ip, int port, int net_id)
		{
			memset(&this->target_addr, 0, sizeof(sockaddr_in));
			this->target_addr.sin_family = AF_INET;
			this->target_addr.sin_addr.S_un.S_addr = ip;
			this->target_addr.sin_port = port;
			
			this->net_id = net_id;

			this->unreachable_count = 0;
			this->rto_tick_counter = 0;
			
			memset(this->ack_list, 0, sizeof(this->ack_list));
			memset(this->ack_list_count, 0, sizeof(this->ack_list_count));

			this->rto_tick = _RESEND_TIME;

			this->seq_count = 0;
			this->ack_count = 0;

			this->recv_seq_count = 0;
			this->recv_ack_count = 0;

			this->recv_tick = 0;

			this->last_check_time = 0;
			this->send_bytes = 0;
			this->count = 0;
		}

		rudp_connection::~rudp_connection()
		{
			for (unsigned int i = 0; i < this->alloced_queue.size(); i++)
				delete[] this->alloced_queue[i];
		}
		
		bool rudp_connection::check_unreachable(unsigned int time_tick, rudp_socket_frame* sock)
		{
			if (this->send_queue.size() >= _RTO_LIST_MAX)
			{
				if (this->rto_tick_counter == 0)
				{
					this->rto_tick_counter = time_tick;
				}
				else
				{
					if (time_tick - this->rto_tick_counter >= _RTO_TICK_MAX)
						return false;
				}
			}
			else
			{
				this->unreachable_count = 0;
				this->rto_tick_counter = 0;
			}

			for (std::list<PACKET_QUEUE*>::iterator iter = this->send_queue.begin(); iter != this->send_queue.end(); iter++)
			{
				if (time_tick - (*iter)->tick >= rto_tick)
				{
					sock->send_to((*iter)->data, (*iter)->len, &this->target_addr);
					(*iter)->tick = time_tick;
				}
			}

			return true;
		}

		bool rudp_connection::check_recv_tick(unsigned int current_tick)
		{
			return (this->recv_tick + _CHECK_RECVTICK >= current_tick);
		}

		void rudp_connection::flush_ack(rudp_socket_frame* sock)
		{
			for (int i = 0; i < 3; i++)
			{
				if (this->ack_list_count[i] > 0)
				{
					unsigned char packet[sizeof(RELIABLE_UDP_HEADER) + 16 * 2];
					RELIABLE_UDP_HEADER* packet_header = reinterpret_cast<RELIABLE_UDP_HEADER*>(packet);

					packet_header->combo = _PACKET_HEADER((i + 4), (this->ack_list_count[i] << 3));
					memcpy(packet + sizeof(RELIABLE_UDP_HEADER), this->ack_list[i], this->ack_list_count[i] * 2);

					packet_header->crc = 0;
					packet_header->crc = rudp_connection::get_crc(packet, sizeof(RELIABLE_UDP_HEADER) + this->ack_list_count[i] * 2);

					sock->send_to(packet, sizeof(RELIABLE_UDP_HEADER) + this->ack_list_count[i] * 2, &this->target_addr);
					this->ack_list_count[i] = 0;
				}
			}
		}

		void rudp_connection::ping_check(rudp_socket_frame* sock)
		{
			RELIABLE_UDP_HEADER packet;
			memset(&packet, 0, sizeof(packet));

			packet.combo = _PACKET_HEADER(3, 0);
			packet.crc = 0;
			packet.crc = rudp_connection::get_crc(reinterpret_cast<unsigned char*>(&packet), sizeof(packet));

			sock->send_to(&packet, sizeof(packet), &this->target_addr);
		}
			
		int rudp_connection::send_data(void* data, int length, int priority, rudp_socket_frame* sock)
		{
			unsigned char buffer[512];
			RELIABLE_UDP_HEADER* header = reinterpret_cast<RELIABLE_UDP_HEADER*>(buffer);

			if (length > sizeof(buffer) + sizeof(RELIABLE_UDP_HEADER))
				return -1;

			header->flags = 0x00;

			if (priority == _FAST)
			{
				header->combo = _PACKET_HEADER(_FAST, 0);
			}	
			else if (priority == _RELIABLE_NOORDER)
			{
				header->combo = _PACKET_HEADER(_RELIABLE_NOORDER, this->ack_count);
				this->ack_count += 8;
			}
			else if (priority == _RELIABLE)
			{
				header->combo = _PACKET_HEADER(_RELIABLE, this->seq_count);
				this->seq_count += 8;
			}	
			else
				return -1;

			memcpy(buffer + sizeof(RELIABLE_UDP_HEADER), data, length);

			header->crc = 0;
			header->crc = rudp_connection::get_crc(buffer, sizeof(RELIABLE_UDP_HEADER) + length);

			if (priority != _FAST)
			{
				PACKET_QUEUE* queue = this->create_queue(sock, buffer, sizeof(RELIABLE_UDP_HEADER) + length, priority == _RELIABLE_NOORDER ? 4 : 5);

				if (queue == NULL)
					return -1;

				this->send_queue.push_back(queue);

				if (!(this->ack_count & (7 * 8)) || !(this->seq_count & (7 * 8)) && this->rto_tick > 100)
					this->rto_tick -= 1;
			}

			sock->send_to(buffer, sizeof(RELIABLE_UDP_HEADER) + length, &this->target_addr);
			return length;
		}

		void rudp_connection::recv_data(void* data, int length, rudp_socket_frame* sock)
		{
			unsigned int current_tick = sock->get_current_tick();
			this->recv_tick = current_tick;

			RELIABLE_UDP_HEADER* header = reinterpret_cast<RELIABLE_UDP_HEADER*>(data);

			if (length < sizeof(RELIABLE_UDP_HEADER))
				return;

			unsigned char crc = header->crc;
			header->crc = 0;

			if (crc != rudp_connection::get_crc(reinterpret_cast<unsigned char*>(data), length))
				return;
	
			switch(_PACKET_TYPE(header->flags))
			{
				case 0: 
					sock->recv_data(this->net_id, reinterpret_cast<char*>(data) + sizeof(RELIABLE_UDP_HEADER), length - sizeof(RELIABLE_UDP_HEADER));
					break;

				case 1 : 
					this->send_ack(0x04, _PACKET_ACK(header->ack), &this->target_addr, sock);

					if (((_PACKET_SEQ(header->seq) < this->recv_ack_count && static_cast<int>(_PACKET_SEQ(header->seq)) + 0x8000 > static_cast<int>(this->recv_ack_count)) ||
						(_PACKET_SEQ(header->seq) > this->recv_ack_count && static_cast<int>(_PACKET_SEQ(header->seq)) - 0x8000 > static_cast<int>(this->recv_ack_count))))
						return;

					if (this->recv_ack_count != _PACKET_ACK(header->ack))
					{
						for(std::list<unsigned short>::iterator iter = this->ack_queue.begin(); iter != this->ack_queue.end(); iter++)
							if ((*iter) == _PACKET_ACK(header->ack))
								return;

						this->ack_queue.push_back(_PACKET_ACK(header->ack));
					}	
					else
					{
						int last_count = 0;
						this->recv_ack_count += 8;

						do 
						{
							last_count = this->recv_ack_count;

							for (std::list<unsigned short>::iterator iter = this->ack_queue.begin(); iter != this->ack_queue.end(); iter++)
							{
								if ((*iter) == this->recv_ack_count)
								{
									this->ack_queue.erase(iter);
									this->recv_ack_count += 8;
									break;
								}
							}
						}	
						while (last_count != this->recv_ack_count);
					}

					sock->recv_data(this->net_id, reinterpret_cast<char*>(data) + sizeof(RELIABLE_UDP_HEADER), length - sizeof(RELIABLE_UDP_HEADER));
					break;

				case 2 : 
				{
					if (((_PACKET_SEQ(header->seq) < this->recv_seq_count && static_cast<int>(_PACKET_SEQ(header->seq)) + 0x8000 > static_cast<int>(this->recv_seq_count)) ||
						(_PACKET_SEQ(header->seq) > this->recv_seq_count && static_cast<int>(_PACKET_SEQ(header->seq)) - 0x8000 > static_cast<int>(this->recv_seq_count))))
					{
						this->send_ack(0x05, _PACKET_ACK(header->ack), &this->target_addr, sock);
						return ;
					}

					if (this->recv_seq_count != _PACKET_SEQ(header->seq))
					{
						for (std::list<PACKET_QUEUE*>::iterator iter = this->recv_queue.begin(); iter != this->recv_queue.end(); iter++)
						{
							if ((*iter)->seq == _PACKET_SEQ(header->seq))
							{
								this->send_ack(0x05, _PACKET_ACK(header->ack), &this->target_addr, sock);
								return;
							}
						}				
						
						this->send_ack(0x06, _PACKET_ACK(header->ack), &this->target_addr, sock);

						sock->enter();
						PACKET_QUEUE* queue = this->create_queue(sock, data, length);

						if (queue != NULL)
							this->recv_queue.push_back(queue);
						else
							sock->disconnect_ptr(this, true);

						sock->leave();
						return;
					}
					
					this->send_ack(0x05, _PACKET_ACK(header->ack), &this->target_addr, sock);
					this->recv_seq_count += 8;

					sock->recv_data(this->net_id, reinterpret_cast<char*>(data) + sizeof(RELIABLE_UDP_HEADER), length - sizeof(RELIABLE_UDP_HEADER));

					if (!sock->is_alive())
						break;

					unsigned short last_count = 0;

					do 
					{
						last_count = this->recv_seq_count;

						for(std::list<PACKET_QUEUE*>::iterator iter = this->recv_queue.begin(); iter != this->recv_queue.end(); iter++)
						{
							if (_PACKET_SEQ((*iter)->seq) == this->recv_seq_count)
							{
								sock->recv_data(this->net_id, reinterpret_cast<char*>((*iter)->data) + sizeof(RELIABLE_UDP_HEADER), (*iter)->len - sizeof(RELIABLE_UDP_HEADER));

								sock->enter();
								this->release_queue(*iter);
								sock->leave();

								this->recv_queue.erase(iter);
								this->recv_seq_count += 8;
								break;
							}
						}
					}	
					while (last_count != this->recv_seq_count);

					break;
				}
					
				case 3:
				{
					// PING PONG
					//_RELIABLE_UDP_HEADER ack;
					//ack.combo	= _PACKET_HEADER(6, 0);//_PACKET_PINGCNT(header->pingcnt));
					//ack.crc		= 0;
					//ack.crc		= GetCRC((unsigned char*)&ack, sizeof(ack));
					//pSocket->SendTo(&ack, sizeof(ack), &m_TargetAddr);
					break;
				}

				case 4: 
				case 5:
					if (_PACKET_ACKN(header->acknum) > 16) 
						break;

					sock->enter();

					if (this->remove_queue(_PACKET_TYPE(header->flags), reinterpret_cast<unsigned short*>(&header[1]), _PACKET_ACKN(header->acknum)) == false)
					{
						if (this->rto_tick < 450)
							this->rto_tick += 15;
					}

					sock->leave();
					break;

				case 6:
					if (_PACKET_ACKN(header->acknum) > 16)
						break;

					sock->enter();
					this->resend_queue(_PACKET_TYPE(header->flags), reinterpret_cast<unsigned short*>(&header[1]), _PACKET_ACKN(header->acknum), sock);
					sock->leave();
					break;

				case 7 :
					sock->enter();
					sock->disconnect_ptr(this, true);
					sock->leave();
					break;

				default :
					break;
			}
		}

		int rudp_connection::get_id() const
		{
			return this->net_id;
		}

		sockaddr_in* rudp_connection::get_addr()
		{
			return &this->target_addr;
		}

		int rudp_connection::get_send_queue_size()
		{
			return static_cast<int>(this->send_queue.size());
		}
		
		unsigned char rudp_connection::get_crc(const unsigned char* data, int length)
		{
			unsigned int code = 0;
			
			for (int i = 0; i < length; i++)
				code += data[i];
			
			return (0x78 - code + length);
		}

		void rudp_connection::send_ack(int flags, int ack, sockaddr_in* addr, rudp_socket_frame* sock)
		{
			int location = flags - 4;

			for (int i = 0; i < this->ack_list_count[location]; i++)
				if (this->ack_list[location][i] == ack)
					return;

			if (this->ack_list_count[location] >= 16)
				this->flush_ack(sock);

			this->ack_list[location][this->ack_list_count[location]++] = ack;
		}
			
		bool rudp_connection::resend_queue(unsigned short type, unsigned short* ack_list, int ack_num, rudp_socket_frame* sock)
		{
			for (std::list<PACKET_QUEUE*>::iterator iter = this->send_queue.begin(); iter != this->send_queue.end(); iter++)
			{
				PACKET_QUEUE* packet_queue = (*iter);

				for (int i = 0; i < ack_num; i++)
				{
					if (packet_queue->seq < ack_list[i])
					{
						sock->send_to(packet_queue->data, packet_queue->len, &this->target_addr);
						packet_queue->tick = sock->get_current_tick();
					}
					else if (packet_queue->seq <= ack_list[i])
						return true;
				}
			}
			return false;
		}

		bool rudp_connection::remove_queue(unsigned short type, unsigned short* ack_list, int ack_num)
		{
			int count = 0;

			for (std::list<PACKET_QUEUE*>::iterator iter = this->send_queue.begin(); iter != this->send_queue.end() && count < ack_num;)
			{
				PACKET_QUEUE* packet_queue = (*iter);

				for (int i = 0; i < ack_num; i++)
				{
					if (packet_queue->seq == ack_list[i] && packet_queue->type == type)
					{
						if (i < ack_num)
						{
							this->release_queue(packet_queue);
							this->send_queue.erase(iter);

							iter = this->send_queue.begin();
							count++;
						}
						else
							iter++;

						break;
					}
				}
			}

			return (count > 0);
		}

		PACKET_QUEUE* rudp_connection::create_queue(rudp_socket_frame* sock, void* buffer, int length, int type)
		{
			int level = rudp_connection::mem_level(sizeof(PACKET_QUEUE) + length);

			if (this->empty_queue[level].empty())
			{
				static const int level_sizes[] = { 32, 64, 128, 256, 512 };

				char* pointer = new char[1024];

				if (pointer == NULL)
					return NULL;

				this->alloced_queue.push_back(pointer);

				for (int i = 0; i < 1024; i += level_sizes[level])
					this->empty_queue[level].push_back(reinterpret_cast<PACKET_QUEUE*>(pointer + i));
			}
			
			PACKET_QUEUE* queue = this->empty_queue[level].back();
			this->empty_queue[level].pop_back();

			queue->seq = _PACKET_SEQ(reinterpret_cast<RELIABLE_UDP_HEADER*>(buffer)->seq);
			queue->type = type;
			queue->len = static_cast<unsigned short>(length);
			queue->origintick = queue->tick = sock->get_current_tick();

			memcpy(queue->data, buffer, length);
			return queue;
		}

		void rudp_connection::release_queue(PACKET_QUEUE* queue)
		{
			this->empty_queue[rudp_connection::mem_level(sizeof(PACKET_QUEUE) + queue->len)].push_back(queue);
		}

		int rudp_connection::mem_level(int size)
		{
			if (size <= 64)
				return size <= 32 ? 0 : 1;

			if (size <= 256)
				return size <= 128 ? 2 : 3;

			return size <= 512 ? 4 : 5;
		}
	}
}