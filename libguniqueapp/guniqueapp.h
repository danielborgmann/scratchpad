/* GUniqueApp - A library for GNOME single-instance applications
 * Copyright (C) 2006  Vytautas Liuolia
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
 
#ifndef _GUNIQUEAPP_H
#define _GUNIQUEAPP_H

#include <config.h>
#include <gtk/gtkwindow.h>
#include "guniqueapp-base.h"
#include "guniqueapp-bacon.h"
#include "guniqueapp-dbus.h"

GUniqueApp* g_unique_app_get (gchar *name);
GUniqueApp* g_unique_app_get_with_startup_id (gchar *name, const gchar* startup_id);

/* Additional functions for older GTK versions */
#ifdef OLDER_GTK_VERSION
void gtk_window_set_startup_id (GtkWindow *window, const gchar *startup_id);
#endif

#endif /* _GUNIQUEAPP_H */
