#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"

#include "tusb_lwip_glue.h"
#include "lwip/apps/httpd.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Define MIME types for various file extensions
#define HTTP_HDR_HTML              "Content-Type: text/html\r\n\r\n"
#define HTTP_HDR_CONTENT_TYPE_JS   "Content-Type: application/javascript\r\n\r\n"
#define HTTP_HDR_CONTENT_TYPE_CSS  "Content-Type: text/css\r\n\r\n"

#define LED_PIN     25

// Custom HTTP header generation for different file types
const char *get_http_header(const char *uri) {
    // Check file extension
    const char *ext = strrchr(uri, '.');
    
    if (ext) {
        if (strcmp(ext, ".js") == 0) {
            return HTTP_HDR_CONTENT_TYPE_JS;
        } else if (strcmp(ext, ".css") == 0) {
            return HTTP_HDR_CONTENT_TYPE_CSS;
        }
    }
    
    // Default to HTML for other files
    return HTTP_HDR_HTML;
}

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

// Handler for JavaScript files
static const char *cgi_serve_js(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    // This is a special handler that will set content type to JavaScript
    // The actual file will be served by the filesystem, but with the right MIME type
    return "/js/pokemon-parser.js";
}

// Handler for Pokemon data JavaScript file
static const char *cgi_serve_pokemon_data(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    // This is a special handler for the pokemon-data.js file
    return "/js/pokemon-data.js";
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
        "/js/pokemon-parser.js",
        cgi_serve_js
    },
    {
        "/js/pokemon-data.js",
        cgi_serve_pokemon_data
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
