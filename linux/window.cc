#include <bitsdojo_window_linux/bitsdojo_window_plugin.h>
#include <linux/input-event-codes.h>
#include <sys/mman.h>
#include <xkbcommon/xkbcommon.h>

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

  struct zwp_input_method_v2* input_method;
  struct zwp_virtual_keyboard_v1* virtual_keyboard;

  struct xkb_keymap* kmap;
  int kmap_fd;
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
  KeebieWindowPrivate* priv = reinterpret_cast<KeebieWindowPrivate*>(keebie_window_get_instance_private(self));

  FlMethodResponse* response = nullptr;
  const gchar* method_name = fl_method_call_get_name(method_call);

  if (g_strcmp0(method_name, "sendKey") == 0 && keebie_window_is_keyboard(self)) {
    FlValue* args = fl_method_call_get_args(method_call);
    const gchar* arg_type = fl_value_get_string(fl_value_lookup(args, fl_value_new_string("type")));
    const gchar* arg_name = fl_value_get_string(fl_value_lookup(args, fl_value_new_string("name")));
    const gchar* arg_shifted_name = fl_value_get_string(fl_value_lookup(args, fl_value_new_string("shiftedName")));
    gboolean arg_is_shifted = fl_value_get_bool(fl_value_lookup(args, fl_value_new_string("isShifted")));
    int64_t arg_row_no = fl_value_get_int(fl_value_lookup(args, fl_value_new_string("rowNo")));
    int64_t arg_key_no = fl_value_get_int(fl_value_lookup(args, fl_value_new_string("keyNo")));

    (void)arg_name;
    (void)arg_shifted_name;
    (void)arg_is_shifted;

    if (priv->virtual_keyboard != nullptr) {
      xkb_keycode_t key = 0;
      if (g_strcmp0(arg_type, "regular") == 0) {
        char code = (char)('E' - arg_row_no);
        gchar* id = g_strdup_printf("A%c%02lu", code, arg_key_no + 1) - 8;
        key = xkb_keymap_key_by_name(priv->kmap, id);
        g_free(id);
      } else if (g_strcmp0(arg_type, "space") == 0) {
        key = KEY_SPACE;
      } else if (g_strcmp0(arg_type, "enter") == 0) {
        key = KEY_ENTER;
      } else if (g_strcmp0(arg_type, "backspace") == 0) {
        key = KEY_BACKSPACE;
      } else {
        response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
      }

      if (key > 0) {
        long t = get_time_ms();
        if (arg_is_shifted) zwp_virtual_keyboard_v1_key(priv->virtual_keyboard, t, KEY_LEFTSHIFT, WL_KEYBOARD_KEY_STATE_PRESSED);

        zwp_virtual_keyboard_v1_key(priv->virtual_keyboard, t, key, WL_KEYBOARD_KEY_STATE_PRESSED);
        zwp_virtual_keyboard_v1_key(priv->virtual_keyboard, t, key, WL_KEYBOARD_KEY_STATE_RELEASED);

        if (arg_is_shifted) zwp_virtual_keyboard_v1_key(priv->virtual_keyboard, t, KEY_LEFTSHIFT, WL_KEYBOARD_KEY_STATE_RELEASED);

        response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_null()));
      }
    } else {
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
    FlValue* args = fl_method_call_get_args(method_call);
    const gchar* arg_locale = fl_value_get_string(fl_value_lookup(args, fl_value_new_string("locale")));
    FlValue* arg_planes = fl_value_lookup(args, fl_value_new_string("planes"));

    GString* kmap = g_string_new(nullptr);
    g_string_append_printf(kmap, "xkb_keymap {\n\txkb_keycodes \"keebie-%s\" {\n\t\tinclude \"evdev\"\n\t};\n\txkb_symbols \"keebie-%s\" {\n", arg_locale, arg_locale);

    size_t i = 0;
    for (size_t plane_i = 0; plane_i < fl_value_get_length(arg_planes); plane_i++) {
      FlValue* rows = fl_value_get_list_value(arg_planes, plane_i);
      for (size_t row_i = 0; row_i < fl_value_get_length(rows); row_i++) {
        FlValue* keys = fl_value_get_list_value(rows, row_i);
        for (size_t key_i = 0; key_i < fl_value_get_length(keys); key_i++) {
          FlValue* key = fl_value_get_list_value(keys, key_i);

          const gchar* key_type = fl_value_get_string(fl_value_lookup(key, fl_value_new_string("type")));
          const gchar* key_name = fl_value_get_string(fl_value_lookup(key, fl_value_new_string("name")));
          const gchar* key_shifted_name = fl_value_get_string(fl_value_lookup(key, fl_value_new_string("shiftedName")));

          gboolean needs_comma = FALSE;
          gboolean did_print = FALSE;

          char code = (char)('E' - row_i);

          if (g_strcmp0(key_type, "regular") == 0) {
            if (strlen(key_name) > 0) {
              needs_comma = TRUE;
              did_print = TRUE;

              wchar_t c;
              mbstowcs(&c, key_name, sizeof (wchar_t));

              g_string_append_printf(kmap, "\t\tkey <A%d%02lu> { [ U%04X", code, key_i, c);
            }

            if (strlen(key_shifted_name) > 0) {
              did_print = TRUE;

              wchar_t c;
              mbstowcs(&c, key_shifted_name, sizeof (wchar_t));

              if (needs_comma) kmap = g_string_append(kmap, ", ");
              else g_string_append_printf(kmap, "\t\tkey <A%d%02lu> { [ ", code, key_i);

              g_string_append_printf(kmap, "U%04X", c);
            }
          } else if (g_strcmp0(key_type, "space") == 0) {
            g_string_append_printf(kmap, "\t\tkey <A%d%02lu> { [ space", code, key_i);
            did_print = TRUE;
          } else if (g_strcmp0(key_type, "backspace") == 0) {
            g_string_append_printf(kmap, "\t\tkey <A%d%02lu> { [ BackSpace", code, key_i);
            did_print = TRUE;
          }

          if (did_print) {
            kmap = g_string_append(kmap, " ] };\n");
            i++;
          }
        }
      }
    }

    g_string_append_printf(kmap, "\t};\n\txkb_types \"keebie-%s\" {\n\t\tvirtual_modifiers Keebie;\n\t\t", arg_locale);
    kmap = g_string_append(kmap, "type \"ONE_LEVEL\" { ");
    kmap = g_string_append(kmap, "level_name[Level1]= \"Any\";");
    kmap = g_string_append(kmap, "};");

    kmap = g_string_append(kmap, "type \"TWO_LEVEL\" { ");
    kmap = g_string_append(kmap, "level_name[Level1]= \"Base\";");
    kmap = g_string_append(kmap, "};");

    kmap = g_string_append(kmap, "type \"ALPHABETIC\" { ");
    kmap = g_string_append(kmap, "level_name[Level1]= \"Base\";");
    kmap = g_string_append(kmap, "};");

    kmap = g_string_append(kmap, "type \"KEYPAD\" { ");
    kmap = g_string_append(kmap, "level_name[Level1]= \"Base\";");
    kmap = g_string_append(kmap, "};");

    kmap = g_string_append(kmap, "type \"SHIFT + ALT\" { ");
    kmap = g_string_append(kmap, "level_name[Level1]= \"Base\";");

    g_string_append_printf(kmap, "}; }; xkb_compatibility \"keebie-%s\" { ", arg_locale);
    kmap = g_string_append(kmap, "interpret Any+AnyOf(all) { ");
    kmap = g_string_append(kmap, "action= SetMods(modifiers=modMapMods,clearLocks); ");
    kmap = g_string_append(kmap, "}; }; };");

    gchar* kmap_str = g_string_free_and_steal(kmap);
    size_t kmap_size = strlen(kmap_str) + 1;

    int rw_fd = -1, ro_fd = -1;
    if (!allocate_shm_file_pair(kmap_size, &rw_fd, &ro_fd)) {
      response = FL_METHOD_RESPONSE(fl_method_error_response_new("utils", "Failed to allocate SHM", args));
    } else {
      void* dst = mmap(NULL, kmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, rw_fd, 0);
      if (dst == MAP_FAILED) {
        close(ro_fd);
        response = FL_METHOD_RESPONSE(fl_method_error_response_new("utils", "Failed to map memory", args));
      } else {
        memcpy(dst, kmap_str, kmap_size);
      	munmap(dst, kmap_size);

      	if (priv->virtual_keyboard != nullptr) {
      	  zwp_virtual_keyboard_v1_keymap(
      	    priv->virtual_keyboard,
      	    XKB_KEYMAP_FORMAT_TEXT_V1,
      	    ro_fd,
      	    kmap_size
    	    );
      	}

      	if (priv->kmap_fd > 0) {
      	  close(priv->kmap_fd);
      	}

    	  priv->kmap_fd = ro_fd;

      	if (priv->kmap != nullptr) {
      	  xkb_keymap_unref(priv->kmap);
      	}

        KeebieApplication* app = KEEBIE_APPLICATION(gtk_window_get_application(GTK_WINDOW(self)));
      	struct xkb_context* context = keebie_application_get_xkb_context(app);
      	priv->kmap = xkb_keymap_new_from_string(context, kmap_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
      	g_assert(priv->kmap != nullptr);
      }
    }

    free(kmap_str);
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

    struct zwp_input_method_manager_v2* input_method_manager = keebie_application_get_input_method_manager(app);
    if (input_method_manager != nullptr) {
      priv->input_method = zwp_input_method_manager_v2_get_input_method(input_method_manager, seat);
    }

    struct zwp_virtual_keyboard_manager_v1* virtual_keyboard_manager = keebie_application_get_virtual_keyboard_manager(app);
    if (virtual_keyboard_manager != nullptr) {
      priv->virtual_keyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(virtual_keyboard_manager, seat);
    }
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
  g_clear_pointer(&priv->input_method, zwp_input_method_v2_destroy);
  g_clear_pointer(&priv->virtual_keyboard, zwp_virtual_keyboard_v1_destroy);
  g_clear_pointer(&priv->kmap, xkb_keymap_unref);

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
