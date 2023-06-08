#pragma once

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <linux/input-event-codes.h>
#include <xkbcommon/xkbcommon.h>

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#include <wayland-client.h>
#endif

#include "input-method-unstable-v2-client.h"
#include "virtual-keyboard-unstable-v1-client.h"

G_DECLARE_FINAL_TYPE(KeebieApplication, keebie_application, KEEBIE, APPLICATION, GtkApplication);

KeebieApplication* keebie_application_new();
FlDartProject* keebie_application_get_dart_project(KeebieApplication* self);

struct xkb_context* keebie_application_get_xkb_context(KeebieApplication* self);

struct wl_seat* keebie_application_get_wayland_seat(KeebieApplication* self);
struct zwp_input_method_manager_v2* keebie_application_get_input_method_manager(KeebieApplication* self);
struct zwp_input_method_v2* keebie_application_get_input_method(KeebieApplication* self);
struct zwp_virtual_keyboard_manager_v1* keebie_application_get_virtual_keyboard_manager(KeebieApplication* self);

gboolean keebie_application_commit_text(KeebieApplication* self, const char* text);
gboolean keebie_application_send_key(KeebieApplication* self, uint32_t key);
gboolean keebie_application_delete_surrounding(KeebieApplication* self, uint32_t before, uint32_t after);
void keebie_application_keymap(KeebieApplication* self);