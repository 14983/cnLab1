#ifndef NOTIFY_H
#define NOTIFY_H

#define USE_NOTIFY_SEND

#include <string>
#include <cstdlib>
#include <thread>

#ifdef USE_NOTIFY_SEND
void notify(const std::string &message) {
    std::string command = "notify-send \"" + message + "\"";
    system(command.c_str());
}
#endif

#ifdef USE_GTK
#include <gtk/gtk.h>
void on_response(GtkDialog *dialog, int response_id) {
    gtk_widget_destroy(GTK_WIDGET(dialog));
}
void show_notification(const std::string &message) {
    GtkWidget *dialog;
    gtk_init(0, nullptr);
    dialog = gtk_message_dialog_new(
        nullptr,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s", message.c_str()
    );

    g_signal_connect(dialog, "response", G_CALLBACK(on_response), NULL);
    gtk_widget_show_all(dialog);
    gtk_main();
}
void notify(const std::string &message) {
    std::thread notification_thread(show_notification, message);
    notification_thread.detach();  // Detach the thread to run independently
}
#endif

#endif