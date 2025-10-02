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

#include "glib.h"
#include "glibconfig.h"
#include "libxfce4panel/xfce-panel-plugin.h"
#include <errno.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>

#include "sample.h"
#include "sample-dialogs.h"

/* default settings */
#define DEFAULT_SETTING1 NULL
#define DEFAULT_ICON_SIZE 12
#define DEFAULT_SETTING3 FALSE

#define DEFAULT_PANEL_ICON_NAME "system-software-update"

/* prototypes */
static void sample_construct(XfcePanelPlugin *plugin);

/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER(sample_construct);

static void sample_read(SamplePlugin *sample) {
  XfceRc *rc;
  gchar *file;
  const gchar *value;

  /* get the plugin config file location */
  file = xfce_panel_plugin_save_location(sample->plugin, TRUE);

  if (G_LIKELY(file != NULL)) {
    /* open the config file, readonly */
    rc = xfce_rc_simple_open(file, TRUE);

    /* cleanup */
    g_free(file);

    if (G_LIKELY(rc != NULL)) {
      /* read the settings */
      value = xfce_rc_read_entry(rc, "setting1", DEFAULT_SETTING1);
      sample->setting1 = g_strdup(value);

      sample->icon_size =
          xfce_rc_read_int_entry(rc, "icon_size", DEFAULT_ICON_SIZE);
      sample->setting3 =
          xfce_rc_read_bool_entry(rc, "setting3", DEFAULT_SETTING3);

      /* cleanup */
      xfce_rc_close(rc);

      /* leave the function, everything went well */
      return;
    }
  }

  /* something went wrong, apply default values */
  DBG("Applying default settings");

  sample->setting1 = g_strdup(DEFAULT_SETTING1);
  sample->icon_size = DEFAULT_ICON_SIZE;
  sample->setting3 = DEFAULT_SETTING3;
}

static SamplePlugin *sample_new(XfcePanelPlugin *plugin) {
  SamplePlugin *sample;
  GtkOrientation orientation;

  /* allocate memory for the plugin structure */
  sample = g_slice_new0(SamplePlugin);

  /* pointer to plugin */
  sample->plugin = plugin;

  /* read the user settings */
  sample_read(sample);

  /* get the current orientation */
  orientation = xfce_panel_plugin_get_orientation(plugin);

  /* create some panel widgets */
  sample->ebox = gtk_event_box_new();
  gtk_widget_show(sample->ebox);

  sample->hvbox = gtk_box_new(orientation, 2);
  gtk_widget_show(sample->hvbox);
  gtk_container_add(GTK_CONTAINER(sample->ebox), sample->hvbox);

  /* some sample widgets */
  sample->icon =
      gtk_image_new_from_icon_name(DEFAULT_PANEL_ICON_NAME, GTK_ICON_SIZE_MENU);
  gtk_image_set_pixel_size(GTK_IMAGE(sample->icon), sample->icon_size);
  gtk_widget_show(sample->icon);
  gtk_box_pack_start(GTK_BOX(sample->hvbox), sample->icon, FALSE, FALSE, 0);

  return sample;
}

static void sample_free(XfcePanelPlugin *plugin, SamplePlugin *sample) {
  GtkWidget *dialog;

  /* check if the dialog is still open. if so, destroy it */
  dialog = g_object_get_data(G_OBJECT(plugin), "dialog");
  if (G_UNLIKELY(dialog != NULL))
    gtk_widget_destroy(dialog);

  /* destroy the panel widgets */
  gtk_widget_destroy(sample->hvbox);

  /* cleanup the settings */
  if (G_LIKELY(sample->setting1 != NULL))
    g_free(sample->setting1);

  /* free the plugin structure */
  g_slice_free(SamplePlugin, sample);
}

static void sample_orientation_changed(XfcePanelPlugin *plugin,
                                       GtkOrientation orientation,
                                       SamplePlugin *sample) {
  /* change the orientation of the box */
  gtk_orientable_set_orientation(GTK_ORIENTABLE(sample->hvbox), orientation);
}

static gboolean sample_size_changed(XfcePanelPlugin *plugin, gint size,
                                    SamplePlugin *sample) {
  GtkOrientation orientation;

  /* get the orientation of the plugin */
  orientation = xfce_panel_plugin_get_orientation(plugin);

  /* set the widget size */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request(GTK_WIDGET(plugin), -1, size);
  else
    gtk_widget_set_size_request(GTK_WIDGET(plugin), size, -1);

  /* we handled the orientation */
  return TRUE;
}

static void on_count_of_available_updates_finished(GObject *source,
                                                   GAsyncResult *result,
                                                   gpointer user_data) {
  GtkLabel *label = GTK_LABEL(user_data);
  GError *error = NULL;
  gchar *stdout_buff = NULL;

  if (g_subprocess_communicate_utf8_finish(G_SUBPROCESS(source), result,
                                           &stdout_buff, NULL, &error)) {
    gtk_label_set_text(label,
                       stdout_buff ? g_strchomp(stdout_buff) : "no output");
  } else {
    g_printerr("Couldn't run the list-updates callback: %s", error->message);
    g_error_free(error);
  }

  g_object_unref(source);
}

static void count_of_available_updates(GtkLabel *label) {
  GError *error = NULL;

  GSubprocess *proc =
      g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE, &error, "sh", "-c",
                       "zypper list-updates | wc -l", NULL);

  if (!proc) {
    gtk_label_set_text(GTK_LABEL(label), error->message);
    g_error_free(error);
    return;
  }

  g_subprocess_communicate_utf8_async(
      proc, NULL, NULL, on_count_of_available_updates_finished, label);
}

static void on_system_update(GObject *source, GAsyncResult *result,
                             gpointer user_data) {
  GError *error = NULL;
  GtkLabel *label = GTK_LABEL(user_data);

  if (g_subprocess_communicate_utf8_finish(G_SUBPROCESS(source), result, NULL,
                                           NULL, &error)) {
    count_of_available_updates(label);
  } else {
    g_printerr("Couldn't finish the system update: %s", error->message);
    g_error_free(error);
  }

  g_object_unref(source);
}

static gboolean on_plugin_click(GtkWidget *widget, GdkEventButton *event,
                                gpointer user_data) {
  GError *error = NULL;
  GtkLabel *label = GTK_LABEL(user_data);

  if (event->type == GDK_BUTTON_PRESS && event->button == 1) {

    GSubprocess *proc = g_subprocess_new(
        G_SUBPROCESS_FLAGS_STDOUT_PIPE, &error, "exo-open", "--launch",
        "TerminalEmulator", "sudo zypper dup", NULL);

    if (!proc) {
      g_printerr("Couldn't launch the update process: %s", error->message);
      g_error_free(error);
      return FALSE;
    }

    g_subprocess_communicate_utf8_async(proc, NULL, NULL, on_system_update,
                                        label);
  }

  return FALSE;
}

static void sample_construct(XfcePanelPlugin *plugin) {
  SamplePlugin *sample;

  /* setup transation domain */
  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* create the plugin */
  sample = sample_new(plugin);

  /* add the ebox to the panel */
  gtk_container_add(GTK_CONTAINER(plugin), sample->ebox);

  GtkWidget *label = gtk_label_new("running");
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(sample->hvbox), label, FALSE, FALSE, 0);

  count_of_available_updates(GTK_LABEL(label));

  gtk_widget_add_events(sample->ebox, GDK_BUTTON_PRESS_MASK);

  g_signal_connect(sample->ebox, "button-press-event",
                   G_CALLBACK(on_plugin_click), label);

  /* show the panel's right-click menu on this ebox */
  xfce_panel_plugin_add_action_widget(plugin, sample->ebox);

  /* connect plugin signals */
  g_signal_connect(G_OBJECT(plugin), "free-data", G_CALLBACK(sample_free),
                   sample);

  g_signal_connect(G_OBJECT(plugin), "save", G_CALLBACK(sample_save), sample);

  g_signal_connect(G_OBJECT(plugin), "size-changed",
                   G_CALLBACK(sample_size_changed), sample);

  g_signal_connect(G_OBJECT(plugin), "orientation-changed",
                   G_CALLBACK(sample_orientation_changed), sample);

  /* show the configure menu item and connect signal */
  xfce_panel_plugin_menu_show_configure(plugin);
  g_signal_connect(G_OBJECT(plugin), "configure-plugin",
                   G_CALLBACK(sample_configure), sample);

  /* show the about menu item and connect signal */
  xfce_panel_plugin_menu_show_about(plugin);
  g_signal_connect(G_OBJECT(plugin), "about", G_CALLBACK(sample_about), NULL);
}

void sample_save(XfcePanelPlugin *plugin, SamplePlugin *sample) {
  XfceRc *rc;
  gchar *file;

  file = xfce_panel_plugin_save_location(plugin, TRUE);
  if (!file) {
    g_printerr("Couldn't create or open the configuration file");
    return;
  }

  rc = xfce_rc_simple_open(file, FALSE);
  g_free(file);

  if (rc) {
    xfce_rc_write_int_entry(rc, "icon_size", sample->icon_size);
  }
}
