#pragma once

#include "generic.hpp"

namespace game
{
	class account
	{
	public:
		account()
			: username_(""), password_(""), character_name_("")
		{
		
		}

		account(std::string const& username, std::string const& password, std::string const& character_name)
			: username_(username), password_(password), character_name_(character_name)
		{
		
		}

		~account()
		{
		
		}
		
		std::string& username()
		{
			return this->username_;
		}
		
		std::string& password()
		{
			return this->password_;
		}

		std::string& character_name()
		{
			return this->character_name_;
		}

	private:
		std::string username_;
		std::string password_;
		std::string character_name_;
	};
}