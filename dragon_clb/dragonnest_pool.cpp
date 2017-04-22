#include "dragonnest_pool.hpp"

namespace dragonnest_clb
{
	bool dragonnest_pool::create_instance(game::account& account)
	{
		if (this->find(account.username()))
		{
			return false;
		}

		std::shared_ptr<game::dragonnest> dragonnest(std::make_shared<game::dragonnest>(account));

		if (!dragonnest.get()->reset())
		{
			return false;
		}

		this->pool.push_back(dragonnest);
		return true;
	}
	
	bool dragonnest_pool::delete_instance(std::string const& username)
	{
		return this->find(username, [this](std::vector<std::shared_ptr<game::dragonnest>>::iterator& iterator) -> bool
		{
			this->pool.erase(iterator);
			return true;
		});
	}
	
	bool dragonnest_pool::on_socket(SOCKET desc, unsigned short type)
	{
		return this->find(desc, [&](std::vector<std::shared_ptr<game::dragonnest>>::iterator& iterator) -> bool
		{
			switch (type)
			{
			case FD_CLOSE:
				return (*iterator).get()->notify_close(desc);

			case FD_CONNECT:
				return (*iterator).get()->notify_connect(desc);

			case FD_READ:
				return (*iterator).get()->notify_read(desc);

			case FD_WRITE:
				return (*iterator).get()->notify_write(desc);

			default:
				break;
			}

			return false;
		});
	}
	
	dragonnest_pool::dragonnest_pool()
	{
		this->pool.clear();
	}

	dragonnest_pool::~dragonnest_pool()
	{
		this->pool.clear();
	}
	
	bool dragonnest_pool::find(std::string const& username, std::function<bool(std::vector<std::shared_ptr<game::dragonnest>>::iterator& iterator)> fn)
	{
		if (this->pool.empty())
		{
			return false;
		}

		std::vector<std::shared_ptr<game::dragonnest>>::iterator iterator = std::find_if(this->pool.begin(), this->pool.end(), 
			[&](std::shared_ptr<game::dragonnest> const& dragonnest) -> bool 
		{ 
			return (utility::upper(username).compare(utility::upper(dragonnest.get()->account().username())) == 0);
		});

		if (iterator == this->pool.end())
		{
			return false;	
		}

		return fn(iterator);
	}
	
	bool dragonnest_pool::find(SOCKET desc, std::function<bool(std::vector<std::shared_ptr<game::dragonnest>>::iterator& iterator)> fn)
	{
		if (this->pool.empty())
		{
			return false;
		}

		std::vector<std::shared_ptr<game::dragonnest>>::iterator iterator = std::find_if(this->pool.begin(), this->pool.end(), 
			[&](std::shared_ptr<game::dragonnest> const& dragonnest) -> bool 
		{ 
			return (desc == dragonnest.get()->get_desc());
		});

		if (iterator == this->pool.end())
		{
			return false;	
		}

		return fn(iterator);
	}
}