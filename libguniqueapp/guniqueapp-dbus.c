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
#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>  
#include "guniqueapp-dbus.h"

/* DBus stuff */ 

#define GUNIQUEAPP_FACTORY_TYPE (guniqueapp_factory_get_type ())

typedef struct _GUniqueAppFactory {
	GObject object;
	GUniqueAppDBus *parent;
} GUniqueAppFactory;

typedef struct _GUniqueAppFactoryClass {
	GObjectClass object_class;
} GUniqueAppFactoryClass;

static gboolean guniqueapp_send_message (GUniqueAppFactory *factory, int command, const char *data, const char *startup_id, guint workspace, GError **error);

#include "guniqueapp-bindings.h"
#include "guniqueapp-glue.h"
 
struct _GUniqueAppDBusPrivate {
	gboolean dispose_has_run;
	
	/* Are we in factory (i.e. "server") mode? */
	gboolean is_factory;
	
	/* DBus proxy */
	DBusGProxy *proxy;
};

/* GuniqueAppFactory */

#define GUNIQUEAPP_FACTORY_BASE_NAME "org.gnome.GUniqueApp."

static gboolean guniqueapp_send_message (GUniqueAppFactory *factory, int command, const char *data, const char *startup_id, guint workspace, GError **error)
{
	GUniqueAppDBus *app = G_UNIQUE_APP_DBUS (factory-> parent);
	g_signal_emit (app, G_UNIQUE_APP_GET_CLASS (app)-> message_signal_id,
			0 /* details */,
			(GUniqueAppCommand) command, data, startup_id, workspace); 
	return TRUE;
}

static void guniqueapp_factory_class_init (GUniqueAppFactoryClass *factory_class)
{
	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (factory_class), &dbus_glib_guniqueapp_object_info);
}

static void guniqueapp_factory_init (GUniqueAppFactory *factory)
{
}

G_DEFINE_TYPE(GUniqueAppFactory, guniqueapp_factory, G_TYPE_OBJECT);

/**********************/

static GUniqueAppClass* parent_class = NULL;

/* GUniqueApp methods */

static gboolean g_unique_app_dbus_is_running (GUniqueApp* app);
static void g_unique_app_dbus_send_message (GUniqueApp *app, GUniqueAppCommand command, const gchar* data, const gchar *startup_id, guint workspace);

/* Private GUniqueAppDBus methods */

static gboolean g_unique_app_dbus_register_factory (GUniqueAppDBus *app, gchar *factory_name);
static void g_unique_app_dbus_register_client (GUniqueAppDBus *app, gchar *factory_name);

/* GUniqueAppDBus object stuff */

static GObject *g_unique_app_dbus_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties)
{
	GObject *obj;
	obj = G_OBJECT_CLASS (parent_class)-> constructor (type, n_construct_properties, construct_properties);
	
	gchar* name;
	g_object_get(obj, "name", &name, NULL);
	
	GUniqueAppDBus *self = G_UNIQUE_APP_DBUS (obj);  
	
	if (name != NULL)
	{
		gchar *factory_name = g_strconcat (GUNIQUEAPP_FACTORY_BASE_NAME, name, NULL);
		
		if (!g_unique_app_dbus_register_factory (self, factory_name))
			g_unique_app_dbus_register_client (self, factory_name);
			
		g_free (factory_name);
	}
	
	return obj;
}

static void g_unique_app_dbus_dispose (GObject *obj) 
{
	GUniqueAppDBus *self = (GUniqueAppDBus*) obj; 

	if (self-> priv-> dispose_has_run) return;
	self-> priv-> dispose_has_run = TRUE;
	
	/* Delete DBus stuff */
   
	G_OBJECT_CLASS (parent_class)-> dispose (obj);  
}

static void g_unique_app_dbus_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)-> finalize (obj);
}

static void g_unique_app_dbus_init (GTypeInstance *instance, gpointer g_class)
{
	GUniqueAppDBus *self = G_UNIQUE_APP_DBUS (instance); 
	self-> priv = G_TYPE_INSTANCE_GET_PRIVATE (self, G_TYPE_UNIQUE_APP_DBUS, GUniqueAppDBusPrivate);

	self-> priv-> dispose_has_run = FALSE;
	self-> priv-> is_factory = TRUE;
	self-> priv-> proxy = NULL;
}

static void g_unique_app_dbus_class_init (GUniqueAppDBusClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass); 
	GUniqueAppClass *guniqueapp_class = G_UNIQUE_APP_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class-> constructor = g_unique_app_dbus_constructor;
	gobject_class-> dispose = g_unique_app_dbus_dispose;
	gobject_class-> finalize = g_unique_app_dbus_finalize;
	
	guniqueapp_class-> is_running = g_unique_app_dbus_is_running;
	guniqueapp_class-> send_message = g_unique_app_dbus_send_message;
	
	g_type_class_add_private (klass, sizeof (GUniqueAppDBusPrivate));
}

GType g_unique_app_dbus_get_type (void)
{
	static GType type = 0;
	
	if (type == 0)
	{
		static const GTypeInfo info = 
		{
			sizeof (GUniqueAppDBusClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) g_unique_app_dbus_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (GUniqueAppDBus),
			0,      /* n_preallocs */
			(GInstanceInitFunc) g_unique_app_dbus_init  /* instance_init */
		};
	
		type = g_type_register_static (G_TYPE_UNIQUE_APP, "GUniqueAppDBusType", &info, 0);
	}
	
	return type;
}

/* GUniqueAppDBus methods */

GUniqueAppDBus *g_unique_app_dbus_new (const gchar* name)
{
	return g_object_new (G_TYPE_UNIQUE_APP_DBUS, "name", name, NULL);
}

GUniqueAppDBus *g_unique_app_dbus_new_with_startup_id (const gchar* name, const gchar* startup_id)
{
	return g_object_new (G_TYPE_UNIQUE_APP_DBUS, "name", name, "startup-id", startup_id, NULL);
}

/* Private GUniqueAppDBus methods */

static gboolean g_unique_app_dbus_register_factory (GUniqueAppDBus *app, gchar *factory_name)
{
	DBusGConnection *connection;
	DBusGProxy *proxy;
	GError *error = NULL;
	GUniqueAppFactory *factory;
	guint32 request_name_ret;

	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (connection == NULL)
	{
		g_error_free (error);
		return FALSE;
	}

	proxy = dbus_g_proxy_new_for_name (connection,
					   DBUS_SERVICE_DBUS,
					   DBUS_PATH_DBUS,
					   DBUS_INTERFACE_DBUS);
	if (!org_freedesktop_DBus_request_name (proxy, factory_name, 0, &request_name_ret, &error))
	{
		g_error_free (error);
		return FALSE;
	}

	if (request_name_ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
	{
		return FALSE;
	}

	factory = g_object_new (GUNIQUEAPP_FACTORY_TYPE, NULL);
	dbus_g_connection_register_g_object (connection, "/Factory", G_OBJECT (factory));
	factory-> parent = app;
					     
	app-> priv-> is_factory = TRUE;
	return TRUE;
}

static void g_unique_app_dbus_register_client (GUniqueAppDBus *app, gchar *factory_name)
{
	DBusGConnection *connection;
	GError *error = NULL;
	
	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (connection == NULL)
	{
		g_warning ("GUniqueAppDBus: failed to open connection to bus (%s).", error->message);
		g_error_free (error);
	}
	
	else app-> priv-> proxy = dbus_g_proxy_new_for_name
		(connection, factory_name, "/Factory", "org.gnome.GUniqueAppInterface");  
		
	app-> priv-> is_factory = FALSE;
}

/* Overriden GUniqueApp methods */

static gboolean g_unique_app_dbus_is_running (GUniqueApp* app)
{
	GUniqueAppDBus *self = G_UNIQUE_APP_DBUS (app);
	return !self-> priv-> is_factory;
}

static void g_unique_app_dbus_send_message (GUniqueApp *app, GUniqueAppCommand command, const gchar* data, const gchar *startup_id, guint workspace)
{
	GError *error = NULL;
	GUniqueAppDBus *self = G_UNIQUE_APP_DBUS (app);

	if (self-> priv-> proxy == NULL)
	{
		g_warning ("GUniqueAppDBus: failed to send a message");
		return;
	}
	
	if (!org_gnome_GUniqueAppInterface_send_message (self-> priv-> proxy, command, data, startup_id, workspace, &error))
	{
		g_warning ("GUniqueAppDBus: failed to send a message (%s).", error-> message);
		g_error_free (error);
	}
}
