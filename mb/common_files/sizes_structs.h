#ifndef SIZES_STRUCTS_H
#define SIZES_STRUCTS_H

// Protocol constants
#define MAX_REGIONS 64
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

#endif