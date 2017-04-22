#pragma once

#include "generic.hpp"
#include "widget.hpp"

#pragma comment(lib, "comctl32")

namespace dragonnest_clb
{
	namespace gui
	{

		template <typename T>
		class tab_control : public widget
		{
			typedef struct _tab_item
			{
				TCITEMHEADER item_header;
				T custom_data;
			} tab_item;
			
			typedef std::function<bool(int index, T data)> on_removed_tab_event_t;

		public:
			tab_control(HINSTANCE instance = NULL)
				: widget(0, instance), current_index(0), has_remove_event(false), on_removed_tab_event(nullptr)
			{
				this->relatives.clear();
			}

			~tab_control()
			{
				this->relatives.clear();
			}

			bool assemble(widget& parent_widget, rectangle& rect, bool has_remove_event = false)
			{
				if (!this->create(WC_TABCONTROL, "", rect, WS_VISIBLE | WS_CHILD, NULL, parent_widget.get_handle()))
				{
					return false;
				}

				this->has_remove_event = has_remove_event;
				this->set_message_handlers();

				TabCtrl_SetItemExtra(this->get_handle(), sizeof(T));
				TabCtrl_SetPadding(this->get_handle(), 10, 0);
				return true;			
			}
			
			bool add_tab(std::string const& caption)
			{
				tab_item tab_control_item;
				tab_control_item.item_header.mask = TCIF_TEXT | TCIF_PARAM; 
				tab_control_item.item_header.pszText = const_cast<char*>(caption.c_str());
				tab_control_item.custom_data = NULL;

				int index = TabCtrl_InsertItem(this->get_handle(), this->current_index, &tab_control_item);

				if (index == -1)
				{
					return false;
				}
				
				if (this->relatives.size() <= static_cast<std::size_t>(this->current_index))
				{
					this->relatives.resize(this->current_index + 1);
				}
				
				this->set_current_selection(this->current_index);
				this->current_index++;
				return true;
			}

			bool remove_tab(int index)
			{
				if (on_removed_tab_event)
				{
					if (!on_removed_tab_event(index, this->get_data(index)))
					{
						return false;
					}
				}

				if (index == this->get_current_selection())
				{
					if ((index + 1) < this->get_item_count())
					{
						this->next();
					}
					else if (this->get_item_count() >= 2)
					{
						this->prev();
					}
					else if (this->get_item_count() == 1)
					{
						this->set_current_selection(0);
					}
					else
					{
						this->set_current_selection(-1);
					}
				}

				if (!TabCtrl_DeleteItem(this->get_handle(), index))
				{
					return false;
				}
				
				this->activate_relative(index, false);
				this->relatives.erase(this->relatives.begin() + index);
				this->current_index--;
				return true;
			}

			bool rename_tab(int index, std::string const& caption)
			{
				tab_item tab_control_item;
				tab_control_item.item_header.mask = TCIF_TEXT; 
				tab_control_item.item_header.pszText = const_cast<char*>(caption.c_str());

				return (TabCtrl_SetItem(this->get_handle(), index, &tab_control_item) != FALSE);
			}

			int get_item_count()
			{
				return TabCtrl_GetItemCount(this->get_handle());
			}

			int get_current_selection()
			{
				return TabCtrl_GetCurSel(this->get_handle());
			}

			bool set_current_selection(int index)
			{
				int old_selection = TabCtrl_SetCurSel(this->get_handle(), index);

				if (old_selection != -1)
				{
					this->activate_relative(old_selection, false);
					this->activate_relative(index, true);
				}

				return (old_selection != -1);
			}
			
			void prev()
			{
				int current_selection = this->get_current_selection();

				if (current_selection != -1)
				{
					if ((current_selection - 1) >= 0)
					{
						this->set_current_selection(current_selection - 1);
					}
					else
					{
						this->set_current_selection(this->current_index - 1);
					}
				}
			}

			void next()
			{
				int current_selection = this->get_current_selection();
				
				if (current_selection != -1)
				{
					if ((current_selection + 1) < this->current_index)
					{
						this->set_current_selection(current_selection + 1);
					}
					else
					{
						this->set_current_selection(0);
					}
				}
			}

			bool set_data(int index, T data)
			{
				tab_item tab_control_item;
				tab_control_item.item_header.mask = TCIF_PARAM; 
				tab_control_item.custom_data = data;
				
				return (TabCtrl_SetItem(this->get_handle(), index, &tab_control_item) != FALSE);
			}

			T get_data(int index)
			{
				tab_item tab_control_item;
				tab_control_item.item_header.mask = TCIF_PARAM; 
				
				if (!TabCtrl_GetItem(this->get_handle(), index, &tab_control_item))
				{
					return static_cast<T>(0);
				}

				return tab_control_item.custom_data;
			}

			void relate(int index, widget& widget)
			{
				if (index < this->current_index)
				{
					try
					{
						this->relatives.at(index) = &widget;
					}
					catch (std::exception& exception)
					{
						OutputDebugString(exception.what());
					}

					widget.show(index == get_current_selection());
				}
			}

			void set_on_removed_event(on_removed_tab_event_t on_removed_tab_event)
			{
				this->on_removed_tab_event = on_removed_tab_event;
			}

		protected:
			void activate_relative(int index, bool show)
			{
				if (index != -1)
				{
					try
					{
						if (this->relatives.at(index))
						{
							this->relatives.at(index)->show(show);
						}
					}
					catch (std::exception& exception)
					{
						OutputDebugString(exception.what());
					}
				}
			}

			void set_message_handlers()
			{
				this->add_message_handlers(3,
					message_pair(CUSTOM_NOTIFY, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
					{
						NMHDR* custom_notify = reinterpret_cast<NMHDR*>(lParam);
						
						if (custom_notify->code == TCN_SELCHANGING)
						{
							this->activate_relative(TabCtrl_GetCurSel(hWnd), false);
						}
						else if (custom_notify->code == TCN_SELCHANGE)
						{
							this->activate_relative(TabCtrl_GetCurSel(hWnd), true);
						}
						else if (custom_notify->code == NM_RCLICK)
						{
							if (this->has_remove_event)
							{
								TCHITTESTINFO tabcontrol_hittest;

								GetCursorPos(&tabcontrol_hittest.pt);
								ScreenToClient(hWnd, &tabcontrol_hittest.pt);
			
								int tab_focus = TabCtrl_HitTest(hWnd, &tabcontrol_hittest);

								if (tab_focus != -1)
								{
									this->remove_tab(tab_focus);
								}
							}
						}

						return 0;
					}),
					message_pair(WM_ERASEBKGND, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
					{
						SetBkColor(reinterpret_cast<HDC>(wParam), this->get_background_color());

						/* calculate rects */
						RECT rc_main_body;
						GetClientRect(hWnd, &rc_main_body);

						RECT rc_total_tab;
						SetRectEmpty(&rc_total_tab);
						
						for (int i = 0, tab_count = TabCtrl_GetItemCount(hWnd); i < tab_count; i++)
						{
							RECT rc_tab;
							TabCtrl_GetItemRect(hWnd, i, &rc_tab);

							UnionRect(&rc_total_tab, &rc_tab, &rc_total_tab);
						}

						int tab_height = (rc_total_tab.bottom - rc_total_tab.top);
						InflateRect(&rc_total_tab, 2, 3);
						
						/* full width of the tab contol above top of the tabs */
						RECT rc_background(rc_main_body);
						rc_background.bottom = rc_total_tab.top + 3;

						ExtTextOut(reinterpret_cast<HDC>(wParam), rc_background.left, rc_background.top, 
							ETO_CLIPPED | ETO_OPAQUE, &rc_background, "", 0, NULL);
	
						/* width of the tab control's visible background (including bottom pixel) to the left of the tabs */
						rc_background = rc_main_body;
						rc_background.right = 2;
						rc_background.bottom = rc_background.top + (tab_height + 2);

						ExtTextOut(reinterpret_cast<HDC>(wParam), rc_background.left, rc_background.top, 
							ETO_CLIPPED | ETO_OPAQUE, &rc_background, "", 0, NULL);
	
						/* width of the tab control's visible background (including bottom pixel) to the right of the tabs */
						rc_background = rc_main_body;
						rc_background.left += (rc_total_tab.right - rc_total_tab.left) - 2;
						rc_background.bottom = rc_background.top + (tab_height + 2);

						ExtTextOut(reinterpret_cast<HDC>(wParam), rc_background.left, rc_background.top, 
							ETO_CLIPPED | ETO_OPAQUE, &rc_background, "", 0, NULL);

						return 1;
					}),
					message_pair(WM_PAINT, [this](HWND hWnd, WPARAM wParam, LPARAM lParam) -> LRESULT
					{
						PAINTSTRUCT paint_struct;
						HDC hdc_paint = BeginPaint(hWnd, &paint_struct);

						SelectObject(hdc_paint, this->get_font());
						
						/* draw main body */
						RECT rc_main_body;
						GetClientRect(hWnd, &rc_main_body);

						RECT rc_page(rc_main_body);
						TabCtrl_AdjustRect(hWnd, FALSE, &rc_page);

						rc_main_body.top = rc_page.top - 2;
						FillRect(hdc_paint, &rc_main_body, CreateSolidBrush(GetSysColor(COLOR_3DFACE)));

						/* draw tabs */
						TCHITTESTINFO tabcontrol_hittest;

						GetCursorPos(&tabcontrol_hittest.pt);
						ScreenToClient(hWnd, &tabcontrol_hittest.pt);
			
						int tab_focus = TabCtrl_HitTest(hWnd, &tabcontrol_hittest);
						int tab_selection = TabCtrl_GetCurSel(this->get_handle());
						
						int tab_count = TabCtrl_GetItemCount(this->get_handle());

						if (!tab_count)
						{
							return 0;
						}
						
						for (int i = 0; i < tab_count; i++)
						{
							if (i == tab_selection)
							{
								continue;
							}

							RECT rc_item;
							TabCtrl_GetItemRect(this->get_handle(), i, &rc_item);

							this->draw_item(hWnd, hdc_paint, rc_item, i, false, tab_focus == i);
						}
						
						RECT rc_item;
						TabCtrl_GetItemRect(this->get_handle(), tab_selection, &rc_item);

						this->draw_item(hWnd, hdc_paint, rc_item, tab_selection, true, tab_focus == tab_selection);

						EndPaint(hWnd, &paint_struct);
						return 0;
					})
				);
			}

			void draw_item(HWND hwnd, HDC hdc, RECT rc_item, int index, bool selected = false, bool focused = false)
			{
				const int padding = 2;

				/* background */
				RECT rc_background(rc_item);

				if (index == 0)
				{
					rc_background.left -= 2;
				}

				if (focused && !selected)
				{
					FillRect(hdc, &rc_background, CreateSolidBrush(selected ? RGB(200, 200, 200) : RGB(186, 211, 253)));
				}
				else
				{
					FillRect(hdc, &rc_background, CreateSolidBrush(selected ? RGB(246, 246, 246) : RGB(209, 255, 209)));
				}
				
				/* get item information */
				RECT rc_temp_item(rc_item);
				rc_temp_item.left += padding;
				rc_temp_item.top += padding;

				SetBkMode(hdc, TRANSPARENT);

				char tab_control_item_buffer[256];
				memset(tab_control_item_buffer, 0, sizeof(tab_control_item_buffer));

				TC_ITEM tab_control_item;
				tab_control_item.mask = TCIF_TEXT | TCIF_IMAGE;
				tab_control_item.pszText = tab_control_item_buffer;
				tab_control_item.cchTextMax = sizeof(tab_control_item_buffer);

				TabCtrl_GetItem(hwnd, index, &tab_control_item);

				/* icon */
				HIMAGELIST image_list = TabCtrl_GetImageList(hwnd);

				if (image_list)
				{
					RECT rc_icon(rc_item);

					if (index != 0)
					{
						rc_icon.left += 1;
					}

					rc_icon.top += 2;

					ImageList_Draw(image_list, index, hdc, rc_icon.left, rc_icon.top, ILD_TRANSPARENT);
					rc_temp_item.left += 16 + padding;
				}

				/* text */
				rc_temp_item.right -= padding;

				SetTextColor(hdc, (selected ? GetSysColor(COLOR_WINDOWTEXT) : RGB(94, 137, 92)));
				DrawText(hdc, tab_control_item_buffer, strlen(tab_control_item_buffer), &rc_temp_item, DT_NOPREFIX | DT_CENTER);

				/* border */
				RECT rc_border(rc_item);

				RECT rc_left_border(rc_border);
				rc_left_border.left -= (index == 0 ? 2 : 1);
				rc_left_border.right = rc_left_border.left + 1;
				FillRect(hdc, &rc_left_border, CreateSolidBrush(RGB(0,0,0)));
				
				RECT rc_top_border(rc_border);
				rc_top_border.bottom = rc_top_border.top + 1;
				rc_top_border.left -= (index == 0);
				//FillRect(hdc, &rc_top_border, CreateSolidBrush(RGB(0,0,0)));

				RECT rc_right_border(rc_border);
				rc_right_border.left = rc_right_border.right - 1;
				FillRect(hdc, &rc_right_border, CreateSolidBrush(RGB(0,0,0)));
			}

		private:
			int current_index;
			std::vector<widget*> relatives;

			bool has_remove_event;
			on_removed_tab_event_t on_removed_tab_event;
		};
	}
}