#pragma once

#include "generic.hpp"
#include "stage_base.hpp"

#include "dragonnest.hpp"

#include "account.hpp"

#include "channel_info.hpp"
#include "character_info.hpp"
#include "world_info.hpp"

namespace game
{
	namespace stage
	{
		class login_stage : public stage_base
		{
		public:
			login_stage(dragonnest* client, account& account);
			~login_stage();
			
		protected:
			bool on_enter_stage();
			bool handle_packet(unsigned short header, network::io_packet& packet);
	
		private:
			bool send_check_version_request();
			bool send_check_login_request();
			bool send_server_list_request();
			bool send_select_server_request(unsigned char server_index);
			bool send_select_character_request(unsigned int character_id);
			bool send_select_channel_request(unsigned int channel_id);

			bool on_check_version(network::io_packet& packet);
			bool on_check_login(network::io_packet& packet);
			bool on_server_list(network::io_packet& packet);
			bool on_character_list(network::io_packet& packet);
			bool on_channel_list(network::io_packet& packet);

		private:
			account account;

			std::vector<world_info> worlds;
			std::vector<channel_info> channels;
			std::vector<character_info> characters;
		};
	}
}