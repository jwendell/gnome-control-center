/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 *
 * Copyright (C) 2012, 2013 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Debarshi Ray <debarshir@gnome.org>
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#define GOA_API_IS_SUBJECT_TO_CHANGE
#define GOA_BACKEND_API_IS_SUBJECT_TO_CHANGE
#include <goabackend/goabackend.h>

#include <egg-list-box/egg-list-box.h>
#include <libgd/gd-stack.h>

#include "cc-online-accounts-add-account-dialog.h"

#define BRANDED_PAGE "_branded"
#define OTHER_PAGE "_other"

struct _GoaPanelAddAccountDialogPrivate
{
  EggListBox *branded_list_box;
  EggListBox *contacts_list_box;
  EggListBox *mail_list_box;
  EggListBox *ticketing_list_box;
  GError *error;
  GoaClient *client;
  GoaObject *object;
  GtkListStore *list_store;
  GtkWidget *contacts_grid;
  GtkWidget *mail_grid;
  GtkWidget *ticketing_grid;
  GtkWidget *stack;
  gboolean add_other;
};

#define GOA_ADD_ACCOUNT_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GOA_TYPE_PANEL_ADD_ACCOUNT_DIALOG, GoaPanelAddAccountDialogPrivate))

enum
{
  PROP_0,
  PROP_CLIENT,
};

enum
{
  COLUMN_PROVIDER,
  COLUMN_ICON,
  COLUMN_MARKUP,
  N_COLUMNS
};

G_DEFINE_TYPE (GoaPanelAddAccountDialog, goa_panel_add_account_dialog, GTK_TYPE_DIALOG)

static void
add_account_dialog_add_account (GoaPanelAddAccountDialog *add_account, GoaProvider *provider)
{
  GoaPanelAddAccountDialogPrivate *priv = add_account->priv;
  GList *children;
  GList *l;
  GtkWidget *action_area;
  GtkWidget *vbox;

  action_area = gtk_dialog_get_action_area (GTK_DIALOG (add_account));
  vbox = gtk_dialog_get_content_area (GTK_DIALOG (add_account));
  children = gtk_container_get_children (GTK_CONTAINER (vbox));
  for (l = children; l != NULL; l = l->next)
    {
      GtkWidget *child = l->data;
      if (child != action_area)
        gtk_container_remove (GTK_CONTAINER (vbox), child);
    }
  g_list_free (children);

  priv->object = goa_provider_add_account (provider,
                                           priv->client,
                                           GTK_DIALOG (add_account),
                                           GTK_BOX (vbox),
                                           &priv->error);
}

static void
list_box_child_activated_cb (GoaPanelAddAccountDialog *add_account, GtkWidget *child)
{
  GoaProvider *provider;

  provider = g_object_get_data (G_OBJECT (child), "provider");
  if (provider == NULL)
    {
      gd_stack_set_visible_child_name (GD_STACK (add_account->priv->stack), OTHER_PAGE);
      return;
    }

  add_account_dialog_add_account (add_account, provider);
  gtk_dialog_response (GTK_DIALOG (add_account), GTK_RESPONSE_OK);
}

static void
list_box_separator_cb (GtkWidget **separator, GtkWidget *child, GtkWidget *before, gpointer user_data)
{
  if (*separator == NULL && before != NULL)
    {
      *separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

      /* https://bugzilla.gnome.org/show_bug.cgi?id=690545 */
      g_object_ref_sink (*separator);
      gtk_widget_show (*separator);
    }
}

static void
add_account_dialog_create_group_ui (GoaPanelAddAccountDialog *add_account,
                                    EggListBox **list_box,
                                    GtkWidget **group_grid,
                                    GtkWidget *page_grid,
                                    const gchar *name)
{
  GtkWidget *label;
  GtkWidget *sw;
  gchar *markup;

  *group_grid = gtk_grid_new ();
  gtk_widget_set_no_show_all (*group_grid, TRUE);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (*group_grid), GTK_ORIENTATION_VERTICAL);
  gtk_grid_set_row_spacing (GTK_GRID (*group_grid), 6);
  gtk_container_add (GTK_CONTAINER (page_grid), *group_grid);

  markup = g_strdup_printf ("<b>%s</b>", name);
  label = gtk_label_new (NULL);
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_label_set_markup (GTK_LABEL (label), markup);
  gtk_container_add (GTK_CONTAINER (*group_grid), label);
  g_free (markup);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_hexpand (sw, TRUE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (*group_grid), sw);

  *list_box = egg_list_box_new ();
  egg_list_box_add_to_scrolled (*list_box, GTK_SCROLLED_WINDOW (sw));
  egg_list_box_set_selection_mode (*list_box, GTK_SELECTION_NONE);
  egg_list_box_set_separator_funcs (*list_box, list_box_separator_cb, NULL, NULL);
  g_signal_connect_swapped (*list_box, "child-activated",
                            G_CALLBACK (list_box_child_activated_cb), add_account);
}

static void
add_account_dialog_create_provider_ui (GoaPanelAddAccountDialog *add_account,
                                       GoaProvider *provider,
                                       EggListBox *list_box)
{
  GIcon *icon;
  GtkWidget *row_grid;
  GtkWidget *image;
  GtkWidget *label;
  gchar *markup;
  gchar *name;

  row_grid = gtk_grid_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (row_grid), GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_set_column_spacing (GTK_GRID (row_grid), 6);
  gtk_container_add (GTK_CONTAINER (list_box), row_grid);

  if (provider == NULL)
    {
      g_object_set_data (G_OBJECT (row_grid), "provider", NULL);
      icon = g_themed_icon_new_with_default_fallbacks ("goa-account");
      name = g_strdup (_("Other"));
    }
  else
    {
      g_object_set_data_full (G_OBJECT (row_grid), "provider", g_object_ref (provider), g_object_unref);
      icon = goa_provider_get_provider_icon (provider, NULL);
      name = goa_provider_get_provider_name (provider, NULL);
    }

  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (row_grid), image);

  markup = g_strdup_printf ("<b>%s</b>", name);
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), markup);
  gtk_container_add (GTK_CONTAINER (row_grid), label);
  g_free (markup);

  g_free (name);
  g_object_unref (icon);
}

static void
goa_panel_add_account_dialog_realize (GtkWidget *widget)
{
  GoaPanelAddAccountDialog *add_account = GOA_PANEL_ADD_ACCOUNT_DIALOG (widget);
  GtkWidget *button;
  GtkWindow *parent;

  parent = gtk_window_get_transient_for (GTK_WINDOW (add_account));
  if (parent != NULL)
    {
      gint width;
      gint height;

      gtk_window_get_size (parent, &width, &height);
      gtk_widget_set_size_request (GTK_WIDGET (add_account), (gint) (0.5 * width), (gint) (0.9 * height));
    }

  GTK_WIDGET_CLASS (goa_panel_add_account_dialog_parent_class)->realize (widget);

  button = gtk_dialog_get_widget_for_response (GTK_DIALOG (add_account), GTK_RESPONSE_CANCEL);
  gtk_widget_grab_focus (button);
}

static void
goa_panel_add_account_dialog_dispose (GObject *object)
{
  GoaPanelAddAccountDialog *add_account = GOA_PANEL_ADD_ACCOUNT_DIALOG (object);
  GoaPanelAddAccountDialogPrivate *priv = add_account->priv;

  g_clear_object (&priv->object);
  g_clear_object (&priv->client);

  G_OBJECT_CLASS (goa_panel_add_account_dialog_parent_class)->dispose (object);
}

static void
goa_panel_add_account_dialog_finalize (GObject *object)
{
  GoaPanelAddAccountDialog *add_account = GOA_PANEL_ADD_ACCOUNT_DIALOG (object);
  GoaPanelAddAccountDialogPrivate *priv = add_account->priv;

  if (priv->error != NULL)
    g_error_free (priv->error);

  G_OBJECT_CLASS (goa_panel_add_account_dialog_parent_class)->finalize (object);
}

static void
goa_panel_add_account_dialog_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  GoaPanelAddAccountDialog *add_account = GOA_PANEL_ADD_ACCOUNT_DIALOG (object);
  GoaPanelAddAccountDialogPrivate *priv = add_account->priv;

  switch (prop_id)
    {
    case PROP_CLIENT:
      priv->client = GOA_CLIENT (g_value_dup_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
goa_panel_add_account_dialog_init (GoaPanelAddAccountDialog *add_account)
{
  GoaPanelAddAccountDialogPrivate *priv;
  GtkWidget *sw;
  GtkWidget *vbox;
  GtkWidget *grid;

  add_account->priv = GOA_ADD_ACCOUNT_DIALOG_GET_PRIVATE (add_account);
  priv = add_account->priv;

  gtk_container_set_border_width (GTK_CONTAINER (add_account), 6);
  gtk_window_set_modal (GTK_WINDOW (add_account), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (add_account), FALSE);
  /* translators: This is the title of the "Add Account" dialog. */
  gtk_window_set_title (GTK_WINDOW (add_account), _("Add Account"));

  vbox = gtk_dialog_get_content_area (GTK_DIALOG (add_account));
  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 5);
  gtk_widget_set_margin_bottom (grid, 6);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (grid), GTK_ORIENTATION_VERTICAL);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 12);
  gtk_container_add (GTK_CONTAINER (vbox), grid);

  priv->list_store = gtk_list_store_new (N_COLUMNS, GOA_TYPE_PROVIDER, G_TYPE_ICON, G_TYPE_STRING);

  priv->stack = gd_stack_new ();
  gd_stack_set_transition_type (GD_STACK (priv->stack), GD_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_container_add (GTK_CONTAINER (grid), priv->stack);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
  gtk_widget_set_hexpand (sw, TRUE);
  gtk_widget_set_vexpand (sw, TRUE);
  gd_stack_add_named (GD_STACK (priv->stack), sw, BRANDED_PAGE);

  priv->branded_list_box = egg_list_box_new ();
  egg_list_box_add_to_scrolled (priv->branded_list_box, GTK_SCROLLED_WINDOW (sw));
  egg_list_box_set_selection_mode (priv->branded_list_box, GTK_SELECTION_NONE);
  egg_list_box_set_separator_funcs (priv->branded_list_box, list_box_separator_cb, NULL, NULL);
  g_signal_connect_swapped (priv->branded_list_box, "child-activated",
                            G_CALLBACK (list_box_child_activated_cb), add_account);

  grid = gtk_grid_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (grid), GTK_ORIENTATION_VERTICAL);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 12);
  gd_stack_add_named (GD_STACK (priv->stack), grid, OTHER_PAGE);

  add_account_dialog_create_group_ui (add_account,
                                      &priv->mail_list_box,
                                      &priv->mail_grid,
                                      grid,
                                      _("Mail"));

  add_account_dialog_create_group_ui (add_account,
                                      &priv->contacts_list_box,
                                      &priv->contacts_grid,
                                      grid,
                                      _("Contacts"));

  add_account_dialog_create_group_ui (add_account,
                                      &priv->ticketing_list_box,
                                      &priv->ticketing_grid,
                                      grid,
                                      _("Resources"));

  gd_stack_set_visible_child_name (GD_STACK (priv->stack), BRANDED_PAGE);

  gtk_dialog_add_button (GTK_DIALOG (add_account), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
  gtk_dialog_set_default_response (GTK_DIALOG (add_account), GTK_RESPONSE_CANCEL);
}

static void
goa_panel_add_account_dialog_class_init (GoaPanelAddAccountDialogClass *klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->dispose = goa_panel_add_account_dialog_dispose;
  object_class->finalize = goa_panel_add_account_dialog_finalize;
  object_class->set_property = goa_panel_add_account_dialog_set_property;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->realize = goa_panel_add_account_dialog_realize;

  g_type_class_add_private (object_class, sizeof (GoaPanelAddAccountDialogPrivate));

  g_object_class_install_property (object_class,
                                   PROP_CLIENT,
                                   g_param_spec_object ("client",
							"Goa Client",
							"A Goa client for talking to the Goa daemon.",
							GOA_TYPE_CLIENT,
							G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

GtkWidget *
goa_panel_add_account_dialog_new (GoaClient *client)
{
  return g_object_new (GOA_TYPE_PANEL_ADD_ACCOUNT_DIALOG, "client", client, NULL);
}

void
goa_panel_add_account_dialog_add_provider (GoaPanelAddAccountDialog *add_account, GoaProvider *provider)
{
  GoaPanelAddAccountDialogPrivate *priv = add_account->priv;
  EggListBox *list_box;
  GoaProviderGroup group;
  GtkWidget *group_grid = NULL;

  g_return_if_fail (provider != NULL);

  group = goa_provider_get_provider_group (provider);

  /* The list of providers returned by GOA are sorted such that all
   * the branded providers are at the beginning of the list, followed
   * by the others. Since this is the order in which they are added,
   * we can rely on this fact, which helps to simplify the code.
   */
  if (group != GOA_PROVIDER_GROUP_BRANDED && !priv->add_other)
    {
      add_account_dialog_create_provider_ui (add_account, NULL, priv->branded_list_box);
      priv->add_other = TRUE;
    }

  switch (group)
    {
    case GOA_PROVIDER_GROUP_BRANDED:
      list_box = priv->branded_list_box;
      break;
    case GOA_PROVIDER_GROUP_CONTACTS:
      group_grid = priv->contacts_grid;
      list_box = priv->contacts_list_box;
      break;
    case GOA_PROVIDER_GROUP_MAIL:
      group_grid = priv->mail_grid;
      list_box = priv->mail_list_box;
      break;
    case GOA_PROVIDER_GROUP_TICKETING:
      group_grid = priv->ticketing_grid;
      list_box = priv->ticketing_list_box;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  add_account_dialog_create_provider_ui (add_account, provider, list_box);

  if (group_grid != NULL)
    {
      gtk_widget_set_no_show_all (group_grid, FALSE);
      gtk_widget_show_all (group_grid);
    }
}

GoaObject *
goa_panel_add_account_dialog_get_account (GoaPanelAddAccountDialog *add_account, GError **error)
{
  GoaPanelAddAccountDialogPrivate *priv = add_account->priv;

  if (error != NULL && priv->error != NULL)
    *error = g_error_copy (priv->error);

  if (priv->object != NULL)
    g_object_ref (priv->object);

  return priv->object;
}
