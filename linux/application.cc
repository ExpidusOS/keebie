#include <sys/mman.h>

#include "application.h"
#include "window.h"
#include "utils.h"

struct _KeebieApplication {
  GtkApplication parent_instance;

  KeebieWindow* keyboard_window;

  char** dart_entrypoint_arguments;
  bool launch_settings;

  struct wl_seat* seat;
  struct zwp_input_method_manager_v2* input_method_manager;
  struct zwp_input_method_v2* input_method;

  struct zwp_virtual_keyboard_manager_v1* virtual_keyboard_manager;
  struct zwp_virtual_keyboard_v1* virtual_keyboard;

  struct xkb_context* xkb_context;
  struct xkb_keymap* xkb_keymap;
  uint32_t keymap_fmt;
  int32_t keymap_fd;
  uint32_t keymap_size;

  uint32_t im_serial;
};

G_DEFINE_TYPE(KeebieApplication, keebie_application, GTK_TYPE_APPLICATION);

static void keebie_application_im_activate(void* data, struct zwp_input_method_v2* zwp_input_method_v2) {
  KeebieApplication* self = KEEBIE_APPLICATION(data);
  gtk_widget_show_all(GTK_WIDGET(self->keyboard_window));
}

static void keebie_application_im_deactivate(void* data, struct zwp_input_method_v2* zwp_input_method_v2) {
  KeebieApplication* self = KEEBIE_APPLICATION(data);
  gtk_widget_hide(GTK_WIDGET(self->keyboard_window));
}

static void keebie_application_im_surrounding_text(void* data, struct zwp_input_method_v2* zwp_input_method_v2, const char* text, uint32_t cursor, uint32_t anchor) {}

static void keebie_application_im_text_change_cause(void* data, struct zwp_input_method_v2* zwp_input_method_v2, uint32_t cause) {}

static void keebie_application_im_content_type(void* data, struct zwp_input_method_v2* zwp_input_method_v2, uint32_t hint, uint32_t purpose) {}

static void keebie_application_im_done(void* data, struct zwp_input_method_v2* zwp_input_method_v2) {}

static void keebie_application_im_unavailable(void* data, struct zwp_input_method_v2* zwp_input_method_v2) {
  g_error("IM is not available");
}

static const struct zwp_input_method_v2_listener keebie_application_im_listener = {
  .activate = keebie_application_im_activate,
  .deactivate = keebie_application_im_deactivate,
  .surrounding_text = keebie_application_im_surrounding_text,
  .text_change_cause = keebie_application_im_text_change_cause,
  .content_type = keebie_application_im_content_type,
  .done = keebie_application_im_done,
  .unavailable = keebie_application_im_unavailable,
};

static void keebie_application_kb_keymap(void* data, struct wl_keyboard* wl_keyboard, uint32_t fmt, int32_t fd, uint32_t size) {
  KeebieApplication* self = KEEBIE_APPLICATION(data);

  if (self->keymap_fd > 0) {
    close(self->keymap_fd);
  }

  self->keymap_fmt = fmt;
  self->keymap_fd = fd;
  self->keymap_size = size;

  keebie_application_keymap(self);
}

static void keebie_application_kb_enter(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surf, struct wl_array* keys) {
}

static void keebie_application_kb_leave(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surf) {
}

static void keebie_application_kb_key(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
}

static void keebie_application_kb_modifiers(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
}

static void keebie_application_kb_repeat_info(void* data, struct wl_keyboard* wl_keyboard, int32_t rate, int32_t delay) {}

static const struct wl_keyboard_listener keebie_application_kb_listener = {
  .keymap = keebie_application_kb_keymap,
  .enter = keebie_application_kb_enter,
  .leave = keebie_application_kb_leave,
  .key = keebie_application_kb_key,
  .modifiers = keebie_application_kb_modifiers,
  .repeat_info = keebie_application_kb_repeat_info,
};

static void wayland_register_global(void* data, struct wl_registry* registry, uint32_t name, const char* iface, uint32_t version) {
  KeebieApplication* self = KEEBIE_APPLICATION(data);

  if (g_strcmp0(iface, wl_seat_interface.name) == 0) {
    self->seat = reinterpret_cast<struct wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, version));
    g_assert(self->seat != nullptr);
  } else if (g_strcmp0(iface, zwp_input_method_manager_v2_interface.name) == 0) {
    self->input_method_manager = reinterpret_cast<struct zwp_input_method_manager_v2*>(wl_registry_bind(registry, name, &zwp_input_method_manager_v2_interface, version));
    g_assert(self->input_method_manager != nullptr);
  } else if (g_strcmp0(iface, zwp_virtual_keyboard_manager_v1_interface.name) == 0) {
    self->virtual_keyboard_manager = reinterpret_cast<struct zwp_virtual_keyboard_manager_v1*>(wl_registry_bind(registry, name, &zwp_virtual_keyboard_manager_v1_interface, version));
    g_assert(self->virtual_keyboard_manager != nullptr);
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

  if (GDK_IS_WAYLAND_DISPLAY(gdisp)) {
    struct wl_display* disp = gdk_wayland_display_get_wl_display(gdisp);
    struct wl_registry* registry = wl_display_get_registry(disp);

    wl_registry_add_listener(registry, &registry_listener, reinterpret_cast<void*>(self));
    wl_display_roundtrip(disp);

    if (!self->launch_settings) {
      struct wl_keyboard* keyboard = wl_seat_get_keyboard(self->seat);

      if (self->input_method_manager != nullptr) {
        self->input_method = zwp_input_method_manager_v2_get_input_method(self->input_method_manager, self->seat);
        zwp_input_method_v2_add_listener(self->input_method, &keebie_application_im_listener, self);
      }

      if (self->virtual_keyboard_manager != nullptr) {
        self->virtual_keyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(self->virtual_keyboard_manager, self->seat);
      }

      if (keyboard != nullptr) {
        wl_keyboard_add_listener(keyboard, &keebie_application_kb_listener, self);
      } else {
        struct xkb_rule_names names;
        names.rules = nullptr;
        names.model = nullptr;
        names.layout = nullptr;
        names.variant = nullptr;
        names.options = nullptr;

        self->xkb_keymap = xkb_keymap_new_from_names(self->xkb_context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
        g_assert(self->xkb_keymap != nullptr);

        self->keymap_fmt = XKB_KEYMAP_FORMAT_TEXT_V1;

        char* keymap_str = xkb_keymap_get_as_string(self->xkb_keymap, (enum xkb_keymap_format)self->keymap_fmt);
        self->keymap_size = strlen(keymap_str) + 1;

        int ro_fd = -1;
        g_assert(allocate_shm_file_pair(self->keymap_size, &self->keymap_fd, &ro_fd));

        void* dst = mmap(NULL, self->keymap_size, PROT_READ | PROT_WRITE, MAP_SHARED, self->keymap_fd, 0);
        g_assert(dst != MAP_FAILED);

        memcpy(dst, keymap_str, self->keymap_size);
        munmap(dst, self->keymap_size);

        keebie_application_keymap(self);
      }
    }
  }

  if (self->launch_settings) {
    KeebieWindow* win = keebie_window_new(self, FALSE);
    gtk_application_add_window(GTK_APPLICATION(self), GTK_WINDOW(win));
    gtk_widget_show_all(GTK_WIDGET(win));
  } else {
    self->keyboard_window = keebie_window_new(self, TRUE);
    gtk_application_add_window(GTK_APPLICATION(self), GTK_WINDOW(self->keyboard_window));
  }
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
  g_clear_pointer(&self->input_method_manager, zwp_input_method_manager_v2_destroy);
  g_clear_pointer(&self->input_method, zwp_input_method_v2_destroy);
  g_clear_pointer(&self->virtual_keyboard_manager, zwp_virtual_keyboard_manager_v1_destroy);
  g_clear_pointer(&self->virtual_keyboard, zwp_virtual_keyboard_v1_destroy);
  g_clear_pointer(&self->xkb_context, xkb_context_unref);
  g_clear_pointer(&self->xkb_keymap, xkb_keymap_unref);
  g_clear_object(&self->keyboard_window);

  if (self->keymap_fd > 0) {
    close(self->keymap_fd);
    self->keymap_fd = 0;
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

struct wl_seat* keebie_application_get_wayland_seat(KeebieApplication* self) {
  return self->seat;
}

struct zwp_input_method_manager_v2* keebie_application_get_input_method_manager(KeebieApplication* self) {
  return self->input_method_manager;
}

struct zwp_input_method_v2* keebie_application_get_input_method(KeebieApplication* self) {
  return self->input_method;
}

struct zwp_virtual_keyboard_manager_v1* keebie_application_get_virtual_keyboard_manager(KeebieApplication* self) {
  return self->virtual_keyboard_manager;
}

gboolean keebie_application_commit_text(KeebieApplication* self, const char* text) {
  if (self->input_method != nullptr) {
    zwp_input_method_v2_commit_string(self->input_method, text);
    zwp_input_method_v2_commit(self->input_method, self->im_serial++);
    return TRUE;
  }
  return FALSE;
}

gboolean keebie_application_send_key(KeebieApplication* self, uint32_t key) {
  if (self->virtual_keyboard != nullptr) {
    long time = get_time_ms();

    zwp_virtual_keyboard_v1_key(self->virtual_keyboard, time, key, WL_KEYBOARD_KEY_STATE_PRESSED);
    zwp_virtual_keyboard_v1_key(self->virtual_keyboard, time, key, WL_KEYBOARD_KEY_STATE_RELEASED);
    return TRUE;
  }
  return FALSE;
}

gboolean keebie_application_delete_surrounding(KeebieApplication* self, uint32_t before, uint32_t after) {
  if (self->virtual_keyboard != nullptr) {
    while ((before--) > 0) {
      keebie_application_send_key(self, KEY_BACKSPACE);
    }

    while ((after--) > 0) {
      keebie_application_send_key(self, KEY_INSERT);
    }
    return TRUE;
  }

  if (self->input_method != nullptr) {
    zwp_input_method_v2_delete_surrounding_text(self->input_method, before, after);
    zwp_input_method_v2_commit_string(self->input_method, "");
    zwp_input_method_v2_commit(self->input_method, self->im_serial++);
    return TRUE;
  }
  return FALSE;
}

void keebie_application_keymap(KeebieApplication* self) {
  if (self->virtual_keyboard != nullptr) {
    zwp_virtual_keyboard_v1_keymap(
      self->virtual_keyboard,
      self->keymap_fmt,
      self->keymap_fd,
      self->keymap_size
    );
  }
}