#pragma once

#include "generic.hpp"
#include "dragonnest.hpp"

namespace dragonnest_clb
{
	class dragonnest_pool
	{
	public:
		static dragonnest_pool& get_instance()
		{
			static dragonnest_pool manager_pool;
			return manager_pool;
		}

		bool create_instance(game::account& account);
		bool delete_instance(std::string const& username);
		
		bool on_socket(SOCKET desc, unsigned short type);

	private:
		dragonnest_pool();
		virtual ~dragonnest_pool();
		
		bool find(std::string const& username, std::function<bool(std::vector<std::shared_ptr<game::dragonnest>>::iterator& iterator)> fn = 
			[](std::vector<std::shared_ptr<game::dragonnest>>::iterator& iterator) -> bool { return true; });

		bool find(SOCKET desc, std::function<bool(std::vector<std::shared_ptr<game::dragonnest>>::iterator& iterator)> fn = 
			[](std::vector<std::shared_ptr<game::dragonnest>>::iterator& iterator) -> bool { return true; });

	private:
		std::vector<std::shared_ptr<game::dragonnest>> pool;
	};
}