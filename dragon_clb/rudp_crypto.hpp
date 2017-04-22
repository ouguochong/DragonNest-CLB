#pragma once

const int DNGAME_PACKET_HEADERSIZE = 7;

#pragma pack(push, 1)
struct DNGAME_PACKET
{
	unsigned short datasize; // 02 00 A2 0A 00 35 15 11 00 D8 B9 56 B1 59 BD 3E 8E 82 FD 8A
	unsigned char header;
	unsigned char sub_header;
	unsigned short checksum;
	unsigned char seq;	
	char data[1024 * 4];
};
#pragma pack(pop)

namespace game
{
	namespace network
	{
		namespace crypto
		{
			class rudp_crypto
			{
			public:
				rudp_crypto();
				virtual ~rudp_crypto();

			protected:
				int encode_game_packet(void* packet, int main_header, int sub_header, void* data, int length, unsigned char seq = 0);
				bool decode_game_packet(DNGAME_PACKET* packet);

				int calc_game_packet_size(void* packet, int length);

			private:
				unsigned char encode_game_packet_size(unsigned char codes);
				unsigned char decode_game_packet_size(unsigned char codes);
			};
		}
	}
}