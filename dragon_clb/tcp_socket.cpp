#include "tcp_socket.hpp"
#include "tea_crypto.hpp"

#include "main_form.hpp"

namespace game
{
	namespace network
	{
		const unsigned int header_length = 3;

		tcp_socket::tcp_socket()
			: sock(INVALID_SOCKET), recv_offset(0)
		{
			memset(this->recv_buffer, 0, sizeof(this->recv_buffer));
		}

		tcp_socket::~tcp_socket()
		{
			if (this->sock != INVALID_SOCKET)
			{
				this->try_disconnect();
			}
		}

		bool tcp_socket::try_connect(std::string const& ip, unsigned short port)
		{
			this->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			if (this->sock == INVALID_SOCKET)
			{
				return false;
			}

			if (WSAAsyncSelect(this->sock, dragonnest_clb::gui::main_form::get_instance().get_handle(), 
				WM_SOCKET, FD_CLOSE | FD_CONNECT | FD_READ | FD_WRITE))
			{
				return false;
			}

			sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
			addr.sin_port = htons(port);

			printf("[tcp_socket] connecting to %s:%d\n", ip.c_str(), port);

			return ((connect(this->sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != SOCKET_ERROR) || (WSAGetLastError() == WSAEWOULDBLOCK));
		}

		bool tcp_socket::try_disconnect()
		{
			if (this->sock == INVALID_SOCKET)
			{
				return false;
			}
			
			if (shutdown(this->sock, SD_BOTH) == SOCKET_ERROR)
			{
				return false;
			}
			
			if (closesocket(this->sock) == SOCKET_ERROR)
			{
				return false;
			}
			
			this->sock = INVALID_SOCKET;
			return true;
		}
	
		bool tcp_socket::recv_packets(std::vector<io_packet>& packets)
		{
			for (unsigned int recv_size = 0xDEADBEEF; recv_size != 0 && recv_size != SOCKET_ERROR; 
				this->recv_offset += (recv_size == SOCKET_ERROR ? 0 : recv_size))
			{ 
				recv_size = recv(this->sock, reinterpret_cast<char*>(this->recv_buffer + this->recv_offset), 
					sizeof(this->recv_buffer) - this->recv_offset, 0);
				
				if (recv_size == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
				{
					printf("[tcp_socket] SOCKET_ERROR: %d\n", WSAGetLastError());
					return false;
				}
			}
			
			while (this->recv_offset >= header_length)
			{
				unsigned int packet_length_total = (this->recv_buffer[0] | (this->recv_buffer[1] << 8) | (this->recv_buffer[2] << 16));
				unsigned int packet_length = packet_length_total - header_length;

				if (this->recv_offset < packet_length)
				{
					break;
				}
				
				crypto::tea::get_instance().decrypt(this->recv_buffer + header_length, packet_length);
				packets.push_back(network::io_packet(this->recv_buffer + header_length + 2, packet_length - 2));
				
				this->recv_offset -= packet_length_total;
				memcpy(this->recv_buffer, this->recv_buffer + packet_length_total, this->recv_offset);
				memset(this->recv_buffer + this->recv_offset, 0, packet_length_total);
			}

			return true;
		}
		
		bool tcp_socket::send_packet(io_packet& packet)
		{
			std::size_t packet_size = header_length + 2 + packet.get_size();
			unsigned char* packet_buffer = new unsigned char[packet_size];
			
			packet_buffer[0] = static_cast<unsigned char>(packet_size & 0xFF);
			packet_buffer[1] = static_cast<unsigned char>((packet_size >> 8) & 0xFF);
			packet_buffer[2] = static_cast<unsigned char>((packet_size >> 16) & 0xFF);
			packet_buffer[3] = LOBYTE(static_cast<unsigned short>(packet_size - header_length));
			packet_buffer[4] = HIBYTE(static_cast<unsigned short>(packet_size - header_length));
			
			memcpy(packet_buffer + header_length + 2, packet.get_data(), packet.get_size());
			crypto::tea::get_instance().encrypt(packet_buffer + header_length, packet.get_size() + 2);

			bool result = this->raw_send(packet_buffer, packet_size);

			delete[] packet_buffer;
			return result;
		}
		
		SOCKET tcp_socket::get_desc() const
		{
			return this->sock;
		}

		bool tcp_socket::raw_recv(unsigned char* buffer, std::size_t size)
		{
			for (int data_read = 0, offset = 0, data_to_read = size; data_to_read > 0; data_to_read -= data_read, offset += data_read)
			{
				data_read = recv(this->sock, reinterpret_cast<char*>(buffer) + offset, data_to_read, 0);
	
				if (data_read == 0 || data_read == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						printf("[tcp_socket] recv() returned %d -> WSAGetLastError() = %d\n", data_read, WSAGetLastError());
					}

					return false;
				}
			}

			return true;
		}

		bool tcp_socket::raw_send(unsigned char const* buffer, std::size_t size)
		{
			for (int data_sent = 0, offset = 0, data_to_send = size; data_to_send > 0; data_to_send -= data_sent, offset += data_sent)
			{
				data_sent = send(this->sock, reinterpret_cast<const char*>(buffer) + offset, data_to_send, 0);
	
				if (data_sent == 0 || data_sent == SOCKET_ERROR)
				{
					printf("[tcp_socket] send() returned %d -> WSAGetLastError() = %d\n", data_sent, WSAGetLastError());
					return false;
				}
			}

			return true;
		}
	}
}