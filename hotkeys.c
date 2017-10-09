/*************************************************************************
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 2007      Bryan Christ <bryan.christ@hp.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------*/

#include <inttypes.h>

#include <viper.h>

#include "vwm.h"
#include "hotkeys.h"
#include "mainmenu.h"
#include "private.h"

#define     KEY_PLUS            '+'
#define     KEY_CTRL_DOWN       525

#define     KEY_MINUS           '-'
#define     KEY_CTRL_UP         566

#define     KEY_GREATER_THAN    '>'
#define     KEY_CTRL_RIGHT      560

#define     KEY_LESS_THAN       '<'
#define     KEY_CTRL_LEFT       545

static void vwm_default_WINDOW_INCREASE_HEIGHT(WINDOW *);
static void vwm_default_WINDOW_DECREASE_HEIGHT(WINDOW *);
static void vwm_default_WINDOW_INCREASE_WIDTH(WINDOW *);
static void vwm_default_WINDOW_DECREASE_WIDTH(WINDOW *);


int32_t
vwm_kmio_dispatch_hook_enter(int32_t keystroke)
{
    vwm_t       *vwm;

    vwm = vwm_get_instance();

    if(keystroke == VWM_HOTKEY_WM)
    {
        vwm->state ^= VWM_STATE_ACTIVE;

        if(vwm->state & VWM_STATE_ACTIVE)
            vwm_default_VWM_START((void*)TOPMOST_WINDOW);
        else
            vwm_default_VWM_STOP((void*)TOPMOST_WINDOW);

        return -1;
    }

    if(vwm->state & VWM_STATE_ACTIVE)
    {
        switch(keystroke)
        {
            case 17:
                vwm_default_WINDOW_CLOSE(TOPMOST_WINDOW); return -1;
            case KEY_TAB:
                vwm_default_WINDOW_CYCLE(); return -1;
            case KEY_UP:
                vwm_default_WINDOW_MOVE_UP(TOPMOST_WINDOW); return -1;
            case KEY_DOWN:
                vwm_default_WINDOW_MOVE_DOWN(TOPMOST_WINDOW); return -1;
            case KEY_LEFT:
                vwm_default_WINDOW_MOVE_LEFT(TOPMOST_WINDOW); return -1;
            case KEY_RIGHT:
                vwm_default_WINDOW_MOVE_RIGHT(TOPMOST_WINDOW); return -1;

            case KEY_PLUS:
            case KEY_CTRL_DOWN:
                vwm_default_WINDOW_INCREASE_HEIGHT(TOPMOST_WINDOW); return -1;
            case KEY_MINUS:
            case KEY_CTRL_UP:
                vwm_default_WINDOW_DECREASE_HEIGHT(TOPMOST_WINDOW); return -1;
            case KEY_GREATER_THAN:
            case KEY_CTRL_RIGHT:
                vwm_default_WINDOW_INCREASE_WIDTH(TOPMOST_WINDOW); return -1;
            case KEY_LESS_THAN:
            case KEY_CTRL_LEFT:
                vwm_default_WINDOW_DECREASE_WIDTH(TOPMOST_WINDOW); return -1;

            default:
                // endwin();
                // printf("%d\n",keystroke);
                // exit(0);
                return keystroke;
        }
    }

    if(!(vwm->state & VWM_STATE_ACTIVE))
    {
        switch(keystroke)
        {
            case VWM_HOTKEY_MENU:
                vwm_main_menu_hotkey(); return -1;
            default:
                return keystroke;
        }
    }

    return keystroke;
}

void
vwm_default_VWM_START(WINDOW *topmost_window)
{
    vwm_t       *vwm;
    WINDOW      *wallpaper_wnd;
    uintmax_t   msg_id;

    vwm = vwm_get_instance();

    wallpaper_wnd = viper_screen_get_wallpaper();
    vwm->wallpaper_agent(wallpaper_wnd, (void*)1);

    viper_event_run(topmost_window, "window-deactivate");

    msg_id = vwm_panel_message_find(VWM_MAIN_MENU_HELP);
    if(msg_id != 0) vwm_panel_message_del(msg_id);

    msg_id = vwm_panel_message_add(VWM_WM_HELP, -1);
    vwm_panel_message_promote(msg_id);

    viper_screen_redraw(REDRAW_ALL);
    flash();

    return;
}

void
vwm_default_VWM_STOP(WINDOW *topmost_window)
{
    vwm_t           *vwm;
    WINDOW          *wallpaper_wnd;
    uintmax_t       msg_id;

	vwm = vwm_get_instance();

    wallpaper_wnd = viper_screen_get_wallpaper();
    vwm->wallpaper_agent(wallpaper_wnd, (void*)0);

	viper_event_run(topmost_window, "window-activate");

    msg_id = vwm_panel_message_find(VWM_WM_HELP);
    if(msg_id != 0) vwm_panel_message_del(msg_id);

    msg_id = vwm_panel_message_add(VWM_MAIN_MENU_HELP, -1);
    vwm_panel_message_promote(msg_id);

	viper_screen_redraw(REDRAW_ALL);
	flash();

	return;
}

void
vwm_default_WINDOW_CLOSE(WINDOW *topmost_window)
{
	viper_event_run(topmost_window,"window-close");

	return;
}

void
vwm_default_WINDOW_CYCLE(void)
{
	viper_deck_cycle(VECTOR_BOTTOM_TO_TOP);

	return;
}

void
vwm_default_WINDOW_MOVE_UP(WINDOW *topmost_window)
{
	viper_mvwin_rel(topmost_window, 0, -1);

	return;
}

void
vwm_default_WINDOW_MOVE_DOWN(WINDOW *topmost_window)
{
	viper_mvwin_rel(topmost_window, 0, 1);

	return;
}

void
vwm_default_WINDOW_MOVE_LEFT(WINDOW *topmost_window)
{
	viper_mvwin_rel(topmost_window, -1, 0);

	return;
}

void
vwm_default_WINDOW_MOVE_RIGHT(WINDOW *topmost_window)
{
	viper_mvwin_rel(topmost_window, 1, 0);

	return;
}

static void
vwm_default_WINDOW_INCREASE_HEIGHT(WINDOW *topmost_window)
{
	viper_wresize_rel(topmost_window, 0, 1);

	return;
}

static void
vwm_default_WINDOW_DECREASE_HEIGHT(WINDOW *topmost_window)
{
	viper_wresize_rel(topmost_window, 0, -1);

	return;
}

static void
vwm_default_WINDOW_INCREASE_WIDTH(WINDOW *topmost_window)
{
	viper_wresize_rel(topmost_window, 1, 0);

	return;
}

static void
vwm_default_WINDOW_DECREASE_WIDTH(WINDOW *topmost_window)
{
	viper_wresize_rel(topmost_window, -1, 0);

	return;
}


