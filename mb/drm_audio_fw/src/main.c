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
        memcpy(user_name, (char *)USER_NAMES[uid], USER_NAME_SZ);
        return TRUE;
    }
    memcpy(user_name, "<unknown user>", USER_NAME_SZ);
    return FALSE;
}


// looks up the username corresponding to the uid
u8 username_to_uid(char *user_name) {
    for (int i = 0; i < NUM_USERS; i++) {
        if (!strncmp(user_name, USER_NAMES[i], USER_NAME_SZ)) {
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
        memcpy((void*)c->username, s.username, USER_NAME_SZ);
        memcpy((void*)c->pin, s.pin, MAX_PIN_SZ);
    } else {
        //Create temporary buffer
        char user_buffer[USER_NAME_SZ];
        char pin_buffer[MAX_PIN_SZ];
        //copy to temporary buffer
        memcpy(user_buffer, (void*)c->username, USER_NAME_SZ);
        memcpy(pin_buffer, (void*)c->pin, MAX_PIN_SZ);

        for (int i = 0; i < NUM_PROVISIONED_USERS; i++) {    
            // search for matching username
            if (!strncmp(user_buffer, USERNAMES[i], USER_NAME_SZ)) {
                // check if pin matches
                if(hydro_pwhash_verify(PIN_HASHES[i]//const uint8_t  stored[hydro_pwhash_STOREDBYTES], //WE SHOULDN'T NEED TO CAST THESE, BUT DOING SO WILL PRB REDUCE COMPILER WARNINGS...
                            pin_buffer, //const char* passwd //should this be &?
                            PIN_MAX_SZ, //TREAT ALL PASSWORDS AS 0 PADDING (appended)
                            PIN_HASH_MASTER_KEY,//const uint8_t  master_key[hydro_pwhash_MASTERKEYBYTES],
                            PIN_HASH_OPSLIMIT, PIN_HASH_MEMLIMIT, PIN_HASH_THREADS)) {
                    
                    //update states
                    memcpy(s.username, user_buffer, USER_NAME_SZ);
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
        memset((void*)c->username, 0, USER_NAME_SZ);
        memset((void*)c->pin, 0, MAX_PIN_SZ);
        memset(s.username, 0, USER_NAME_SZ);
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
        memset((void*)c->username, 0, USER_NAME_SZ);
        memset((void*)c->pin, 0, MAX_PIN_SZ);
        memset(s.username, 0, USER_NAME_SZ);
        memset(s.pin, 0, MAX_PIN_SZ);
    } else {
        mb_printf("Not logged in\r\n");
    }
}

/*
void share_song();
    Adds a user to the song's list of users. Upon calling, ./miPod should
    have loaded the appropriate .drm.s file into cmd.
*/
void share_song() {
    // grab this for later...
    char *sharee_str = (char *)c->username;
    // make a buffer to hold the whole .drm.s file
    song_s *drm_s_buffer = (song_s *)malloc(DRM_S_SZ);
    if(drm_s_buffer == NULL) {
        // malloc failed... how should we handle this better?
        return;
    }

    // copy the whole enchilada
    memcpy(drm_s_buffer, c->song_s, DRM_S_SZ);
    // integrity check
    //...
    //...
    //...
    // load the song_s metadata into the mb state
    s.song_md.region_vector = drm_s_buffer->region_vector;
    s.song_md.song_id = drm_s_buffer->song_id;
    s.song_md.owner_id = drm_s_buffer->owner_id;
    s.song_md.user_vector = drm_s_buffer->user_vector;

    // do initial permission checks...
    // check for authorized user
    if (!s.logged_in) {
        //mb_printf("No user logged in");
        free(drm_s_buffer);
        return;
    } 
    // check if provisioned for matching region
    if((s.song_md.region_vector & PROVISIONED_RIDS) != 0) {
        //mb_printf("Song not provisioned for this region!");
        free(drm_s_buffer);
        return;
    }
    // check if NOT owned by current user
    if(s.uid != s.song_md.owner_id) {
        //mb_printf("Current user does not own this song!");
        free(drm_s_buffer);
        return;
    }
    // check if shared with current user
    if(check_bit(s.song_md.user_vector, s.uid)) {
        //mb_printf("Song is already shared with this user!");
        free(drm_s_buffer);
        return;
    }

    //make some buffers
    u8 metakey[PIN_MAX_SZ + REGION_SECRET_SZ]; // metakey = pin+regionsecret
    u8 region_secret[REGION_SECRET_SZ];
    u8 key[SONG_KEY_SZ];
    u8 enc_key[ENC_SONG_KEY_SZ]; 

    // decrypt region secret //maybe do this here, instead of branching function call?
    int region_id = -1;
    for (int i = 0; i < MAX_REGIONS; i++) {
        //find an index of a region key collision
        if(check_bit(s.song_md.region_vector,i) && check_bit(PROVISIONED_RIDS,i)){
            region_id = i;
            // run the decryption
            if (hydro_secretbox_decrypt(metakey+PIN_MAX_SZ, 
                                        drm_s_buffer->region_secrets[i], 
                                        REGION_SECRET_SZ + hydro_secretbox_HEADERBYTES, 
                                        0, 
                                        REGION_CONTEXT, 
                                        system_region_keys[i]) != 0) {
                // error decrypting!
                free(drm_s_buffer);
                return;
            }
            break;
        }
    }

    if(region_id < 0){
        // Invalid decryption... abort!
        free(drm_s_buffer);
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
        return;
    }

    // get who we are sharing with
    u8 sharee = username_to_uid(sharee_str);
    if(sharee == 255) {
        // User does not exist!
        free(drm_s_buffer);
        return;
    }

    // encrypt song key
    hydro_secretbox_encrypt(enc_key, key, ENC_SONG_KEY_SZ, 0, SHARE_CONTEXT, system_sharing_key);

    // store modified data in .drm.s
    memcpy(drm_s_buffer->shared_secrets[sharee].packet1, packet1, hydro_kx_N_PACKET1BYTES);
    memcpy(drm_s_buffer->shared_secrets[sharee].songkey, enc_key, ENC_SONG_KEY_SZ);
    set_bit(drm_s_buffer->user_vector, sharee);

    //resign .drm.s
    //TODO
    //..
    //..
    //(write calculated signature directly to s)

    // write back to cmd
    memcpy(c->song_s, drm_s_buffer, DRM_S_SZ);
    //petalinux should write this back! (Lazily...) Perhaps we write a special byte here (and on fails) to signal we are done?
    free(drm_s_buffer);
    return;
}


// plays a song and looks for play-time commands
void play_song() {

    //Lots to do here... and in such little time, uh oh.
    //
    //check if owner or shared
    //derive key
    //verify whole song
    //verify block, write block, loop
    
    u32 counter = 0, rem, cp_num, cp_xfil_cnt, offset, dma_cnt, length, *fifo_fill;

    mb_printf("Reading Audio File...");
    load_song_md();

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
    fifo_fill = (u32 *)XPAR_FIFO_COUNT_AXI_GPIO_0_BASEADDR;
	
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
                interruptable = TRUE;
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
        Xil_MemCpy((void *)(XPAR_MB_DMA_AXI_BRAM_CTRL_0_S_AXI_BASEADDR + offset),
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

    mb_printf("Audio DRM Module has Booted\n\r");

    // Handle commands forever
    while(1) {

        // wait for interrupt to start
        if (InterruptProcessed) {
            //set_working(); //optimized out (:
            
            // disable interrupts
            DisableInterruptSystem();
            InterruptProcessed = FALSE;

            // c->cmd is set by the miPod player
            switch (c->cmd) {
            case LOGIN:
                login();
                break;
            case LOGOUT:
                logout();
                break;
            case QUERY_PLAYER://implement in ./miPod, then remove this
                query_player(); 
                break;
            case QUERY_SONG://implement in ./miPod, then remove this
                query_song(); 
                break;
            case SHARE:
                share_song();
                break;
            case PLAY:
                play_song();
                mb_printf("Done Playing Song\r\n");
                break;
            case DIGITAL_OUT:
                digital_out();
                break;
            default:
                //invalid command...
                break;
            }

            // reenable interrupts
            EnableInterruptSystem();

            // reset statuses and sleep to allow player to recognize WORKING state
            strncpy((char *)c->username, s.username, USER_NAME_SZ);
            c->login_status = s.logged_in;
            usleep(500);
            set_stopped();
        }
    }

    cleanup_platform();
    return 0;
}
