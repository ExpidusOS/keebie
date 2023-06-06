#pragma once

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include "application.h"

G_BEGIN_DECLS

G_DECLARE_DERIVABLE_TYPE(KeebieWindow, keebie_window, KEEBIE, WINDOW, GtkApplicationWindow);

struct _KeebieWindowClass {
  GtkApplicationWindowClass parent_class;

  gboolean (*is_keyboard)(KeebieWindow* win);
};

KeebieWindow* keebie_window_new(KeebieApplication* application, gboolean is_keyboard);
gboolean keebie_window_is_keyboard(KeebieWindow* self);

G_END_DECLS