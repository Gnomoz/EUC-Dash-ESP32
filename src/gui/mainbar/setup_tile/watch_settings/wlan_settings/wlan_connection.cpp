/****************************************************************************
 *   Modified 2020 Jesper Ortlund  
 *   Tu May 22 21:23:51 2020
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
#include "config.h"
#include "wlan_settings.h"

#include "gui/mainbar/mainbar.h"
#include "gui/mainbar/setup_tile/setup_tile.h"
#include "gui/keyboard.h"
#include "gui/setup.h"

#include "hardware/wifictl.h"
#include "hardware/motor.h"
#include "hardware/blectl.h"
#include "hardware/json_psram_allocator.h"
#include "gui/mainbar/setup_tile/watch_settings/watch_settings.h"

#include <WiFi.h>

LV_IMG_DECLARE(exit_32px);

lv_obj_t *wifi_connection_tile=NULL;
lv_style_t wifi_connection_style;
lv_style_t wifi_connection_heading_style;
lv_style_t wifi_connection_data_style;
lv_style_t wifi_list_style;
uint32_t wifi_connection_tile_num;

lv_obj_t *wifi_password_tile=NULL;
lv_style_t wifi_password_style;
lv_obj_t *wifi_password_name_label=NULL;
lv_obj_t *wifi_password_pass_textfield=NULL;
uint32_t wifi_password_tile_num;

lv_obj_t *wifi_onoff=NULL;
lv_obj_t *wifiname_list=NULL;

lv_obj_t *wifi_connection_label=NULL;
lv_obj_t *wifi_connection_ssid=NULL;

static void enter_wifi_connection_event_cb( lv_obj_t * obj, lv_event_t event );
static void exit_wifi_connection_event_cb( lv_obj_t * obj, lv_event_t event );
static void wifi_onoff_event_handler(lv_obj_t * obj, lv_event_t event);
void wifi_connection_enter_pass_event_cb( lv_obj_t * obj, lv_event_t event );
bool wifi_setup_wifictl_event_cb( EventBits_t event, void *arg );

LV_IMG_DECLARE(lock_16px);
LV_IMG_DECLARE(unlock_16px);
LV_IMG_DECLARE(check_32px);
LV_IMG_DECLARE(exit_32px);
LV_IMG_DECLARE(trash_32px);
LV_IMG_DECLARE(wifi_32px);

void wlan_connection_tile_pre_setup( void ){
    watch_settings_register_menu_item(&wifi_32px, enter_wifi_connection_event_cb, "connect wifi");
}

void wlan_connection_tile_setup( void ) {
    // get an app tile and copy mainstyle
    wifi_connection_tile_num = setup_get_submenu_tile_num();
    wifi_password_tile_num = setup_get_submenu2_tile_num();
    wifi_connection_tile = mainbar_get_tile_obj( wifi_connection_tile_num );
    lv_obj_clean(wifi_connection_tile);
    lv_style_copy( &wifi_connection_style, mainbar_get_style() );
    lv_style_set_bg_color( &wifi_connection_style, LV_OBJ_PART_MAIN, LV_COLOR_BLACK );
    lv_style_set_bg_opa( &wifi_connection_style, LV_OBJ_PART_MAIN, LV_OPA_100);
    lv_style_set_border_width( &wifi_connection_style, LV_OBJ_PART_MAIN, 0);

    lv_style_copy( &wifi_connection_heading_style, &wifi_connection_style );
    lv_style_set_text_color( &wifi_connection_heading_style, LV_OBJ_PART_MAIN, LV_COLOR_WHITE );
    lv_style_copy( &wifi_connection_data_style, &wifi_connection_style );
    lv_style_set_text_color( &wifi_connection_data_style, LV_OBJ_PART_MAIN, LV_COLOR_LIME );

    lv_obj_add_style( wifi_connection_tile, LV_OBJ_PART_MAIN, &wifi_connection_style );

    lv_obj_t *exit_btn = lv_imgbtn_create( wifi_connection_tile, NULL);
    lv_imgbtn_set_src( exit_btn, LV_BTN_STATE_RELEASED, &exit_32px);
    lv_imgbtn_set_src( exit_btn, LV_BTN_STATE_PRESSED, &exit_32px);
    lv_imgbtn_set_src( exit_btn, LV_BTN_STATE_CHECKED_RELEASED, &exit_32px);
    lv_imgbtn_set_src( exit_btn, LV_BTN_STATE_CHECKED_PRESSED, &exit_32px);
    lv_obj_add_style( exit_btn, LV_IMGBTN_PART_MAIN, &wifi_connection_style );
    lv_obj_align( exit_btn, wifi_connection_tile, LV_ALIGN_IN_TOP_LEFT, 10, 10 );
    lv_obj_set_event_cb( exit_btn, exit_wifi_connection_event_cb );
    
    lv_obj_t *exit_label = lv_label_create( wifi_connection_tile, NULL);
    lv_obj_add_style( exit_label, LV_OBJ_PART_MAIN, &wifi_connection_heading_style );
    lv_label_set_text( exit_label, "connect wlan");
    lv_obj_align( exit_label, exit_btn, LV_ALIGN_OUT_RIGHT_MID, 15, 0 );

    /*Copy the first switch and turn it ON*/    
    wifi_onoff = lv_switch_create( wifi_connection_tile, NULL );
    lv_obj_add_protect( wifi_onoff, LV_PROTECT_CLICK_FOCUS);
    lv_obj_add_style( wifi_onoff, LV_SWITCH_PART_INDIC, mainbar_get_switch_style()  );
    lv_obj_add_style( wifi_onoff, LV_SLIDER_PART_KNOB, mainbar_get_knob_style() );
    lv_switch_off( wifi_onoff, LV_ANIM_ON );
    lv_obj_align( wifi_onoff, exit_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0 );
    lv_obj_set_event_cb( wifi_onoff, wifi_onoff_event_handler);

    wifi_connection_label = lv_label_create( wifi_connection_tile, NULL );
    lv_label_set_text( wifi_connection_label, "connected:");
    lv_obj_align( wifi_connection_label, wifi_connection_tile, LV_ALIGN_IN_TOP_LEFT, 10, 45 );
    
    wifi_connection_ssid = lv_label_create( wifi_connection_tile, NULL );
    lv_obj_add_style( wifi_connection_ssid, LV_OBJ_PART_MAIN, &wifi_connection_data_style );
    lv_label_set_text( wifi_connection_ssid, "NULL");
    lv_obj_align( wifi_connection_ssid, wifi_connection_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0 );

    wifiname_list = lv_list_create( wifi_connection_tile, NULL);
    lv_obj_set_size( wifiname_list, lv_disp_get_hor_res( NULL ), 160);
    lv_style_init( &wifi_list_style  );
    lv_style_set_border_width( &wifi_list_style , LV_OBJ_PART_MAIN, 0);
    lv_style_set_radius( &wifi_list_style , LV_OBJ_PART_MAIN, 0);
    lv_obj_add_style( wifiname_list, LV_OBJ_PART_MAIN, &wifi_list_style  );
    lv_obj_align( wifiname_list, wifi_connection_tile, LV_ALIGN_IN_TOP_MID, 0, 80);

    wlan_password_tile_setup( wifi_password_tile_num );

    wifictl_register_cb( WIFICTL_ON | WIFICTL_OFF | WIFICTL_SCAN | WIFICTL_CONNECT, wifi_setup_wifictl_event_cb, "wifi network scan" );
}

void wlan_setup_display_ssid(String ssid) {
    if (wifi_connection_ssid != NULL) {
        lv_label_set_text( wifi_connection_ssid, ssid.c_str());
        lv_obj_align( wifi_connection_ssid, wifi_connection_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0 );
    }
}

bool wifi_setup_wifictl_event_cb( EventBits_t event, void *arg ) {
    String getssid;
    switch( event ) {
        case    WIFICTL_ON:
            lv_switch_on( wifi_onoff, LV_ANIM_OFF );
            getssid = WiFi.SSID();
            wlan_setup_display_ssid(getssid);
            break;
        case    WIFICTL_OFF:
            getssid = WiFi.SSID();
            wlan_setup_display_ssid(getssid);
            lv_switch_off( wifi_onoff, LV_ANIM_OFF );
            while ( lv_list_remove( wifiname_list, 0 ) );
            break;
        case    WIFICTL_CONNECT:
            getssid = WiFi.SSID();
            wlan_setup_display_ssid(getssid);
            break;
        case    WIFICTL_SCAN:
            getssid = WiFi.SSID();
            wlan_setup_display_ssid(getssid);
            while ( lv_list_remove( wifiname_list, 0 ) );

            int len = WiFi.scanComplete();
            Serial.printf("%d ssids detected\n", len);
            for( int i = 0 ; i < len ; i++ ) {
                if ( wifictl_is_known( WiFi.SSID(i).c_str() ) ) {
                    Serial.printf( "%d found known ssid: %s\n", i, WiFi.SSID(i).c_str() );
                    lv_obj_t * wifiname_list_btn = lv_list_add_btn( wifiname_list, &unlock_16px, WiFi.SSID(i).c_str() );
                    lv_obj_set_event_cb( wifiname_list_btn, wifi_connection_enter_pass_event_cb);
                }
                else {
                    Serial.printf( "%d found unknown ssid: %s\n", i, WiFi.SSID(i).c_str() );
                    lv_obj_t * wifiname_list_btn = lv_list_add_btn( wifiname_list, &lock_16px, WiFi.SSID(i).c_str() );
                    lv_obj_set_event_cb( wifiname_list_btn, wifi_connection_enter_pass_event_cb);
                }
            }            
            break;
    }
    return( true );
}

static void enter_wifi_connection_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       wlan_connection_tile_setup();
                                        mainbar_jump_to_tilenumber( wifi_connection_tile_num, LV_ANIM_OFF );
                                        break;
    }
}

static void exit_wifi_connection_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mainbar_jump_to_tilenumber( watch_get_tile_num(), LV_ANIM_OFF );
                                        break;
    }
}

static void wifi_onoff_event_handler(lv_obj_t * obj, lv_event_t event) {
    switch( event ) {
        case( LV_EVENT_VALUE_CHANGED ): if( lv_switch_get_state( obj ) ) {
                                            wifictl_on();
                                        }
                                        else {
                                            wifictl_off();
                                        }
    }
}

void wifi_connection_enter_pass_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):   lv_label_set_text( wifi_password_name_label, lv_list_get_btn_text(obj) );
                                    lv_textarea_set_text( wifi_password_pass_textfield, "");
                                    mainbar_jump_to_tilenumber( wifi_password_tile_num, LV_ANIM_ON );
    }
}

static void exit_wifi_password_event_cb( lv_obj_t * obj, lv_event_t event );
static void wlan_password_event_cb(lv_obj_t * obj, lv_event_t event);
static void apply_wifi_password_event_cb(  lv_obj_t * obj, lv_event_t event );
static void delete_wifi_password_event_cb(  lv_obj_t * obj, lv_event_t event );

void wlan_password_tile_setup( uint32_t wifi_password_tile_num ) {
    // get an app tile and copy mainstyle
    wifi_password_tile = mainbar_get_tile_obj( wifi_password_tile_num );
    lv_style_copy( &wifi_password_style, mainbar_get_style() );
    lv_style_set_bg_color( &wifi_password_style, LV_OBJ_PART_MAIN, LV_COLOR_BLACK);
    lv_style_set_bg_opa( &wifi_password_style, LV_OBJ_PART_MAIN, LV_OPA_100);
    lv_style_set_border_width( &wifi_password_style, LV_OBJ_PART_MAIN, 0);
    lv_obj_add_style( wifi_password_tile, LV_OBJ_PART_MAIN, &wifi_password_style );

    lv_obj_t *exit_btn = lv_imgbtn_create( wifi_password_tile, NULL);
    lv_imgbtn_set_src( exit_btn, LV_BTN_STATE_RELEASED, &exit_32px);
    lv_imgbtn_set_src( exit_btn, LV_BTN_STATE_PRESSED, &exit_32px);
    lv_imgbtn_set_src( exit_btn, LV_BTN_STATE_CHECKED_RELEASED, &exit_32px);
    lv_imgbtn_set_src( exit_btn, LV_BTN_STATE_CHECKED_PRESSED, &exit_32px);
    lv_obj_add_style( exit_btn, LV_IMGBTN_PART_MAIN, &wifi_password_style );
    lv_obj_align( exit_btn, wifi_password_tile, LV_ALIGN_IN_TOP_LEFT, 10, 10 );
    lv_obj_set_event_cb( exit_btn, exit_wifi_password_event_cb );
    
    wifi_password_name_label = lv_label_create( wifi_password_tile, NULL);
    lv_obj_add_style( wifi_password_name_label, LV_OBJ_PART_MAIN, &wifi_password_style  );
    lv_label_set_text( wifi_password_name_label, "wlan setting");
    lv_obj_align( wifi_password_name_label, exit_btn, LV_ALIGN_OUT_RIGHT_MID, 5, 0 );

    wifi_password_pass_textfield = lv_textarea_create( wifi_password_tile, NULL);
    lv_textarea_set_text( wifi_password_pass_textfield, "");
    lv_textarea_set_pwd_mode( wifi_password_pass_textfield, false);
    lv_textarea_set_one_line( wifi_password_pass_textfield, true);
    lv_textarea_set_cursor_hidden( wifi_password_pass_textfield, true);
    lv_obj_set_width( wifi_password_pass_textfield, lv_disp_get_hor_res( NULL ) );
    lv_obj_align( wifi_password_pass_textfield, wifi_password_tile, LV_ALIGN_IN_TOP_LEFT, 0, 75);
    lv_obj_set_event_cb( wifi_password_pass_textfield, wlan_password_event_cb );

    lv_obj_t *mac_label = lv_label_create( wifi_password_tile, NULL);
    lv_obj_add_style( mac_label, LV_IMGBTN_PART_MAIN, &wifi_password_style );
    lv_obj_set_width( mac_label, lv_disp_get_hor_res( NULL ) );
    lv_obj_align( mac_label, wifi_password_tile, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
    lv_label_set_text_fmt( mac_label, "MAC: %s", WiFi.macAddress().c_str());

    lv_obj_t *apply_btn = lv_imgbtn_create( wifi_password_tile, NULL);
    lv_imgbtn_set_src( apply_btn, LV_BTN_STATE_RELEASED, &check_32px);
    lv_imgbtn_set_src( apply_btn, LV_BTN_STATE_PRESSED, &check_32px);
    lv_imgbtn_set_src( apply_btn, LV_BTN_STATE_CHECKED_RELEASED, &check_32px);
    lv_imgbtn_set_src( apply_btn, LV_BTN_STATE_CHECKED_PRESSED, &check_32px);
    lv_obj_add_style( apply_btn, LV_IMGBTN_PART_MAIN, &wifi_password_style );
    lv_obj_align( apply_btn, wifi_password_pass_textfield, LV_ALIGN_OUT_BOTTOM_RIGHT, -10, 10 );
    lv_obj_set_event_cb( apply_btn, apply_wifi_password_event_cb );

    lv_obj_t *delete_btn = lv_imgbtn_create( wifi_password_tile, NULL);
    lv_imgbtn_set_src( delete_btn, LV_BTN_STATE_RELEASED, &trash_32px);
    lv_imgbtn_set_src( delete_btn, LV_BTN_STATE_PRESSED, &trash_32px);
    lv_imgbtn_set_src( delete_btn, LV_BTN_STATE_CHECKED_RELEASED, &trash_32px);
    lv_imgbtn_set_src( delete_btn, LV_BTN_STATE_CHECKED_PRESSED, &trash_32px);
    lv_obj_add_style( delete_btn, LV_IMGBTN_PART_MAIN, &wifi_password_style );
    lv_obj_align( delete_btn, wifi_password_pass_textfield, LV_ALIGN_OUT_BOTTOM_LEFT, 10, 10 );
    lv_obj_set_event_cb( delete_btn, delete_wifi_password_event_cb );
}

static void apply_wifi_password_event_cb(  lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       wifictl_insert_network( lv_label_get_text( wifi_password_name_label ), lv_textarea_get_text( wifi_password_pass_textfield ) );
                                        keyboard_hide();
                                        mainbar_jump_to_tilenumber( wifi_connection_tile_num, LV_ANIM_ON );
                                        break;
    }
}

static void delete_wifi_password_event_cb(  lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       wifictl_delete_network( lv_label_get_text( wifi_password_name_label ) );
                                        keyboard_hide();
                                        mainbar_jump_to_tilenumber( wifi_connection_tile_num, LV_ANIM_ON );
                                        break;
    }
}

static void wlan_password_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       keyboard_set_textarea( obj );
                                        break;
    }
}

static void exit_wifi_password_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       keyboard_hide();
                                        mainbar_jump_to_tilenumber( wifi_connection_tile_num, LV_ANIM_ON );
                                        break;
    }
}
