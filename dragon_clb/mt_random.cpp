#include "mt_random.hpp"

#include <time.h>

namespace game
{
	namespace network
	{
		namespace crypto
		{
			#define mix_bits(u, v) (((u) & UMASK) | ((v) & LMASK))
			#define twist(u, v) ((mix_bits(u,v) >> 1) ^ ((v) & 1UL ? MATRIX_A : 0UL))

			mt_random::mt_random()
				: initialized(false)
			{
			
			}

			mt_random::~mt_random()
			{
			
			}

			void mt_random::srand(unsigned long seed)
			{
				this->seed = seed;
				this->state[0] = (seed & 0xFFFFFFFFUL);

				for (int i = 1; i < N; i++) 
				{
					this->state[i] = (1812433253UL * (this->state[i - 1] ^ (this->state[i - 1] >> 30)) + i);
					this->state[i] &= 0xFFFFFFFFUL;
				}

				this->left = 1;
				this->initialized = true;
			}

			void mt_random::next_state()
			{
				unsigned long* p = this->state;

				if (!initialized) 
				{
					this->srand(static_cast<unsigned long>(time(0)));
				}

				this->left = N;
				this->next = this->state;

				for (int i = N - M + 1; --i; p++)
				{
					*p = p[M] ^ twist(p[0], p[1]);
				}

				for (int i = M; --i; p++)
				{
					*p = p[M - N] ^ twist(p[0], p[1]);
				}

				*p = p[M - N] ^ twist(p[0], this->state[0]);
			}

			float mt_random::genrand_real2()
			{
				return static_cast<float>(rand()) * (1.f / 4294967296.f);
			}

			int mt_random::rand()
			{
				if (!initialized) 
				{
					this->left = 1;
				}

				if (!(--this->left)) 
				{
					next_state();
				}

				int y = *this->next++;
				y ^= (y >> 11);
				y ^= (y << 7) & 0x9d2c5680UL;
				y ^= (y << 15) & 0xefc60000UL;
				y ^= (y >> 18);

				if (y < 0) 
				{
					y = -y;
				}

				return y;
			}

			int mt_random::rand(unsigned long range)
			{
				unsigned int used = (used >> 1) | (used >> 2) | (used >> 4) | (used >> 8) | (used >> 16);

				int n = 0;

				do 
				{
					n = static_cast<unsigned int>(rand()) & used;
				}
				while (n >= static_cast<int>(range));

				return n;
			}

			float mt_random::rand(float small_value, float large_value)
			{
				float interval = genrand_real2();
				return small_value * interval + large_value * (1 - interval);	
			}

			int mt_random::rand(int small_value, int large_value)
			{
				return rand(large_value - small_value + 1) + small_value;
			}

			void mt_random::lock_seed()
			{
				this->seed_lock = this->get_seed();
			}

			void mt_random::unlock_seed()
			{
				this->srand(this->seed_lock);
			}
			
			unsigned long mt_random::get_seed()
			{
				return this->seed;
			}
		}
	}
}