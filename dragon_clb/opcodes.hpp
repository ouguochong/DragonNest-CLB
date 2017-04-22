#pragma once

#include "generic.hpp"

namespace game
{
	namespace opcode
	{
		namespace out
		{
			namespace login
			{
				enum
				{
					check_version				= 0x0101,
					check_login					= 0x0201,
					server_list					= 0x0301,
					select_server				= 0x0401,
					select_character			= 0x0501,
					create_character			= 0x0601,
					delete_character			= 0x0701,
					select_channel				= 0x0801,
					back_button					= 0x0901,
					channel_list				= 0x0A01,
					back_button_login			= 0x0B01,

					sea_check_login				= 0x2301,
					europe_check_login			= 0x2801,
				};
			}
			
			namespace system
			{
				enum
				{
					connect_village				= 0x0102,
					village_ready				= 0x0202,
					connect_request				= 0x0302,
					connect_game				= 0x0402,
					ready_to_receive			= 0x0502,
					intended_disconnect			= 0x0602,
					peer_disconnect				= 0x0702,
					reconnect_login				= 0x0802,
					abandon_stage				= 0x0902,
				};
			}
			
			namespace character
			{
				enum
				{
					enter						= 0x0103,
					add_quick_slot				= 0x0203,
					delete_quick_slot			= 0x0303,
					look_user					= 0x0403,
					complete_loading			= 0x0503,
				};
			}

			namespace party
			{
				enum
				{
					create_party				= 0x0106,
					destroy_party				= 0x0206,
					join_party					= 0x0306,
					party_list_info				= 0x0406,
					refresh_gate_info			= 0x0506,
					start_stage					= 0x0606,
				};
			}
		}

		namespace in
		{
			namespace login
			{
				enum
				{
					check_version				= 0x0101,
					check_login					= 0x0201,
					server_list					= 0x0301,
					character_list				= 0x0401,
					channel_list				= 0x0501,
					select_character			= 0x0601,
					create_character			= 0x0701,
					delete_character			= 0x0801,
					tutorial_map_info			= 0x0901,
					wait_user					= 0x0A01,
					back_button_login			= 0x0B01,
					check_block					= 0x0C01,
				};
			}
			
			namespace system
			{
				enum
				{
					village_info				= 0x0102,
					connect_village				= 0x0202,
					game_info					= 0x0302,
					peer_connect_request		= 0x0402,
					connected_result			= 0x0502,
					tcp_connect_req				= 0x0602,
					reconnect_req				= 0x0702,
					reconnect_login				= 0x0802,
					countdown_msg				= 0x0902
				};
			}
			
			namespace character
			{
				enum
				{
					map_info					= 0x0103,
					enter						= 0x0203,
					enter_user					= 0x0303,
					leave_user					= 0x0403,
					enter_npc					= 0x0503,
					leave_npc					= 0x0603,
				};
			}
			
			namespace party
			{
				enum
				{
					create_party				= 0x0106,
					destroy_party				= 0x0206,
					join_party					= 0x0306,
					refresh_party				= 0x0406,
					party_out					= 0x0506,
					party_list_info				= 0x0606,
					refresh_gate_info			= 0x0706,
					gate_info					= 0x0806,
				};
			}
		}
	}
}