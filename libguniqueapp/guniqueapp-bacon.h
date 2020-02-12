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
 
#ifndef _GUNIQUEAPP_BACON_H
#define _GUNIQUEAPP_BACON_H

#include "guniqueapp-base.h"

#define G_TYPE_UNIQUE_APP_BACON			(g_unique_app_bacon_get_type ())
#define G_UNIQUE_APP_BACON(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_UNIQUE_APP_BACON, GUniqueAppBacon))
#define G_UNIQUE_APP_BACON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_UNIQUE_APP_BACON, GUniqueAppBaconClass))
#define G_IS_UNIQUE_APP_BACON(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_UNIQUE_APP_BACON))
#define G_IS_UNIQUE_APP_BACON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_UNIQUE_APP_BACON))
#define G_UNIQUE_APP_BACON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_UNIQUE_APP_BACON, GUniqueAppBaconClass))

typedef struct _GUniqueAppBacon GUniqueAppBacon;
typedef struct _GUniqueAppBaconClass GUniqueAppBaconClass;
typedef struct _GUniqueAppBaconPrivate GUniqueAppBaconPrivate;

struct _GUniqueAppBacon {
	GUniqueApp parent;
	
	/* private */
	GUniqueAppBaconPrivate *priv;
};

struct _GUniqueAppBaconClass {
	GUniqueAppClass parent;
};

GType g_unique_app_bacon_get_type (void);

/* Method definitions */

GUniqueAppBacon *g_unique_app_bacon_new (const gchar* name);
GUniqueAppBacon *g_unique_app_bacon_new_with_startup_id (const gchar* name, const gchar* startup_id);

#endif /* _GUNIQUEAPP_BACON_H */
