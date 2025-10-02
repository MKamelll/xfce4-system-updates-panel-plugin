/*  $Id$
 *
 *  Copyright (C) 2019 John Doo <john@foo.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"
#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif

#include <gtk/gtk.h>
#include <string.h>

#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4ui/libxfce4ui.h>

#include "sample.h"
#include "sample-dialogs.h"

/* the website url */
#define PLUGIN_WEBSITE "https://docs.xfce.org/panel-plugins/xfce4-sample-plugin"

/* config */
#define DEFAULT_DIALOG_WIDTH 340
#define DEFAULT_DIALOG_HEIGHT 240

static void sample_configure_response(GtkWidget *dialog, gint response,
                                      SamplePlugin *sample) {
  gboolean result;

  if (response == GTK_RESPONSE_HELP) {
    /* show help */
#if LIBXFCE4UI_CHECK_VERSION(4, 21, 0)
    result = g_spawn_command_line_async(
        "xfce-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);
#else
    result = g_spawn_command_line_async(
        "exo-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);
#endif
    if (G_UNLIKELY(result == FALSE))
      g_warning(_("Unable to open the following url: %s"), PLUGIN_WEBSITE);
  } else {

    GtkWidget *spin_button =
        g_object_get_data(G_OBJECT(dialog), "icon-size-spin");

    if (spin_button) {
      sample->icon_size =
          gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_button));
    }

    /* remove the dialog data from the plugin */
    g_object_set_data(G_OBJECT(sample->plugin), "dialog", NULL);

    /* save the plugin */
    sample_save(sample->plugin, sample);

    /* destroy the properties dialog */
    gtk_widget_destroy(dialog);
  }
}

void on_icon_size_changed(GtkWidget *spin_button_widget, gpointer user_data)
{
  GtkSpinButton *spin_button = GTK_SPIN_BUTTON(spin_button_widget);
  SamplePlugin *sample = user_data;

  sample->icon_size = gtk_spin_button_get_value_as_int(spin_button);
  gtk_image_set_pixel_size(GTK_IMAGE(sample->icon), sample->icon_size);
  
}
  

void sample_configure(XfcePanelPlugin *plugin, SamplePlugin *sample) {
  GtkWidget *dialog, *content;

  if (sample->settings_dialog != NULL) {
    gtk_window_present(GTK_WINDOW(sample->settings_dialog));
    return;
  }

  /* create the dialog */
  sample->settings_dialog = dialog = xfce_titled_dialog_new_with_mixed_buttons(
      _("Sample Plugin"),
      GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))),
      GTK_DIALOG_DESTROY_WITH_PARENT, "help-browser-symbolic", _("_Help"),
      GTK_RESPONSE_HELP, "window-close-symbolic", _("_Close"), GTK_RESPONSE_OK,
      NULL);
  g_object_add_weak_pointer(G_OBJECT(sample->settings_dialog),
                            (gpointer *)&sample->settings_dialog);

  /* center dialog on the screen */
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(dialog), DEFAULT_DIALOG_WIDTH,
                              DEFAULT_DIALOG_HEIGHT);

  /* set dialog icon */
  gtk_window_set_icon_name(GTK_WINDOW(dialog), "xfce4-settings");

  /* link the dialog to the plugin, so we can destroy it when the plugin
   * is closed, but the dialog is still open */
  g_object_set_data(G_OBJECT(plugin), "dialog", dialog);

  /* connect the response signal to the dialog */
  g_signal_connect(G_OBJECT(dialog), "response",
                   G_CALLBACK(sample_configure_response), sample);

  content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(content), hbox);

  GtkWidget *set_icon_size_label = gtk_label_new("Set Icon Size");
  gtk_box_pack_start(GTK_BOX(hbox), set_icon_size_label, TRUE, FALSE, 0);
  gtk_widget_show(set_icon_size_label);

  GtkWidget *spin_button = gtk_spin_button_new_with_range(0, 100, 1);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_button), TRUE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), sample->icon_size);
  gtk_box_pack_start(GTK_BOX(hbox), spin_button, TRUE, FALSE, 0);
  g_signal_connect(spin_button, "value-changed", G_CALLBACK(on_icon_size_changed), sample);
  gtk_widget_show(spin_button);

  g_object_set_data(G_OBJECT(dialog), "icon-size-spin", spin_button);
  gtk_widget_show(hbox);

  /* show the entire dialog */
  gtk_widget_show(dialog);
}

void sample_about(XfcePanelPlugin *plugin) {
  /* about dialog code. you can use the GtkAboutDialog
   * or the XfceAboutInfo widget */
  const gchar *auth[] = {"Xfce development team <xfce4-dev@xfce.org>", NULL};

  gtk_show_about_dialog(
      NULL, "logo-icon-name", "xfce4-sample-plugin", "license",
      xfce_get_license_text(XFCE_LICENSE_TEXT_GPL), "version", VERSION_FULL,
      "program-name", PACKAGE_NAME, "comments", _("This is a sample plugin"),
      "website", PLUGIN_WEBSITE, "copyright",
      "Copyright \xc2\xa9 2006-" COPYRIGHT_YEAR " The Xfce development team",
      "authors", auth, NULL);
}
