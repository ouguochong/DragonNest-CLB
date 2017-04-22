#include "main_form.hpp"
#include "dragonnest_pool.hpp"

#include <fstream>
#include <regex>

namespace dragonnest_clb
{
	WPARAM execute()
	{
		MSG message;
		while (GetMessage(&message, 0, 0, 0) > 0)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		return message.wParam;
	}

	namespace gui
	{
		void main_form::on_tab()
		{
			if (GetFocus() == this->profile_username.get_handle())
			{
				SetFocus(this->profile_password.get_handle());
			}
			else if (GetFocus() == this->profile_password.get_handle())
			{
				SetFocus(this->profile_character_name.get_handle());
			}
			else if (GetFocus() == this->profile_character_name.get_handle())
			{
				SetFocus(this->profile_username.get_handle());
			}
		}

		void main_form::on_enter()
		{
			if (GetFocus() == this->profile_username.get_handle() || 
				GetFocus() == this->profile_password.get_handle() ||
				GetFocus() == this->profile_character_name.get_handle())
			{
				this->profile_add.push();
			}
		}

		main_form::main_form(std::string const& class_name, std::string const& window_name, const char* icon_name) 
			: widget(0, 0, true), menu_bar(NULL)
		{
			if (!this->register_class(class_name, icon_name))
			{
				throw std::exception("Failed to register main window class!");
			}

			if (!this->create_window(class_name, window_name, rectangle(0, 0, 455, 410)))
			{
				throw std::exception("Failed to create main window!");
			}

			if (!this->initialize())
			{
				throw std::exception("Failed to initialize main window!");
			}

			this->set_message_handlers();
		}

		main_form::~main_form()
		{

		}

		bool main_form::initialize()
		{
			this->accounts.clear();

			/* menu */
			this->menu_bar = CreateMenu();
			AppendMenu(this->menu_bar, MF_STRING, IDM_CREATE_NEW_INSTANCE, "+");
			AppendMenu(this->menu_bar, MF_STRING, IDM_DESTROY_CURRENT_INSTANCE, "-");
			
			SetMenu(this->get_handle(), this->menu_bar);
			
			/* profile list */
			this->profile_list.assemble(*this, rectangle(5, 5, 225, 300), [this](std::size_t index) -> void
			{
				if (dragonnest_pool::get_instance().create_instance(this->accounts.at(index)))
				{
					this->activity_list.add_item(2, this->accounts.at(index).username().c_str(),
						this->accounts.at(index).character_name().c_str());	
				}
			});
			
			this->profile_list.add_column("username", 200);
			
			this->profile_list.add_menu_item("remove profile", [this](std::size_t index) -> void
			{
				this->accounts.erase(this->accounts.begin() + index);
				this->profile_list.remove_item(index);
			});
			this->profile_list.add_menu_item("clear profiles", [this](std::size_t index) -> void
			{
				this->accounts.clear();
				this->profile_list.clear_items();
			});

			/* profile information */
			this->widget_container.push_back(std::make_unique<label>(*this, "Username", rectangle(10, 310, 50, 20)));
			this->profile_username.assemble(*this, rectangle(70, 310, 160, 20), false, false, true);
			this->profile_username.set_cue_banner("Enter username here...");
			this->profile_username.set_maximum_length(30);
			this->profile_username.set_text("creativ3");

			this->widget_container.push_back(std::make_unique<label>(*this, "Password", rectangle(10, 335, 50, 20)));
			this->profile_password.assemble(*this, rectangle(70, 335, 160, 20), false, true, true);
			this->profile_password.set_cue_banner("Enter password here...");
			this->profile_password.set_maximum_length(17);
			this->profile_password.set_text("salamangkero");

			this->widget_container.push_back(std::make_unique<label>(*this, "Character", rectangle(10, 360, 50, 20)));
			this->profile_character_name.assemble(*this, rectangle(70, 360, 160, 20), false, false, true);
			this->profile_character_name.set_cue_banner("Enter character name here...");
			this->profile_character_name.set_maximum_length(17);
			this->profile_character_name.set_text("oKANEo");

			this->profile_add.assemble(*this, "Add profile", rectangle(5, 385, 225, 20), [this]() -> bool
			{
				std::string username = utility::lower(this->profile_username.get_text());
				std::string password = this->profile_password.get_text();
				std::string character_name = utility::lower(this->profile_character_name.get_text());

				if (!this->add_profile_to_list(username, password, character_name))
				{
					return false;
				}
				
				this->profile_username.set_text("");
				this->profile_password.set_text("");
				this->profile_character_name.set_text("");
				return true;
			});

			this->load_profiles();

			/* activity list */
			this->activity_list.assemble(*this, rectangle(235, 5, 215, 400), [this](std::size_t index) -> void
			{
				if (dragonnest_pool::get_instance().delete_instance(this->activity_list.get_item_text(index, 0)))
				{
					this->activity_list.remove_item(index);	
				}
			});
			
			this->activity_list.add_column("username", 100);
			this->activity_list.add_column("character name", 90);
			return true;
		}
		
		bool main_form::add_profile_to_list(std::string const& username, std::string const& password, std::string const& character_name)
		{
			if (username.empty() || password.empty() || character_name.empty())
			{
				return false;
			}

			if (!this->accounts.empty())
			{
				std::vector<game::account>::iterator iterator = std::find_if(this->accounts.begin(), this->accounts.end(), 
					[&](game::account& account) -> bool
				{ 
					return (utility::upper(account.username()).compare(utility::upper(username)) == 0);
				});

				if (iterator != this->accounts.end())
				{
					return false;	
				}
			}

			this->accounts.push_back(game::account(username, password, character_name));
			return this->profile_list.add_item(1, username.c_str());
		}
		
		void main_form::load_profiles()
		{
			std::ifstream profile_file("sparta_profiles.txt");

			if (profile_file.is_open())
			{
				std::string profile_string;

				while (profile_file.good())
				{
					std::getline(profile_file, profile_string);
				
					if (profile_string.empty())
					{
						continue;
					}

					std::regex regex_pattern("(.+)\\|(.+)\\|(.+)");
					std::smatch matches;

					if (std::regex_search(profile_string, matches, regex_pattern))
					{
						try
						{
							this->add_profile_to_list(matches[1], matches[2], matches[3]);
						}
						catch (...)	
						{ 
							continue;
						}
					}
				}

				profile_file.close();
			}
		}

		void main_form::save_profiles()
		{
			std::ofstream profile_file("sparta_profiles.txt");

			if (profile_file.is_open())
			{
				for (game::account& account : this->accounts)
				{
					if (profile_file.good())
					{
						profile_file << account.username() << "|" << account.password() << "|" << account.character_name() << "\n";
					}
				}

				profile_file.close();
			}
		}

		bool main_form::register_class(std::string const& class_name, const char* icon_name)
		{
			WNDCLASSEX wcex;
			memset(&wcex, 0, sizeof(WNDCLASSEX));

			wcex.cbSize = sizeof(WNDCLASSEX);
			wcex.lpfnWndProc = main_form::window_proc;
			wcex.hInstance = this->get_instance_handle();
			wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
			wcex.hIcon = LoadIcon(this->get_instance_handle(), icon_name);
			wcex.hIconSm = LoadIcon(this->get_instance_handle(), icon_name);
			wcex.hbrBackground = CreateSolidBrush(this->get_background_color());
			wcex.lpszClassName = class_name.c_str();

			return (RegisterClassEx(&wcex) != NULL);
		}

		bool main_form::create_window(std::string const& class_name, std::string const& window_name, rectangle& rect)
		{
			RECT rc_window = { rect.get_x(), rect.get_y(), rect.get_width(), rect.get_height() };
			AdjustWindowRect(&rc_window, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, TRUE);
			
			rect.set_width_height(rc_window.right - rc_window.left, rc_window.bottom - rc_window.top);

			if (!rect.get_x() && !rect.get_y())
			{
				RECT rc;
				SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);

				rect.set_x_y((rc.right / 2) - (rect.get_width() / 2), (rc.bottom / 2) - (rect.get_height() / 2));
			}
			
			return this->create(class_name, window_name, rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
		}
		
		void main_form::set_message_handlers()
		{
			this->add_message_handlers(6,
				message_pair(WM_CLOSE, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					this->save_profiles();
					DestroyWindow(hWnd);
					return 0;
				}),
				message_pair(WM_DESTROY, [](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					PostQuitMessage(0);
					return 0;
				}),
				message_pair(WM_CTLCOLORLISTBOX, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					return SendMessage(reinterpret_cast<HWND>(lParam), CUSTOM_CTLCOLOR, wParam, lParam);
				}),
				message_pair(WM_NOTIFY, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					return SendMessage(reinterpret_cast<NMHDR*>(lParam)->hwndFrom, CUSTOM_NOTIFY, wParam, lParam);
				}),
				message_pair(WM_COMMAND, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					if (LOWORD(wParam) == IDM_CREATE_NEW_INSTANCE)
					{

					}
					else if (LOWORD(wParam) == IDM_DESTROY_CURRENT_INSTANCE)
					{

					}

					return SendMessage(reinterpret_cast<HWND>(lParam), CUSTOM_COMMAND, wParam, lParam);
				}),
				message_pair(WM_SOCKET, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					if (WSAGETSELECTERROR(lParam))
					{
						printf("WSAGETSELECTERROR - %d\n", WSAGETSELECTERROR(lParam));
						return 0;
					}

					return static_cast<LRESULT>(dragonnest_pool::get_instance().on_socket(
						static_cast<SOCKET>(wParam), WSAGETSELECTEVENT(lParam)));
				})	
			);
		}
	}
}