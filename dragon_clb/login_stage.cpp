#include "login_stage.hpp"

#include "config.hpp"
#include "opcodes.hpp"

namespace game
{
	namespace stage
	{
		login_stage::login_stage(dragonnest* client, game::account& account)
			: stage_base(client), account(account)
		{
	
		}

		login_stage::~login_stage()
		{
			printf("[login_stage] leaving stage\n");
		}
		
		bool login_stage::on_enter_stage()
		{
			printf("[login_stage] entering stage\n");
			return this->send_check_version_request();
		}

		bool login_stage::handle_packet(unsigned short header, network::io_packet& packet)
		{
			switch (header)
			{
			case opcode::in::login::check_version:
				return this->on_check_version(packet);

			case opcode::in::login::check_login:
				return this->on_check_login(packet);

			case opcode::in::login::server_list:
				return this->on_server_list(packet);

			case opcode::in::login::character_list:
				return this->on_character_list(packet);

			case opcode::in::login::channel_list:
				return this->on_channel_list(packet);
				
			default:
				printf("[login_stage] unhandled packet with header: %04X\n", header);
				break;
			}

			return true;
		}
			
		bool login_stage::send_check_version_request()
		{
			network::io_packet packet(opcode::out::login::check_version);
			packet.write1(config::client::nation);							// cNation
			packet.write1(config::client::version);							// cVersion
			packet.write1(0x01);											// bCheck
			packet.write2(config::client::major_version);					// nMajorVersion
			packet.write2(config::client::minor_version);					// nMinorVersion
			
			return this->send_packet(packet);
		}

		bool login_stage::send_check_login_request()
		{
			network::io_packet packet(opcode::out::login::sea_check_login);
			packet.write_string(this->account.username(), 81);				// szAccountName
			packet.write_string(this->account.password(), 31);				// szPassword
			packet.write_string(inet_ntoa(this->get_local_ip()), 32);		// szVirtualIp
			packet.write_string("50-E5-49-3A-E9-18", 18);					// HWID
			packet.write2(0x0000);
			
			return this->send_packet(packet);
		}

		bool login_stage::send_server_list_request()
		{
			return this->send_packet(network::io_packet(opcode::out::login::server_list));
		}
		
		bool login_stage::send_select_server_request(unsigned char server_index)
		{
			network::io_packet packet(opcode::out::login::select_server);
			packet.write1(server_index);									// cServerIndex
			
			return this->send_packet(packet);
		}
		
		bool login_stage::send_select_character_request(unsigned int character_id)
		{
			network::io_packet packet(opcode::out::login::select_character);
			packet.write4(character_id);									// cCharIndex
			packet.write4(0x00000000);
			packet.write4(0x044F426D);
			
			packet.write4(0x00000007);
			packet.write4(0x00000007);
			packet.write4(0x00000007);
			packet.write4(0x00000008);

//			packet.write8(0x00000000);										// wszSecondAuthPW (wchar_t[4])

			//for (int i = 0; i < 4; i++)
			//{
			//	packet.write4(0x00000000);
			//}
		
			return this->send_packet(packet);
		}
		
		bool login_stage::send_select_channel_request(unsigned int channel_id)
		{
			network::io_packet packet(opcode::out::login::select_channel);
			packet.write4(channel_id);										// nChannelID

			return this->send_packet(packet);
		}

		bool login_stage::on_check_version(network::io_packet& packet)
		{
			return this->send_check_login_request();
		}

		bool login_stage::on_check_login(network::io_packet& packet)
		{
			unsigned int session_id = packet.read4();						// nSessionID
			unsigned char error_code = packet.read4();						// nRet

			switch (error_code)
			{
			case 0x04:
				printf("-> login failed (already logged in)\n");
				return false;

			case 0x66:
				printf("-> login failed (wrong infos)\n");
				return false;

			default:
				break;
			}
			
			return this->send_server_list_request();
		}

		bool login_stage::on_server_list(network::io_packet& packet)
		{
			this->worlds.clear();

			unsigned char world_count = packet.read1();						// cServerCount

			for (unsigned char i = 0; i < world_count; i++)
			{
				unsigned char world_id = packet.read1();					// cServerIndex
				std::string world_name = packet.read_wide_string(64);		// wszServerName (wchar_t[64])

				packet.read4();												// nWorldMaxUser
				packet.read4();												// nWorldCurUser
				packet.read1();												// bOnline
				packet.read1();												// <unknown>
				packet.read1();												// cMyCharCount

				this->worlds.push_back(world_info(world_id, world_name));
				//printf("-> World info (%d): %s\n", world_id, world_name.c_str());
			}

			return this->send_select_server_request(this->worlds[0].id());
		}
		
		bool login_stage::on_character_list(network::io_packet& packet)
		{
			this->characters.clear();

			unsigned int unknown_1 = packet.read4();						// nRet
			unsigned short unknown_2 = packet.read2();
			unsigned short unknown_3 = packet.read2();

			unsigned char character_count = packet.read1();					// cCharCount

			for (unsigned char i = 0; i < character_count; i++)
			{
				packet.read1();	// 0x01
				packet.read1();	// 0x01

				std::string character_name = packet.read_wide_string(17);	// wszCharacterName (wchar_t[17])
				unsigned int character_id = packet.read4();					// 

				this->characters.push_back(character_info(character_id, character_name));
				printf("-> Character info (%d): %s | %08X\n", i, character_name.c_str(), character_id);

				packet.indent(197);
			}
			
			for (character_info& character : this->characters)
			{
				if (utility::lower(character.name()).compare(this->account.character_name()) == 0)
				{
					return this->send_select_character_request(character.id());
				}
			}
			
			printf("-> [WARNING - ERROR] Couldn't find desired character.\n");
			return true;
		}
		
		bool login_stage::on_channel_list(network::io_packet& packet)
		{
			this->channels.clear();

			packet.read4();													// nRet
			packet.read1();													// cFailCount

			unsigned char channel_count = packet.read1();					// cCount
			
			for (unsigned char i = 0; i < channel_count; i++)
			{
				unsigned int channel_id = packet.read4();					// nChannelID
				packet.read2();												// nChannelIdx
				unsigned int map_id = packet.read4();						// nMapIdx
				packet.read2();												// nCurrentUserCount
				packet.read2();												// nMaxUserCount
				packet.read4();												// nChannelAttribute
				packet.read4();												// nMeritBonusID
				packet.read1();												// cMinLevel
				packet.read1();												// cMaxLevel
				packet.read1();												// cVillageID
				packet.read1();												// bVisibility
				std::string channel_ip = packet.read_string(32);			// szIP (char[32])
				unsigned short channel_port = packet.read2();				// nPort
				packet.read4();												// nLimitLevel
				packet.read1();												// bShow
				packet.indent(7);											// <unknown>

				if (map_id != 40 && map_id != 23)
					this->channels.push_back(channel_info(channel_id, channel_ip, channel_port));

				//printf("-> Channel info (%d): %d - %s:%d\n", i, channel_id, channel_ip.c_str(), channel_port);
			}

			return this->send_select_channel_request(this->channels[0].id());
		}
	}
}