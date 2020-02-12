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

#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <gdk/gdkx.h>

#ifdef HAVE_DBUS
#include <dbus/dbus-glib.h>
#endif

#include "guniqueapp.h"

GUniqueApp* g_unique_app_get (gchar *name)
{
	return g_unique_app_get_with_startup_id (name, NULL);
}

GUniqueApp* g_unique_app_get_with_startup_id (gchar *name, const gchar* startup_id)
{
#ifdef HAVE_DBUS
	GError *error = NULL;
	DBusGConnection *connection;
	
	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (connection == NULL) {
		g_error_free (error);
		return FALSE;
	}
	
	if (connection != NULL)
		return G_UNIQUE_APP (g_unique_app_dbus_new_with_startup_id (name, startup_id));
	else 
#endif
		return G_UNIQUE_APP (g_unique_app_bacon_new_with_startup_id (name, startup_id));
}

/* Additional functions for older GTK versions */

#ifdef OLDER_GTK_VERSION

void gtk_window_set_startup_id (GtkWindow *window, const gchar *startup_id)
{
	gchar* timestr = g_strrstr (startup_id, "_TIME");
	
	if (timestr != NULL)
	{
		gulong timestamp;
		gchar *end;
		errno = 0;

		/* Skip past the "_TIME" part */
		timestr += 5;

		timestamp = strtoul (timestr, &end, 0);
		if (end != timestr && errno == 0)
		gtk_window_present_with_time (window, timestamp);
	}
}

#endif
