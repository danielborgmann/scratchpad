/* GUniqueApp - A library for GNOME single-instance applications
 * Copyright (C) 2006  Vytautas Liuolia
 *
 * Some parts ported from bacon-message-connection,
 * Copyright (C) 2003  Bastien Nocera <hadess@hadess.net>
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "guniqueapp-bacon.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

struct _GUniqueAppBaconPrivate {
	gboolean dispose_has_run;
	
	/* A server accepts connections */
	gboolean is_server;
	
	/* A pointer to the server instance */
	GUniqueAppBacon* server;

	/* The socket path itself */
	char *path;

	/* File descriptor of the socket */
	int fd;
	/* Channel to watch */
	GIOChannel *chan;
	/* Event id returned by g_io_add_watch() */
	int conn_id;

	/* Connections accepted by this connection */
	GSList *accepted_connections;	
};

static GUniqueAppClass* parent_class = NULL;

/* GUniqueApp methods */

static gboolean g_unique_app_bacon_is_running (GUniqueApp* app);
static void g_unique_app_bacon_send_message (GUniqueApp *app, GUniqueAppCommand command, const gchar* data, const gchar *startup_id, guint workspace);

/* Private GUniqueAppBacon methods */

static gboolean g_unique_app_bacon_setup_connection (GUniqueAppBacon *app);
static void g_unique_app_bacon_accept_new_connection (GUniqueAppBacon *app);
static gboolean g_unique_app_bacon_try_server (GUniqueAppBacon *app);
static gboolean g_unique_app_bacon_try_client (GUniqueAppBacon *app);
static gboolean g_unique_app_bacon_new_connection (GUniqueAppBacon *app, const char *prefix);
static void g_unique_app_bacon_free_connection (GUniqueAppBacon *app);
static void g_unique_app_bacon_process_message (GUniqueAppBacon *app, gchar *message);

/* GUniqueAppBacon object stuff */

static GObject *g_unique_app_bacon_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties)
{
	GObject *obj;
	obj = G_OBJECT_CLASS (parent_class)-> constructor (type, n_construct_properties, construct_properties);
	
	gchar* name;
	g_object_get (obj, "name", &name, NULL);
	
	if (name != NULL) g_unique_app_bacon_new_connection (G_UNIQUE_APP_BACON (obj), name); 
	
	return obj;
}

static void g_unique_app_bacon_dispose (GObject *obj) 
{
	GUniqueAppBacon *self = (GUniqueAppBacon*) obj; 

	if (self-> priv-> dispose_has_run) return;
	self-> priv-> dispose_has_run = TRUE;
	
	g_unique_app_bacon_free_connection(self);
   
	G_OBJECT_CLASS (parent_class)-> dispose (obj);  
}

static void g_unique_app_bacon_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)-> finalize (obj);
}

static void g_unique_app_bacon_init (GTypeInstance *instance, gpointer g_class)
{
	GUniqueAppBacon *self = G_UNIQUE_APP_BACON (instance); 
	self-> priv = G_TYPE_INSTANCE_GET_PRIVATE (self, G_TYPE_UNIQUE_APP_BACON, GUniqueAppBaconPrivate);

	self-> priv-> dispose_has_run = FALSE;  
	self-> priv-> server = self;
}

static void g_unique_app_bacon_class_init (GUniqueAppBaconClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass); 
	GUniqueAppClass *guniqueapp_class = G_UNIQUE_APP_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class-> constructor = g_unique_app_bacon_constructor;
	gobject_class-> dispose = g_unique_app_bacon_dispose;
	gobject_class-> finalize = g_unique_app_bacon_finalize;
	
	guniqueapp_class-> is_running = g_unique_app_bacon_is_running;
	guniqueapp_class-> send_message = g_unique_app_bacon_send_message;
	
	g_type_class_add_private (klass, sizeof (GUniqueAppBaconPrivate));
}

GType g_unique_app_bacon_get_type (void)
{
	static GType type = 0;
	
	if (type == 0)
	{
		static const GTypeInfo info = 
		{
			sizeof (GUniqueAppBaconClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) g_unique_app_bacon_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (GUniqueAppBacon),
			0,      /* n_preallocs */
			(GInstanceInitFunc) g_unique_app_bacon_init  /* instance_init */
		};
	
		type = g_type_register_static (G_TYPE_UNIQUE_APP, "GUniqueAppBaconType", &info, 0);
	}
	
	return type;
}

/* GUniqueAppBacon methods */

GUniqueAppBacon *g_unique_app_bacon_new (const gchar* name)
{
	return g_object_new (G_TYPE_UNIQUE_APP_BACON, "name", name, NULL);
}

GUniqueAppBacon *g_unique_app_bacon_new_with_startup_id (const gchar* name, const gchar* startup_id)
{
	return g_object_new (G_TYPE_UNIQUE_APP_BACON, "name", name, "startup-id", startup_id, NULL);
}
 
/* Various utility functions ported from bacon-message-conection */

static gboolean
test_is_socket (const char *path)
{
	struct stat s;

	if (stat (path, &s) == -1)
		return FALSE;

	if (S_ISSOCK (s.st_mode))
		return TRUE;

	return FALSE;
}

static gboolean is_owned_by_user_and_socket (const char *path)
{
	struct stat s;

	if (stat (path, &s) == -1)
		return FALSE;

	if (s.st_uid != geteuid ())
		return FALSE;

	if ((s.st_mode & S_IFSOCK) != S_IFSOCK)
		return FALSE;
	
	return TRUE;
}

static char* find_file_with_pattern (const char *dir, const char *pattern)
{
	GDir *filedir;
	char *found_filename;
	const char *filename;
	GPatternSpec *pat;

	filedir = g_dir_open (dir, 0, NULL);
	if (filedir == NULL)
		return NULL;

	pat = g_pattern_spec_new (pattern);
	if (pat == NULL)
	{
		g_dir_close (filedir);
		return NULL;
	}

	found_filename = NULL;

	while ((filename = g_dir_read_name (filedir)))
	{
		if (g_pattern_match_string (pat, filename))
		{
			char *tmp = g_build_filename (dir, filename, NULL);
			if (is_owned_by_user_and_socket (tmp))
				found_filename = g_strdup (filename);
			g_free (tmp);
		}

		if (found_filename != NULL)
			break;
	}

	g_pattern_spec_free (pat);
	g_dir_close (filedir);

	return found_filename;
}

static char* socket_filename (const char *prefix)
{
	char *pattern, *newfile, *path, *filename;
	const char *tmpdir;

	pattern = g_strdup_printf ("guniqueapp.%s.%s.*", prefix, g_get_user_name ());
	tmpdir = g_get_tmp_dir ();
	filename = find_file_with_pattern (tmpdir, pattern);
	if (filename == NULL)
	{
		newfile = g_strdup_printf ("guniqueapp.%s.%s.%u", prefix,
				g_get_user_name (), g_random_int ());
		path = g_build_filename (tmpdir, newfile, NULL);
		g_free (newfile);
	} else {
		path = g_build_filename (tmpdir, filename, NULL);
		g_free (filename);
	}

	g_free (pattern);
	return path;
}

static gboolean server_cb (GIOChannel *source, GIOCondition condition, gpointer data)
{
	GUniqueAppBacon *app = G_UNIQUE_APP_BACON (data);
	char *message, *subs, buf;
	int cd, rc, offset;
	gboolean finished;

	offset = 0;
	if (app-> priv-> is_server && app-> priv-> fd == g_io_channel_unix_get_fd (source)) {
		g_unique_app_bacon_accept_new_connection (app);
		return TRUE;
	}
	message = g_malloc (1);
	cd = app-> priv-> fd;
	rc = read (cd, &buf, 1);
	while (rc > 0 && buf != '\n')
	{
		message = g_realloc (message, rc + offset + 1);
		message[offset] = buf;
		offset = offset + rc;
		rc = read (cd, &buf, 1);
	}
	if (rc <= 0) {
		g_io_channel_shutdown (app-> priv-> chan, FALSE, NULL);
		g_io_channel_unref (app-> priv-> chan);
		app-> priv-> chan = NULL;
		close (app-> priv-> fd);
		app-> priv-> fd = -1;
		g_free (message);
		app-> priv-> conn_id = 0;

		return FALSE;
	}
	message[offset] = '\0';

	subs = message;
	finished = FALSE;

	while (finished == FALSE && *subs != '\0')
	{
		g_unique_app_bacon_process_message(app, subs);

		subs += strlen (subs) + 1;
		if (subs - message >= offset)
			finished = TRUE;
	}

	g_free (message);

	return TRUE;
}

/* Private GUniqueAppBacon methods, some adapted from Bastien's code */

static gboolean g_unique_app_bacon_setup_connection (GUniqueAppBacon *app)
{
	g_return_val_if_fail (app-> priv-> chan == NULL, FALSE);

	app-> priv-> chan = g_io_channel_unix_new (app-> priv-> fd);
	if (!app-> priv-> chan) 
		return FALSE;
	
	g_io_channel_set_line_term (app-> priv-> chan, "\n", 1);
	app-> priv-> conn_id = g_io_add_watch (app-> priv-> chan, G_IO_IN, server_cb, app);

	return TRUE;
}

static void g_unique_app_bacon_accept_new_connection (GUniqueAppBacon *app)
{
	GUniqueAppBacon *child_app;
	int alen;

	g_return_if_fail (app-> priv-> is_server);

	child_app = g_object_new (G_TYPE_UNIQUE_APP_BACON, NULL);
	child_app-> priv-> is_server = FALSE;
	child_app-> priv-> server = app;

	child_app-> priv-> fd = accept (app-> priv-> fd, NULL, (guint*) &alen);

	app-> priv-> accepted_connections =
		g_slist_prepend (app-> priv-> accepted_connections, child_app);

	g_unique_app_bacon_setup_connection (child_app);
}

static gboolean g_unique_app_bacon_try_server (GUniqueAppBacon *app)
{
	struct sockaddr_un uaddr;

	uaddr.sun_family = AF_UNIX;
	strncpy (uaddr.sun_path, app-> priv-> path,
			MIN (strlen(app-> priv-> path)+ 1, UNIX_PATH_MAX));
	app-> priv-> fd = socket (PF_UNIX, SOCK_STREAM, 0);
	if (bind (app-> priv-> fd, (struct sockaddr *) &uaddr, sizeof (uaddr)) == -1)
	{
		app-> priv-> fd = -1;
		return FALSE;
	}
	listen (app-> priv-> fd, 5);

	return g_unique_app_bacon_setup_connection (app);
}

static gboolean g_unique_app_bacon_try_client (GUniqueAppBacon *app)
{
	struct sockaddr_un uaddr;

	uaddr.sun_family = AF_UNIX;
	strncpy (uaddr.sun_path, app-> priv-> path,
			MIN(strlen(app-> priv-> path)+1, UNIX_PATH_MAX));
	app-> priv-> fd = socket (PF_UNIX, SOCK_STREAM, 0);
	if (connect (app-> priv-> fd, (struct sockaddr *) &uaddr,
				sizeof (uaddr)) == -1)
	{
		app-> priv-> fd = -1;
		return FALSE;
	}

	return g_unique_app_bacon_setup_connection (app);
}

static gboolean g_unique_app_bacon_new_connection (GUniqueAppBacon *app, const char *prefix)
{
	g_return_val_if_fail (prefix != NULL, FALSE);

	app-> priv-> path = socket_filename (prefix);

	if (test_is_socket (app-> priv-> path) == FALSE)
	{
		if (!g_unique_app_bacon_try_server (app))
		{
			g_unique_app_bacon_free_connection (app);
			return FALSE;
		}

		app-> priv-> is_server = TRUE;
		return TRUE;
	}

	if (g_unique_app_bacon_try_client (app) == FALSE)
	{
		unlink (app-> priv-> path);
		g_unique_app_bacon_try_server (app);
		if (app-> priv-> fd == -1)
		{
			g_unique_app_bacon_free_connection (app);
			return FALSE;
		}

		app-> priv-> is_server = TRUE;
		return TRUE;
	}

	app-> priv-> is_server = FALSE;
	return TRUE;
}

static void g_unique_app_bacon_free_connection (GUniqueAppBacon *app)
{
	GSList *child_conn;

	g_return_if_fail (app != NULL);
	/* Only servers can accept other connections */
	g_return_if_fail (app-> priv-> is_server != FALSE ||
			  app-> priv-> accepted_connections == NULL);

	child_conn = app-> priv-> accepted_connections;
	while (child_conn != NULL) {
		g_object_unref (child_conn->data);
		child_conn = g_slist_next (child_conn);
	}
	g_slist_free (app-> priv-> accepted_connections);

	if (app-> priv-> conn_id) {
		g_source_remove (app-> priv-> conn_id);
		app-> priv-> conn_id = 0;
	}
	if (app-> priv-> chan) {
		g_io_channel_shutdown (app-> priv-> chan, FALSE, NULL);
		g_io_channel_unref (app-> priv-> chan);
	}

	if (app-> priv-> is_server) {
		unlink (app-> priv-> path);
	}
	if (app-> priv-> fd != -1) {
		close (app-> priv-> fd);
	}

	g_free (app-> priv-> path);
}

static void g_unique_app_bacon_process_message (GUniqueAppBacon *app, gchar *message)
{
	gchar **elements = g_strsplit (message, "\t", 4);
	gchar **iter = elements;
	GUniqueAppCommand cmd = G_UNIQUE_APP_ACTIVATE;
	gchar *msg = NULL;
	gchar *startup_id = NULL;
	guint workspace = 0;
	
	if (*iter != NULL)
	{
		cmd = (GUniqueAppCommand) atoi (*iter);
		iter++;
	}
	
	if (*iter != NULL)
	{
		startup_id = g_strcompress (*iter);
		iter++;
	}
	
	if (*iter != NULL)
	{
		msg = g_strcompress (*iter);
		iter++;
	}
	
	if (*iter != NULL)
	{
		workspace = atoi (*iter);
		g_signal_emit (app-> priv-> server, G_UNIQUE_APP_GET_CLASS (app-> priv-> server)-> message_signal_id,
			0 /* details */,
			cmd, msg, startup_id, workspace); 
	}
	else g_warning ("GUniqueAppBacon: incomplete message.");
	
	g_free (startup_id);
	g_free (msg);
	g_strfreev (elements);
}

/* Overriden GUniqueApp methods */

static gboolean g_unique_app_bacon_is_running (GUniqueApp* app)
{
	GUniqueAppBacon *self = G_UNIQUE_APP_BACON (app);
	return !self-> priv-> is_server;
}

static void g_unique_app_bacon_send_message (GUniqueApp *app, GUniqueAppCommand command, const gchar* data, const gchar *startup_id, guint workspace)
{
	GUniqueAppBacon *self = G_UNIQUE_APP_BACON (app);
	gchar *escaped_data = g_strescape (data, NULL);
	gchar *escaped_id = g_strescape (startup_id, NULL);
	
	gchar *message = g_strdup_printf ("%i\t%s\t%s\t%u", (int) command, escaped_id, escaped_data, (unsigned int) workspace);
	g_free (escaped_data);
	g_free (escaped_id);
	
	g_io_channel_write_chars (self-> priv-> chan, message, strlen (message),
				  NULL, NULL);
	g_io_channel_write_chars (self-> priv-> chan, "\n", 1, NULL, NULL);
	g_io_channel_flush (self-> priv-> chan, NULL);
	
	g_free (message);
}
 
