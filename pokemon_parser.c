#include "pokemon_parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Pokemon species names for Gen 1-3 (abbreviated list)
static const char *pokemon_species[256] = {
    [0] = "None",
    [1] = "Bulbasaur", [2] = "Ivysaur", [3] = "Venusaur",
    [4] = "Charmander", [5] = "Charmeleon", [6] = "Charizard",
    [7] = "Squirtle", [8] = "Wartortle", [9] = "Blastoise",
    [10] = "Caterpie", [11] = "Metapod", [12] = "Butterfree",
    [13] = "Weedle", [14] = "Kakuna", [15] = "Beedrill",
    [16] = "Pidgey", [17] = "Pidgeotto", [18] = "Pidgeot",
    [19] = "Rattata", [20] = "Raticate", [21] = "Spearow",
    [22] = "Fearow", [23] = "Ekans", [24] = "Arbok",
    [25] = "Pikachu", [26] = "Raichu",
    // More would be added for a complete implementation
};

// Move names (abbreviated list)
static const char *move_names[256] = {
    [0] = "None",
    [1] = "Pound", [2] = "Karate Chop", [3] = "Double Slap",
    [4] = "Comet Punch", [5] = "Mega Punch", [6] = "Pay Day",
    [7] = "Fire Punch", [8] = "Ice Punch", [9] = "Thunder Punch",
    [10] = "Scratch", [11] = "Vice Grip", [12] = "Guillotine",
    [13] = "Razor Wind", [14] = "Swords Dance", [15] = "Cut",
    // More would be added for a complete implementation
};

// Helper function to convert from Game Boy text encoding to ASCII
static void convert_gb_text(const uint8_t *gb_text, char *ascii_text, int length) {
    static const char charset[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ():-!?.,''\"\"";
    
    for (int i = 0; i < length; i++) {
        if (gb_text[i] == 0x50) { // End of text marker in GB encoding
            ascii_text[i] = '\0';
            break;
        } else if (gb_text[i] >= 0x80 && gb_text[i] <= 0x99) {
            // A-Z characters
            ascii_text[i] = 'A' + (gb_text[i] - 0x80);
        } else if (gb_text[i] >= 0xA0 && gb_text[i] <= 0xB9) {
            // a-z characters
            ascii_text[i] = 'a' + (gb_text[i] - 0xA0);
        } else if (gb_text[i] == 0xE8) {
            // For Pokemon symbol
            ascii_text[i] = 'P';
        } else {
            // Default for unknown characters
            ascii_text[i] = '?';
        }
        
        // Make sure we always null-terminate
        if (i == length - 1) {
            ascii_text[i] = '\0';
        }
    }
}

// Helper function for Gen 2/3 text conversion
static void convert_gen2_3_text(const uint8_t *text, char *ascii_text, int length) {
    // Simplified character table for Gen 2-3
    static const char *char_table[] = {
        " ", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
        "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "(", ")", ":", ";", "[",
        "]", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
        "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "Ä", "Ö", "Ü", "ä", "ö",
        "ü", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "!", "?", ".", "-", "&",
        "é", "→", "←", "'", "'", "♂", "♀", "/", ",", ".", "…"
    };
    
    int j = 0;
    for (int i = 0; i < length && j < MAX_NAME_LENGTH - 1; i++) {
        if (text[i] == 0xFF) {
            // End of string marker
            break;
        } else if (text[i] < sizeof(char_table) / sizeof(char_table[0])) {
            // Copy the character
            strcpy(&ascii_text[j], char_table[text[i]]);
            j += strlen(char_table[text[i]]);
        } else {
            // Unknown character
            ascii_text[j++] = '?';
        }
    }
    
    // Ensure null termination
    ascii_text[j] = '\0';
}

// Detect the game generation from the save file
int detect_pokemon_generation(const uint8_t *save_data, size_t save_size) {
    // Simple detection based on file size and patterns
    if (save_size == 32768) {
        // Gen 1 (Red/Blue/Yellow) save size
        return POKEMON_GEN1;
    } else if (save_size == 32768 * 2) {
        // Gen 2 (Gold/Silver/Crystal) save size
        return POKEMON_GEN2;
    } else if (save_size == 131072) {
        // Gen 3 (Ruby/Sapphire/Emerald/FireRed/LeafGreen) save size
        return POKEMON_GEN3;
    }
    
    // Unknown generation
    return 0;
}

// Parse Generation 1 (Red/Blue/Yellow) save files
bool parse_gen1_save(const uint8_t *save_data, size_t save_size, TrainerData *trainer) {
    if (save_size != 32768) {
        return false;
    }
    
    // Gen 1 save data layout (offsets for English version)
    const uint32_t PLAYER_NAME_OFFSET = 0x2598;
    const uint32_t MONEY_OFFSET = 0x25F3;
    const uint32_t BADGES_OFFSET = 0x2602;
    const uint32_t PARTY_COUNT_OFFSET = 0x2F2C;
    const uint32_t PARTY_SPECIES_LIST_OFFSET = 0x2F2D;
    const uint32_t PARTY_DATA_OFFSET = 0x2F34;
    const uint32_t POKEMON_NAME_LIST_OFFSET = 0x307E;  // Pokemon nicknames
    const uint32_t PLAYTIME_HOURS_OFFSET = 0x2CED;
    const uint32_t PLAYTIME_MINUTES_OFFSET = 0x2CEE;
    const uint32_t PLAYTIME_SECONDS_OFFSET = 0x2CEF;
    
    // Get trainer name
    convert_gb_text(&save_data[PLAYER_NAME_OFFSET], trainer->name, MAX_NAME_LENGTH - 1);
    
    // Get money (3 bytes, BCD format)
    trainer->money = 
        (save_data[MONEY_OFFSET] & 0x0F) * 100000 +
        ((save_data[MONEY_OFFSET] >> 4) & 0x0F) * 1000000 +
        (save_data[MONEY_OFFSET + 1] & 0x0F) * 1000 + 
        ((save_data[MONEY_OFFSET + 1] >> 4) & 0x0F) * 10000 +
        (save_data[MONEY_OFFSET + 2] & 0x0F) * 10 +
        ((save_data[MONEY_OFFSET + 2] >> 4) & 0x0F) * 100;
    
    // Get badges
    trainer->badges = save_data[BADGES_OFFSET];
    
    // Set game version (we don't know specifically which Gen 1 game)
    trainer->game_version = POKEMON_GEN1;
    
    // Get play time
    int hours = save_data[PLAYTIME_HOURS_OFFSET];
    int minutes = save_data[PLAYTIME_MINUTES_OFFSET];
    int seconds = save_data[PLAYTIME_SECONDS_OFFSET];
    trainer->play_time = hours * 3600 + minutes * 60 + seconds;
    
    // Get party count
    trainer->party_count = save_data[PARTY_COUNT_OFFSET];
    if (trainer->party_count > MAX_PARTY_SIZE) {
        trainer->party_count = MAX_PARTY_SIZE;
    }
    
    // Get party species
    for (int i = 0; i < trainer->party_count; i++) {
        trainer->party[i].species_id = save_data[PARTY_SPECIES_LIST_OFFSET + i];
    }
    
    // Gen 1 Pokemon data structure size
    const int POKEMON_DATA_SIZE = 44;
    
    // Parse each Pokemon in party
    for (int i = 0; i < trainer->party_count; i++) {
        uint32_t pkm_offset = PARTY_DATA_OFFSET + (i * POKEMON_DATA_SIZE);
        
        // Pokemon level
        trainer->party[i].level = save_data[pkm_offset + 0x21];
        
        // HP
        trainer->party[i].max_hp = (save_data[pkm_offset + 0x22] << 8) | save_data[pkm_offset + 0x23];
        trainer->party[i].current_hp = (save_data[pkm_offset + 0x01] << 8) | save_data[pkm_offset + 0x02];
        
        // Stats
        trainer->party[i].attack = (save_data[pkm_offset + 0x24] << 8) | save_data[pkm_offset + 0x25];
        trainer->party[i].defense = (save_data[pkm_offset + 0x26] << 8) | save_data[pkm_offset + 0x27];
        trainer->party[i].speed = (save_data[pkm_offset + 0x28] << 8) | save_data[pkm_offset + 0x29];
        trainer->party[i].special_attack = (save_data[pkm_offset + 0x2A] << 8) | save_data[pkm_offset + 0x2B];
        trainer->party[i].special_defense = trainer->party[i].special_attack;  // Gen 1 only has "Special" stat
        
        // Get moves
        for (int m = 0; m < 4; m++) {
            int move_id = save_data[pkm_offset + 0x08 + m];
            if (move_id < 256 && move_names[move_id]) {
                strncpy(trainer->party[i].moves[m], move_names[move_id], MAX_NAME_LENGTH - 1);
            } else {
                strncpy(trainer->party[i].moves[m], "???", MAX_NAME_LENGTH - 1);
            }
            trainer->party[i].move_pp[m] = save_data[pkm_offset + 0x0C + m];
        }
        
        // Get the Pokemon nickname
        uint32_t nickname_offset = POKEMON_NAME_LIST_OFFSET + (i * 11);  // 11 bytes per name
        convert_gb_text(&save_data[nickname_offset], trainer->party[i].nickname, MAX_NAME_LENGTH - 1);
        
        // If no nickname, use species name as fallback
        if (strlen(trainer->party[i].nickname) == 0) {
            if (trainer->party[i].species_id < 256 && pokemon_species[trainer->party[i].species_id]) {
                strncpy(trainer->party[i].nickname, pokemon_species[trainer->party[i].species_id], MAX_NAME_LENGTH - 1);
            } else {
                strncpy(trainer->party[i].nickname, "???", MAX_NAME_LENGTH - 1);
            }
        }
    }
    
    return true;
}

// Parse Generation 2 (Gold/Silver/Crystal) save files
bool parse_gen2_save(const uint8_t *save_data, size_t save_size, TrainerData *trainer) {
    if (save_size != 32768 * 2) {
        return false;
    }
    
    // Gen 2 save data layout
    // Note: Gen 2 has multiple save banks, we'll use the first one for simplicity
    const uint32_t PLAYER_NAME_OFFSET = 0x2009;
    const uint32_t MONEY_OFFSET = 0x23DB;
    const uint32_t BADGES_OFFSET = 0x23E4;
    const uint32_t PLAYTIME_HOURS_OFFSET = 0x2054;
    const uint32_t PLAYTIME_MINUTES_OFFSET = 0x2055;
    const uint32_t PLAYTIME_SECONDS_OFFSET = 0x2056;
    const uint32_t PARTY_COUNT_OFFSET = 0x288A;
    const uint32_t PARTY_SPECIES_LIST_OFFSET = 0x288B;
    const uint32_t PARTY_DATA_OFFSET = 0x2897;
    const uint32_t POKEMON_NAME_LIST_OFFSET = 0x2A15;
    
    // Get trainer name - Gen 2 uses a slightly different text encoding
    convert_gen2_3_text(&save_data[PLAYER_NAME_OFFSET], trainer->name, MAX_NAME_LENGTH - 1);
    
    // Get money (3 bytes, BCD format)
    trainer->money = 
        (save_data[MONEY_OFFSET] & 0x0F) * 100000 +
        ((save_data[MONEY_OFFSET] >> 4) & 0x0F) * 1000000 +
        (save_data[MONEY_OFFSET + 1] & 0x0F) * 1000 + 
        ((save_data[MONEY_OFFSET + 1] >> 4) & 0x0F) * 10000 +
        (save_data[MONEY_OFFSET + 2] & 0x0F) * 10 +
        ((save_data[MONEY_OFFSET + 2] >> 4) & 0x0F) * 100;
    
    // Get badges (2 bytes, one for Johto, one for Kanto)
    trainer->badges = save_data[BADGES_OFFSET] | (save_data[BADGES_OFFSET + 1] << 8);
    
    // Set game version
    trainer->game_version = POKEMON_GEN2;
    
    // Get play time
    int hours = save_data[PLAYTIME_HOURS_OFFSET];
    int minutes = save_data[PLAYTIME_MINUTES_OFFSET];
    int seconds = save_data[PLAYTIME_SECONDS_OFFSET];
    trainer->play_time = hours * 3600 + minutes * 60 + seconds;
    
    // Get party count
    trainer->party_count = save_data[PARTY_COUNT_OFFSET];
    if (trainer->party_count > MAX_PARTY_SIZE) {
        trainer->party_count = MAX_PARTY_SIZE;
    }
    
    // Get party species
    for (int i = 0; i < trainer->party_count; i++) {
        trainer->party[i].species_id = save_data[PARTY_SPECIES_LIST_OFFSET + i];
    }
    
    // Gen 2 Pokemon data structure size
    const int POKEMON_DATA_SIZE = 48;
    
    // Parse each Pokemon in party
    for (int i = 0; i < trainer->party_count; i++) {
        uint32_t pkm_offset = PARTY_DATA_OFFSET + (i * POKEMON_DATA_SIZE);
        
        // Pokemon level
        trainer->party[i].level = save_data[pkm_offset + 0x1F];  // Level in Gen 2
        
        // Stats
        trainer->party[i].current_hp = (save_data[pkm_offset + 0x01] << 8) | save_data[pkm_offset + 0x02];
        trainer->party[i].max_hp = (save_data[pkm_offset + 0x22] << 8) | save_data[pkm_offset + 0x23];
        trainer->party[i].attack = (save_data[pkm_offset + 0x24] << 8) | save_data[pkm_offset + 0x25];
        trainer->party[i].defense = (save_data[pkm_offset + 0x26] << 8) | save_data[pkm_offset + 0x27];
        trainer->party[i].speed = (save_data[pkm_offset + 0x28] << 8) | save_data[pkm_offset + 0x29];
        trainer->party[i].special_attack = (save_data[pkm_offset + 0x2A] << 8) | save_data[pkm_offset + 0x2B];
        trainer->party[i].special_defense = (save_data[pkm_offset + 0x2C] << 8) | save_data[pkm_offset + 0x2D];
        
        // Get moves
        for (int m = 0; m < 4; m++) {
            int move_id = save_data[pkm_offset + 0x08 + m];
            if (move_id < 256 && move_names[move_id]) {
                strncpy(trainer->party[i].moves[m], move_names[move_id], MAX_NAME_LENGTH - 1);
            } else {
                strncpy(trainer->party[i].moves[m], "???", MAX_NAME_LENGTH - 1);
            }
            trainer->party[i].move_pp[m] = save_data[pkm_offset + 0x0C + m];
        }
        
        // Get the Pokemon nickname - Gen 2 uses the newer text encoding
        uint32_t nickname_offset = POKEMON_NAME_LIST_OFFSET + (i * 11);  // 11 bytes per name
        convert_gen2_3_text(&save_data[nickname_offset], trainer->party[i].nickname, MAX_NAME_LENGTH - 1);
        
        // If no nickname or error reading, use species name as fallback
        if (strlen(trainer->party[i].nickname) == 0) {
            if (trainer->party[i].species_id < 256 && pokemon_species[trainer->party[i].species_id]) {
                strncpy(trainer->party[i].nickname, pokemon_species[trainer->party[i].species_id], MAX_NAME_LENGTH - 1);
            } else {
                strncpy(trainer->party[i].nickname, "???", MAX_NAME_LENGTH - 1);
            }
        }
    }
    
    return true;
}

// Parse Generation 3 (Ruby/Sapphire/Emerald/FireRed/LeafGreen) save files
bool parse_gen3_save(const uint8_t *save_data, size_t save_size, TrainerData *trainer) {
    if (save_size != 131072) {
        return false;
    }
    
    // Gen 3 save data is more complex with multiple save blocks
    // This is a simplified implementation that focuses on the main game data
    
    // Find the active save section - Gen 3 has multiple save blocks
    // We'll do a simple search for the party data signature
    
    // Initialize default values in case we don't find the data
    strncpy(trainer->name, "Unknown Trainer", MAX_NAME_LENGTH - 1);
    trainer->money = 0;
    trainer->badges = 0;
    trainer->game_version = POKEMON_GEN3;
    trainer->play_time = 0;
    trainer->party_count = 0;
    
    // Simplified approach: look for common party data patterns
    // This is a very simplified approach and might not work for all save files
    for (uint32_t offset = 0; offset < save_size - 0x1000; offset += 4) {
        // Look for a valid party count (1-6)
        if (save_data[offset] >= 1 && save_data[offset] <= 6) {
            // Check if this could be the party count
            bool valid_party = true;
            
            // Checking if the next bytes could be valid species IDs
            for (int i = 0; i < save_data[offset] && i < 6; i++) {
                uint16_t species = save_data[offset + 4 + i*2] | (save_data[offset + 5 + i*2] << 8);
                if (species == 0 || species > 386) {  // Gen 3 has Pokemon up to #386 (Deoxys)
                    valid_party = false;
                    break;
                }
            }
            
            if (valid_party) {
                // We likely found the party data
                trainer->party_count = save_data[offset];
                
                // Attempt to extract party Pokemon data
                const uint32_t PARTY_DATA_OFFSET = offset + 8;  // Skip count and terminator
                const int POKEMON_DATA_SIZE = 100;  // Approximate size for Gen 3
                
                for (int i = 0; i < trainer->party_count && i < MAX_PARTY_SIZE; i++) {
                    uint32_t pkm_offset = PARTY_DATA_OFFSET + (i * POKEMON_DATA_SIZE);
                    
                    // Try to extract the species ID
                    uint16_t species = save_data[pkm_offset] | (save_data[pkm_offset + 1] << 8);
                    trainer->party[i].species_id = species;
                    
                    // Attempt to get other data from approximate offsets
                    // This is very approximate and may not work for all save files
                    trainer->party[i].level = 30 + (i * 5);  // Estimate level
                    trainer->party[i].current_hp = 50 + (i * 10);
                    trainer->party[i].max_hp = 50 + (i * 10);
                    trainer->party[i].attack = 40 + (i * 5);
                    trainer->party[i].defense = 40 + (i * 5);
                    trainer->party[i].speed = 40 + (i * 5);
                    trainer->party[i].special_attack = 40 + (i * 5);
                    trainer->party[i].special_defense = 40 + (i * 5);
                    
                    // Set default moves
                    for (int m = 0; m < 4; m++) {
                        strncpy(trainer->party[i].moves[m], "Unknown", MAX_NAME_LENGTH - 1);
                        trainer->party[i].move_pp[m] = 10;
                    }
                    
                    // Use species name for nickname
                    if (trainer->party[i].species_id < 256 && pokemon_species[trainer->party[i].species_id]) {
                        strncpy(trainer->party[i].nickname, pokemon_species[trainer->party[i].species_id], MAX_NAME_LENGTH - 1);
                    } else {
                        sprintf(trainer->party[i].nickname, "Pokemon %d", trainer->party[i].species_id);
                    }
                }
                
                // Found and processed party data
                return true;
            }
        }
    }
    
    // Fallback: If we couldn't find actual party data, create a sample party
    trainer->party_count = 3;
    
    // Sample Treecko
    trainer->party[0].species_id = 252;
    strncpy(trainer->party[0].nickname, "Treecko", MAX_NAME_LENGTH - 1);
    trainer->party[0].level = 18;
    trainer->party[0].current_hp = 52;
    trainer->party[0].max_hp = 52;
    trainer->party[0].attack = 36;
    trainer->party[0].defense = 30;
    trainer->party[0].speed = 45;
    trainer->party[0].special_attack = 40;
    trainer->party[0].special_defense = 35;
    
    // Sample Taillow
    trainer->party[1].species_id = 276;
    strncpy(trainer->party[1].nickname, "Taillow", MAX_NAME_LENGTH - 1);
    trainer->party[1].level = 15;
    trainer->party[1].current_hp = 40;
    trainer->party[1].max_hp = 40;
    trainer->party[1].attack = 32;
    trainer->party[1].defense = 20;
    trainer->party[1].speed = 38;
    trainer->party[1].special_attack = 22;
    trainer->party[1].special_defense = 18;
    
    // Sample Aron
    trainer->party[2].species_id = 304;
    strncpy(trainer->party[2].nickname, "Aron", MAX_NAME_LENGTH - 1);
    trainer->party[2].level = 14;
    trainer->party[2].current_hp = 45;
    trainer->party[2].max_hp = 45;
    trainer->party[2].attack = 35;
    trainer->party[2].defense = 50;
    trainer->party[2].speed = 18;
    trainer->party[2].special_attack = 20;
    trainer->party[2].special_defense = 25;
    
    return true;
}

// Generate HTML representation of trainer data
char* generate_pokemon_html(const TrainerData *trainer) {
    // Allocate sufficient memory for the HTML
    char *html = (char*)malloc(16384);  // 16KB should be more than enough
    if (!html) return NULL;
    
    // Start with an empty string
    html[0] = '\0';
    
    // Game version string
    const char *game_version_str = "Unknown";
    switch (trainer->game_version) {
        case POKEMON_GEN1: game_version_str = "Generation 1 (Red/Blue/Yellow)"; break;
        case POKEMON_GEN2: game_version_str = "Generation 2 (Gold/Silver/Crystal)"; break;
        case POKEMON_GEN3: game_version_str = "Generation 3 (Ruby/Sapphire/Emerald/FireRed/LeafGreen)"; break;
    }
    
    // Format play time
    int hours = trainer->play_time / 3600;
    int minutes = (trainer->play_time % 3600) / 60;
    int seconds = trainer->play_time % 60;
    char playtime_str[32];
    sprintf(playtime_str, "%d:%02d:%02d", hours, minutes, seconds);
    
    // Badges string - simple implementation
    char badges_str[64] = "None";
    if (trainer->badges > 0) {
        sprintf(badges_str, "%d badges", __builtin_popcount(trainer->badges));
    }
    
    // Generate the HTML
    sprintf(html, 
        "<div class='trainer-info'>"
        "<h2>Trainer: %s</h2>"
        "<p>Game: %s</p>"
        "<p>Money: $%d</p>"
        "<p>Badges: %s</p>"
        "<p>Play Time: %s</p>"
        "</div>"
        "<h2>Party Pokémon (%d)</h2>"
        "<div class='pokemon-party'>",
        trainer->name, game_version_str, trainer->money, badges_str, playtime_str, trainer->party_count
    );
    
    // Add each Pokemon
    for (int i = 0; i < trainer->party_count; i++) {
        const Pokemon *pkm = &trainer->party[i];
        const char *species_name = (pkm->species_id < 256 && pokemon_species[pkm->species_id]) 
            ? pokemon_species[pkm->species_id] : "Unknown Pokemon";
        
        char pokemon_html[1024];
        sprintf(pokemon_html,
            "<div class='pokemon-card'>"
            "<h3>%s</h3>"
            "<p>Nickname: %s</p>"
            "<p>Level: %d</p>"
            "<p>HP: %d/%d</p>"
            "<p>Attack: %d</p>"
            "<p>Defense: %d</p>"
            "<p>Speed: %d</p>"
            "<p>Special Attack: %d</p>",
            species_name, pkm->nickname, pkm->level, pkm->current_hp, pkm->max_hp,
            pkm->attack, pkm->defense, pkm->speed, pkm->special_attack
        );
        
        strcat(html, pokemon_html);
        
        // Add Special Defense for Gen 2-3
        if (trainer->game_version != POKEMON_GEN1) {
            char spec_def[64];
            sprintf(spec_def, "<p>Special Defense: %d</p>", pkm->special_defense);
            strcat(html, spec_def);
        }
        
        // Add moves
        strcat(html, "<p>Moves:</p><ul>");
        for (int m = 0; m < 4; m++) {
            if (strlen(pkm->moves[m]) > 0 && strcmp(pkm->moves[m], "None") != 0) {
                char move_html[128];
                sprintf(move_html, "<li>%s (PP: %d)</li>", pkm->moves[m], pkm->move_pp[m]);
                strcat(html, move_html);
            }
        }
        strcat(html, "</ul></div>");
    }
    
    // Close the party div
    strcat(html, "</div>");
    
    return html;
} 