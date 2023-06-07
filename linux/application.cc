#include <xkbcommon/xkbcommon.h>

#include "application.h"
#include "window.h"
#include "utils.h"

struct _KeebieApplication {
  GtkApplication parent_instance;

  char** dart_entrypoint_arguments;
  bool launch_settings;

  struct zwp_virtual_keyboard_manager_v1* virtual_keyboard_manager;
  struct zwp_virtual_keyboard_v1* virtual_keyboard;
  struct wl_seat* seat;

  struct xkb_context* xkb_context;
  struct xkb_keymap* xkb_keymap;
  int keymap_fd;
};

G_DEFINE_TYPE(KeebieApplication, keebie_application, GTK_TYPE_APPLICATION);

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

static void keebie_application_activate(GApplication* application) {
  KeebieApplication* self = KEEBIE_APPLICATION(application);

  GdkDisplay* gdisp = gdk_display_get_default();
  g_assert(gdisp != nullptr);

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

  if (GDK_IS_WAYLAND_DISPLAY(gdisp)) {
    struct wl_display* disp = gdk_wayland_display_get_wl_display(gdisp);
    struct wl_registry* registry = wl_display_get_registry(disp);

    self->virtual_keyboard_manager = nullptr;
    wl_registry_add_listener(registry, &registry_listener, reinterpret_cast<void*>(self));
    wl_display_roundtrip(disp);
  }

  KeebieWindow* win = keebie_window_new(self, !self->launch_settings);
  gtk_application_add_window(GTK_APPLICATION(self), GTK_WINDOW(win));
  gtk_widget_show_all(GTK_WIDGET(win));
}

static gboolean keebie_application_local_command_line(GApplication* application, gchar*** arguments, int* exit_status) {
  KeebieApplication* self = KEEBIE_APPLICATION(application);
  size_t dart_i = 0;
  for (size_t i = 0; (*arguments)[i] != nullptr; i++) {
    gchar* arg = (*arguments)[i];
    if (g_strcmp0(arg, "--settings") == 0 || g_strcmp0(arg, "--keyboard") == 0) {
      self->launch_settings = g_strcmp0(arg, "--settings") == 0;
    } else {
      if (self->dart_entrypoint_arguments == nullptr) {
        self->dart_entrypoint_arguments = reinterpret_cast<char**>(g_malloc0(sizeof (char*)));
      } else {
        self->dart_entrypoint_arguments = reinterpret_cast<char**>(g_realloc(self->dart_entrypoint_arguments, sizeof (char*) * (dart_i + 1)));
      }

      g_assert(self->dart_entrypoint_arguments != nullptr);
      self->dart_entrypoint_arguments[dart_i++] = g_strdup(arg);
    }
  }

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

FlDartProject* keebie_application_get_dart_project(KeebieApplication* self) {
  FlDartProject* project = fl_dart_project_new();
  fl_dart_project_set_dart_entrypoint_arguments(project, self->dart_entrypoint_arguments);
  return project;
}

struct zwp_virtual_keyboard_manager_v1* keebie_application_get_wayland_virtual_keyboard_manager(KeebieApplication* self) {
  return self->virtual_keyboard_manager;
}

struct wl_seat* keebie_application_get_wayland_seat(KeebieApplication* self) {
  return self->seat;
}

struct xkb_keymap* keebie_application_get_keymap(KeebieApplication* self) {
  return self->xkb_keymap;
}