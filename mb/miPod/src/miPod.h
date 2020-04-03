/*
 * miPod.h
 *
 *  Created on: Jan 9, 2020
 *      Author: ectf
 */

#ifndef SRC_MIPOD_H_
#define SRC_MIPOD_H_

// fixed-width types
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// miPod constants
#define USR_CMD_SZ 64
#define LOAD_FILE_MAX 0xFFFFFFFF

// protocol constants
#define MAX_REGIONS 32
#define REGION_NAME_SZ 64
#define MAX_USERS 64
#define USER_NAME_SZ 64
#define MAX_PIN_SZ 64
#define MAX_SONG_SZ (1<<25)
#define MAX_SONG_NAME_SZ 64 //update support for this in provisioning scripts...

// song constants
#define REGION_SECRET_SZ 16
#define SONG_KEY_SZ 32
#define ENC_SONG_KEY_SZ 32

// TODO: libhydrogen constants
#define hydro_secretbox_HEADERBYTES 0
#define hydro_kx_N_PACKET1BYTES 0

// printing utility
#define MP_PROMPT "mP> "
#define mp_printf(...) printf(MP_PROMPT __VA_ARGS__)

#define USER_PROMPT "miPod %s# "
#define print_prompt() printf(USER_PROMPT, "")
#define print_prompt_msg(...) printf(USER_PROMPT, __VA_ARGS__)


// struct to interpret shared buffer as a query
typedef struct {
    int num_regions;
    int num_users;
    char owner[USER_NAME_SZ];
    char regions[MAX_REGIONS * REGION_NAME_SZ];
    char users[MAX_USERS * USER_NAME_SZ];
} query;

// simulate array of 64B names without pointer indirection
#define q_region_lookup(q, i) (q.regions + (i * REGION_NAME_SZ))
#define q_user_lookup(q, i) (q.users + (i * USER_NAME_SZ))


//struct for interpreting shared secrets
typedef struct __attribute__((__packed__)) {
    u8 header[hydro_secretbox_HEADERBYTES]; 
    u8 secret[REGION_SECRET_SZ];
} region_secret;

//struct for interpreting shared secrets
typedef struct __attribute__((__packed__)) {
    u8 packet1[hydro_kx_N_PACKET1BYTES]; 
    u8 songkey[ENC_SONG_KEY_SZ];
} shared_secret;

// struct for interpreting .drm files
typedef struct __attribute__((__packed__)) {
    //0 bytes
    u64 region_vector;
    region_secret region_secrets[MAX_REGIONS];
    u8 song_id;
    u8 owner_id;
    u8 nonce[16]; //this may not be needed...
    u8 packing1[4];   //WAV metadata
    u32 file_size;    //WAV metadata
    u8 packing2[32];  //WAV metadata
    u32 wav_size;     //WAV metadata 
    //
    u8 padding[0];
    //
    u8 signature[16];//add signature size here 
} song;

// struct for interpreting .drm.p files
typedef struct __attribute__((__packed__)) {
    u8 song_id;
    u8 owner_id;
    u8 nonce[16]; //this may not be needed...
    u8 packing1[4];   //WAV metadata
    u32 file_size;    //WAV metadata
    u8 packing2[32];  //WAV metadata
    u32 wav_size;     //WAV metadata
    //
    u8 padding[0];
    //
    u8 signature[16];//add signature size here 
} song_p;

// struct for interpreting .drm.s files
typedef struct __attribute__((__packed__)) {
    u64 region_vector;
    region_secret region_secrets[MAX_REGIONS];
    u8 song_id;
    u8 owner_id;
    u8 nonce[16]; //this may not be needed...
    u64 user_vector;
    shared_secret shared_secrets[MAX_USERS];
    //
    u8 padding[0];//?
    //
    u8 signature[16];//add signature size here 
} song_s;

// shared buffer values
enum commands { QUERY_PLAYER, QUERY_SONG, LOGIN, LOGOUT, SHARE, PLAY, STOP, DIGITAL_OUT, PAUSE, RESTART, FF, RW };
enum states   { STOPPED, WORKING, PLAYING, PAUSED };


// struct to interpret shared command channel
typedef volatile struct __attribute__((__packed__)) {
    char cmd;                   // from commands enum
    char drm_state;             // from states enum
    char login_status;          // 0 = logged off, 1 = logged on
    char padding;               // not used
    char username[USER_NAME_SZ]; // stores logged in or attempted username
    char pin[MAX_PIN_SZ];       // stores logged in or attempted pin

    // shared buffer is either a .drm,.song_p,.song_s
    union {
        song song;
        song_p song_p;
        song_s song_s;
    };
} cmd_channel;

#endif /* SRC_MIPOD_H_ */
