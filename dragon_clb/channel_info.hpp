#pragma once

#include "generic.hpp"

namespace game
{
	class channel_info
	{
	public:
		channel_info(unsigned int id, std::string const& ip, unsigned short port)
			: id_(id), ip_(ip), port_(port)
		{
			
		}

		unsigned int& id()
		{
			return this->id_;
		}

		std::string& ip()
		{
			return this->ip_;
		}

		unsigned short& port()
		{
			return this->port_;
		}

	private:
		unsigned int id_;

		std::string ip_;
		unsigned short port_;
	};
}