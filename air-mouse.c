#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <string.h>

#include <pthread.h>


#define MOUSEMOVE_PORT 6666
#define KEYINPUT_PORT 6667
#define DISCOVERY_PORT 7777

#define COMPATIBILITY_CODE 8

#define MOUSE_MOVE -9
#define KB_INPUT -10

// Temporary. TODO: Remove
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

const char PREFIX[] = { 87, 52, 109, 98, 68 };

size_t getPostPrefixIndex(char buff[], size_t size) {
    int i = 0;
    while(i < 5 && i < size) {
        if(PREFIX[i] != buff[i]) {
            printf("Not equal\n");
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

void* discovery_t() {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(fd < 0) {
        // TODO: Set status
        return NULL;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(DISCOVERY_PORT);
    
    if(bind(fd, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
        // TODO: Set status
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
    
    // TODO: Set status positive
    
    while(size = recvfrom(fd, buff, 8, 0, (struct sockaddr*) &client_addr, (socklen_t*) &caddr_size) != -1) {
        if(size > 1) {
            uint16_t client_version;
            memcpy(&client_version, &buff, 2);
            // TODO: Compare compatibility
        }
        else {
            sendto(fd, ver, (int) strlen(ver), 0, (const struct sockaddr*) &client_addr, sizeof(client_addr));
            printf("Announced!\n");
        }
        
        memset(&client_addr, 0, caddr_size);
    }
    
    // TODO: Set status
    
    close(fd);
}

void* mouse_move_t() {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(fd < 0) {
        //perror("Could not create socket");
        // TODO: Set status
        return NULL;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(MOUSEMOVE_PORT);
    
    if(bind(fd, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
        // TODO: Set status
        close(fd);
        return NULL;
    }
    
    size_t size;
	
	char buff[13];
	memset(buff, 0, 13);
	
	float x = 0, y = 0;
	
	char cmd[50];
	memset(cmd, 0, 50);
    
    // TODO: Set status positive
    
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
    
    // TODO: Set status
    
    close(fd);
}

void* key_input_t() {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(fd < 0) {
        // TODO: Set status
        return NULL;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(KEYINPUT_PORT);
    
    if(bind(fd, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
        // TODO: Set status
        close(fd);
        printf("Exiting key thread..\n");
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
    
    // TODO: Set status positive
    
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
                    i += 3;
                }
                
                i += 3;
            }
            else if(buff[i] == MOUSE_MOVE && (i + 2) < size	) {
                // TODO: Implement
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
    
    // TODO: Set status
    
    close(fd);
}

int main(int argc, char* args[]) {
    pthread_t discovery_t_id;
    pthread_create(&discovery_t_id, NULL, discovery_t, NULL);
    
    pthread_t move_t_id;
    pthread_create(&move_t_id, NULL, mouse_move_t, NULL);
    
    pthread_t key_t_id;
    pthread_create(&key_t_id, NULL, key_input_t, NULL);
    
    pthread_join(discovery_t_id, NULL);
    pthread_join(move_t_id, NULL);
    pthread_join(key_t_id, NULL);
    
    return 0;
}
