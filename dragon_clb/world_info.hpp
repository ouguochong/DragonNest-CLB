#pragma once

#include "generic.hpp"

namespace game
{
	class world_info
	{
	public:
		world_info(unsigned char id, std::string const& name)
			: id_(id), name_(name)
		{
			
		}

		unsigned char& id()
		{
			return this->id_;
		}
		
		std::string& name()
		{
			return this->name_;
		}

	private:
		unsigned char id_;
		std::string name_;
	};
}