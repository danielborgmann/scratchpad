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
 
#ifndef _GUNIQUEAPP_BASE_H
#define _GUNIQUEAPP_BASE_H

#include <glib-object.h>

#define G_TYPE_UNIQUE_APP		(g_unique_app_get_type ())
#define G_UNIQUE_APP(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_UNIQUE_APP, GUniqueApp))
#define G_UNIQUE_APP_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_UNIQUE_APP, GUniqueAppClass))
#define G_IS_UNIQUE_APP(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_UNIQUE_APP))
#define G_IS_UNIQUE_APP_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_UNIQUE_APP))
#define G_UNIQUE_APP_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_UNIQUE_APP, GUniqueAppClass))

typedef enum {
      G_UNIQUE_APP_ACTIVATE, /* Just switch to the already running instance */
      G_UNIQUE_APP_NEW, /* Create a new document */
      G_UNIQUE_APP_OPEN, /* Open the given URI */
      G_UNIQUE_APP_CUSTOM /* Send a custom message */ 
} GUniqueAppCommand;

GType g_unique_app_command_get_type (void);
#define G_TYPE_UNIQUE_APP_COMMAND (g_unique_app_command_get_type ())

typedef struct _GUniqueApp GUniqueApp;
typedef struct _GUniqueAppClass GUniqueAppClass;
typedef struct _GUniqueAppPrivate GUniqueAppPrivate;

struct _GUniqueApp {
	GObject parent;
	
	/* private */
	GUniqueAppPrivate *priv;
};

struct _GUniqueAppClass {
	GObjectClass parent;
	
	guint message_signal_id;
	gboolean (*is_running) (GUniqueApp *self);
	void (*send_message) (GUniqueApp *self, GUniqueAppCommand command, const gchar *data, const gchar *startup_id, guint workspace);
	void (*message) (GUniqueApp *self, GUniqueAppCommand command, gchar *data, gchar *startup_id, guint workspace);
};

GType g_unique_app_get_type (void);

/* Method definitions */

gboolean g_unique_app_is_running (GUniqueApp *app);
void g_unique_app_send_message (GUniqueApp *app, GUniqueAppCommand command, const gchar* data);
void g_unique_app_activate (GUniqueApp *app);
void g_unique_app_new_document (GUniqueApp *app);
void g_unique_app_open_uri (GUniqueApp *app, const gchar *uri);
void g_unique_app_custom_message (GUniqueApp *app, const gchar *data);

#endif /* _GUNIQUEAPP_BASE_H */
