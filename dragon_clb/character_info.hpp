#pragma once

#include "generic.hpp"

namespace game
{
	class character_info
	{
	public:
		character_info(unsigned int id, std::string const& name)
			: id_(id), name_(name)
		{
			
		}

		unsigned int& id()
		{
			return this->id_;
		}
		
		std::string& name()
		{
			return this->name_;
		}

	private:
		unsigned int id_;
		std::string name_;
	};
}