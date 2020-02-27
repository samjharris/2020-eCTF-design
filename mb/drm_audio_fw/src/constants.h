#ifndef SRC_CONSTANTS_H_
#define SRC_CONSTANTS_H_

#include "xil_printf.h"

// shared DDR address
#define SHARED_DDR_BASE (0x20000000 + 0x1CC00000)

// memory constants
#define CHUNK_SZ 16000
#define FIFO_CAP 4096*4

// number of seconds to record/playback
#define PREVIEW_TIME_SEC 30

// ADC/DAC sampling rate in Hz
#define AUDIO_SAMPLING_RATE 48000
#define BYTES_PER_SAMP 2
#define PREVIEW_SZ (PREVIEW_TIME_SEC * AUDIO_SAMPLING_RATE * BYTES_PER_SAMP)

// printing utility
#define MB_PROMPT "\r\nMB> "
#define mb_printf(...) xil_printf(MB_PROMPT __VA_ARGS__)

// protocol constants
#define MAX_REGIONS 32
#define REGION_NAME_SZ 64
#define MAX_USERS 64
#define USERNAME_SZ 64
#define MAX_PIN_SZ 64
#define MAX_SONG_SZ (1<<25)

// LED colors and controller
struct color {
    u32 r;
    u32 g;
    u32 b;
};

// struct to interpret shared buffer as a query
typedef struct {
    u8 song_id;
    u8 owner_id;
    u64 region_vector;
    u64 region_vector;
} query;

// custom type for region keys
typedef u8 region_key[16];

// custom type for pin hash
typedef u8 pin_hash[16];

// struct for interpreting .drm files
typedef struct __attribute__((__packed__)) {
    //0 bytes
    u64 region_vector;
    u8 region_secrets[16 * MAX_REGIONS]; 
    u8 song_id;
    u8 owner_id;
    u8 nonce[16];
    u8 packing1[4];   //WAV metadata
    u32 file_size;    //WAV metadata
    u8 packing2[32];  //WAV metadata
    u32 wav_size;     //WAV metadata 
    //610 bytes
    u8 padding[414]
    //1024 bytes
    u8 signature[16];//add signature size here 
} song;

// struct for interpreting .drm.p files
typedef struct __attribute__((__packed__)) {
    u8 song_id;
    u8 owner_id;
    u8 nonce[16];
    u8 packing1[4];   //WAV metadata
    u32 file_size;    //WAV metadata
    u8 packing2[32];  //WAV metadata
    u32 wav_size;     //WAV metadata
    //104 bytes
    u8 padding[920]
    //1024 bytes
    u8 signature[16];//add signature size here 
} song_p;

// struct for interpreting .drm.s files
typedef struct __attribute__((__packed__)) {
    u8 song_id;
    u64 user_vector;
    shared_secret shared_secrets[64];
    //2120 bytes
    u8 padding[952]
    //3072 bytes
    u8 signature[16];//add signature size here 
} song_s;

//struct for interpreting shared secrets
typedef struct __attribute__((__packed__)) {
    u8 nonce[16];
    u8 songkey[16];
} shared_secret;


// shared buffer values
enum commands { QUERY_PLAYER, QUERY_SONG, LOGIN, LOGOUT, SHARE, PLAY, STOP, DIGITAL_OUT, PAUSE, RESTART, FF, RW };
enum states   { STOPPED, WORKING, PLAYING, PAUSED };


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

// local store for drm metadata
typedef struct {
    u8 song_id;
    u8 owner_id;
    u64 region_vector;
    u64 region_vector;
} song_md;

// store of internal state
typedef struct {
    u8 logged_in;               // whether or not a user is logged on
    u8 uid;                     // logged on user id
    char username[USERNAME_SZ]; // logged on username
    char pin[MAX_PIN_SZ];       // logged on pin
    song_md song_md;            // current song metadata
} internal_state;


#endif /* SRC_CONSTANTS_H_ */
