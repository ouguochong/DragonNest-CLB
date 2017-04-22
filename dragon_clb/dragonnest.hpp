#pragma once

#include "generic.hpp"

#include "account.hpp"
#include "session.hpp"

#include "stage_base.hpp"

namespace game
{
	enum class server_type
	{
        login,
		village,
		field
	};

	class dragonnest
	{
	public:
		dragonnest(game::account& account);
		~dragonnest();

		bool reset();

		bool migrate(server_type type, std::string const& ip, unsigned short tcp_port, unsigned short rudp_port = 0);
	
		bool notify_close(SOCKET desc);
		bool notify_connect(SOCKET desc);
		bool notify_read(SOCKET desc);
		bool notify_write(SOCKET desc);

		SOCKET get_desc() const;

		game::account& account();
		game::session& session();

	private:
		game::account account_;
		game::session session_;
		
		std::unique_ptr<stage::stage_base> stage;
	};
}