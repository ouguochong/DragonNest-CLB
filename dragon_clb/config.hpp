#pragma once

#include "generic.hpp"

namespace game
{
	namespace config
	{
		namespace ahn
		{
			const unsigned int hackshield_option = 0x1A8A21AE;
		}

		namespace server
		{
			const std::string ip = "203.116.155.9";
			const unsigned int port = 14301;
		}

		namespace client
		{
			const unsigned char nation = 51;
			const unsigned char version = 6;

			const unsigned short major_version = 1;
			const unsigned short minor_version = 1685;
		}
	}
}