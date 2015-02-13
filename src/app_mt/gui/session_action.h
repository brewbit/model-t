/*
 * session_action.h
 *
 *  Created on: Feb 12, 2015
 *      Author: besta_000
 */

#ifndef SESSION_ACTION_H_
#define SESSION_ACTION_H_

#include "widget.h"
#include "temp_control.h"

widget_t* session_action_screen_create(temp_controller_id_t controller, controller_settings_t* settings);

#endif /* SESSION_ACTION_H_ */
