#include <bitsdojo_window_linux/bitsdojo_window_plugin.h>

#include <gtk-layer-shell.h>
#include "virtual-keyboard-unstable-v1-client.h"
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include <sys/mman.h>

#include "application.h"
#include "utils.h"

#include <flutter_linux/flutter_linux.h>
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include "flutter/generated_plugin_registrant.h"

struct _KeebieApplication {
  GtkApplication parent_instance;
  GtkWindow* window;

  FlMethodChannel* method_channel;
  char** dart_entrypoint_arguments;

  struct zwp_virtual_keyboard_manager_v1* virtual_keyboard_manager;
  struct zwp_virtual_keyboard_v1* virtual_keyboard;
  struct wl_seat* seat;

  struct xkb_context* xkb_context;
  struct xkb_keymap* xkb_keymap;
  int keymap_fd;
};

G_DEFINE_TYPE(KeebieApplication, keebie_application, GTK_TYPE_APPLICATION)

static void wayland_register_global(void* data, struct wl_registry* registry, uint32_t name, const char* iface, uint32_t version) {
  KeebieApplication* self = KEEBIE_APPLICATION(data);

  if (g_strcmp0(iface, zwp_virtual_keyboard_manager_v1_interface.name) == 0) {
    self->virtual_keyboard_manager = reinterpret_cast<struct zwp_virtual_keyboard_manager_v1*>(wl_registry_bind(registry, name, &zwp_virtual_keyboard_manager_v1_interface, version));
    g_assert(self->virtual_keyboard_manager != nullptr);
  } else if (g_strcmp0(iface, wl_seat_interface.name) == 0) {
    self->seat = reinterpret_cast<struct wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, version));
    g_assert(self->seat != nullptr);
  }
}

static const struct wl_registry_listener registry_listener = {
  .global = wayland_register_global,
};

static gboolean window_draw(GtkWidget* widget, cairo_t* cr, gpointer data) {
  cairo_save(cr);
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint(cr);
  cairo_restore(cr);
  return FALSE;
}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call, gpointer user_data) {
  KeebieApplication* self = KEEBIE_APPLICATION(user_data);

  FlMethodResponse* response = nullptr;
  const gchar* method_name = fl_method_call_get_name(method_call);

  if (g_strcmp0(method_name, "sendKey") == 0) {
    FlValue* args = fl_method_call_get_args(method_call);
    const gchar* arg_type = fl_value_get_string(fl_value_lookup(args, fl_value_new_string("type")));
    const gchar* arg_name = fl_value_get_string(fl_value_lookup(args, fl_value_new_string("name")));
    const gchar* arg_shifted_name = fl_value_get_string(fl_value_lookup(args, fl_value_new_string("shiftedName")));
    gboolean arg_is_shifted = fl_value_get_bool(fl_value_lookup(args, fl_value_new_string("isShifted")));

    if (self->virtual_keyboard == nullptr) {
      g_warning("Not implemented: %s %s %s %d", arg_type, arg_name, arg_shifted_name, arg_is_shifted);
      response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
    } else {
      uint32_t code = 0;
      if (g_strcmp0(arg_type, "backspace") == 0) {
        code = XKB_KEY_BackSpace;
      } else if (g_strcmp0(arg_type, "regular") == 0) {
        response = FL_METHOD_RESPONSE(fl_method_error_response_new("regularKey", "Key is not a recognized key event type.", args));
      } else {
        response = FL_METHOD_RESPONSE(fl_method_error_response_new("unknownKey", "Key type is not a recognized key event type.", args));
      }

      if (response == nullptr) {
        long t = get_time_ms();
        zwp_virtual_keyboard_v1_key(self->virtual_keyboard, t, code, WL_KEYBOARD_KEY_STATE_PRESSED);
        zwp_virtual_keyboard_v1_key(self->virtual_keyboard, t, code, WL_KEYBOARD_KEY_STATE_RELEASED);
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_null()));
      }
    }
  } else if (g_strcmp0(method_name, "getMonitorGeometry") == 0) {
    GdkScreen* screen = gtk_widget_get_screen(GTK_WIDGET(self->window));
    g_assert(screen != nullptr);

    GdkWindow* win = gtk_widget_get_window(GTK_WIDGET(self->window));
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
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  GError* error = NULL;
  if (!fl_method_call_respond(method_call, response, &error))
    g_warning("Failed to send response: %s", error->message);
}

static void keebie_application_activate(GApplication* application) {
  KeebieApplication* self = KEEBIE_APPLICATION(application);
  GtkWindow* window = GTK_WINDOW(gtk_application_window_new(GTK_APPLICATION(application)));
  self->window = window;

  auto bdw = bitsdojo_window_from(window);
  bdw->setCustomFrame(true);

  gtk_layer_init_for_window(window);
  gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_TOP);
  gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
  gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
  gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

  gtk_widget_set_app_paintable(GTK_WIDGET(window), TRUE);
  gtk_widget_show(GTK_WIDGET(window));

  GdkDisplay* gdisp = gtk_widget_get_display(GTK_WIDGET(window));
  g_assert(gdisp != nullptr);

  GdkWindow* win = gtk_widget_get_window(GTK_WIDGET(window));
  g_assert(win != nullptr);

  self->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  g_assert(self->xkb_context != nullptr);

  struct xkb_rule_names names;
  names.rules = nullptr;
  names.model = nullptr;
  names.layout = nullptr;
  names.variant = nullptr;
  names.options = nullptr;
  self->xkb_keymap = xkb_keymap_new_from_names(self->xkb_context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
  g_assert(self->xkb_keymap != nullptr);

  if (GDK_IS_WAYLAND_WINDOW(win) && GDK_IS_WAYLAND_DISPLAY(gdisp)) {
    gdk_wayland_window_announce_csd(GDK_WAYLAND_WINDOW(win));

    struct wl_display* disp = gdk_wayland_display_get_wl_display(gdisp);
    struct wl_registry* registry = wl_display_get_registry(disp);
    struct wl_surface* surf = gdk_wayland_window_get_wl_surface(GDK_WAYLAND_WINDOW(win));
    (void)surf;

    self->virtual_keyboard_manager = nullptr;
    wl_registry_add_listener(registry, &registry_listener, reinterpret_cast<void*>(self));
    wl_display_roundtrip(disp);

    if (self->virtual_keyboard_manager != nullptr) {
      self->virtual_keyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(self->virtual_keyboard_manager, self->seat);
      g_assert(self->virtual_keyboard != nullptr);

      char* keymap_str = xkb_keymap_get_as_string(self->xkb_keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
      g_assert(keymap_str != nullptr);

      size_t keymap_size = strlen(keymap_str) + 1;

      int rw_fd = -1, ro_fd = -1;
      g_assert(allocate_shm_file_pair(keymap_size, &rw_fd, &ro_fd));

      void* dst = mmap(NULL, keymap_size, PROT_READ | PROT_WRITE, MAP_SHARED, rw_fd, 0);
      close(rw_fd);
      g_assert(dst != MAP_FAILED);

      memcpy(dst, keymap_str, keymap_size);
    	munmap(dst, keymap_size);

    	self->keymap_fd = ro_fd;
    	zwp_virtual_keyboard_v1_keymap(self->virtual_keyboard, XKB_KEYMAP_FORMAT_TEXT_V1, self->keymap_fd, keymap_size);
    }
  }

  GdkScreen* screen = gtk_widget_get_screen(GTK_WIDGET(window));
  g_assert(screen != nullptr);

  GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
  if (visual != nullptr && gdk_screen_is_composited(screen)) {
    gtk_widget_set_visual(GTK_WIDGET(window), visual);
  }

  g_signal_connect(G_OBJECT(window), "draw", G_CALLBACK(window_draw), NULL);

  g_autoptr(FlDartProject) project = fl_dart_project_new();
  fl_dart_project_set_dart_entrypoint_arguments(project, self->dart_entrypoint_arguments);

  FlView* view = fl_view_new(project);

  FlBinaryMessenger* messenger = fl_engine_get_binary_messenger(fl_view_get_engine(view));
  self->method_channel = fl_method_channel_new(messenger, "keebie", FL_METHOD_CODEC(fl_standard_method_codec_new()));
  fl_method_channel_set_method_call_handler(self->method_channel, method_call_cb, self, nullptr);

  gtk_widget_show(GTK_WIDGET(view));
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(view));

  fl_register_plugins(FL_PLUGIN_REGISTRY(view));
  gtk_widget_grab_focus(GTK_WIDGET(view));
}

static gboolean keebie_application_local_command_line(GApplication* application, gchar*** arguments, int* exit_status) {
  KeebieApplication* self = KEEBIE_APPLICATION(application);
  self->dart_entrypoint_arguments = g_strdupv(*arguments + 1);

  g_autoptr(GError) error = nullptr;
  if (!g_application_register(application, nullptr, &error)) {
     g_warning("Failed to register: %s", error->message);
     *exit_status = 1;
     return TRUE;
  }

  g_application_activate(application);
  *exit_status = 0;
  return TRUE;
}

static void keebie_application_dispose(GObject* object) {
  KeebieApplication* self = KEEBIE_APPLICATION(object);
  g_clear_pointer(&self->dart_entrypoint_arguments, g_strfreev);

  if (self->xkb_keymap != nullptr) {
    xkb_keymap_unref(self->xkb_keymap);
  }

  if (self->xkb_context != nullptr) {
    xkb_context_unref(self->xkb_context);
  }

  if (self->virtual_keyboard != nullptr) {
    zwp_virtual_keyboard_v1_destroy(self->virtual_keyboard);
  }

  if (self->virtual_keyboard_manager != nullptr) {
    zwp_virtual_keyboard_manager_v1_destroy(self->virtual_keyboard_manager);
  }
  G_OBJECT_CLASS(keebie_application_parent_class)->dispose(object);
}

static void keebie_application_class_init(KeebieApplicationClass* klass) {
  G_APPLICATION_CLASS(klass)->activate = keebie_application_activate;
  G_APPLICATION_CLASS(klass)->local_command_line = keebie_application_local_command_line;
  G_OBJECT_CLASS(klass)->dispose = keebie_application_dispose;
}

static void keebie_application_init(KeebieApplication* self) {}

KeebieApplication* keebie_application_new() {
  return KEEBIE_APPLICATION(g_object_new(keebie_application_get_type(),
    "application-id", APPLICATION_ID,
    "flags", G_APPLICATION_NON_UNIQUE,
    nullptr));
}
