/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright 2012  Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Written by: Stef Walter <stefw@gnome.org>
 */

#ifndef __UM_REALM_MANAGER_H__
#define __UM_REALM_MANAGER_H__

#include "um-realm-generated.h"

G_BEGIN_DECLS

typedef enum {
	UM_REALM_ERROR_BAD_LOGIN,
	UM_REALM_ERROR_BAD_PASSWORD,
	UM_REALM_ERROR_GENERIC,
} UmRealmErrors;

#define UM_REALM_ERROR             (um_realm_error_get_quark ())

GQuark           um_realm_error_get_quark         (void) G_GNUC_CONST;

#define UM_TYPE_REALM_MANAGER      (um_realm_manager_get_type ())
#define UM_REALM_MANAGER(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), UM_TYPE_REALM_MANAGER, UmRealmManager))
#define UM_IS_REALM_MANAGER(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UM_TYPE_REALM_MANAGER))

typedef struct _UmRealmManager UmRealmManager;

GType            um_realm_manager_get_type        (void) G_GNUC_CONST;

void             um_realm_manager_new             (GCancellable *cancellable,
                                                   GAsyncReadyCallback callback,
                                                   gpointer user_data);

UmRealmManager * um_realm_manager_new_finish      (GAsyncResult *result,
                                                   GError **error);

void             um_realm_manager_discover        (UmRealmManager *self,
                                                   const gchar *input,
                                                   GCancellable *cancellable,
                                                   GAsyncReadyCallback callback,
                                                   gpointer user_data);

GList *          um_realm_manager_discover_finish (UmRealmManager *self,
                                                   GAsyncResult *result,
                                                   GError **error);

GList *          um_realm_manager_get_realms      (UmRealmManager *self);

void             um_realm_login                   (const gchar *realm_name,
                                                   const gchar *domain,
                                                   const gchar *login,
                                                   const gchar *password,
                                                   GCancellable *cancellable,
                                                   GAsyncReadyCallback callback,
                                                   gpointer user_data);

gboolean         um_realm_login_finish            (GAsyncResult *result,
                                                   GBytes **credentials,
                                                   GError **error);

void             um_realm_join                    (UmRealmKerberos *realm,
                                                   GBytes *credentials,
                                                   GCancellable *cancellable,
                                                   GAsyncReadyCallback callback,
                                                   gpointer user_data);

gboolean         um_realm_join_finish             (UmRealmKerberos *self,
                                                   GAsyncResult *result,
                                                   GError **error);


G_END_DECLS

#endif /* __UM_REALM_H__ */