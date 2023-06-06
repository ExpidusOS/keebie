#pragma once

#include <gtk/gtk.h>

G_DECLARE_FINAL_TYPE(KeebieApplication, keebie_application, KEEBIE, APPLICATION, GtkApplication);

KeebieApplication* keebie_application_new();