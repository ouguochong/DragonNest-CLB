#pragma once

#include "generic.hpp"
#include "stage_base.hpp"

#include "dragonnest.hpp"

#include "session.hpp"

namespace game
{
	namespace stage
	{
		class village_stage : public stage_base
		{
		public:
			village_stage(dragonnest* client, session& session);
			~village_stage();
			
		protected:
			bool on_enter_stage();
			bool handle_packet(unsigned short header, network::io_packet& packet);

		private:
			bool send_connect_village_request();
			bool send_village_ready_request();

			bool send_character_enter();
			bool send_character_complete_loading();

			bool send_refresh_gate_info(bool enter, float x, float y, float z);
			bool send_start_stage(char select_dungeon_index, int difficulty);
			
			bool on_connect_village(network::io_packet& packet);
			bool on_map_info(network::io_packet& packet);

			bool on_refresh_gate_info(network::io_packet& packet);
			
		private:
			session session;
		};
	}
}