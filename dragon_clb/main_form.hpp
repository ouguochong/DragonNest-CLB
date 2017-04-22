#pragma once

#include "generic.hpp"
#include "widget.hpp"

#include "resource.hpp"

#include "button.hpp"
#include "label.hpp"
#include "listbox.hpp"
#include "listview.hpp"
#include "textbox.hpp"

#include "account.hpp"

namespace dragonnest_clb
{
	WPARAM execute();

	namespace gui
	{
		#define COLOR_RED		RGB(255, 0, 0)
		#define COLOR_YELLOW	RGB(255, 255, 0)
		#define COLOR_WHITE		RGB(255, 255, 255)

		enum class table_update_type
		{
			stage,
			current_action
		};

		class main_form : public widget
		{
		public:
			static main_form& get_instance()
			{
				static main_form instance("sparta_window_class", "Sparta Bot - DragonNest CLB", MAKEINTRESOURCE(IDI_MAIN_ICON));
				return instance;
			}
			
			void on_tab();
			void on_enter();

			bool update_table_data(table_update_type type, std::size_t index, unsigned int value);
			bool update_table_string(table_update_type type, std::size_t index, std::string const& string);

		private:
			main_form(std::string const& class_name, std::string const& window_name, const char* icon_name = IDI_APPLICATION);
			~main_form();

			bool initialize();

			bool add_profile_to_list(std::string const& username, std::string const& password, std::string const& character_name);

			void load_profiles();
			void save_profiles();

			bool register_class(std::string const& class_name, const char* icon_name);
			bool create_window(std::string const& class_name, std::string const& window_name, rectangle& rect);

			void set_message_handlers();

		private:
			HMENU menu_bar;
			std::vector<std::unique_ptr<widget>> widget_container;

			listview profile_list;
			textbox profile_username;
			textbox profile_password;
			textbox profile_character_name;
			button profile_add;
			
			listview activity_list;
			
			std::vector<game::account> accounts;
		};
	}
}