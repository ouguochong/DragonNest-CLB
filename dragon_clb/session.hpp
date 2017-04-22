#pragma once

#include "generic.hpp"

namespace game
{
	class session
	{
	public:
		session()
			: session_id_(0), account_database_id_(0), certifying_key_(0)
		{
		
		}
		
		session(unsigned int session_id, unsigned int account_database_id, unsigned __int64 certifying_key)
			: session_id_(session_id), account_database_id_(account_database_id), certifying_key_(certifying_key)
		{
		
		}

		~session()
		{
		
		}
		
		unsigned int& session_id()
		{
			return this->session_id_;
		}
		
		unsigned int& account_database_id()
		{
			return this->account_database_id_;
		}
		
		unsigned __int64& certifying_key()
		{
			return this->certifying_key_;
		}

	private:
		unsigned int session_id_;
		unsigned int account_database_id_;
		unsigned __int64 certifying_key_;
	};
}