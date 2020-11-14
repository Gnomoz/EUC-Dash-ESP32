/****************************************************************************
 *   Copyright  2020  Jesper Ortlund
 ****************************************************************************/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
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
#include <Arduino.h>
#include "tripinfo_tile.h"
#include "gui/mainbar/mainbar.h"
#include "hardware/pmu.h"
#include "hardware/wheelctl.h"
#include "hardware/dashboard.h"

uint32_t tripinfo_tile_num;
lv_task_t *tripinfo_task = nullptr;

void lv_tripinfo_task(lv_task_t *tripinfo_task);
void tripinfo_setup_styles( void );
void tripinfo_setup_obj( void );
void tripinfo_update( void );
void tripinfo_activate_cb(void);
void tripinfo_hibernate_cb(void);

static lv_obj_t *tripinfo_cont = NULL;
static lv_style_t *style;
lv_style_t tripinfo_style;
lv_style_t tripinfo_heading_style;
lv_style_t tripinfo_data_style;

lv_obj_t *odometer_data;
lv_obj_t *trip_data;
lv_obj_t *max_speed_data;
lv_obj_t *max_current_data;
lv_obj_t *max_power_data;
lv_obj_t *ride_time_data;

void tripinfo_tile_setup(void)
{
    tripinfo_tile_num = mainbar_add_tile(1, 1, "trip tile");
    tripinfo_cont = mainbar_get_tile_obj(tripinfo_tile_num);
    //fulldash_cont = mainbar_get_tile_obj( mainbar_add_tile( 1, 0, "fulldash tile" ) );
    style = mainbar_get_style();
    Serial.println("setting up tripinfo_tile");
    tripinfo_setup_styles();
    tripinfo_setup_obj();

    mainbar_add_tile_activate_cb(tripinfo_tile_num, tripinfo_activate_cb);
    mainbar_add_tile_hibernate_cb(tripinfo_tile_num, tripinfo_hibernate_cb);
}

void tripinfo_setup_styles( void ) {
    lv_style_copy( &tripinfo_style, mainbar_get_style() );
    lv_style_set_bg_color( &tripinfo_style, LV_OBJ_PART_MAIN, LV_COLOR_BLACK);
    lv_style_set_bg_opa( &tripinfo_style, LV_OBJ_PART_MAIN, LV_OPA_100);
    lv_style_set_border_width( &tripinfo_style, LV_OBJ_PART_MAIN, 0);
    lv_obj_add_style( tripinfo_cont, LV_OBJ_PART_MAIN, &tripinfo_style );

    lv_style_copy( &tripinfo_heading_style, &tripinfo_style );
    lv_style_set_text_color( &tripinfo_heading_style, LV_OBJ_PART_MAIN, LV_COLOR_WHITE );

    lv_style_copy( &tripinfo_data_style, &tripinfo_style );
    lv_style_set_text_color( &tripinfo_data_style, LV_OBJ_PART_MAIN, LV_COLOR_LIME );
}

void tripinfo_setup_obj( void ) {
    lv_obj_t *heading_label = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( heading_label, LV_OBJ_PART_MAIN, &tripinfo_heading_style );
    lv_label_set_text( heading_label, "trip information");
    lv_obj_align( heading_label, tripinfo_cont, LV_ALIGN_IN_TOP_MID, 0, 5 );

    lv_obj_t *odometer_label = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( odometer_label, LV_OBJ_PART_MAIN, &tripinfo_style );
    lv_label_set_text( odometer_label, "odometer");
    lv_obj_align( odometer_label, tripinfo_cont, LV_ALIGN_IN_TOP_LEFT, 5, 30 );
    odometer_data = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( odometer_data, LV_OBJ_PART_MAIN, &tripinfo_data_style  );
    lv_label_set_text( odometer_data, "300 km");
    lv_obj_align( odometer_data, tripinfo_cont, LV_ALIGN_IN_TOP_RIGHT, -5, 30 );

    lv_obj_t *trip_label = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( trip_label, LV_OBJ_PART_MAIN, &tripinfo_style );
    lv_label_set_text( trip_label, "trip meter");
    lv_obj_align( trip_label, odometer_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0 );
    trip_data = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( trip_data, LV_OBJ_PART_MAIN, &tripinfo_data_style  );
    lv_label_set_text( trip_data, "300 km");
    lv_obj_align( trip_data, odometer_data, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0 );

    lv_obj_t *max_speed_label = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( max_speed_label, LV_OBJ_PART_MAIN, &tripinfo_style );
    lv_label_set_text( max_speed_label, "max speed");
    lv_obj_align( max_speed_label, trip_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0 );
    max_speed_data = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( max_speed_data, LV_OBJ_PART_MAIN, &tripinfo_data_style  );
    lv_label_set_text( max_speed_data, "30 kmh");
    lv_obj_align( max_speed_data, trip_data, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0 );

    lv_obj_t *max_current_label = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( max_current_label, LV_OBJ_PART_MAIN, &tripinfo_style );
    lv_label_set_text( max_current_label, "max current");
    lv_obj_align( max_current_label, max_speed_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0 );
    max_current_data = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( max_current_data, LV_OBJ_PART_MAIN, &tripinfo_data_style  );
    lv_label_set_text( max_current_data, "24 A");
    lv_obj_align( max_current_data, max_speed_data, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0 );

    lv_obj_t *max_power_label = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( max_power_label, LV_OBJ_PART_MAIN, &tripinfo_style );
    lv_label_set_text( max_power_label, "max power");
    lv_obj_align( max_power_label, max_current_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0 );
    max_power_data = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( max_power_data, LV_OBJ_PART_MAIN, &tripinfo_data_style  );
    lv_label_set_text( max_power_data, "1300 W");
    lv_obj_align( max_power_data, max_current_data, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0 );

    lv_obj_t *ride_time_label = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( ride_time_label, LV_OBJ_PART_MAIN, &tripinfo_style );
    lv_label_set_text( ride_time_label, "riding time");
    lv_obj_align( ride_time_label, max_power_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0 );
    ride_time_data = lv_label_create( tripinfo_cont, NULL);
    lv_obj_add_style( ride_time_data, LV_OBJ_PART_MAIN, &tripinfo_data_style  );
    lv_label_set_text( ride_time_data, "35 min");
    lv_obj_align( ride_time_data, max_power_data, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0 );
}

void tripinfo_activate_cb(void)
{
    tripinfo_task = lv_task_create(lv_tripinfo_task, 1000, LV_TASK_PRIO_LOWEST, NULL);
    lv_task_ready(tripinfo_task);
}

void tripinfo_hibernate_cb(void)
{
    lv_task_del(tripinfo_task);
}

void tripinfo_tile_reload(void)
{
    lv_obj_del(tripinfo_cont);
    tripinfo_tile_setup();
}

uint32_t tripinfo_get_tile(void)
{
    return tripinfo_tile_num;
}

void lv_tripinfo_task ( lv_task_t *tripinfo_task )
{
    tripinfo_update();
}

void tripinfo_update( void ) {
    char temp[16]="";
    
    if (dashboard_get_config(DASHBOARD_IMPDIST)) {
        float impodo = wheelctl_get_data(WHEELCTL_ODO) / 1.6;
        snprintf( temp, sizeof( temp ), "%0.1f mi", impodo );
    } else {
        snprintf( temp, sizeof( temp ), "%0.1f km", wheelctl_get_data(WHEELCTL_ODO) );
    }
    lv_label_set_text( odometer_data, temp);
    lv_obj_align( odometer_data, tripinfo_cont, LV_ALIGN_IN_TOP_RIGHT, -5, 30 );

    if (dashboard_get_config(DASHBOARD_IMPDIST)) {
        float imptrip = wheelctl_get_data(WHEELCTL_TRIP) / 1.6;
        snprintf( temp, sizeof( temp ), "%0.2f mi", imptrip );
    } else {
        snprintf( temp, sizeof( temp ), "%0.2f km", wheelctl_get_data(WHEELCTL_TRIP) );
    }
    lv_label_set_text( trip_data, temp);
    lv_obj_align( trip_data, odometer_data, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0 );

    if (dashboard_get_config(DASHBOARD_IMPDIST)) {
        float imptrip = wheelctl_get_max_data(WHEELCTL_SPEED) / 1.6;
        snprintf( temp, sizeof( temp ), "%0.2f mph", imptrip );
    } else {
        snprintf( temp, sizeof( temp ), "%0.2f kmh", wheelctl_get_max_data(WHEELCTL_SPEED) );
    }
    lv_label_set_text( max_speed_data, temp);
    lv_obj_align( max_speed_data, trip_data, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0 );

    snprintf( temp, sizeof( temp ), "%0.2f A", wheelctl_get_max_data(WHEELCTL_CURRENT) );
    lv_label_set_text( max_current_data, temp);
    lv_obj_align( max_current_data, max_speed_data, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0 );

    snprintf( temp, sizeof( temp ), "%0.1f W", wheelctl_get_max_data(WHEELCTL_POWER) );
    lv_label_set_text( max_power_data, temp);
    lv_obj_align( max_power_data, max_current_data, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0 );

    snprintf( temp, sizeof( temp ), "%0.0f s", wheelctl_get_data(WHEELCTL_RIDETIME) );
    lv_label_set_text( ride_time_data, temp);
    lv_obj_align( ride_time_data, max_power_data, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0 );
}
