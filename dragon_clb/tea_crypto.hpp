#pragma once

#include "generic.hpp"

namespace game
{
	namespace network
	{
		namespace crypto
		{
			class tea
			{
			public:
				static tea& get_instance()
				{
					static tea crypto_client;
					return crypto_client;
				}
				
				void encrypt(unsigned char* buffer, const unsigned int size);
				void decrypt(unsigned char* buffer, const unsigned int size);

			protected:
				tea();
				~tea();
	
			private:
				enum
				{
					delta		= 0x9E3779B9,
					block_size	= 8,
					key_count	= 256,
					key_seed	= 0x84873294,
				};

				unsigned int keys[key_count][4];
			};
		}
	}
}