#include "dragonnest.hpp"

#include "dragonnest_pool.hpp"
#include "main_form.hpp"

#include "config.hpp"

#include "field_stage.hpp"
#include "login_stage.hpp"
#include "village_stage.hpp"

namespace game
{
	dragonnest::dragonnest(game::account& account)
		: stage(nullptr), account_(account)
	{

	}

	dragonnest::~dragonnest()
	{
		this->stage.reset();
	}
	
	bool dragonnest::reset()
	{
		return this->migrate(server_type::login, config::server::ip, config::server::port);
	}

	bool dragonnest::migrate(server_type type, std::string const& ip, unsigned short tcp_port, unsigned short rudp_port)
	{
		this->stage.reset();

        switch (type)
        {
        case server_type::login:
			this->stage = std::make_unique<stage::login_stage>(this, this->account());
            break;
			
        case server_type::village:
			this->stage = std::make_unique<stage::village_stage>(this, this->session());
            break;

		case server_type::field:
			this->stage = std::make_unique<stage::field_stage>(this, this->session(), rudp_port);
			break;

		default:
			printf("Unknown stage requested!\n");
			break;
        }
		
		return (this->stage.get() && this->stage.get()->connect(ip, tcp_port));
	}
	
	bool dragonnest::notify_close(SOCKET desc)
	{
		//printf("[dragonnest] %s notify_close\n", this->account().username().c_str());
		return this->reset();
	}

	bool dragonnest::notify_connect(SOCKET desc)
	{
		//printf("[dragonnest] %s notify_connect\n", this->account().username().c_str());
		return this->stage.get()->handle_connect();
	}
	
	bool dragonnest::notify_read(SOCKET desc)
	{
		//printf("[dragonnest] %s notify_read\n", this->account().username().c_str());

		if (!this->stage.get()->handle_read())
		{
			return this->reset();
		}

		return true;
	}

	bool dragonnest::notify_write(SOCKET desc)
	{
		//printf("[dragonnest] %s notify_write\n", this->account().username().c_str());
		return true;
	}
	
	SOCKET dragonnest::get_desc() const
	{
		return (this->stage.get() ? this->stage.get()->get_desc() : INVALID_SOCKET);
	}

	account& dragonnest::account()
	{
		return this->account_;
	}

	session& dragonnest::session()
	{
		return this->session_;
	}
}