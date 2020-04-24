#ifndef SIZES_STRUCTS_H
#define SIZES_STRUCTS_H

#ifndef hydrogen_H
#error Include this file after including the file defining libhydrogen size macros
#endif

// fixed-width types
#ifndef MICROBLAZE
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#endif

// Protocol constants
#define MAX_REGIONS 32
#define REGION_NAME_SZ 64
#define MAX_USERS 64
#define USERNAME_SZ 64
#define MAX_PIN_SZ 64
#define MAX_SONG_SZ (1<<25)
//update support for this in provisioning scripts...
#define MAX_SONG_NAME_SZ 64

// song secret sizes
#define REGION_SECRET_SZ 16
#define SONG_KEY_SZ 32
#define ENC_SONG_KEY_SZ 32

#define CHUNK_SZ 8000
#define NONCE_LEN 16 //calculate this...

// Sizes of structs
#define DRM_SZ sizeof(song)
#define DRM_S_SZ sizeof(song_s)
#define DRM_P_SZ sizeof(song_p)

//struct for interpreting shared secrets
typedef struct __attribute__((__packed__)) {
    u8 header[hydro_secretbox_HEADERBYTES]; 
    u8 secret[REGION_SECRET_SZ];
} region_secret;

// struct for interpreting .drm files (full song)
typedef struct __attribute__((__packed__)) {
    u32 region_vector;
    region_secret region_secrets[MAX_REGIONS];
    u8 song_id;
    u8 owner_id;
    //u8 nonce[NONCE_LEN];
    u8 packing1[4];   //WAV metadata
    u32 file_size;    //WAV metadata
    u8 packing2[32];  //WAV metadata
    u32 wav_size;     //WAV metadata 
    u8 signature[hydro_sign_BYTES]; // Over metadata
    // actual audio data
} song;

// struct for interpreting .drm.p files (preview)
typedef struct __attribute__((__packed__)) {
    u8 song_id;
    u8 owner_id;
    //u8 nonce[NONCE_LEN];
    u8 packing1[4];   //WAV metadata
    u32 file_size;    //WAV metadata
    u8 packing2[32];  //WAV metadata
    u32 wav_size;     //WAV metadata
    u8 signature[hydro_sign_BYTES]; // Over metadata
    // actual audio data
} song_p;

// struct for interpreting .drm.s files (sharing data)
typedef struct __attribute__((__packed__)) {
    u32 region_vector;
    region_secret region_secrets[MAX_REGIONS];
    u8 song_id;
    u8 owner_id;
    u64 user_vector;
    u8 shared_key[hydro_secretbox_HEADERBYTES + hydro_secretbox_KEYBYTES];
    u8 hash[hydro_hash_BYTES];
} song_s;

// shared buffer values
enum commands { QUERY_PLAYER, QUERY_SONG, LOGIN, LOGOUT, SHARE, PLAY, STOP, DIGITAL_OUT, PAUSE, RESTART, FF, RW };
enum states   { INVALID_ZERO_STATE, STOPPED, WORKING, PLAYING, PAUSED, LOAD_PREVIEW, LOAD_FULL };

// struct to interpret shared command channel
typedef volatile struct __attribute__((__packed__)) {
    char cmd;                   // from commands enum
    char drm_state;             // from states enum
    char login_status;          // 0 = logged off, 1 = logged on
    char padding;               // not used
    char username[USERNAME_SZ]; // stores logged in or attempted username
    char pin[MAX_PIN_SZ];       // stores logged in or attempted pin

    // shared buffer is either a .drm,.song_p,.song_s
    union {
        song song;
        song_p song_p;
        song_s song_s;
    };
} cmd_channel;
#endif
