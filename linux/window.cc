#include <bitsdojo_window_linux/bitsdojo_window_plugin.h>
#include <math.h>

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#include <gtk-layer-shell.h>
#endif

#include "flutter/generated_plugin_registrant.h"
#include "window.h"
#include "utils.h"

typedef struct _KeebieWindowPrivate {
  FlView* view;

  FlMethodChannel* method_channel;
  gboolean is_keyboard;
} KeebieWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(KeebieWindow, keebie_window, GTK_TYPE_APPLICATION_WINDOW);

enum {
  PROP_0,
  PROP_IS_KEYBOARD,
  N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { nullptr };

static void keebie_window_method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call, gpointer user_data) {
  KeebieWindow* self = KEEBIE_WINDOW(user_data);

  FlMethodResponse* response = nullptr;
  const gchar* method_name = fl_method_call_get_name(method_call);

  if (g_strcmp0(method_name, "sendKey") == 0 && keebie_window_is_keyboard(self)) {
    FlValue* args = fl_method_call_get_args(method_call);
    const gchar* arg_type = fl_value_get_string(fl_value_lookup(args, fl_value_new_string("type")));
    const gchar* arg_name = fl_value_get_string(fl_value_lookup(args, fl_value_new_string("name")));
    const gchar* arg_shifted_name = fl_value_get_string(fl_value_lookup(args, fl_value_new_string("shiftedName")));
    gboolean arg_is_shifted = fl_value_get_bool(fl_value_lookup(args, fl_value_new_string("isShifted")));

    KeebieApplication* app = KEEBIE_APPLICATION(gtk_window_get_application(GTK_WINDOW(self)));
    g_assert(app != nullptr);

    if (g_strcmp0(arg_type, "backspace") == 0) {
      keebie_application_delete_surrounding(app, 1, 0);
      response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_null()));
    } else if (g_strcmp0(arg_type, "enter") == 0) {
      keebie_application_send_key(app, KEY_ENTER);
      response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_null()));
    } else if (g_strcmp0(arg_type, "changeLang") == 0) {
    } else if (g_strcmp0(arg_type, "space") == 0) {
      keebie_application_commit_text(app, " ");
      response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_null()));
    } else if (g_strcmp0(arg_type, "regular") == 0) {
      keebie_application_commit_text(app, arg_is_shifted && strlen(arg_shifted_name) > 0 ? arg_shifted_name : arg_name);
      response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_null()));
    }

    if (response == nullptr) {
      response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
    }
  } else if (g_strcmp0(method_name, "isKeyboard") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_bool(keebie_window_is_keyboard(self))));
  } else if (g_strcmp0(method_name, "getMonitorGeometry") == 0) {
    GdkScreen* screen = gtk_widget_get_screen(GTK_WIDGET(self));
    g_assert(screen != nullptr);

    GdkWindow* win = gtk_widget_get_window(GTK_WIDGET(self));
    g_assert(win != nullptr);

    GdkDisplay* display = gdk_screen_get_display(screen);
    g_assert(display != nullptr);

    GdkMonitor* monitor = gdk_display_get_monitor_at_window(display, win);
    g_assert(monitor != nullptr);

    GdkRectangle geom;
    gdk_monitor_get_geometry(monitor, &geom);

    FlValue* value = fl_value_new_map();
    fl_value_set_string_take(value, "width", fl_value_new_int(geom.width));
    fl_value_set_string_take(value, "height", fl_value_new_int(geom.height));
    fl_value_set_string_take(value, "x", fl_value_new_int(geom.x));
    fl_value_set_string_take(value, "y", fl_value_new_int(geom.y));
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(value));
  } else if (g_strcmp0(method_name, "setExclusive") == 0) {
    gboolean is_keyboard = keebie_window_is_keyboard(self);
    bool enabled = fl_value_get_bool(fl_method_call_get_args(method_call));

    GdkWindow* win = gtk_widget_get_window(GTK_WIDGET(self));
    if (GDK_IS_WAYLAND_WINDOW(win) && is_keyboard) {
      bool is_enabled = gtk_layer_get_exclusive_zone(GTK_WINDOW(self)) > 0;

      if (enabled && !is_enabled) {
        gtk_layer_auto_exclusive_zone_enable(GTK_WINDOW(self));
      } else if (!enabled && is_enabled) {
        gtk_layer_set_exclusive_zone(GTK_WINDOW(self), 0);
      }

      response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_null()));
    } else {
      response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
    }
  } else if (g_strcmp0(method_name, "openWindow") == 0) {
    FlValue* args = fl_method_call_get_args(method_call);
    gboolean arg_keyboard = fl_value_get_bool(fl_value_lookup(args, fl_value_new_string("keyboard")));

    KeebieApplication* app = KEEBIE_APPLICATION(gtk_window_get_application(GTK_WINDOW(self)));
    g_assert(app != nullptr);

    KeebieWindow* new_win = keebie_window_new(app, arg_keyboard);
    gtk_application_add_window(GTK_APPLICATION(self), GTK_WINDOW(new_win));
    gtk_widget_show_all(GTK_WIDGET(new_win));
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_null()));
  } else if (g_strcmp0(method_name, "announceLayout") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_null()));
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  GError* error = NULL;
  if (!fl_method_call_respond(method_call, response, &error))
    g_warning("Failed to send response: %s", error->message);
}

static gboolean keebie_window_draw(GtkWidget* widget, cairo_t* cr) {
  GTK_WIDGET_CLASS(keebie_window_parent_class)->draw(widget, cr);

  cairo_save(cr);
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint(cr);
  cairo_restore(cr);
  return FALSE;
}

static gboolean keebie_window_is_keyboard_impl(KeebieWindow* self) {
  return reinterpret_cast<KeebieWindowPrivate*>(keebie_window_get_instance_private(self))->is_keyboard;
}

gboolean keebie_window_configure_event(GtkWidget* widget, GdkEventConfigure* event) {
  gboolean result = GTK_WIDGET_CLASS(keebie_window_parent_class)->configure_event(widget, event);

  KeebieWindow* self = KEEBIE_WINDOW(widget);
  KeebieApplication* app = KEEBIE_APPLICATION(gtk_window_get_application(GTK_WINDOW(self)));
  g_assert(app != nullptr);

  GdkScreen* screen = gtk_widget_get_screen(GTK_WIDGET(self));
  g_assert(screen != nullptr);

  GdkWindow* win = gtk_widget_get_window(widget);
  gboolean is_keyboard = keebie_window_is_keyboard(self);

  GdkDisplay* display = gdk_screen_get_display(screen);
  g_assert(display != nullptr);

  GdkMonitor* monitor = gdk_display_get_monitor_at_window(display, win);
  g_assert(monitor != nullptr);

  GdkRectangle geom;
  gdk_monitor_get_geometry(monitor, &geom);

  gint width;
  gint height;
  gtk_window_get_size(GTK_WINDOW(self), &width, &height);

  int horiz = abs((geom.width - width) / 2);
  if (GDK_IS_WAYLAND_WINDOW(win) && is_keyboard) {
    gtk_layer_set_margin(GTK_WINDOW(self), GTK_LAYER_SHELL_EDGE_LEFT, horiz);
    gtk_layer_set_margin(GTK_WINDOW(self), GTK_LAYER_SHELL_EDGE_RIGHT, horiz);
  }

  return result;
}

static void keebie_window_realize(GtkWidget* widget) {
  GTK_WIDGET_CLASS(keebie_window_parent_class)->realize(widget);

  KeebieWindow* self = KEEBIE_WINDOW(widget);
  KeebieWindowPrivate* priv = reinterpret_cast<KeebieWindowPrivate*>(keebie_window_get_instance_private(self));

  KeebieApplication* app = KEEBIE_APPLICATION(gtk_window_get_application(GTK_WINDOW(self)));
  g_assert(app != nullptr);

  GdkWindow* win = gtk_widget_get_window(widget);
  gboolean is_keyboard = keebie_window_is_keyboard(self);

  if (GDK_IS_WAYLAND_WINDOW(win)) {
    gdk_wayland_window_announce_csd(GDK_WAYLAND_WINDOW(win));
  }

  if (GDK_IS_WAYLAND_WINDOW(win) && is_keyboard) {
    gtk_layer_init_for_window(GTK_WINDOW(self));
    gtk_layer_set_layer(GTK_WINDOW(self), GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_anchor(GTK_WINDOW(self), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(self), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(self), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

    struct wl_seat* seat = keebie_application_get_wayland_seat(app);
    g_assert(seat != nullptr);
  }

  GdkScreen* screen = gtk_widget_get_screen(widget);
  g_assert(screen != nullptr);

  GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
  if (visual != nullptr && gdk_screen_is_composited(screen)) {
    gtk_widget_set_visual(GTK_WIDGET(self), visual);
  }

  FlDartProject* project = keebie_application_get_dart_project(app);
  priv->view = fl_view_new(project);
  gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(priv->view));

  FlBinaryMessenger* messenger = fl_engine_get_binary_messenger(fl_view_get_engine(priv->view));
  priv->method_channel = fl_method_channel_new(messenger, "keebie", FL_METHOD_CODEC(fl_standard_method_codec_new()));
  fl_method_channel_set_method_call_handler(priv->method_channel, keebie_window_method_call_cb, self, nullptr);

  fl_register_plugins(FL_PLUGIN_REGISTRY(priv->view));
}

static void keebie_window_constructed(GObject* obj) {
  G_OBJECT_CLASS(keebie_window_parent_class)->constructed(obj);

  KeebieWindow* self = KEEBIE_WINDOW(obj);

  auto bdw = bitsdojo_window_from(GTK_WINDOW(self));
  bdw->setCustomFrame(true);

  gtk_widget_set_app_paintable(GTK_WIDGET(self), TRUE);
}

static void keebie_window_dispose(GObject* obj) {
  KeebieWindow* self = KEEBIE_WINDOW(obj);
  KeebieWindowPrivate* priv = reinterpret_cast<KeebieWindowPrivate*>(keebie_window_get_instance_private(self));

  g_clear_object(&priv->method_channel);
  g_clear_object(&priv->view);

  G_OBJECT_CLASS(keebie_window_parent_class)->dispose(obj);
}

static void keebie_window_set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec) {
  KeebieWindow* self = KEEBIE_WINDOW(obj);
  KeebieWindowPrivate* priv = reinterpret_cast<KeebieWindowPrivate*>(keebie_window_get_instance_private(self));

  switch (prop_id) {
    case PROP_IS_KEYBOARD:
      priv->is_keyboard = g_value_get_boolean(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
      break;
  }
}

static void keebie_window_get_property(GObject* obj, guint prop_id, GValue* value, GParamSpec* pspec) {
  KeebieWindow* self = KEEBIE_WINDOW(obj);
  KeebieWindowPrivate* priv = reinterpret_cast<KeebieWindowPrivate*>(keebie_window_get_instance_private(self));

  switch (prop_id) {
    case PROP_IS_KEYBOARD:
      g_value_set_boolean(value, priv->is_keyboard);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
      break;
  }
}

static void keebie_window_class_init(KeebieWindowClass* klass) {
  klass->is_keyboard = keebie_window_is_keyboard_impl;

  GObjectClass* obj_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);

  obj_class->constructed = keebie_window_constructed;
  obj_class->dispose = keebie_window_dispose;
  obj_class->set_property = keebie_window_set_property;
  obj_class->get_property = keebie_window_get_property;

  widget_class->draw = keebie_window_draw;
  widget_class->realize = keebie_window_realize;
  widget_class->configure_event = keebie_window_configure_event;

  obj_properties[PROP_IS_KEYBOARD] = g_param_spec_boolean(
    "is-keyboard",
    "Is Keyboard",
    "Defines whether or not this window is the keyboard or settings.",
    TRUE,
    (GParamFlags)(G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
  );
  g_object_class_install_properties(obj_class, N_PROPERTIES, obj_properties);
}

static void keebie_window_init(KeebieWindow* self) {}

KeebieWindow* keebie_window_new(KeebieApplication* application, gboolean is_keyboard) {
  return KEEBIE_WINDOW(g_object_new(keebie_window_get_type(),
    "application", application,
    "is-keyboard", is_keyboard,
    nullptr));
}

gboolean keebie_window_is_keyboard(KeebieWindow* self) {
  g_assert(self != NULL);
  g_assert(KEEBIE_IS_WINDOW(self));

  KeebieWindowClass* klass = KEEBIE_WINDOW_GET_CLASS(self);
  g_assert(klass->is_keyboard != nullptr);
  return klass->is_keyboard(self);
}
