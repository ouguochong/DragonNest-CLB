#pragma once

#include "generic.hpp"

namespace game
{
	namespace network
	{
		namespace crypto
		{
			class mt_random
			{
			public:
				mt_random();
				~mt_random();
				
				void srand(unsigned long seed);

				void next_state();
				float genrand_real2();

				int rand();
				int rand(unsigned long range);
				float rand(float small_value, float large_value);
				int rand(int small_value, int large_value);
				
				void lock_seed();
				void unlock_seed();

				unsigned long get_seed();

			private:
				enum
				{
					N = 624,
					M = 397,
					MATRIX_A = 0x9908b0dfUL,
					UMASK = 0x80000000UL,
					LMASK = 0x7fffffffUL,
				};
				
				bool initialized;
				int left;

				unsigned long state[624]; /* the array for the state vector  */
				unsigned long* next;

				unsigned long seed;
				unsigned long seed_lock;
			};
		}
	}
}