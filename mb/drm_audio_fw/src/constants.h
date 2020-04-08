#ifndef SRC_CONSTANTS_H_
#define SRC_CONSTANTS_H_

//#include "xil_printf.h"
#include "hydrogen.h"
#include "sizes_structs.h"
#include "address_defines.h"

// shared DDR address
#define SHARED_DDR_BASE (0x20000000 + 0x1CC00000)

// memory constants
#define CHUNK_SZ 8000
#define FIFO_CAP 1024*4

// number of seconds to record/playback
#define PREVIEW_TIME_SEC 30

// ADC/DAC sampling rate in Hz
#define AUDIO_SAMPLING_RATE 48000
#define BYTES_PER_SAMP 2
#define PREVIEW_SZ (PREVIEW_TIME_SEC * AUDIO_SAMPLING_RATE * BYTES_PER_SAMP)

// password hashing constants
#define PIN_HASH_CONTEXT "PINCX___"
#define PIN_HASH_OPSLIMIT 10000 //?
#define PIN_HASH_MEMLIMIT 0 //?
#define PIN_HASH_THREADS  1 //?
#define PIN_MIN_SZ 8
#define PIN_MAX_SZ 64

// region secret constants
#define REGION_CONTEXT "REGIONCX"

// song key constants
#define SONG_KEY_SKI 1 //subkey index for use in kd function
#define SONG_KEY_CONTEXT "SONGCX__"

// shared secret constants
#define SHARED_SECRET_SZ (hydro_secretbox_HEADERBYTES + hydro_secretbox_KEYBYTES)
#define SHARE_CONTEXT "SHARECX_"

// local store for drm metadata (query as well)
typedef struct {
    u8 song_id;
    u8 owner_id;
    u64 region_vector;
    u64 user_vector;
} song_md;

// store of internal state
typedef struct {
    u8 logged_in;                // whether or not a user is logged on
    u8 uid;                      // logged on user id
    char username[USERNAME_SZ]; // logged on username
    char pin[MAX_PIN_SZ];        // logged on pin
    u8 key[SONG_KEY_SZ];         // if we've derived the key, temporarily store it here
    song_md song_md;             // current song metadata
} internal_state;

#endif /* SRC_CONSTANTS_H_ */
