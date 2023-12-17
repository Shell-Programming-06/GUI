
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 9000
#define BUFFER_SIZE 1024

GtkWidget *text_view;
GtkTextBuffer *buffer;
GtkWidget *entry;

int client_socket;
pthread_t receive_thread;

void append_text(const gchar *text) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, text, -1);
}

void receive_message() {
    char buffer[BUFFER_SIZE];
    while (1) {
        int valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread <= 0) {
            break;
        }
        buffer[valread] = '\0';
        append_text("Server: ");
        append_text(buffer);
        append_text("\n");
    }
    return NULL;
}

void send_message(GtkWidget *button, gpointer user_data) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    send(client_socket, text, strlen(text), 0);

    append_text("You: ");
    append_text(text);
    append_text("\n");

    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

void setup_gui(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *send_button;
    GtkWidget *scrolled_window;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    send_button = gtk_button_new_with_label("Send");
    gtk_box_pack_start(GTK_BOX(vbox), send_button, FALSE, FALSE, 0);

    g_signal_connect(send_button, "clicked", G_CALLBACK(send_message), NULL);

    gtk_widget_show_all(window);

    g_thread_new("receiver", (GThreadFunc)receive_message, NULL);

    pthread_create(&receive_thread, NULL, receive_message, NULL);
    
    gtk_main();
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    int valread = read(client_socket, buffer, BUFFER_SIZE);
    buffer[valread] = '\0';
    printf("%s", buffer);

    setup_gui(argc, argv);

    pthread_join(receive_thread, NULL);

    close(client_socket);

    return 0;
}

