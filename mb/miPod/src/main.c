/*
 * eCTF Collegiate 2020 miPod Example Code
 * Linux-side DRM driver
 */


#include "miPod.h"
#include "device_publics.h"

#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <linux/gpio.h>
#include <string.h>


volatile cmd_channel *c;


//////////////////////// UTILITY FUNCTIONS ////////////////////////

// cleanup dynamically allocated memory
static void cleanup_free(void *p) {
  free(*(void**) p);
}

// sends a command to the microblaze using the shared command channel and interrupt
void send_command(int cmd) {
    memcpy((void*)&c->cmd, &cmd, 1);

    //trigger gpio interrupt
    system("devmem 0x41200000 32 0");
    system("devmem 0x41200000 32 1");
}


// parses the input of a command with up to two arguments
// any arguments not present will be set to NULL
void parse_input(char *input, char **cmd, char **arg1, char **arg2) {
    *cmd = strtok(input, " \r\n");
    *arg1 = strtok(NULL, " \r\n");
    *arg2 = strtok(NULL, " \r\n");
}


// prints the help message while not in playback
void print_help() {
    mp_printf("miPod options:\r\n");
    mp_printf("  login <username> <pin>: log on to a miPod account (must be logged out)\r\n");
    mp_printf("  logout: log off of a miPod account (must be logged in)\r\n");
    mp_printf("  query <song.drm>: display information about the song\r\n");
    mp_printf("  share <song.drm> <username>: share the song with the specified user\r\n");
    mp_printf("  play <song.drm>: play the song\r\n");
    mp_printf("  digital_out <song.drm>: play the song to digital out\r\n");
    mp_printf("  exit: exit miPod\r\n");
    mp_printf("  help: display this message\r\n");
}


// prints the help message while in playback
void print_playback_help() {
    mp_printf("miPod playback options:\r\n");
    mp_printf("  stop: stop playing the song\r\n");
    mp_printf("  pause: pause the song\r\n");
    mp_printf("  resume: resume the paused song\r\n");
    mp_printf("  restart: restart the song\r\n");
    mp_printf("  ff: fast forwards 5 seconds(unsupported)\r\n");
    mp_printf("  rw: rewind 5 seconds (unsupported)\r\n");
    mp_printf("  help: display this message\r\n");
}

// sync with the microblaze interupt-disabling by napping a tad
void mb_sync(int s) {
    sleep(s);
}

// loads a file into the song buffer with the associate
// only reads up to min(buf_size, file size)
// returns the size of the file or 0 on error
size_t load_file(char *fname, size_t buf_size, char song_buf[buf_size]) {
    int fd;
    struct stat sb;

    fd = open(fname, O_RDONLY);
    if (fd == -1){
    	/* special case - don't print error for missing share (.drm.s) files */
    	if (strncmp(&fname[strlen(fname) - 2], ".s", sizeof(".s")) != 0) {
    		mp_printf("Failed to open file! Error = %d\r\n", errno);
    	}
        return 0;
    }

    if (fstat(fd, &sb) == -1){
        mp_printf("Failed to stat file! Error = %d\r\n", errno);
        return 0;
    }

    size_t read_amt = sb.st_size;
    if (buf_size < read_amt) {
    	read_amt = buf_size;
    }

    read(fd, song_buf, read_amt);
    close(fd);

    mp_printf("Loaded file into buffer (%u/%uB)\r\n", (unsigned) read_amt, (unsigned) sb.st_size);
    return sb.st_size;
}


//////////////////////// COMMAND FUNCTIONS ////////////////////////


// attempts to log in for a user
void login(char *username, char *pin) {
    if (!username || !pin) {
        mp_printf("Invalid user name/PIN\r\n");
        print_help();
        return;
    }

    // drive DRM
    strcpy((void*)c->username, username);
    strcpy((void*)c->pin, pin);
    send_command(LOGIN);
}


// logs out for a user
void logout() {
    // drive DRM
    send_command(LOGOUT);
}


// queries the DRM about the player
// DRM will fill shared buffer with query content
void query_player() {
    // print the regions
    mp_printf("Regions: ");
    for (u32 i = 0; i < NUM_REGIONS; ++i) {
		printf("%s%c ", REGION_NAMES[i], (i + 1 == NUM_REGIONS) ? ' ' : ',');
    }
    printf("\r\n");

    mp_printf("Authorized users: ");
    for (u32 i = 0; i < NUM_USERS; ++i) {
    	printf("%s%c ", USERNAMES[i], (i + 1 == NUM_USERS) ? ' ' : ',');
    }
    printf("\r\n");
}

// queries the DRM about a song
void query_song(char *song_name) {
	// load the song into a local buffer
	song temp_song = {0};
    if (!load_file(song_name, sizeof(temp_song), (char *) &temp_song)) {
        mp_printf("Failed to load song!\r\n");
        return;
    }

    /* how many regions is this song provisioned for? */
    uint32_t pop_cnt = __builtin_popcount((uint32_t) ((temp_song.region_vector & 0xFFFFFFFF00000000) >> 32));
    pop_cnt += __builtin_popcount((uint32_t) (temp_song.region_vector & 0xFFFFFFFF));

    // print the regions
    mp_printf("Regions: ");
    for (u32 i = 0; i < NUM_REGIONS; ++i) {
    	if (((1UL << i) & temp_song.region_vector)) {
    		printf("%s%c ", REGION_NAMES[i], --pop_cnt ? ',' : ' ');
    	}
    }
    printf("\r\n");

    assert(temp_song.owner_id < NUM_USERS);
    mp_printf("Owner: %s", USERNAMES[temp_song.owner_id]);
    printf("\r\n");

    // format the shared file name
    size_t share_file_len = strlen(song_name) + 3; // song_name + ".s\0"
    char *share_file_name __attribute__((cleanup(cleanup_free))) = calloc(share_file_len, 1);
    snprintf(share_file_name, share_file_len, "%s.s", song_name);

    // TODO: Cameron - should generate a .drm.s file to test with

    // read the share file into a local buffer if it exists
    song_s share_song = {0};
    mp_printf("Authorized users: ");
    if (!load_file(share_file_name, sizeof(share_song), (char *) &share_song)) {
    	printf("\r\n");
    	return; // no share file found - no authorized users
    }

    /* how many users is this song shared with? */
    pop_cnt = __builtin_popcount((uint32_t) ((share_song.user_vector & 0xFFFFFFFF00000000) >> 32));
    pop_cnt += __builtin_popcount((uint32_t) (share_song.user_vector & 0xFFFFFFFF));

    for (u32 i = 0; i < NUM_USERS; ++i) {
    	if (((1UL << i) & share_song.user_vector)) {
    		printf("%s%c ", USERNAMES[i], --pop_cnt ? ',' : ' ');
    	}
    }
    printf("\r\n");
}


// attempts to share a song with a user
void share_song(char *song_name, char *username) {
    int fd;
    unsigned int length;
    ssize_t wrote, written = 0;

    char* song_name_share = (char *)malloc(MAX_SONG_NAME_SZ + 1 + 2);
    
    if (!username || !song_name) {
        mp_printf("Need song name and username\r\n");
        print_help();
    }

    //adjusts "SongName.drm" to "SongName.drm.s"
    strncpy(song_name_share, song_name, MAX_SONG_NAME_SZ);
    strncat(song_name_share, ".s", 2);

    // load the song into the shared buffer
    if (!load_file(song_name, LOAD_FILE_MAX, (void*)&c->song_s)) {
        mp_printf("Failed to load song!\r\n");
        free(song_name_share);
        return;
    }

    // copy the sharee into the username field
    strncpy((char *)c->username, username, USER_NAME_SZ);

    // drive DRM
    send_command(SHARE);
    while (c->drm_state == STOPPED) continue; // wait for DRM to start working
    while (c->drm_state == WORKING) continue; // wait for DRM to share song

    // request was rejected if c->song_s.song_id was set to 255
    if (c->song.wav_size == 255) {
        mp_printf("Share rejected\r\n");
        free(song_name_share);
        return;
    }

    // open output file
    fd = open(song_name_share, O_WRONLY);
    if (fd == -1){
        mp_printf("Failed to open file! Error = %d\r\n", errno);
        free(song_name_share);
        return;
    }

    // write song dump to file
    mp_printf("Writing song share to file '%s' (%dB)\r\n", song_name_share, DRM_S_SZ);
    while (written < DRM_S_SZ) {
        wrote = write(fd, (char *)&c->song + written, DRM_S_SZ - written);
        if (wrote == -1) {
            mp_printf("Error in writing file! Error = %d\r\n", errno);
            free(song_name_share);
            return;
        }
        written += wrote;
    }
    close(fd);
    mp_printf("Finished writing file\r\n");
    free(song_name_share);
}


// plays a song and enters the playback command loop
int play_song(char *song_name) {
    char usr_cmd[USR_CMD_SZ + 1], *cmd = NULL, *arg1 = NULL, *arg2 = NULL;

    char* song_name_share = (char *)malloc(MAX_SONG_NAME_SZ + 1 + 2);

    // load song into shared buffer
    if (!load_file(song_name, LOAD_FILE_MAX, (void*)&c->song)) {
        mp_printf("Failed to load song!\r\n");
        return 0;
    }

    // drive the DRM
    send_command(PLAY);
    while (c->drm_state == STOPPED) continue; // wait for DRM to start playing

    // play loop
    while(1) {
        // get a valid command
        do {
            print_prompt_msg(song_name);
            fgets(usr_cmd, USR_CMD_SZ, stdin);

            // exit playback loop if DRM has finished song
            if (c->drm_state == STOPPED) {
                mp_printf("Song finished\r\n");
                return 0;
            }
        } while (strlen(usr_cmd) < 2);

        // parse and handle command
        parse_input(usr_cmd, &cmd, &arg1, &arg2);
        if (!cmd) {
            continue;
        } else if (!strcmp(cmd, "help")) {
            print_playback_help();
        } else if (!strcmp(cmd, "resume")) {
            send_command(PLAY);
            usleep(200000); // wait for DRM to print
        } else if (!strcmp(cmd, "pause")) {
            send_command(PAUSE);
            usleep(200000); // wait for DRM to print
        } else if (!strcmp(cmd, "stop")) {
            send_command(STOP);
            usleep(200000); // wait for DRM to print
            break;
        } else if (!strcmp(cmd, "restart")) {
            send_command(RESTART);
        } else if (!strcmp(cmd, "exit")) {
            mp_printf("Exiting...\r\n");
            send_command(STOP);
            return -1;
        } else if (!strcmp(cmd, "rw")) {
            mp_printf("Unsupported feature.\r\n\r\n");
            print_playback_help();
        } else if (!strcmp(cmd, "ff")) {
            mp_printf("Unsupported feature.\r\n\r\n");
            print_playback_help();
        } else if (!strcmp(cmd, "lyrics")) {
            mp_printf("Unsupported feature.\r\n\r\n");
            print_playback_help();
        } else {
            mp_printf("Unrecognized command.\r\n\r\n");
            print_playback_help();
        }
    }

    return 0;
}


// turns DRM song into original WAV for digital output
void digital_out(char *song_name) {
    char fname[64];

    // load file into shared buffer
    if (!load_file(song_name, LOAD_FILE_MAX, (void*)&c->song)) {
        mp_printf("Failed to load song!\r\n");
        return;
    }

    // drive DRM
    send_command(DIGITAL_OUT);
    while (c->drm_state == STOPPED) continue; // wait for DRM to start working
    while (c->drm_state == WORKING) continue; // wait for DRM to dump file

    // open digital output file
    int written = 0, wrote, length = c->song.file_size + 8;
    int fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd == -1){
        mp_printf("Failed to open file! Error = %d\r\n", errno);
        return;
    }

    // write song dump to file
    mp_printf("Writing song to file '%s' (%dB)\r\n", fname, length);
    while (written < length) {
        wrote = write(fd, (char *)&c->song + written, length - written);
        if (wrote == -1) {
            mp_printf("Error in writing file! Error = %d \r\n", errno);
            return;
        }
        written += wrote;
    }
    close(fd);
    mp_printf("Finished writing file\r\n");
}


//////////////////////// MAIN ////////////////////////


int main(int argc, char** argv)
{
    int mem;
    char usr_cmd[USR_CMD_SZ + 1], *cmd = NULL, *arg1 = NULL, *arg2 = NULL;
    memset(usr_cmd, 0, USR_CMD_SZ + 1);

    // open command channel
    mem = open("/dev/uio0", O_RDWR);
    c = mmap(NULL, sizeof(cmd_channel), PROT_READ | PROT_WRITE,
             MAP_SHARED, mem, 0);
    if (c == MAP_FAILED){
        mp_printf("MMAP Failed! Error = %d\r\n", errno);
        return -1;
    }
    mp_printf("Command channel open at %p (%dB)\r\n", c, sizeof(cmd_channel));

    // dump player information before command loop
    query_player();

    // go into command loop until exit is requested
    while (1) {
        // get command
        print_prompt();
        fgets(usr_cmd, USR_CMD_SZ, stdin);

        // parse and handle command
        parse_input(usr_cmd, &cmd, &arg1, &arg2);
        if (!cmd) {
            continue;
        } else if (!strcmp(cmd, "help")) {
            print_help();
        } else if (!strcmp(cmd, "login")) {
            login(arg1, arg2);
            mb_sync(1);
        } else if (!strcmp(cmd, "logout")) {
            logout();
            mb_sync(1);
        } else if (!strcmp(cmd, "query")) {
            query_song(arg1);
        } else if (!strcmp(cmd, "play")) {
            // break if exit was commanded in play loop
            if (play_song(arg1) < 0) {
                break;
            }
        } else if (!strcmp(cmd, "digital_out")) {
            digital_out(arg1);
            mb_sync(1);
        } else if (!strcmp(cmd, "share")) {
            share_song(arg1, arg2);
            mb_sync(4);
        } else if (!strcmp(cmd, "exit")) {
            mp_printf("Exiting...\r\n");
            break;
        } else {
            mp_printf("Unrecognized command.\r\n\r\n");
            print_help();
        }
    }

    // unmap the command channel
    munmap((void*)c, sizeof(cmd_channel));

    return 0;
}
