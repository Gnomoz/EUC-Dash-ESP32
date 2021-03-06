/****************************************************************************
 *   Modified 2021 Jesper Ortlund
 *   Copyright  2020  Dirk Brosswick
 *   Email: dirk.brosswick@googlemail.com
 ****************************************************************************/
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdio.h>
#include "config.h"

#include "mainbar.h"
#include "main_tile/main_tile.h"
#include "setup_tile/setup_tile.h"
#include "dashboard_tile/dashboard_tile.h"
#include "gui/keyboard.h"
#include "hardware/blectl.h"
#include "hardware/dashboard.h"

LV_FONT_DECLARE(DIN1451_m_cond_24);
LV_FONT_DECLARE(DIN1451_m_cond_28);

static lv_style_t mainbar_style;
static lv_style_t mainbar_switch_style;
static lv_style_t mainbar_button_style;
static lv_style_t mainbar_slider_style;
static lv_style_t mainbar_knob_style;

static lv_obj_t *mainbar = NULL;

static lv_tile_t *tile = NULL;
static lv_point_t *tile_pos_table = NULL;
//static lv_point_t *tile_pos_table;
static uint32_t current_tile = 0;
static uint32_t tile_entrys = 0;
static uint32_t app_tile_pos = MAINBAR_APP_TILE_X_START;
uint32_t main_tile_nr = 0;
bool dashboard_default = true;

void mainbar_setup( void ) {
    log_i("mainbar setup style");
    lv_style_init( &mainbar_style );
    lv_style_set_radius( &mainbar_style, LV_OBJ_PART_MAIN, 0 );
    lv_style_set_bg_color( &mainbar_style, LV_OBJ_PART_MAIN, LV_COLOR_BLACK );
    lv_style_set_bg_opa( &mainbar_style, LV_OBJ_PART_MAIN, LV_OPA_0 );
    lv_style_set_border_width( &mainbar_style, LV_OBJ_PART_MAIN, 0 );
    lv_style_set_text_font(&mainbar_style, LV_STATE_DEFAULT, &DIN1451_m_cond_24);
    lv_style_set_text_color( &mainbar_style, LV_OBJ_PART_MAIN, mainbar_text_colour );
    lv_style_set_image_recolor( &mainbar_style, LV_OBJ_PART_MAIN, LV_COLOR_WHITE );

    log_i("mainbar setup switch style");
    lv_style_init( &mainbar_switch_style );
    lv_style_set_bg_color( &mainbar_switch_style, LV_STATE_CHECKED, mainbar_switch_colour );

    log_i("mainbar setup slider style");
    lv_style_init( &mainbar_slider_style );
    lv_style_set_bg_color( &mainbar_slider_style, LV_STATE_DEFAULT, mainbar_switch_colour );

    log_i("mainbar setup knob style");
    lv_style_init( &mainbar_knob_style );
    lv_style_set_bg_color( &mainbar_knob_style, LV_STATE_DEFAULT, mainbar_text_colour );
    
    log_i("mainbar setup button style");
    lv_style_init( &mainbar_button_style );
    lv_style_set_radius( &mainbar_button_style, LV_STATE_DEFAULT, 25 );
    lv_style_set_border_color( &mainbar_button_style, LV_STATE_DEFAULT, LV_COLOR_GRAY );
    lv_style_set_border_color( &mainbar_button_style, LV_STATE_CHECKED, mainbar_text_colour );
    lv_style_set_border_color( &mainbar_button_style, LV_STATE_PRESSED, mainbar_text_colour );
    lv_style_set_bg_color( &mainbar_button_style, LV_STATE_DEFAULT, LV_COLOR_GRAY );
    lv_style_set_bg_color( &mainbar_button_style, LV_STATE_CHECKED, mainbar_text_colour );
    lv_style_set_bg_color( &mainbar_button_style, LV_STATE_PRESSED, mainbar_text_colour );
    lv_style_set_text_color( &mainbar_button_style, LV_STATE_CHECKED, LV_COLOR_BLACK );
    lv_style_set_text_color( &mainbar_button_style, LV_STATE_PRESSED, LV_COLOR_BLACK );
    lv_style_set_text_color( &mainbar_button_style, LV_OBJ_PART_MAIN, LV_COLOR_BLACK );
    lv_style_set_border_opa( &mainbar_button_style, LV_STATE_DEFAULT, LV_OPA_70 );
    lv_style_set_border_width( &mainbar_button_style, LV_STATE_DEFAULT, 0 );

    log_i("mainbar setup tileview object");
    mainbar = lv_tileview_create( lv_scr_act(), NULL);
    log_i("mainbar tileview created");
    //lv_tileview_set_valid_positions( mainbar, tile_pos_table, tile_entrys );
    log_i("mainbar tileview positions set");
    lv_tileview_set_edge_flash( mainbar, false);
    lv_obj_add_style( mainbar, LV_OBJ_PART_MAIN, &mainbar_style );
    lv_page_set_scrlbar_mode( mainbar, LV_SCRLBAR_MODE_OFF);

    log_i("mainbar setup event");
    lv_obj_set_event_cb( mainbar, mainbar_event_cb );
    log_i("mainbar setup done");

}

uint32_t mainbar_add_tile( uint16_t x, uint16_t y, const char *id ) {
    
    tile_entrys++;
    log_i("mainbar setup tile %d", tile_entrys);

    if ( tile_pos_table == NULL ) {
        tile_pos_table = ( lv_point_t * )ps_malloc( sizeof( lv_point_t ) * tile_entrys );
        if ( tile_pos_table == NULL ) {
            log_e("tile_pos_table malloc faild");
            while(true);
        }
        tile = ( lv_tile_t * )ps_malloc( sizeof( lv_tile_t ) * tile_entrys );
        if ( tile == NULL ) {
            log_e("tile malloc faild");
            while(true);
        }
    }
    else {
        lv_point_t *new_tile_pos_table;
        lv_tile_t *new_tile;

        new_tile_pos_table = ( lv_point_t * )ps_realloc( tile_pos_table, sizeof( lv_point_t ) * tile_entrys );
        if ( new_tile_pos_table == NULL ) {
            log_e("tile_pos_table realloc faild");
            while(true);
        }
        tile_pos_table = new_tile_pos_table;
        
        new_tile = ( lv_tile_t * )ps_realloc( tile, sizeof( lv_tile_t ) * tile_entrys );
        if ( new_tile == NULL ) {
            log_e("tile realloc faild");
            while(true);
        }
        tile = new_tile;
    }

    tile_pos_table[ tile_entrys - 1 ].x = x;
    tile_pos_table[ tile_entrys - 1 ].y = y;

    lv_obj_t *my_tile = lv_cont_create( mainbar, NULL);  
    tile[ tile_entrys - 1 ].tile = my_tile;
    tile[ tile_entrys - 1 ].activate_cb = NULL;
    tile[ tile_entrys - 1 ].hibernate_cb = NULL;
    tile[ tile_entrys - 1 ].x = x;
    tile[ tile_entrys - 1 ].y = y;
    tile[ tile_entrys - 1 ].id = id;
    lv_obj_set_size( tile[ tile_entrys - 1 ].tile, lv_disp_get_hor_res( NULL ), LV_VER_RES);
    //lv_obj_reset_style_list( tile[ tile_entrys - 1 ].tile, LV_OBJ_PART_MAIN );
    lv_obj_add_style( tile[ tile_entrys - 1 ].tile, LV_OBJ_PART_MAIN, &mainbar_style );
    lv_obj_set_pos( tile[ tile_entrys - 1 ].tile, tile_pos_table[ tile_entrys - 1 ].x * lv_disp_get_hor_res( NULL ) , tile_pos_table[ tile_entrys - 1 ].y * LV_VER_RES );
    lv_tileview_add_element( mainbar, tile[ tile_entrys - 1 ].tile );
    lv_tileview_set_valid_positions( mainbar, tile_pos_table, tile_entrys );
    log_i("add tile: x=%d, y=%d, id=%s", tile_pos_table[ tile_entrys - 1 ].x, tile_pos_table[ tile_entrys - 1 ].y, tile[ tile_entrys - 1 ].id );
    return( tile_entrys - 1 );
}

lv_style_t *mainbar_get_style( void ) {
    return( &mainbar_style );
}

lv_style_t *mainbar_get_switch_style( void ) {
    return( &mainbar_switch_style );
}

lv_style_t *mainbar_get_button_style( void ) {
    return( &mainbar_button_style );
}

lv_style_t *mainbar_get_slider_style( void ) {
    return( &mainbar_slider_style );
}

lv_style_t *mainbar_get_knob_style( void ) {
     return( &mainbar_knob_style );
}

uint32_t mainbar_get_active_tile( void ){
    uint32_t active_tile = *((uint32_t *)lv_event_get_data ());
    return active_tile;
}

void mainbar_event_cb(lv_obj_t * obj, lv_event_t event) {
    if(event == LV_EVENT_VALUE_CHANGED)
    { 
        uint32_t tile_number = *((uint32_t *)lv_event_get_data ());
        // call hibernate callback for the old tile if exist
        if ( tile[ current_tile ].hibernate_cb != NULL ) {
            log_i("call hibernate cb for tile: %d", current_tile );
            tile[ current_tile ].hibernate_cb();
        }
        // call activate callback for the new tile if exist
        if ( tile[ tile_number ].activate_cb != NULL ) { 
            log_i("call activate cb for tile: %d", tile_number );
            tile[ tile_number ].activate_cb();
        }
        current_tile = tile_number;
    }
}

bool mainbar_add_tile_hibernate_cb( uint32_t tile_number, MAINBAR_CALLBACK_FUNC hibernate_cb ) {
    if ( tile_number < tile_entrys ) {
        tile[ tile_number ].hibernate_cb = hibernate_cb;
        return( true );
    }
    else {
        log_e("tile number %d do not exist", tile_number );
        return( false );
    }
}

bool mainbar_add_tile_activate_cb( uint32_t tile_number, MAINBAR_CALLBACK_FUNC activate_cb ) {
    if ( tile_number < tile_entrys ) {
        tile[ tile_number ].activate_cb = activate_cb;
        return( true );
    }
    else {
        log_e("tile number %d do not exist", tile_number );
        return( false );
    }
}

uint32_t mainbar_add_app_tile( uint16_t x, uint16_t y, const char *id ) {
    uint32_t retval = -1;

    for ( int hor = 0 ; hor < x ; hor++ ) {
        for ( int ver = 0 ; ver < y ; ver++ ) {
            if ( retval == -1 ) {
                retval = mainbar_add_tile( hor + app_tile_pos, ver + MAINBAR_APP_TILE_Y_START, id );
            }
            else {
                mainbar_add_tile( hor + app_tile_pos, ver + MAINBAR_APP_TILE_Y_START, id );
            }
        }
    }
    app_tile_pos = app_tile_pos + x + 1;
    return( retval );
}

void mainbar_clear_tile(uint32_t tile_num) {
    lv_obj_clean( tile[ tile_num ].tile );
}

lv_obj_t *mainbar_get_tile_obj( uint32_t tile_number ) {
    if ( tile_number < tile_entrys ) {
        return( tile[ tile_number ].tile );
    }
    else {
        log_e( "tile number %d do not exist", tile_number );
    }
    return( NULL );
}

void mainbar_jump_to_maintile( lv_anim_enable_t anim ) {
    if ( tile_entrys != 0 ) {
        if (blectl_cli_getconnected()){
            //if (dashboard_get_config(DASHBOARD_SIMPLE)) {
            //    main_tile_nr = simpledash_get_tile();
            //}
            main_tile_nr = dashboard_get_tile();
        } else {
            main_tile_nr = main_tile_get_tile_num();
        }
        mainbar_jump_to_tilenumber( main_tile_nr, anim );
        keyboard_hide();
        //statusbar_hide( false );
        //statusbar_expand( false );
    }
    else {
        log_e( "main tile do not exist" );
    }
}

void mainbar_jump_to_tilenumber( uint32_t tile_number, lv_anim_enable_t anim ) {
    if ( tile_number < tile_entrys ) {
        log_i("jump to tile %d from tile %d", tile_number, current_tile );
        lv_tileview_set_tile_act( mainbar, tile_pos_table[ tile_number ].x, tile_pos_table[ tile_number ].y, anim );
        // call hibernate callback for the current tile if exist
        if ( tile[ current_tile ].hibernate_cb != NULL ) {
            log_i("call hibernate cb for tile: %d", current_tile );
            tile[ current_tile ].hibernate_cb();
        }
        // call activate callback for the new tile if exist
        if ( tile[ tile_number ].activate_cb != NULL ) { 
            log_i("call activate cb for tile: %d", tile_number );
            tile[ tile_number ].activate_cb();
        }
        current_tile = tile_number;
    }
    else {
        log_e( "tile number %d do not exist", tile_number );
    }
}

lv_obj_t * mainbar_obj_create(lv_obj_t *parent)
{
    lv_obj_t * child = lv_obj_create( parent, NULL );
    lv_tileview_add_element( mainbar, child );

    return child;
}

void mainbar_add_slide_element(lv_obj_t *element)
{
    lv_tileview_add_element( mainbar, element );
}