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
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include "guniqueapp-base.h"
#include "guniqueapp-marshal.h"

struct _GUniqueAppPrivate {
	gboolean dispose_has_run;
	gchar* name;
	gchar* startup_id;
	guint workspace;	
};

enum {
	G_UNIQUE_APP_NAME = 1,
	G_UNIQUE_APP_STARTUP_ID,
	G_UNIQUE_APP_WORKSPACE
};

static GObjectClass* parent_class = NULL;

static void g_unique_app_message (GUniqueApp *self, GUniqueAppCommand command, gchar* data, gchar *startup_id, guint workspace);

/* Copied from libnautilus/nautilus-program-choosing.c; Needed in case
 * we have no startup id (with its accompanying timestamp).
 */
static guint32 slowly_and_stupidly_obtain_timestamp (Display *xdisplay)
{
	Window xwindow;
	XEvent event;

	XSetWindowAttributes attrs;
	Atom atom_name;
	Atom atom_type;
	char *name;

	attrs.override_redirect = True;
	attrs.event_mask = PropertyChangeMask | StructureNotifyMask;

	xwindow =
	XCreateWindow (xdisplay,
			RootWindow (xdisplay, 0),
			-100, -100, 1, 1,
			0,
			CopyFromParent,
			CopyFromParent,
			CopyFromParent,
			CWOverrideRedirect | CWEventMask,
			&attrs);

	atom_name = XInternAtom (xdisplay, "WM_NAME", TRUE);
	g_assert (atom_name != None);
	atom_type = XInternAtom (xdisplay, "STRING", TRUE);
	g_assert (atom_type != None);

	name = "Fake Window";
	XChangeProperty (xdisplay, 
			xwindow, atom_name,
			atom_type,
			8, PropModeReplace, (unsigned char *)name, strlen (name));

  	XWindowEvent (xdisplay,
			xwindow,
			PropertyChangeMask,
			&event);

	XDestroyWindow(xdisplay, xwindow);

	return event.xproperty.time;
}

/* Get the currently visible workspace for the #GdkScreen.
 * If the X11 window property isn't found, 0 (the first workspace)
 * is returned.
 *
 * Taken from gedit/ galeon
 */
static guint g_unique_app_get_current_workspace (GdkScreen *screen)
{
	GdkWindow *root_win;
	GdkDisplay *display;
	Atom type;
	gint format;
	gulong nitems;
	gulong bytes_after;
	guint *current_desktop;
	guint ret = 0;

	g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

	root_win = gdk_screen_get_root_window (screen);
	display = gdk_screen_get_display (screen);

	XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (root_win),
			    gdk_x11_get_xatom_by_name_for_display (display, "_NET_CURRENT_DESKTOP"),
			    0, G_MAXLONG,
			    False, XA_CARDINAL, &type, &format, &nitems,
			    &bytes_after, (gpointer)&current_desktop);

	if (type == XA_CARDINAL && format == 32 && nitems > 0)
	{
		ret = current_desktop[0];
		XFree (current_desktop);
	}

	return ret;
}

static GObject* g_unique_app_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties)
{
	const gchar* startup_id;
	guint32 timestamp;
	gchar* fake_id = NULL;
	guint workspace;
	
	GObject *obj;
	obj = G_OBJECT_CLASS (parent_class)-> constructor (type, n_construct_properties, construct_properties);
	
	g_object_get (obj, "startup-id", &startup_id, NULL);
	
	/* If startup-id is not provided, lets try to obtain one */
#ifndef OLDER_GTK_VERSION
	if (startup_id == NULL) 
	{
		startup_id = gdk_x11_display_get_startup_notification_id (gdk_display_get_default ());
	}
#endif
	
	/* If the above gave no results, or excluded by #ifndef,
	   let's obtain the timestamp, and fake the startup id */
	if (startup_id == NULL || startup_id == '\0')
	{
		timestamp = slowly_and_stupidly_obtain_timestamp (gdk_display);
		fake_id = g_strdup_printf ("_TIME%lu", (unsigned long) timestamp);
		startup_id = fake_id;
	}
	
	g_object_set (obj, "startup-id", startup_id, NULL);
	if (fake_id != NULL) g_free (fake_id);
	
	/* If no workspace was provided, obtain the initial one */
	g_object_get (obj, "workspace", &workspace, NULL);
	
	if (workspace == 0)
	{
		workspace = g_unique_app_get_current_workspace (gdk_screen_get_default ());
		g_object_set (obj, "workspace", workspace, NULL);
	}
	
	return obj;
}

static void g_unique_app_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GUniqueApp *self = (GUniqueApp*) object;
	
	if (property_id == G_UNIQUE_APP_NAME)
	{
		if (self-> priv-> name != NULL) g_free (self-> priv-> name);
		self-> priv-> name = g_value_dup_string (value);
	}
	
	else if (property_id == G_UNIQUE_APP_STARTUP_ID)
	{
		if (self-> priv-> startup_id != NULL) g_free (self-> priv-> startup_id);
		self-> priv-> startup_id = g_value_dup_string (value);
	}
	
	else if (property_id == G_UNIQUE_APP_WORKSPACE)
	{
		self-> priv-> workspace = g_value_get_uint (value);
	}
	
	else G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void g_unique_app_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GUniqueApp *self = (GUniqueApp*) object;
	
	if (property_id == G_UNIQUE_APP_NAME)
	{
		g_value_set_string (value, self-> priv-> name);
	}
	
	else if (property_id == G_UNIQUE_APP_STARTUP_ID)
	{
		g_value_set_string (value, self-> priv-> startup_id);
	}
	
	else if (property_id == G_UNIQUE_APP_WORKSPACE)
	{
		g_value_set_uint (value, self-> priv-> workspace);
	}
	
	else G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);	 
}


static void g_unique_app_dispose (GObject *obj) 
{
	GUniqueApp *self = (GUniqueApp*) obj; 

	if (self-> priv-> dispose_has_run) return;
	self-> priv-> dispose_has_run = TRUE;  

	if (self-> priv-> name != NULL) g_free (self-> priv-> name);
	if (self-> priv-> startup_id != NULL) g_free (self-> priv-> startup_id);
	
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void g_unique_app_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)-> finalize (obj);
}

static void g_unique_app_init (GTypeInstance *instance, gpointer g_class)
{
	GUniqueApp *self = G_UNIQUE_APP (instance); 
	self-> priv = G_TYPE_INSTANCE_GET_PRIVATE (self, G_TYPE_UNIQUE_APP, GUniqueAppPrivate);

	self-> priv-> dispose_has_run = FALSE;
}

static void g_unique_app_class_init (GUniqueAppClass *klass)
{	
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class-> constructor = g_unique_app_constructor;
	gobject_class-> dispose = g_unique_app_dispose;
	gobject_class-> finalize = g_unique_app_finalize;
	gobject_class-> set_property = g_unique_app_set_property;
	gobject_class-> get_property = g_unique_app_get_property;
	
	GParamSpec *pspec = g_param_spec_string ("name",
				"GUniqueApp application name",
				"Set single-instance application's name",
				NULL /* default value */,
				G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
				
	g_object_class_install_property (gobject_class, G_UNIQUE_APP_NAME, pspec);
				
	pspec = g_param_spec_string ("startup-id",
				"GUniqueApp startup indentifier",
				"Provide a startup identifier",
				NULL /* default value */,
				G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
				
	g_object_class_install_property (gobject_class, G_UNIQUE_APP_STARTUP_ID, pspec);
	
	pspec = g_param_spec_uint ("workspace",
				"GUniqueApp workspace",
				"The workspace where the application was launched",
				0, G_MAXUINT, 0 /* min, max & default value */,
				G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
				
	g_object_class_install_property (gobject_class, G_UNIQUE_APP_WORKSPACE, pspec);			
	
	klass-> is_running = NULL;
	klass-> send_message = NULL;
	klass-> message = g_unique_app_message;
	
	klass-> message_signal_id = g_signal_new ("message",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		G_STRUCT_OFFSET (GUniqueAppClass, message),
		NULL, NULL,
		guniqueapp_marshal_VOID__ENUM_STRING_STRING_UINT,
		G_TYPE_NONE /* return_type */,
		4 /* n_params */,
		G_TYPE_UNIQUE_APP_COMMAND,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_UINT);
	
	g_type_class_add_private (klass, sizeof (GUniqueAppPrivate));
}

GType g_unique_app_get_type (void)
{
	static GType type = 0;
	
	if (type == 0)
	{
		static const GTypeInfo info = 
		{
			sizeof (GUniqueAppClass),
			NULL,   /* base_init */ 
			NULL,   /* base_finalize */
			(GClassInitFunc) g_unique_app_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (GUniqueApp),
			0,      /* n_preallocs */
			(GInstanceInitFunc) g_unique_app_init  /* instance_init */
		};
	
		type = g_type_register_static (G_TYPE_OBJECT, "GUniqueAppType", &info, G_TYPE_FLAG_ABSTRACT);
	}
	
	return type;
}

GType g_unique_app_command_get_type (void)
{
	static GType etype = 0;
	if (etype == 0) {
	static const GFlagsValue values[] = {
		{ G_UNIQUE_APP_ACTIVATE, "G_UNIQUE_APP_ACTIVATE", "activate" },
		{ G_UNIQUE_APP_NEW, "G_UNIQUE_APP_NEW", "new" },
		{ G_UNIQUE_APP_OPEN, "G_UNIQUE_APP_OPEN", "open" },
		{ G_UNIQUE_APP_CUSTOM, "G_UNIQUE_APP_CUSTOM", "custom" },
		{ 0, NULL, NULL }
		};
		
	etype = g_flags_register_static ("GUniqueAppCommand", values);
	}
	
	return etype;
}

/* Method definitions */

static void g_unique_app_message (GUniqueApp *self, GUniqueAppCommand command, gchar* data, gchar *startup_id, guint workspace)
{
	/* Process GDK stuff here */
}

gboolean g_unique_app_is_running (GUniqueApp *app)
{
	return G_UNIQUE_APP_GET_CLASS (app)-> is_running (app);
}

void g_unique_app_send_message(GUniqueApp *app, GUniqueAppCommand command, const gchar* data)
{
	gchar* startup_id = app-> priv-> startup_id;

	if (startup_id == NULL)
		startup_id = ""; 
	
	G_UNIQUE_APP_GET_CLASS (app)-> send_message (app, command, data, startup_id, app-> priv-> workspace);
}

void g_unique_app_activate (GUniqueApp *app)
{
	g_unique_app_send_message (app, G_UNIQUE_APP_ACTIVATE, "");
}

void g_unique_app_new_document (GUniqueApp *app)
{
	g_unique_app_send_message (app, G_UNIQUE_APP_NEW, "");
}

void g_unique_app_open_uri (GUniqueApp *app, const gchar *uri)
{
	g_unique_app_send_message (app, G_UNIQUE_APP_OPEN, uri);
}
 
void g_unique_app_custom_message (GUniqueApp *app, const gchar *data)
{
	g_unique_app_send_message (app, G_UNIQUE_APP_CUSTOM, data);
}

