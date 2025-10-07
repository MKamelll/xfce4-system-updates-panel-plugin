/*  $Id$
 *
 *  Copyright (C) 2012 John Doo <john@foo.org>
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

#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>
#include <time.h>

G_BEGIN_DECLS

/* plugin structure */
typedef struct {
  XfcePanelPlugin *plugin;

  /* panel widgets */
  GtkWidget *ebox;
  GtkWidget *hvbox;
  GtkWidget *label;
  GtkWidget *icon;  

  /* sample settings */
  GtkWidget *settings_dialog;
  gchar *setting1;
  time_t last_run;
  int period_for_rechecking_in_minutes;  
  gint icon_size;  
  gboolean setting3;
} SamplePlugin;

void sample_save(XfcePanelPlugin *plugin, SamplePlugin *sample);

G_END_DECLS

#endif /* !__SAMPLE_H__ */
