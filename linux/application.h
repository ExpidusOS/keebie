#pragma once

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <xkbcommon/xkbcommon.h>

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#include <wayland-client.h>
#endif

#include "virtual-keyboard-unstable-v1-client.h"

G_DECLARE_FINAL_TYPE(KeebieApplication, keebie_application, KEEBIE, APPLICATION, GtkApplication);

KeebieApplication* keebie_application_new();
FlDartProject* keebie_application_get_dart_project(KeebieApplication* self);

struct zwp_virtual_keyboard_manager_v1* keebie_application_get_wayland_virtual_keyboard_manager(KeebieApplication* self);
struct wl_seat* keebie_application_get_wayland_seat(KeebieApplication* self);
struct xkb_keymap* keebie_application_get_keymap(KeebieApplication* self);