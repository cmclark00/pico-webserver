#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"

#include "tusb_lwip_glue.h"
#include "pokemon_parser.h"
#include "lwip/apps/httpd.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define LED_PIN     25

// Maximum file upload size (128KB should be enough for any Pokemon save file)
#define MAX_UPLOAD_SIZE 131072

// Buffer to hold uploaded file
static uint8_t upload_buffer[MAX_UPLOAD_SIZE];
static size_t upload_size = 0;
static bool upload_in_progress = false;
static bool boundary_found = false;
static char boundary[100] = {0};
static bool headers_skipped = false;

// Buffer to hold generated HTML
static char *generated_html = NULL;

// let our webserver do some dynamic handling
static const char *cgi_toggle_led(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    gpio_put(LED_PIN, !gpio_get(LED_PIN));
    return "/index.html";
}

static const char *cgi_reset_usb_boot(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    reset_usb_boot(0, 0);
    return "/index.html";
}

// Process Pokemon save file uploads
static const char *cgi_analyze_pokemon(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    if (upload_size > 0) {
        // Process the previously uploaded file
        TrainerData trainer;
        memset(&trainer, 0, sizeof(TrainerData));
        
        // Detect the game generation
        int generation = detect_pokemon_generation(upload_buffer, upload_size);
        bool parsed = false;
        
        // Parse according to generation
        switch (generation) {
            case POKEMON_GEN1:
                parsed = parse_gen1_save(upload_buffer, upload_size, &trainer);
                break;
            case POKEMON_GEN2:
                parsed = parse_gen2_save(upload_buffer, upload_size, &trainer);
                break;
            case POKEMON_GEN3:
                parsed = parse_gen3_save(upload_buffer, upload_size, &trainer);
                break;
        }
        
        // Generate HTML if parsing succeeded
        if (parsed) {
            // Free previous HTML if it exists
            if (generated_html) {
                free(generated_html);
                generated_html = NULL;
            }
            
            // Generate new HTML
            generated_html = generate_pokemon_html(&trainer);
        }
        
        // Reset for next upload
        upload_size = 0;
        upload_in_progress = false;
        boundary_found = false;
        headers_skipped = false;
        memset(boundary, 0, sizeof(boundary));
    } else {
        // No upload data, use a sample dataset
        TrainerData trainer;
        memset(&trainer, 0, sizeof(TrainerData));
        
        // Sample trainer data
        strncpy(trainer.name, "ASH", MAX_NAME_LENGTH - 1);
        trainer.money = 3500;
        trainer.badges = 0x7; // First 3 badges
        trainer.game_version = POKEMON_GEN1;
        trainer.play_time = 3600 * 10 + 30 * 60; // 10 hours, 30 minutes
        
        // Sample party with 3 Pokemon
        trainer.party_count = 3;
        
        // Pikachu
        trainer.party[0].species_id = 25;
        strncpy(trainer.party[0].nickname, "PIKA", MAX_NAME_LENGTH - 1);
        trainer.party[0].level = 25;
        trainer.party[0].current_hp = 65;
        trainer.party[0].max_hp = 65;
        trainer.party[0].attack = 55;
        trainer.party[0].defense = 40;
        trainer.party[0].speed = 90;
        trainer.party[0].special_attack = 50;
        trainer.party[0].special_defense = 50;
        strncpy(trainer.party[0].moves[0], "Thunderbolt", MAX_NAME_LENGTH - 1);
        strncpy(trainer.party[0].moves[1], "Quick Attack", MAX_NAME_LENGTH - 1);
        strncpy(trainer.party[0].moves[2], "Thunder Wave", MAX_NAME_LENGTH - 1);
        strncpy(trainer.party[0].moves[3], "Slam", MAX_NAME_LENGTH - 1);
        for (int i = 0; i < 4; i++) trainer.party[0].move_pp[i] = 20;
        
        // Charmeleon
        trainer.party[1].species_id = 5;
        strncpy(trainer.party[1].nickname, "CHARMY", MAX_NAME_LENGTH - 1);
        trainer.party[1].level = 22;
        trainer.party[1].current_hp = 62;
        trainer.party[1].max_hp = 62;
        trainer.party[1].attack = 53;
        trainer.party[1].defense = 43;
        trainer.party[1].speed = 65;
        trainer.party[1].special_attack = 60;
        trainer.party[1].special_defense = 60;
        strncpy(trainer.party[1].moves[0], "Ember", MAX_NAME_LENGTH - 1);
        strncpy(trainer.party[1].moves[1], "Slash", MAX_NAME_LENGTH - 1);
        strncpy(trainer.party[1].moves[2], "Growl", MAX_NAME_LENGTH - 1);
        strncpy(trainer.party[1].moves[3], "Leer", MAX_NAME_LENGTH - 1);
        for (int i = 0; i < 4; i++) trainer.party[1].move_pp[i] = 25;
        
        // Bulbasaur
        trainer.party[2].species_id = 1;
        strncpy(trainer.party[2].nickname, "BULBY", MAX_NAME_LENGTH - 1);
        trainer.party[2].level = 18;
        trainer.party[2].current_hp = 51;
        trainer.party[2].max_hp = 51;
        trainer.party[2].attack = 32;
        trainer.party[2].defense = 33;
        trainer.party[2].speed = 30;
        trainer.party[2].special_attack = 40;
        trainer.party[2].special_defense = 40;
        strncpy(trainer.party[2].moves[0], "Vine Whip", MAX_NAME_LENGTH - 1);
        strncpy(trainer.party[2].moves[1], "Leech Seed", MAX_NAME_LENGTH - 1);
        strncpy(trainer.party[2].moves[2], "Tackle", MAX_NAME_LENGTH - 1);
        strncpy(trainer.party[2].moves[3], "Growl", MAX_NAME_LENGTH - 1);
        for (int i = 0; i < 4; i++) trainer.party[2].move_pp[i] = 30;
        
        // Generate HTML
        if (generated_html) {
            free(generated_html);
            generated_html = NULL;
        }
        generated_html = generate_pokemon_html(&trainer);
    }
    
    return "/analyze_pokemon.html";
}

// Helper function to find boundary in multipart form data
static char* find_boundary(const char* data, int data_len) {
    // Look for "boundary=" in the data
    const char* boundary_marker = "boundary=";
    char* boundary_start = strstr(data, boundary_marker);
    
    if (!boundary_start) {
        return NULL;
    }
    
    // Skip past "boundary="
    boundary_start += strlen(boundary_marker);
    
    // Check if the boundary is quoted
    if (*boundary_start == '"') {
        boundary_start++;
        
        // Find the closing quote
        char* boundary_end = strchr(boundary_start, '"');
        if (!boundary_end || (boundary_end - boundary_start) >= 98) {
            return NULL;
        }
        
        // Copy the boundary
        int boundary_len = boundary_end - boundary_start;
        strncpy(boundary, boundary_start, boundary_len);
        boundary[boundary_len] = '\0';
    } else {
        // Find the end of the boundary (usually a CR or LF)
        char* boundary_end = strpbrk(boundary_start, "\r\n");
        if (!boundary_end || (boundary_end - boundary_start) >= 98) {
            return NULL;
        }
        
        // Copy the boundary
        int boundary_len = boundary_end - boundary_start;
        strncpy(boundary, boundary_start, boundary_len);
        boundary[boundary_len] = '\0';
    }
    
    return boundary;
}

// File upload receive callback
err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                       u16_t http_request_len, int content_len, char *response_uri,
                       u16_t response_uri_len, u8_t *post_auto_wnd)
{
    LWIP_UNUSED_ARG(http_request);
    LWIP_UNUSED_ARG(http_request_len);
    LWIP_UNUSED_ARG(content_len);
    LWIP_UNUSED_ARG(post_auto_wnd);

    // Check if the URI is our upload target
    if (strncmp(uri, "/analyze_pokemon", 16) == 0) {
        // Start a new upload
        upload_in_progress = true;
        upload_size = 0;
        boundary_found = false;
        headers_skipped = false;
        memset(boundary, 0, sizeof(boundary));
        
        // Set the response URI
        if (response_uri != NULL) {
            strncpy(response_uri, "/analyze_pokemon.html", response_uri_len);
        }
        
        return ERR_OK;
    }
    
    return ERR_ARG;
}

err_t httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len)
{
    LWIP_UNUSED_ARG(connection);
    
    // Set a proper response URI
    if (response_uri != NULL) {
        strncpy(response_uri, "/analyze_pokemon.html", response_uri_len);
    }
    
    return ERR_OK;
}

// File upload receive callback
err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
    // Point to the data in the packet
    char *data = (char *)p->payload;
    int data_len = p->len;
    
    if (!upload_in_progress) {
        // First packet - initialize the upload
        upload_in_progress = true;
        upload_size = 0;
        boundary_found = false;
        headers_skipped = false;
        
        // Extract the boundary from the Content-Type header
        if (!boundary_found) {
            if (find_boundary(data, data_len)) {
                boundary_found = true;
            }
        }
    }
    
    if (boundary_found && !headers_skipped) {
        // Find the start of file data (after the headers)
        char* file_start = strstr(data, "\r\n\r\n");
        if (file_start) {
            file_start += 4; // Skip past \r\n\r\n
            headers_skipped = true;
            
            // Calculate remaining data in this packet
            int remaining = data_len - (file_start - data);
            
            // Look for the end boundary in this segment
            char end_boundary[103]; // --boundary--\r\n (plus null)
            snprintf(end_boundary, sizeof(end_boundary), "--%s--", boundary);
            char* end_marker = strstr(file_start, end_boundary);
            
            if (end_marker) {
                // We have the end marker in this packet
                int content_len = end_marker - file_start;
                if (upload_size + content_len <= MAX_UPLOAD_SIZE) {
                    memcpy(upload_buffer + upload_size, file_start, content_len);
                    upload_size += content_len;
                }
                upload_in_progress = false;
            } else {
                // No end marker, copy all remaining data
                if (upload_size + remaining <= MAX_UPLOAD_SIZE) {
                    memcpy(upload_buffer + upload_size, file_start, remaining);
                    upload_size += remaining;
                }
            }
        }
    } else if (headers_skipped) {
        // Subsequent packets after headers are skipped
        
        // Check for the end boundary
        char end_boundary[103]; // --boundary--\r\n (plus null)
        snprintf(end_boundary, sizeof(end_boundary), "--%s--", boundary);
        char* end_marker = strstr(data, end_boundary);
        
        if (end_marker) {
            // We found the end marker in this packet
            int content_len = end_marker - data;
            if (upload_size + content_len <= MAX_UPLOAD_SIZE) {
                memcpy(upload_buffer + upload_size, data, content_len);
                upload_size += content_len;
            }
            upload_in_progress = false;
        } else {
            // No end marker, copy the entire packet
            if (upload_size + data_len <= MAX_UPLOAD_SIZE) {
                memcpy(upload_buffer + upload_size, data, data_len);
                upload_size += data_len;
            }
        }
    }
    
    return ERR_OK;
}

static const tCGI cgi_handlers[] = {
    {
        "/toggle_led",
        cgi_toggle_led
    },
    {
        "/reset_usb_boot",
        cgi_reset_usb_boot
    },
    {
        "/analyze_pokemon",
        cgi_analyze_pokemon
    }
};

int main()
{
    // Initialize tinyusb, lwip, dhcpd and httpd
    init_lwip();
    wait_for_netif_is_up();
    dhcpd_init();
    httpd_init();
    http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
    
    // For toggle_led
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true)
    {
        tud_task();
        service_traffic();
    }

    return 0;
}
