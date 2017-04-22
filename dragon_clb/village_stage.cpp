#include "village_stage.hpp"

#include "config.hpp"
#include "opcodes.hpp"
#include "output.hpp"

namespace game
{
	namespace stage
	{
		village_stage::village_stage(dragonnest* client, game::session& session)
			: stage_base(client), session(session)
		{

		}

		village_stage::~village_stage()
		{
			printf("[village_stage] leaving stage\n");
		}

		bool village_stage::on_enter_stage()
		{
			printf("[village_stage] entering stage\n");
			return this->send_connect_village_request();
		}

		bool village_stage::handle_packet(unsigned short header, network::io_packet& packet)
		{
			switch (header)
			{
			case opcode::in::system::connect_village:
				return this->on_connect_village(packet);

			case opcode::in::character::map_info:
				return this->on_map_info(packet);

			case opcode::in::party::refresh_gate_info:
				return this->on_refresh_gate_info(packet);

			default:
				printf("[village_stage] unhandled packet with header: %04X\n", header);
			}

			return true;
		}

		bool village_stage::send_connect_village_request()
		{
			network::io_packet packet(opcode::out::system::connect_village);
			packet.write4(this->session.session_id());						// nSessionID
			packet.write4(this->session.account_database_id());				// nAccountDBID
			packet.write8(this->session.certifying_key());					// biCertifyingKey
			packet.write_wide_string(inet_ntoa(this->get_local_ip()), 32);	// wszVirtualIp

			return this->send_packet(packet);
		}
		
		bool village_stage::send_village_ready_request()
		{
			network::io_packet packet(opcode::out::system::village_ready);
			packet.write1(0x01);											// boFirst
			packet.write1(0x00);											// boChannelMove

			return this->send_packet(packet);
		}
		
		bool village_stage::send_character_enter()
		{
			return this->send_packet(network::io_packet(opcode::out::character::enter));
		}

		bool village_stage::send_character_complete_loading()
		{
			return this->send_packet(network::io_packet(opcode::out::character::complete_loading));
		}
		
		bool village_stage::send_refresh_gate_info(bool enter, float x, float y, float z)
		{
			network::io_packet packet(opcode::out::party::refresh_gate_info);
			packet.write_float(x);											// Position::x
			packet.write_float(y);											// Position::y
			packet.write_float(z);											// Position::z
			packet.write1(static_cast<unsigned char>(enter));				// boEnter

			return this->send_packet(packet);
		}
		
		bool village_stage::send_start_stage(char select_dungeon_index, int difficulty)
		{
			network::io_packet packet(opcode::out::party::start_stage);
			packet.write1(0x00);											// bReturnVillage
			packet.write4(difficulty);										// cDifficulty
			packet.write1(select_dungeon_index);							// cSelectDungeonIndex
			packet.write4(0x00000000);										// 
			packet.write2(0x0000);											//
			packet.write1(0x00);											//
			
			return this->send_packet(packet);
		}
		
		bool village_stage::on_connect_village(network::io_packet& packet)
		{
			packet.read2();													// nRet
			packet.read_string(16);											// szServerVersion (char[16])
			packet.read4();													// nChannelAttr
			packet.read1();													// cWithoutLoading

			return this->send_village_ready_request();
		}
		
		bool village_stage::on_map_info(network::io_packet& packet)
		{
			packet.read4();													// nMapIndex
			packet.read1();													// cMapArrayIndex
			packet.read1();													// cEnviIndex
			packet.read1();													// cEnviArrayIndex
			packet.read4();													// nChannelID
			packet.read2();													// wChannelIdx

			packet.read4();													// TMeritInfo::nID
			packet.read4();													// TMeritInfo::nMinLevel
			packet.read4();													// TMeritInfo::nMaxLevel
			packet.read4();													// TMeritInfo::nMeritType
			packet.read4();													// TMeritInfo::nExtendValue

			return (this->send_character_enter() && this->send_character_complete_loading() && 
				this->send_refresh_gate_info(true, 2078.991455f, -1572.797485f, 2037.664673f));
		}

		bool village_stage::on_refresh_gate_info(network::io_packet& packet)
		{
			return this->send_start_stage(-1, 0);
		}
	}
}