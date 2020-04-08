/*
 * UMass Amherst a/k/a RunDRM
 * eCTF Collegiate 2020 MicroBlaze Code
 * Audio Digital Rights Management
 */

#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xstatus.h"
#include "xaxidma.h"
#include "xil_mem.h"
#include "util.h"
#include "xintc.h"
#include "constants.h"
#include "sleep.h"

#include "hydrogen_inc.h"

#include "device_secrets.h"
#include "device_publics.h"

//////////////////////// GLOBALS ////////////////////////


// audio DMA access
static XAxiDma sAxiDma;

// LED colors and controller
u32 *led = (u32*) XPAR_RGB_PWM_0_PWM_AXI_BASEADDR;
const struct color RED =    {0x01ff, 0x0000, 0x0000};
const struct color YELLOW = {0x01ff, 0x01ff, 0x0000};
const struct color GREEN =  {0x0000, 0x01ff, 0x0000};
const struct color BLUE =   {0x0000, 0x0000, 0x01ff}; 

// change states
#define change_state(state, color) c->drm_state = state; setLED(led, color);
#define set_stopped() change_state(STOPPED, RED)
#define set_working() change_state(WORKING, YELLOW)
#define set_playing() change_state(PLAYING, GREEN)
#define set_paused()  change_state(PAUSED, BLUE)

// shared command channel -- read/write for both PS and PL
volatile cmd_channel *c = (cmd_channel*)SHARED_DDR_BASE;

// internal state store
internal_state s;


//////////////////////// INTERRUPT HANDLING ////////////////////////


// shared variable between main thread and interrupt processing thread
volatile static u8 InterruptProcessed = FALSE;
static XIntc InterruptController;

void myISR(void) {
    InterruptProcessed = TRUE;
}


//////////////////////// UTILITY FUNCTIONS ////////////////////////


// sets the i'th bit of a given vector to 1
void set_bit(u64 vector, u8 i) {
    vector |= ((u64)(1) << i);
    return;
}

// checks the value of the i'th bit of a given vector
int check_bit(u64 vector, u8 i) {
    return (int)(vector << (63-i) >> 63);
}

//secrets.h should hold u64 bit vector "provisioned_regions", plus array of region keys, plus table mapping rid to region name
// returns whether an rid has been provisioned
int is_provisioned_rid(u8 rid) {
    if(rid < 32 && rid >= 0) {
        return check_bit(PROVISIONED_RIDS, rid);
    }
    return FALSE;
}


// looks up the region name corresponding to the rid
int rid_to_region_name(u8 rid, char *region_name) {
    if(is_provisioned_rid(rid)) {
        memcpy(region_name, (char *)REGION_NAMES[rid], REGION_NAME_SZ);
        return TRUE;
    }
    memcpy(region_name, "<unknown region>", REGION_NAME_SZ);
    return FALSE;
}


//secrets.h should hold u64 bit vector "provisioned_users", plus an array of user pw hashes, plus table mapping uid to user name
// returns whether a uid has been provisioned
int is_provisioned_uid(u8 uid) {
    if(uid < 64 && uid >= 0) {
        return check_bit(PROVISIONED_UIDS, uid); 
    }
    return FALSE;
}


// looks up the username corresponding to the uid
int uid_to_username(u8 uid, char *user_name) {
    if(is_provisioned_uid(uid)) {
        memcpy(user_name, (char *)USERNAMES[uid], USERNAME_SZ);
        return TRUE;
    }
    memcpy(user_name, "<unknown user>", USERNAME_SZ);
    return FALSE;
}


// looks up the username corresponding to the uid
u8 username_to_uid(char *user_name) {
    for (int i = 0; i < NUM_USERS; i++) {
        if (!strncmp(user_name, USERNAMES[i], USERNAME_SZ)) {
            return i;
        }
    }
    mb_printf("Could not find username '%s'\r\n", user_name);
    return 255;
}



// loads the song metadata in the shared buffer into the local struct
void load_song_s_md() {
    s.song_md.song_id = c->song_s.song_id;
    s.song_md.owner_id = c->song_s.owner_id;
    s.song_md.region_vector = c->song_s.region_vector;
    s.song_md.user_vector = c->song_s.user_vector;
}


// checks if the song loaded into the shared buffer is locked for the current user
//      returns 0 if not authorized, 1 if owner, 2 if shared with
int is_locked() {
    // check for authorized user
    if (!s.logged_in) {
        //mb_printf("No user logged in");
        return 0;
    } 

    // check if provisioned for matching region
    if((s.song_md.region_vector & PROVISIONED_RIDS) != 0) {
        return 0;
    }

    // check if owned by current user
    if(s.uid == s.song_md.owner_id) {
        return 1;
    }

    // check if shared with current user
    if(check_bit(s.song_md.user_vector, s.uid)) {
        return 2;
    }

    return 0;
}

//////////////////////// COMMAND FUNCTIONS ////////////////////////


// attempt to log in to the credentials in the shared buffer
void login() {
    if (s.logged_in) {
        mb_printf("Already logged in. Please log out first.\r\n");
        memcpy((void*)c->username, s.username, USERNAME_SZ);
        memcpy((void*)c->pin, s.pin, MAX_PIN_SZ);
    } else {
        //Create temporary buffer
        char user_buffer[USERNAME_SZ];
        char pin_buffer[MAX_PIN_SZ];
        //copy to temporary buffer
        memcpy(user_buffer, (void*)c->username, USERNAME_SZ);
        memcpy(pin_buffer, (void*)c->pin, MAX_PIN_SZ);

        for (int i = 0; i < NUM_PROVISIONED_USERS; i++) {    
            // search for matching username
            if (!strncmp(user_buffer, USERNAMES[i], USERNAME_SZ)) {
                // check if pin matches
                if(hydro_pwhash_verify(PIN_HASHES[i],//const uint8_t  stored[hydro_pwhash_STOREDBYTES] //WE SHOULDN'T NEED TO CAST THESE, BUT DOING SO WILL PRB REDUCE COMPILER WARNINGS...
                            pin_buffer, //const char* passwd //should this be &?
                            PIN_MAX_SZ, //TREAT ALL PASSWORDS AS 0 PADDING (appended)
                            system_pwhash_key,//const uint8_t  master_key[hydro_pwhash_MASTERKEYBYTES],
                            PIN_HASH_OPSLIMIT, PIN_HASH_MEMLIMIT, PIN_HASH_THREADS)) {
                    
                    //update states
                    memcpy(s.username, user_buffer, USERNAME_SZ);
                    memcpy(s.pin, pin_buffer, MAX_PIN_SZ);
                    c->login_status = 1;
                    s.logged_in = 1;
                    s.uid = i;
                    mb_printf("Logged in\r\n");
                    return;
                } 
            }
        }

        // reject login attempt
        mb_printf("Invalid login\r\n");
        // clean up
        s.logged_in = 0;
        c->login_status = 0;
        s.uid = 255;
        memset((void*)c->username, 0, USERNAME_SZ);
        memset((void*)c->pin, 0, MAX_PIN_SZ);
        memset(s.username, 0, USERNAME_SZ);
        memset(s.pin, 0, MAX_PIN_SZ);
        return;
    }
}


// attempt to log out
void logout() {
    if (s.logged_in) {
        mb_printf("Logging out...\r\n");
        s.logged_in = 0;
        c->login_status = 0;
        s.uid = 255;
        memset((void*)c->username, 0, USERNAME_SZ);
        memset((void*)c->pin, 0, MAX_PIN_SZ);
        memset(s.username, 0, USERNAME_SZ);
        memset(s.pin, 0, MAX_PIN_SZ);
    } else {
        mb_printf("Not logged in\r\n");
    }
}

/*
void share_song();
    Adds a user to the song's list of users. Upon calling, ./miPod should
    have loaded the appropriate .drm.s file into cmd. If operation fails,
    we signal this by setting c->song_s.song_id == 255
*/
void share_song() {
    // check if we're logged in
    if (!s.logged_in) {
        //mb_printf("No user logged in");
        c->song_s.song_id = 255;
        return;
    } 
    // grab this for later...
    char *sharee_str = (char *)c->username;
    // make a buffer to hold the whole .drm.s file
    song_s *drm_s_buffer = (song_s *)malloc(DRM_S_SZ);
    if(drm_s_buffer == NULL) {
        // malloc failed... how should we handle this better?
        set_paused(); //signal to petalinux that we failed 
        return;
    }

    // copy the whole enchilada
    memcpy(drm_s_buffer, &(c->song_s), DRM_S_SZ);

    // integrity check
    // // call hydro_hash_keygen(system_share_hashing_key); during provisioning
    uint8_t hash[hydro_hash_BYTES];
    hydro_hash_hash(hash, sizeof(hash), drm_s_buffer, DRM_S_SZ - hydro_hash_BYTES, SHARE_CONTEXT, system_share_hashing_key);
    if (hash != drm_s_buffer.hash) {
        //integrity check failed
        free(drm_s_buffer);
        set_paused(); //signal to petalinux that we failed 
        return;
    }

    // load the song_s metadata into the mb state
    s.song_md.region_vector = drm_s_buffer->region_vector;
    s.song_md.song_id = drm_s_buffer->song_id;
    s.song_md.owner_id = drm_s_buffer->owner_id;
    s.song_md.user_vector = drm_s_buffer->user_vector;

    // do initial permission checks...
    // check for authorized user
    // check if provisioned for matching region
    if((s.song_md.region_vector & PROVISIONED_RIDS) != 0) {
        //mb_printf("Song not provisioned for this board's regions!");
        free(drm_s_buffer);
        set_paused(); //signal to petalinux that we failed 
        return;
    }
    // check if NOT owned by current user
    if(s.uid != s.song_md.owner_id) {
        //mb_printf("Current user does not own this song!");
        free(drm_s_buffer);
        set_paused(); //signal to petalinux that we failed 
        return;
    }
    // check if shared with current user
    if(check_bit(s.song_md.user_vector, s.uid)) {
        //mb_printf("Song is already shared with this user!");
        free(drm_s_buffer);
        set_paused(); //signal to petalinux that we failed 
        return;
    }

    //make some buffers
    u8 metakey[PIN_MAX_SZ + REGION_SECRET_SZ]; // metakey = pin+regionsecret
    u8 region_secret[REGION_SECRET_SZ];
    u8 key[hydro_secretbox_KEYBYTES];

    // decrypt region secret //maybe do this here, instead of branching function call?
    int region_id = -1;
    for (int i = 0; i < MAX_REGIONS; i++) {
        //find an index of a region key collision
        if(check_bit(s.song_md.region_vector,i) && check_bit(PROVISIONED_RIDS,i)){
            region_id = i;
            // run the decryption
            if (hydro_secretbox_decrypt(metakey+PIN_MAX_SZ, 
                                        drm_s_buffer->region_secrets[i].secret,
                                        REGION_SECRET_SZ + hydro_secretbox_HEADERBYTES, 
                                        0, 
                                        REGION_CONTEXT, 
                                        system_region_AES_keys[i]) != 0) {
                // error decrypting!
                free(drm_s_buffer);
                set_paused(); //signal to petalinux that we failed 
                return;
            }
            break;
        }
    }

    if(region_id < 0){
        // Invalid decryption... abort!
        free(drm_s_buffer);
        set_paused(); //signal to petalinux that we failed 
        return;
    }
    
    // fill in the rest of the metakey
    memcpy(metakey+0,s.pin,PIN_MAX_SZ);

    // derive key
    if(hydro_kdf_derive_from_key(&key, SONG_KEY_SZ,
                    SONG_KEY_SKI, SONG_KEY_CONTEXT,
                    metakey) < 0){
        // failed to derive keys
        free(drm_s_buffer);
        set_paused(); //signal to petalinux that we failed 
        return;
    }

    // get who we are sharing with
    u8 sharee = username_to_uid(sharee_str);
    if(sharee == 255) {
        // User does not exist!
        free(drm_s_buffer);
        set_paused(); //signal to petalinux that we failed 
        return;
    }

    // encrypt song key, store modified data in .drm.s
    hydro_secretbox_encrypt(drm_s_buffer->shared_key, key, hydro_secretbox_KEYBYTESh, 0, SHARE_CONTEXT, system_sharing_key);
    // set the appropriate bit in the vector
    set_bit(drm_s_buffer->user_vector, sharee);

    // rehash .drm.s
    hydro_hash_hash(hash, sizeof(hash), drm_s_buffer, DRM_S_SZ - hydro_hash_BYTES, SHARE_CONTEXT, system_share_hashing_key);
    // and store that hash in the file
    memcpy(drm_s_buffer->hash,hash,hydro_hash_BYTES);
    
    // write back to cmd
    memcpy(&(c->song_s), drm_s_buffer, DRM_SZ);
    //petalinux should write this back! 
    free(drm_s_buffer);
    set_stopped();
    return;
}

/*
int song_auth();
    Authenticates a song based on logged-in user data. Upon calling, ./miPod
    should have loaded the appropriate .drm.s file into cmd channel.

    returns 0 on failure (we can play preview), 1 on success (we can play the whole song)
*/
int song_auth() {
    // check if we're logged in
    if (!s.logged_in) {
        //mb_printf("No user logged in");
        return 0;
    } 

    // load some metadata...
    s.song_md.song_id = c->song_s.song_id;
    s.song_md.owner_id = c->song_s.owner_id;
    s.song_md.region_vector = c->song_s.region_vector;
    s.song_md.user_vector = c->song_s.user_vector;

    int song_status = 0;

    // check if owned by current user; if so, derive the key
    if(s.uid == s.song_md.owner_id) {
        //mb_printf("Current user owns this song!");
        song_status = 1;

        //make some buffers
        u8 metakey[PIN_MAX_SZ + REGION_SECRET_SZ]; // metakey = pin+regionsecret
        u8 region_secret[REGION_SECRET_SZ];

        // decrypt region secret //maybe do this here, instead of branching function call?
        int region_id = -1;
        for (int i = 0; i < MAX_REGIONS; i++) {
            //find an index of a region key collision
            if(check_bit(s.song_md.region_vector,i) && check_bit(PROVISIONED_RIDS,i)){
                region_id = i;
                // run the decryption
                if (hydro_secretbox_decrypt(metakey+PIN_MAX_SZ, 
                                            c->song_s.region_secrets[i].secret,
                                            REGION_SECRET_SZ + hydro_secretbox_HEADERBYTES, 
                                            0, 
                                            REGION_CONTEXT, 
                                            system_region_AES_keys[i]) != 0) {
                    // error decrypting!
                    return 0;
                }
                break;
            }
        }

        if(region_id < 0){
            // Invalid decryption... abort!
            return 0;
        }
        
        // fill in the rest of the metakey
        memcpy(metakey+0, s.pin, PIN_MAX_SZ);

        // derive key
        if(hydro_kdf_derive_from_key(&s.key, hydro_secretbox_KEYBYTES,
                        SONG_KEY_SKI, SONG_KEY_CONTEXT,
                        metakey) < 0){
            // failed to derive keys
            return 0;
        }
    }

    // check if shared with current user; if so, derive the key
    if(check_bit(s.song_md.user_vector, s.uid)) {
        //mb_printf("Song is shared with this user!");
        song_status = -1;

        //run the decryption
        if (hydro_secretbox_decrypt(s.key, 
                                    (uint8_t *) &(c->song_s.shared_secrets[s.uid]),
                                    hydro_secretbox_KEYBYTES, 
                                    0, 
                                    SHARE_CONTEXT, 
                                    system_sharing_key) != 0) {
            // error decrypting!
            return 0;
        }
    }

    if(song_status == 0) {
        //mb_printf("You are not authorized to play this song!");
        //we can only play the preview...so call play_preview
        return 0;
    }

    //everything looks good!
    return 1;
}

/*
void play_preview();
    Plays a song preview and looks for play-time commands. Upon calling, ./miPod 
    should have loaded the appropriate .drm.p file into cmd channel.
*/
void play_preview() {

    // load some metadata...
    s.song_md.song_id = c->song_s.song_id;

    //make a buffer to hold the song key
    //u8 key[SONG_KEY_SZ] = system_preview_AES_key;

    //now we have the key...
    //
    //integrity check over whole song
    //...
    //...
    //...
    //
    //verify block, write block to DMA, loop
    //...
    //...
    //...
    
    u32 counter = 0, rem, cp_num, cp_xfil_cnt, offset, dma_cnt, length, *fifo_fill;

    mb_printf("Reading Audio File...");
    //load_song_md();

    // get WAV length
    length = c->song.wav_size;
    mb_printf("Song length = %dB", length);

    // truncate song if locked
    if (length > PREVIEW_SZ && is_locked()) {
        length = PREVIEW_SZ;
        mb_printf("Song is locked.  Playing only %ds = %dB\r\n",
                   PREVIEW_TIME_SEC, PREVIEW_SZ);
    } else {
        mb_printf("Song is unlocked. Playing full song\r\n");
    }

    rem = length;
    fifo_fill = (u32 *)FIFO_COUNT_ADDR;
    
    // write entire file to two-block codec fifo
    // writes to one block while the other is being played
    set_playing();
    while(rem > 0) {

        // check for interrupt to stop playback
        while (InterruptProcessed) {

            // disable interrupts
            DisableInterruptSystem(); //what happens after [double-DisableInterrups]? Reminder-to-self: check these logicz
            InterruptProcessed = FALSE;

            switch (c->cmd) {
            case PAUSE:
                mb_printf("Pausing... \r\n");
                set_paused();
                EnableInterruptSystem();
                while (!InterruptProcessed) continue; //what happens after [pause, invalid command, pause]? Reminder-to-self: check these logicz
                break;
            case PLAY:
                mb_printf("Resuming... \r\n");
                set_playing();
                break;
            case STOP:
                mb_printf("Stopping playback...");
                return;
            case RESTART:
                mb_printf("Restarting song... \r\n");
                rem = length; // reset song counter
                set_playing();
            default:
                break;
            }
        }

        // calculate write size and offset
        cp_num = (rem > CHUNK_SZ) ? CHUNK_SZ : rem;
        offset = (counter++ % 2 == 0) ? 0 : CHUNK_SZ;

        // do first mem cpy here into DMA BRAM
        Xil_MemCpy((void *)(DMA_MM2S_ADDR + offset),
                   (void *)(get_drm_song(c->song) + length - rem),
                   (u32)(cp_num));

        //DO INTEGRITY CHECKS HERE

        cp_xfil_cnt = cp_num;

        while (cp_xfil_cnt > 0) {

            // polling while loop to wait for DMA to be ready
            // DMA must run first for this to yield the proper state
            // rem != length checks for first run
            while (XAxiDma_Busy(&sAxiDma, XAXIDMA_DMA_TO_DEVICE)
                   && rem != length && *fifo_fill < (FIFO_CAP - 32));

            // do DMA
            dma_cnt = (FIFO_CAP - *fifo_fill > cp_xfil_cnt)
                      ? FIFO_CAP - *fifo_fill
                      : cp_xfil_cnt;
            fnAudioPlay(sAxiDma, offset, dma_cnt);
            cp_xfil_cnt -= dma_cnt;
        }

        rem -= cp_num;
    }
}


/*
void play_song();
    Plays a song and looks for play-time commands. Upon calling, ./miPod should
    have loaded the appropriate .drm file into cmd.
*/
void play_song() {
    

    //now we have the key in s.key...
    //
    //
    //petalinux must load full song now
    //
    //integrity check over whole song
    //...
    //...
    //...
    //
    //verify block, write block to DMA, loop
    //...
    //...
    //...
    
    u32 counter = 0, rem, cp_num, cp_xfil_cnt, offset, dma_cnt, length, *fifo_fill;

    mb_printf("Reading Audio File...");
    //load_song_md();

    // get WAV length
    length = c->song.wav_size;
    mb_printf("Song length = %dB", length);

    // truncate song if locked
    if (length > PREVIEW_SZ && is_locked()) {
        length = PREVIEW_SZ;
        mb_printf("Song is locked.  Playing only %ds = %dB\r\n",
                   PREVIEW_TIME_SEC, PREVIEW_SZ);
    } else {
        mb_printf("Song is unlocked. Playing full song\r\n");
    }

    rem = length;
    fifo_fill = (u32 *)FIFO_COUNT_ADDR;
	
    // write entire file to two-block codec fifo
    // writes to one block while the other is being played
    set_playing();
    while(rem > 0) {

        // check for interrupt to stop playback
        while (InterruptProcessed) {

            // disable interrupts
            DisableInterruptSystem(); //what happens after [double-DisableInterrups]? Reminder-to-self: check these logicz
            InterruptProcessed = FALSE;

            switch (c->cmd) {
            case PAUSE:
                mb_printf("Pausing... \r\n");
                set_paused();
                EnableInterruptSystem();
                while (!InterruptProcessed) continue; //what happens after [pause, invalid command, pause]? Reminder-to-self: check these logicz
                break;
            case PLAY:
                mb_printf("Resuming... \r\n");
                set_playing();
                break;
            case STOP:
                mb_printf("Stopping playback...");
                return;
            case RESTART:
                mb_printf("Restarting song... \r\n");
                rem = length; // reset song counter
                set_playing();
            default:
                break;
            }
        }

        // calculate write size and offset
        cp_num = (rem > CHUNK_SZ) ? CHUNK_SZ : rem;
        offset = (counter++ % 2 == 0) ? 0 : CHUNK_SZ;

        // do first mem cpy here into DMA BRAM
        Xil_MemCpy((void *)(DMA_MM2S_ADDR + offset),
                   (void *)(get_drm_song(c->song) + length - rem),
                   (u32)(cp_num));

        //DO INTEGRITY CHECKS HERE

        cp_xfil_cnt = cp_num;

        while (cp_xfil_cnt > 0) {

            // polling while loop to wait for DMA to be ready
            // DMA must run first for this to yield the proper state
            // rem != length checks for first run
            while (XAxiDma_Busy(&sAxiDma, XAXIDMA_DMA_TO_DEVICE)
                   && rem != length && *fifo_fill < (FIFO_CAP - 32));

            // do DMA
            dma_cnt = (FIFO_CAP - *fifo_fill > cp_xfil_cnt)
                      ? FIFO_CAP - *fifo_fill
                      : cp_xfil_cnt;
            fnAudioPlay(sAxiDma, offset, dma_cnt);
            cp_xfil_cnt -= dma_cnt;
        }

        rem -= cp_num;
    }
}


// removes DRM data from song for digital out
void digital_out() {

    //Waiting to implement this until the hardware is finalized...

    // remove metadata size from file and chunk sizes
    c->song.file_size -= c->song.md.md_size;
    c->song.wav_size -= c->song.md.md_size;

    if (is_locked() && PREVIEW_SZ < c->song.wav_size) {
        mb_printf("Only playing 30 seconds");
        c->song.file_size -= c->song.wav_size - PREVIEW_SZ;
        c->song.wav_size = PREVIEW_SZ;
    }

    // move WAV file up in buffer, skipping metadata
    mb_printf(MB_PROMPT "Dumping song (%dB)...", c->song.wav_size);
    memmove((void *)&c->song.md, (void *)get_drm_song(c->song), c->song.wav_size);

    mb_printf("Song dump finished\r\n");
}


//////////////////////// MAIN ////////////////////////


int main() {
    u32 status;

    init_platform();
    microblaze_register_handler((XInterruptHandler)myISR, (void *)0);
    microblaze_enable_interrupts();

    // Initialize the interrupt controller driver so that it is ready to use.
    status = XIntc_Initialize(&InterruptController, XPAR_INTC_0_DEVICE_ID);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Set up the Interrupt System.
    status = SetUpInterruptSystem(&InterruptController, (XInterruptHandler)myISR);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Congigure the DMA
    status = fnConfigDma(&sAxiDma);
    if(status != XST_SUCCESS) {
        mb_printf("DMA configuration ERROR\r\n");
        return XST_FAILURE;
    }

    // Start the LED
    enableLED(led);
    set_stopped();

    // clear command channel
    memset((void*)c, 0, sizeof(cmd_channel));

    if (hydro_init() != 0) {
        mb_printf("libhydrogen could not be initialized!");
    } else {
        mb_printf("libhydrogen has been initialized");
    }

    mb_printf("Audio DRM Module has Booted\n\r");

    // Handle commands forever
    while(1) {

        // wait for interrupt to start
        if (InterruptProcessed) {
            set_working(); 
            
            // disable interrupts
            DisableInterruptSystem();
            InterruptProcessed = FALSE;

            // c->cmd is set by the miPod player
            switch (c->cmd) {
            case LOGIN:
                set_working();
                login();
                break;
            case LOGOUT:
                set_working();
                logout();
                break;
            case SHARE:
                set_working();
                share_song();
                break;
            case PLAY:
                set_working();
                if (!song_auth()) {
                    EnableInterruptSystem();
                    set_paused();
                    //wait for interrupt
                    while(!InterruptProcessed) continue;
                    play_preview();
                } else {
                    EnableInterruptSystem();
                    set_playing();
                    //wait for interrupt
                    while(!InterruptProcessed) continue;
                    play_song();
                }

                mb_printf("Done Playing Song\r\n");
                DisableInterruptSystem();
                break;
            case DIGITAL_OUT:
                set_working();
                digital_out();
                break;
            default:
                //invalid command...
                break;
            }

            // reenable interrupts
            EnableInterruptSystem();

            // reset statuses and sleep to allow player to recognize WORKING state
            strncpy((char *)c->username, s.username, USERNAME_SZ);
            c->login_status = s.logged_in;
            usleep(500);
            set_stopped();
        }
    }

    cleanup_platform();
    return 0;
}
