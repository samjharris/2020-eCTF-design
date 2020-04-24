/*
 * miPod.h
 *
 *  Created on: Jan 9, 2020
 *      Author: ectf
 */

#ifndef SRC_MIPOD_H_
#define SRC_MIPOD_H_

// miPod constants
#define USR_CMD_SZ 64
#define LOAD_FILE_MAX 0xFFFFFFFF

// libhydrogen constants
#define hydro_secretbox_HEADERBYTES (20 + 16)
#define hydro_secretbox_KEYBYTES 32
#define hydro_kx_N_PACKET1BYTES (32 + 16)
#define hydro_sign_BYTES 64
#define hydro_hash_BYTES 32

#define SONG_MD_SIGN_CONTEXT "SONGMDCX"

// Signify that the libhydrogen size constants are set up correctly
#define hydrogen_H

#include "sizes_structs.h"

// printing utility
#define MP_PROMPT "mP> "
#define mp_printf(...) printf(MP_PROMPT __VA_ARGS__)

#define USER_PROMPT "miPod %s# "
#define print_prompt() printf(USER_PROMPT, "")
#define print_prompt_msg(...) printf(USER_PROMPT, __VA_ARGS__)

// simulate array of 64B names without pointer indirection
#define q_region_lookup(q, i) (q.regions + (i * REGION_NAME_SZ))
#define q_user_lookup(q, i) (q.users + (i * USER_NAME_SZ))

#endif /* SRC_MIPOD_H_ */
