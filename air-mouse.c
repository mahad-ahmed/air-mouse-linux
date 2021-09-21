#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <string.h>

#include <pthread.h>

#include <gtk/gtk.h>


#define MOUSEMOVE_PORT 6666
#define KEYINPUT_PORT 6667
#define DISCOVERY_PORT 7777

#define COMPATIBILITY_CODE 8

#define MOUSE_MOVE -9
#define KB_INPUT -10

const char PREFIX[] = { 87, 52, 109, 98, 68 };

int SCREEN_WIDTH = 0;
int SCREEN_HEIGHT = 0;

GtkWidget* discovery_label;
GtkWidget* mouse_label;
GtkWidget* keys_label;

size_t getPostPrefixIndex(char buff[], size_t size) {
    int i = 0;
    while(i < 5 && i < size) {
        if(PREFIX[i] != buff[i]) {
            return size;
        }
        i++;
    }

    return i;
}

int invalidPrefix(char buff[], size_t size) {
	for(int i = 0; i < 5; i++) {
	    if(i == size || PREFIX[i] != buff[i]) {
            return 1;
        }
    }

    return 0;
}

void announce(char ver[]) {
    int announce_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);		//IPPROTO_UDP  (17)  can use 0 to let driver(?) decide
	if (announce_fd == -1) {
		return;
	}
	
	setsockopt(announce_fd, SOL_SOCKET, SO_BROADCAST, "1", 1);
	
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("255.255.255.255");
	addr.sin_port = htons(DISCOVERY_PORT);
	
	sendto(announce_fd, ver, (int) strlen(ver), 0, (const struct sockaddr*) &addr, sizeof(addr));
	
	close(announce_fd);
}


void set_discovery_status(GtkLabel* label, int status) {
    if(status) {
        gtk_label_set_markup(label, "DISCOVERY:   <span foreground='green'>OK</span>");
    }
    else {
        gtk_label_set_markup(label, "DISCOVERY:   <span foreground='red'>ERROR</span>");
    }
}

void set_mouse_status(GtkLabel* label, int status) {
    if(status) {
        gtk_label_set_markup(label, "MOUSE:   <span foreground='green'>OK</span>");
    }
    else {
        gtk_label_set_markup(label, "MOUSE:   <span foreground='red'>ERROR</span>");
    }
}

void set_keys_status(GtkLabel* label, int status) {
    if(status) {
        gtk_label_set_markup(label, "KEYS:   <span foreground='green'>OK</span>");
    }
    else {
        gtk_label_set_markup(label, "KEYS:   <span foreground='red'>ERROR</span>");
    }
}


void* discovery_t() {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(fd < 0) {
        set_discovery_status((GtkLabel*) discovery_label, 0);
        return NULL;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(DISCOVERY_PORT);
    
    if(bind(fd, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
        set_discovery_status((GtkLabel*) discovery_label, 0);
        close(fd);
        return NULL;
    }
	
	size_t size;
	char buff[8];
	memset(buff, 0, 8);
	
	struct sockaddr_in client_addr;
	size_t caddr_size = sizeof(client_addr);
	
	// TODO: Add constant id for server/client
	char ver[2] = { COMPATIBILITY_CODE & 0xff, COMPATIBILITY_CODE >> 8 };
    
    announce(ver);
    
    set_discovery_status((GtkLabel*) discovery_label, 1);
    
    while(1) {
        size = recvfrom(fd, buff, 8, 0, (struct sockaddr*) &client_addr, (socklen_t*) &caddr_size);
        if(size == -1) {
            break;
        }
        
        if(size > 1) {
            uint16_t client_version;
            memcpy(&client_version, &buff, 2);
            // TODO: Compare compatibility
        }
        else {
            sendto(fd, ver, (int) strlen(ver), 0, (const struct sockaddr*) &client_addr, sizeof(client_addr));
        }
        
        memset(&client_addr, 0, caddr_size);
    }
    
    set_discovery_status((GtkLabel*) discovery_label, 0);
    
    close(fd);
}

void* mouse_move_t() {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(fd < 0) {
        //perror("Could not create socket");
        set_mouse_status((GtkLabel*) mouse_label, 0);
        return NULL;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(MOUSEMOVE_PORT);
    
    if(bind(fd, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
        set_mouse_status((GtkLabel*) mouse_label, 0);
        close(fd);
        return NULL;
    }
    
    size_t size;
	
	char buff[13];
	memset(buff, 0, 13);
	
	float x = 0, y = 0;
	
	char cmd[50];
	memset(cmd, 0, 50);
    
    set_mouse_status((GtkLabel*) mouse_label, 1);
    
    while(1) {
        size = recv(fd, buff, 13, 0);
        
        if(size == -1) {
            break;
        }
        
        if(invalidPrefix(buff, size)) {
            continue;
        }
        
        memcpy(&x, buff + 5, 4);
		memcpy(&y, buff + 9, 4);
        sprintf(cmd, "xdotool mousemove_relative -- %f %f", x * SCREEN_WIDTH, y * SCREEN_HEIGHT);
        system(cmd);
    }
    
    set_mouse_status((GtkLabel*) mouse_label, 0);
    close(fd);
}

void* key_input_t() {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(fd < 0) {
        set_keys_status((GtkLabel*) keys_label, 0);
        return NULL;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(KEYINPUT_PORT);
    
    if(bind(fd, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
        set_keys_status((GtkLabel*) keys_label, 0);
        close(fd);
        return NULL;
    }
	
	const char mouse_controls[9][24] = {
	    "xdotool mousedown 1",
	    "xdotool mouseup 1",
	    "xdotool mousedown 3",
	    "xdotool mouseup 3",
	    "xdotool click 5",
	    "xdotool click 4",
	    "xdotool click 1",
	    "xdotool click 3",
	    "xdotool click 1 click 1"
    };
    
    char key_cmd[] = "xdotool type _\0";
    
    int size;
    int i;
	
	char buff[512];
	memset(buff, 0, 512);
    
    set_keys_status((GtkLabel*) keys_label, 1);
    
    while(1) {
        size = recv(fd, buff, 512, 0);
        
        if(size == -1) {
            break;
        }
        
        i = getPostPrefixIndex(buff, size);
        
        while(i < size) {
            if(buff[i] == KB_INPUT && (i + 2) < size) {
                key_cmd[13] = buff[i + 2];
                system(key_cmd);
                
                if(buff[i + 1] == 0) {
                    // TODO: Fix other inputs
                    i += 3;
                }
                
                i += 3;
            }
            else if(buff[i] == MOUSE_MOVE && (i + 2) < size	) {
                float x = (((uint8_t) buff[i + 1]) / 256) * SCREEN_WIDTH;
				float y = (((uint8_t) buff[i + 2]) / 256) * SCREEN_HEIGHT;
				char move_cmd[32];
                sprintf(move_cmd, "xdotool mousemove %.2f %.2f", x, y);
                system(move_cmd);
                i += 3;
            }
            else if(buff[i] >= 0 && buff[i] <= 9) {
                system(mouse_controls[buff[i]]);
                i++;
            }
            else {
                break;
            }
        }
    }
    
    set_keys_status((GtkLabel*) keys_label, 0);
    
    close(fd);
}


void start_threads() {
    pthread_t discovery_t_id;
    pthread_create(&discovery_t_id, NULL, discovery_t, NULL);
    
    pthread_t move_t_id;
    pthread_create(&move_t_id, NULL, mouse_move_t, NULL);
    
    pthread_t key_t_id;
    pthread_create(&key_t_id, NULL, key_input_t, NULL);
}


static void on_app_activate(GApplication *app, gpointer data) {
    GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    
    GtkWidget* status_root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    
    if(system("xdotool --version")) {
        printf("xdotool not found!\n");
        GtkWidget* error_label = gtk_label_new(NULL);
        gtk_label_set_markup((GtkLabel*) error_label, "<span foreground='red'>xdotool was not found!</span>");
        gtk_box_pack_start((GtkBox*) status_root, error_label, 0, 0, 0);
    }
    else {
        char output[12];
        FILE *fp = popen("xdotool getdisplaygeometry", "r");
        if(fp != NULL) {
            while(fgets(output, sizeof(output), fp) != NULL) {
                char* next;
                int width = strtol(output, &next, 10);
                int height = strtol(next + 1, NULL, 10);
                if(width > 0) {
                    SCREEN_WIDTH = width;
                }
                if(height > 0) {
                    SCREEN_HEIGHT = height;
                }
            }
            
            pclose(fp);
        }
    }
    
    discovery_label = gtk_label_new(NULL);
    mouse_label = gtk_label_new(NULL);
    keys_label = gtk_label_new(NULL);
    
    gtk_box_pack_start((GtkBox*) status_root, discovery_label, 0, 0, 5);
    gtk_box_pack_start((GtkBox*) status_root, mouse_label, 0, 1, 5);
    gtk_box_pack_start((GtkBox*) status_root, keys_label, 0, 1, 5);
    
    gtk_container_add(GTK_CONTAINER(window), status_root);
    
    gtk_widget_show_all(GTK_WIDGET(window));
    
    start_threads();
}


int main(int argc, char* args[]) {
    /** Start GTK app **/
    GtkApplication *app = gtk_application_new(
        "com.atompunkapps.air-mouse", 
        G_APPLICATION_FLAGS_NONE
    );
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, args);
    // deallocate the application object
    g_object_unref(app);
    
    /*pthread_join(discovery_t_id, NULL);
    pthread_join(move_t_id, NULL);
    pthread_join(key_t_id, NULL);*/
    
    return status;
}
