#include "tea_crypto.hpp"
#include "mt_random.hpp"

namespace game
{
	namespace network
	{
		namespace crypto
		{
			void tea::encrypt(unsigned char* buffer, const unsigned int size)
			{
				unsigned int encode_size = size;
				unsigned int block_count = size / block_size;

				if (size % block_size)
				{
					block_count++;
				}

				unsigned int round = (size % 2) + 1;

				for (unsigned int i = 0; i < block_count; i++)
				{
					unsigned int* key = this->keys[size % key_count];
					unsigned int sum = 0;
					unsigned int limit = round * delta;	
	
					if (encode_size >= block_size)
					{
						unsigned int x;
						memcpy(&x, buffer + (i * block_size), sizeof(unsigned int));

						unsigned int y;
						memcpy(&y, buffer + ((i * block_size) + 4), sizeof(unsigned int));

						while (sum != limit)
						{
							x += (y << 4 ^ y >> 5) + y ^ sum + key[sum & 3];
							sum += delta;
							y += (x << 4 ^ x >> 5) + x ^ sum + key[sum >> 11 & 3];
						}

						memcpy(buffer + (i * block_size), &x, sizeof(unsigned int));
						memcpy(buffer + (i * block_size) + 4, &y, sizeof(unsigned int));

						encode_size -= block_size;
					}
					else
					{
						unsigned char* temp_buffer = buffer + (i * block_size);

						for (unsigned int j = 0; j < encode_size; j++)
						{
							temp_buffer[j] ^= reinterpret_cast<unsigned char*>(key)[j];
						}
					}
				}
			}

			void tea::decrypt(unsigned char* buffer, const unsigned int size)
			{
				unsigned int decrypt_size = size;
				unsigned int block_count = size / block_size;

				if (size % block_size)
				{
					block_count++;
				}

				unsigned int round = (size % 2) + 1;

				for (unsigned int i = 0; i < block_count; i++)
				{
					unsigned int* key = this->keys[size % key_count];
					unsigned int sum = round * delta;

					if (decrypt_size >= block_size)
					{
						unsigned int x;
						memcpy(&x, buffer + (i * block_size), sizeof(unsigned int));

						unsigned int y;
						memcpy(&y, buffer + ((i * block_size) + 4), sizeof(unsigned int));

						while (sum)
						{
							y -= (x << 4 ^ x >> 5) + x ^ sum + key[sum >> 11 & 3];
							sum -= delta;
							x -= (y << 4 ^ y >> 5) + y ^ sum + key[sum & 3];
						}

						memcpy(buffer + (i * block_size), &x, sizeof(unsigned int));
						memcpy(buffer + ((i * block_size) + 4), &y, sizeof(unsigned int));

						decrypt_size -= block_size;
					}
					else
					{
						unsigned char* temp_buffer = buffer + (i * block_size);

						for (unsigned int j = 0; j < decrypt_size; j++)
						{
							temp_buffer[j] ^= reinterpret_cast<unsigned char*>(key)[j];
						}
					}
				}
			}

			tea::tea()
			{
				mt_random rand;
				rand.srand(key_seed);

				for (unsigned int i = 0; i < key_count; i++)
				{
					for (unsigned int j = 0; j < 4; j++)
					{
						this->keys[i][j] = rand.rand();
					}
				}
			}

			tea::~tea()
			{

			}
		}
	}
}