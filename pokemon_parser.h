#ifndef POKEMON_PARSER_H
#define POKEMON_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // For size_t

// Pokemon Generation definitions
#define POKEMON_GEN1 1  // Red, Blue, Yellow
#define POKEMON_GEN2 2  // Gold, Silver, Crystal
#define POKEMON_GEN3 3  // Ruby, Sapphire, Emerald, FireRed, LeafGreen

// Maximum party size is 6 in all games
#define MAX_PARTY_SIZE 6

// Maximum name length (including null terminator)
#define MAX_NAME_LENGTH 11

// Pokemon data structure (common for all generations)
typedef struct {
    int species_id;                 // Pokemon species ID
    char nickname[MAX_NAME_LENGTH]; // Pokemon nickname
    int level;                      // Pokemon level
    int current_hp;                 // Current HP
    int max_hp;                     // Maximum HP
    int attack;                     // Attack stat
    int defense;                    // Defense stat
    int speed;                      // Speed stat
    int special_attack;             // Special Attack (Gen 2-3) or Special (Gen 1)
    int special_defense;            // Special Defense (Gen 2-3, not used in Gen 1)
    char moves[4][MAX_NAME_LENGTH]; // Move names
    int move_pp[4];                 // Move PP values
} Pokemon;

// Trainer data structure
typedef struct {
    char name[MAX_NAME_LENGTH];     // Trainer name
    int money;                      // Money amount
    int badges;                     // Badges as a bit field
    int play_time;                  // Play time in seconds
    int game_version;               // Game version ID
    
    // Party information
    int party_count;                // Number of Pokemon in party
    Pokemon party[MAX_PARTY_SIZE];  // Party Pokemon
} TrainerData;

// Function to detect save file generation
int detect_pokemon_generation(const uint8_t *save_data, size_t save_size);

// Functions to parse different generation save files
bool parse_gen1_save(const uint8_t *save_data, size_t save_size, TrainerData *trainer);
bool parse_gen2_save(const uint8_t *save_data, size_t save_size, TrainerData *trainer);
bool parse_gen3_save(const uint8_t *save_data, size_t save_size, TrainerData *trainer);

// Generate HTML representation of trainer data
char* generate_pokemon_html(const TrainerData *trainer);

#endif // POKEMON_PARSER_H 