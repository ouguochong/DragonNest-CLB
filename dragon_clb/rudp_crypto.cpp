#include "rudp_crypto.hpp"

#include <iostream>

namespace game
{
	namespace network
	{
		namespace crypto
		{
			static unsigned char _table[2][8] = 
			{
				6, 2, 7, 0, 1, 3, 5, 4,
				3, 4, 1, 5, 7, 6, 0, 2
			};


			static unsigned char _mix_bytes[13] = 
			{
				0x3f, 0x26, 0x42, 0xfd, 0x8a, 0x72, 0x34, 0x74, 0xc8, 0xbf, 0xcd, 0xa4, 0x94
			};

			rudp_crypto::rudp_crypto()
			{
			
			}

			rudp_crypto::~rudp_crypto()
			{
			
			}

			int rudp_crypto::encode_game_packet(void* buffer, int main_header, int sub_header, void* data, int length, unsigned char seq)
			{
				DNGAME_PACKET* packet = reinterpret_cast<DNGAME_PACKET*>(buffer);
	
				unsigned char encoded_main_header = this->encode_game_packet_size(main_header);
				unsigned char encoded_sub_header = this->encode_game_packet_size(sub_header);

				packet->seq = this->encode_game_packet_size(seq);
				packet->datasize = length;
				
				unsigned char magickey = encoded_main_header ^ _mix_bytes[length % sizeof(_mix_bytes)];

				packet->header = magickey;
				packet->sub_header = encoded_sub_header ^ _mix_bytes[length % sizeof(_mix_bytes)];

				for(int i = 0, total = 0; i < length; i++)
					packet->data[i] = reinterpret_cast<char*>(data)[i] ^ _mix_bytes[(magickey + i + 7) % sizeof(_mix_bytes)];
	
				packet->checksum = DNGAME_PACKET_HEADERSIZE + length;
				return DNGAME_PACKET_HEADERSIZE + length;
			}

			bool rudp_crypto::decode_game_packet(DNGAME_PACKET* packet)
			{
				unsigned short size = packet->datasize;
				unsigned char magickey = packet->header;

				packet->header = this->decode_game_packet_size(magickey ^ _mix_bytes[size % sizeof(_mix_bytes)]);
				packet->sub_header = this->decode_game_packet_size(packet->sub_header ^ _mix_bytes[size % sizeof(_mix_bytes)]);
				packet->seq = this->decode_game_packet_size(packet->seq);

				for(unsigned short i = 0, total = 0; i < size; i++)
					packet->data[i] ^= _mix_bytes[(magickey + i + 7) % sizeof(_mix_bytes)];
	
				return DNGAME_PACKET_HEADERSIZE + size == packet->checksum ? true : false;
			}

			int rudp_crypto::calc_game_packet_size(void* packet, int length)
			{
				if (length >= DNGAME_PACKET_HEADERSIZE)
					return DNGAME_PACKET_HEADERSIZE + reinterpret_cast<DNGAME_PACKET*>(packet)->datasize;
				
				return sizeof(DNGAME_PACKET);
			}

			unsigned char rudp_crypto::encode_game_packet_size(unsigned char codes)
			{
				return (_table[0][codes & 7] << 5) | ((codes >> 3) & 3) | (_table[0][(codes >> 5) & 7] << 2);
			}

			unsigned char rudp_crypto::decode_game_packet_size(unsigned char codes)
			{
				return _table[1][(codes >> 5) & 7] | (_table[1][(codes >> 2) & 7] << 5) | ((codes & 3) << 3);
			}
		}
	}
}