#include "io_packet.hpp"

#include <codecvt>

namespace game
{
	namespace network
	{
		io_packet::io_packet()
			: offset(0)
		{

		}

		io_packet::io_packet(unsigned char* data, std::size_t size)
			: offset(0), data(data, data + size)
		{

		}

		io_packet::io_packet(unsigned short header)
		{
			this->write<unsigned short>(header);
		}

		io_packet::~io_packet()
		{
			data.clear();
		}

		void io_packet::write1(unsigned char data)
		{
			return this->write<unsigned char>(data);
		}

		void io_packet::write2(unsigned short data)
		{
			return this->write<unsigned short>(data);
		}

		void io_packet::write4(unsigned int data)
		{
			return this->write<unsigned int>(data);
		}

		void io_packet::write8(unsigned __int64 data)
		{
			return this->write<unsigned __int64>(data);
		}
		
		void io_packet::write_float(float data)
		{
			return this->write<float>(data);
		}

		void io_packet::write_buffer(unsigned char* data, std::size_t size)
		{
			std::copy(data, data + size, std::back_inserter(this->data));
		}

		void io_packet::write_string(std::string const& data, std::size_t fixed_size)
		{
			std::copy(data.begin(), data.end(), std::back_inserter(this->data));

			if (fixed_size && (data.length() < fixed_size))
			{
				this->write_zero(fixed_size - data.length());
			}
		}

		void io_packet::write_wide_string(std::string const& data, std::size_t fixed_size)
		{
			std::wstring wide_data_string = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(data);
			const unsigned char* wide_data = reinterpret_cast<const unsigned char*>(wide_data_string.c_str());

			std::copy(&wide_data[0], &wide_data[wide_data_string.length() * 2], std::back_inserter(this->data));
			
			if (fixed_size && ((wide_data_string.length() * 2) < (fixed_size * 2)))
			{
				this->write_zero((fixed_size * 2) - (wide_data_string.length() * 2));
			}
		}

		void io_packet::write_zero(std::size_t length)
		{
			for (std::size_t i = 0; i < length; i++)
			{
				this->data.push_back(0);
			}
		}

		unsigned char io_packet::read1()
		{
			return this->read<unsigned char>();
		}

		unsigned short io_packet::read2()
		{
			return this->read<unsigned short>();
		}

		unsigned int io_packet::read4()
		{
			return this->read<unsigned int>();
		}

		unsigned __int64 io_packet::read8()
		{
			return this->read<unsigned __int64>();
		}

		bool io_packet::read_buffer(unsigned char* buffer, std::size_t length)
		{
			if (!this->check_offset(length))
			{
				return false;
			}

			memcpy(buffer, this->data.data() + this->offset, length);
			this->offset += length;
			return true;
		}

		std::string io_packet::read_string(std::size_t fixed_size)
		{
			if (!fixed_size)
			{
				for (std::size_t length = 0; ; length++)
				{
					if (!this->check_offset(sizeof(char)) || this->read1() == 0)
					{
						return std::string(reinterpret_cast<char*>(this->data.data() + this->offset - (length + 1)), length);
					}
				}
			}
			else
			{
				this->offset += fixed_size;
				return std::string(reinterpret_cast<char*>(this->data.data() + this->offset - fixed_size));
			}

			return std::string("");
		}
		
		std::string io_packet::read_wide_string(std::size_t fixed_size)
		{
			if (!fixed_size)
			{
				for (std::size_t length = 0; ; length++)
				{
					if (!this->check_offset(sizeof(wchar_t)) || this->read2() == 0)
					{
						return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(std::wstring(
							reinterpret_cast<wchar_t*>(this->data.data() + this->offset - ((length * 2) + 2)), length));
					}
				}
			}
			else
			{
				this->offset += (fixed_size * 2);

				return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(std::wstring(
					reinterpret_cast<wchar_t*>(this->data.data() + this->offset - (fixed_size * 2))));
			}

			return std::string("");
		}

		bool io_packet::indent(std::size_t length)
		{
			if (!this->check_offset(length))
			{
				return false;
			}
		
			this->offset += length;
			return true;
		}

		unsigned char* io_packet::get_data()
		{
			return this->data.data();
		}

		unsigned short io_packet::get_header()
		{
			return *reinterpret_cast<unsigned short*>(this->data.data());
		}

		unsigned int io_packet::get_size()
		{
			return this->data.size();
		}
			
		unsigned int io_packet::get_remaining_size()
		{
			return (this->data.size() - this->offset);
		}

		bool io_packet::check_offset(std::size_t size)
		{
			return ((this->offset + size) <= static_cast<std::size_t>(this->data.size()));
		}
	}
}