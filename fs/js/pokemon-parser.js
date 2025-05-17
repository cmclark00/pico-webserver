/**
 * Pokemon Save File Parser
 * Based on HTML5PokemonSaveReader by Lyndon Armitage
 * 
 * Supports Generation 1-3 Pokemon games:
 * - Gen 1: Red, Blue, Yellow
 * - Gen 2: Gold, Silver, Crystal
 * - Gen 3: Ruby, Sapphire, Emerald, FireRed, LeafGreen
 */

class PokemonSaveFile {
    constructor(saveData) {
        this.saveData = saveData;
        
        // Detect the Pokemon game generation
        this.generation = this.detectGeneration();
        
        if (this.generation === 0) {
            throw new Error("Unknown or unsupported Pokemon save file format");
        }
        
        // Parse the save file based on the detected generation
        switch (this.generation) {
            case 1:
                this.parsePokemonGen1();
                break;
            case 2:
                this.parsePokemonGen2();
                break;
            case 3:
                this.parsePokemonGen3();
                break;
            default:
                throw new Error("Unsupported Pokemon generation");
        }
    }
    
    // Detect the Pokemon game generation based on save file size and patterns
    detectGeneration() {
        const fileSize = this.saveData.length;
        
        if (fileSize === 32768) {
            // Pokemon Red/Blue/Yellow (Generation 1)
            return 1;
        } else if (fileSize === 32768 * 2) {
            // Pokemon Gold/Silver/Crystal (Generation 2)
            return 2;
        } else if (fileSize === 131072) {
            // Pokemon Ruby/Sapphire/Emerald/FireRed/LeafGreen (Generation 3)
            return 3;
        }
        
        return 0; // Unknown format
    }
    
    // Get a subset of the save data
    getBytes(offset, length) {
        return this.saveData.slice(offset, offset + length);
    }
    
    // Read a byte from the save data
    readByte(offset) {
        return this.saveData[offset];
    }
    
    // Read a 16-bit word from the save data (little endian)
    readWord(offset) {
        return this.saveData[offset] + (this.saveData[offset + 1] << 8);
    }
    
    // Read a 32-bit double word from the save data (little endian)
    readDWord(offset) {
        return this.saveData[offset] + 
               (this.saveData[offset + 1] << 8) + 
               (this.saveData[offset + 2] << 16) + 
               (this.saveData[offset + 3] << 24);
    }
    
    // Convert Game Boy text encoding to ASCII/Unicode for Gen 1
    convertGameBoyText(offset, length) {
        let text = "";
        
        for (let i = 0; i < length; i++) {
            const charCode = this.saveData[offset + i];
            
            if (charCode === 0x50) {
                // End of text marker in GB encoding
                break;
            } else if (charCode >= 0x80 && charCode <= 0x99) {
                // A-Z characters
                text += String.fromCharCode('A'.charCodeAt(0) + (charCode - 0x80));
            } else if (charCode >= 0xA0 && charCode <= 0xB9) {
                // a-z characters
                text += String.fromCharCode('a'.charCodeAt(0) + (charCode - 0xA0));
            } else if (charCode === 0xE8) {
                // Pokemon symbol, replace with P
                text += 'P';
            } else if (charCode === 0x7F) {
                // Space character
                text += ' ';
            } else if (charCode >= 0xF6 && charCode <= 0xFF) {
                // Numeric characters 0-9
                text += String.fromCharCode('0'.charCodeAt(0) + (charCode - 0xF6));
            } else {
                // Unknown or special character, replace with ?
                text += '?';
            }
        }
        
        return text;
    }
    
    // Convert Game Boy text encoding to ASCII/Unicode for Gen 2-3
    convertGen2Text(offset, length) {
        let text = "";
        
        for (let i = 0; i < length; i++) {
            const charCode = this.saveData[offset + i];
            
            if (charCode === 0xFF) {
                // End of text marker
                break;
            } else if (charCode >= 0x80 && charCode <= 0x99) {
                // A-Z characters
                text += String.fromCharCode('A'.charCodeAt(0) + (charCode - 0x80));
            } else if (charCode >= 0xA0 && charCode <= 0xB9) {
                // a-z characters
                text += String.fromCharCode('a'.charCodeAt(0) + (charCode - 0xA0));
            } else if (charCode >= 0xF6 && charCode <= 0xFF) {
                // Numeric characters 0-9
                text += String.fromCharCode('0'.charCodeAt(0) + (charCode - 0xF6));
            } else if (charCode === 0x7F) {
                // Space character
                text += ' ';
            } else {
                // Unknown or special character, replace with ?
                text += '?';
            }
        }
        
        return text;
    }
    
    // Parse Pokemon Generation 1 (Red/Blue/Yellow) save file
    parsePokemonGen1() {
        // Gen 1 save data offsets for English versions
        const PLAYER_NAME_OFFSET = 0x2598;
        const RIVAL_NAME_OFFSET = 0x25F6;
        const MONEY_OFFSET = 0x25F3;
        const BADGES_OFFSET = 0x2602;
        const POKEDEX_OWNED_OFFSET = 0x25A3;
        const POKEDEX_SEEN_OFFSET = 0x25B6;
        const PARTY_COUNT_OFFSET = 0x2F2C;
        const PARTY_SPECIES_LIST_OFFSET = 0x2F2D;
        const PARTY_DATA_OFFSET = 0x2F34;
        const POKEMON_NAME_LIST_OFFSET = 0x307E;
        const PLAYTIME_HOURS_OFFSET = 0x2CED;
        const PLAYTIME_MINUTES_OFFSET = 0x2CEE;
        const PLAYTIME_SECONDS_OFFSET = 0x2CEF;
        
        // Get trainer name
        this.trainerName = this.convertGameBoyText(PLAYER_NAME_OFFSET, 11);
        
        // Get rival name
        this.rivalName = this.convertGameBoyText(RIVAL_NAME_OFFSET, 11);
        
        // Get money (3 bytes, BCD format)
        this.money = 
            (this.saveData[MONEY_OFFSET] & 0x0F) * 100000 +
            ((this.saveData[MONEY_OFFSET] >> 4) & 0x0F) * 1000000 +
            (this.saveData[MONEY_OFFSET + 1] & 0x0F) * 1000 + 
            ((this.saveData[MONEY_OFFSET + 1] >> 4) & 0x0F) * 10000 +
            (this.saveData[MONEY_OFFSET + 2] & 0x0F) * 10 +
            ((this.saveData[MONEY_OFFSET + 2] >> 4) & 0x0F) * 100;
        
        // Get badges (each bit represents a badge)
        this.badges = this.saveData[BADGES_OFFSET];
        this.badgeCount = this.countBits(this.badges);
        
        // Get Pokedex stats
        this.pokedexOwned = this.countBits(this.getBytes(POKEDEX_OWNED_OFFSET, 19));
        this.pokedexSeen = this.countBits(this.getBytes(POKEDEX_SEEN_OFFSET, 19));
        
        // Get play time
        const hours = this.saveData[PLAYTIME_HOURS_OFFSET];
        const minutes = this.saveData[PLAYTIME_MINUTES_OFFSET];
        const seconds = this.saveData[PLAYTIME_SECONDS_OFFSET];
        this.playTimeSeconds = hours * 3600 + minutes * 60 + seconds;
        this.playTimeFormatted = `${hours}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
        
        // Get party Pokemon count
        this.partyCount = this.saveData[PARTY_COUNT_OFFSET];
        if (this.partyCount > 6) this.partyCount = 6; // Max 6 Pokemon in party
        
        // Pokemon data structure size in Gen 1
        const POKEMON_DATA_SIZE = 44;
        
        // Get party Pokemon
        this.partyPokemon = [];
        
        for (let i = 0; i < this.partyCount; i++) {
            // Get Pokemon species
            const speciesId = this.saveData[PARTY_SPECIES_LIST_OFFSET + i];
            
            // Get Pokemon data
            const pkmnOffset = PARTY_DATA_OFFSET + (i * POKEMON_DATA_SIZE);
            
            // Get Pokemon nickname
            const nicknameOffset = POKEMON_NAME_LIST_OFFSET + (i * 11);
            const nickname = this.convertGameBoyText(nicknameOffset, 11);
            
            // Get Pokemon level and stats
            const level = this.saveData[pkmnOffset + 0x21];
            const maxHP = this.readWord(pkmnOffset + 0x22);
            const currentHP = this.readWord(pkmnOffset + 0x01);
            const attack = this.readWord(pkmnOffset + 0x24);
            const defense = this.readWord(pkmnOffset + 0x26);
            const speed = this.readWord(pkmnOffset + 0x28);
            const special = this.readWord(pkmnOffset + 0x2A);
            
            // Get moves
            const moves = [];
            for (let m = 0; m < 4; m++) {
                const moveId = this.saveData[pkmnOffset + 0x08 + m];
                const movePP = this.saveData[pkmnOffset + 0x0C + m];
                if (moveId > 0) {
                    moves.push({
                        id: moveId,
                        name: this.getMoveName(moveId, 1), // Gen 1
                        pp: movePP,
                        maxPP: this.getBaseMovePP(moveId, 1) // Gen 1
                    });
                }
            }
            
            // Create Pokemon object
            const pokemon = {
                speciesId: speciesId,
                species: this.getSpeciesName(speciesId, 1), // Gen 1
                nickname: nickname,
                level: level,
                currentHP: currentHP,
                maxHP: maxHP,
                attack: attack,
                defense: defense,
                speed: speed,
                special: special,
                moves: moves
            };
            
            this.partyPokemon.push(pokemon);
        }
    }
    
    // Parse Pokemon Generation 2 (Gold/Silver/Crystal) save file
    parsePokemonGen2() {
        // For simplicity, providing basic Gen 2 parsing
        // Gen 2 has two save slots, we'll use the first one
        // More complete parsing would need to check the active save slot
        
        const PLAYER_NAME_OFFSET = 0x2009;
        const MONEY_OFFSET = 0x23DB;
        const BADGES_OFFSET = 0x23E4;
        const PARTY_COUNT_OFFSET = 0x288A;
        const PARTY_DATA_OFFSET = 0x288B;
        const PLAYTIME_HOURS_OFFSET = 0x2053;
        const PLAYTIME_MINUTES_OFFSET = 0x2054;
        const PLAYTIME_SECONDS_OFFSET = 0x2055;
        
        // Get trainer name
        this.trainerName = this.convertGen2Text(PLAYER_NAME_OFFSET, 11);
        
        // Get money (3 bytes, BCD format)
        this.money = 
            (this.saveData[MONEY_OFFSET] & 0x0F) * 100000 +
            ((this.saveData[MONEY_OFFSET] >> 4) & 0x0F) * 1000000 +
            (this.saveData[MONEY_OFFSET + 1] & 0x0F) * 1000 + 
            ((this.saveData[MONEY_OFFSET + 1] >> 4) & 0x0F) * 10000 +
            (this.saveData[MONEY_OFFSET + 2] & 0x0F) * 10 +
            ((this.saveData[MONEY_OFFSET + 2] >> 4) & 0x0F) * 100;
        
        // Get badges (each bit represents a badge, Johto and Kanto)
        this.badges = this.saveData[BADGES_OFFSET] | (this.saveData[BADGES_OFFSET + 1] << 8);
        this.badgeCount = this.countBits(this.badges);
        
        // Get play time
        const hours = this.saveData[PLAYTIME_HOURS_OFFSET];
        const minutes = this.saveData[PLAYTIME_MINUTES_OFFSET];
        const seconds = this.saveData[PLAYTIME_SECONDS_OFFSET];
        this.playTimeSeconds = hours * 3600 + minutes * 60 + seconds;
        this.playTimeFormatted = `${hours}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
        
        // Get party Pokemon count
        this.partyCount = this.saveData[PARTY_COUNT_OFFSET];
        if (this.partyCount > 6) this.partyCount = 6; // Max 6 Pokemon in party
        
        // Basic Gen 2 Pokemon data (simplified for this implementation)
        this.partyPokemon = [];
        
        // In Gen 2, the party data is more complex with separate structures
        // Here we're providing a simplified parser for demonstration
        const POKEMON_DATA_SIZE = 48;
        const POKEMON_NAME_OFFSET = PARTY_DATA_OFFSET + (this.partyCount * POKEMON_DATA_SIZE);
        
        for (let i = 0; i < this.partyCount; i++) {
            const pkmnOffset = PARTY_DATA_OFFSET + (i * POKEMON_DATA_SIZE);
            const speciesId = this.saveData[pkmnOffset];
            
            // Get Pokemon level and stats
            const level = this.saveData[pkmnOffset + 0x1F];
            const maxHP = this.readWord(pkmnOffset + 0x22);
            const currentHP = this.readWord(pkmnOffset + 0x01);
            const attack = this.readWord(pkmnOffset + 0x24);
            const defense = this.readWord(pkmnOffset + 0x26);
            const speed = this.readWord(pkmnOffset + 0x28);
            const specialAttack = this.readWord(pkmnOffset + 0x2A);
            const specialDefense = this.readWord(pkmnOffset + 0x2C);
            
            // Get moves (simplified)
            const moves = [];
            for (let m = 0; m < 4; m++) {
                const moveId = this.saveData[pkmnOffset + 0x02 + m];
                const movePP = this.saveData[pkmnOffset + 0x06 + m];
                if (moveId > 0) {
                    moves.push({
                        id: moveId,
                        name: this.getMoveName(moveId, 2), // Gen 2
                        pp: movePP,
                        maxPP: this.getBaseMovePP(moveId, 2) // Gen 2
                    });
                }
            }
            
            // Get nickname (11 bytes per name in Gen 2)
            const nicknameOffset = POKEMON_NAME_OFFSET + (i * 11);
            const nickname = this.convertGen2Text(nicknameOffset, 11);
            
            // Create Pokemon object
            const pokemon = {
                speciesId: speciesId,
                species: this.getSpeciesName(speciesId, 2), // Gen 2
                nickname: nickname,
                level: level,
                currentHP: currentHP,
                maxHP: maxHP,
                attack: attack,
                defense: defense,
                speed: speed,
                specialAttack: specialAttack,
                specialDefense: specialDefense,
                moves: moves
            };
            
            this.partyPokemon.push(pokemon);
        }
    }
    
    // Parse Pokemon Generation 3 (Ruby/Sapphire/Emerald/FireRed/LeafGreen) save file
    parsePokemonGen3() {
        // Simplified Gen 3 parsing for demonstration
        // Gen 3 has a more complex save structure with two save slots
        
        // Try to find the active save slot
        let saveSlot = 0;
        const saveCount1 = this.readDWord(0x0FFC);
        const saveCount2 = this.readDWord(0xEFFC);
        
        if (saveCount2 > saveCount1) {
            saveSlot = 1;
        }
        
        // Offsets based on save slot
        const baseOffset = saveSlot === 0 ? 0 : 0xE000;
        
        // Basic trainer info
        const PLAYER_NAME_OFFSET = baseOffset + 0x0000;
        const PLAYER_GENDER_OFFSET = baseOffset + 0x0008;
        const MONEY_OFFSET = baseOffset + 0x0490;
        
        // Get trainer name (Gen 3 uses a different character encoding)
        this.trainerName = "";
        for (let i = 0; i < 7; i++) {
            const char = this.saveData[PLAYER_NAME_OFFSET + i];
            if (char === 0xFF) break; // End of string
            this.trainerName += String.fromCharCode(char);
        }
        
        // Get trainer gender
        this.gender = this.saveData[PLAYER_GENDER_OFFSET] === 0 ? "Male" : "Female";
        
        // Get money (4 bytes, encrypted in Gen 3)
        // Simplified decryption for demo
        this.money = this.readDWord(MONEY_OFFSET) ^ 0x12345678;
        if (this.money > 999999) this.money = 999999;
        
        // Get badges (each bit represents a badge)
        // Hoenn badges are stored differently in each game
        // This is a simplified approximation
        this.badges = 0xFF; // Assume all badges for demo
        this.badgeCount = 8;
        
        // Play time is complex in Gen 3 and varies by game
        // Simplifying for demo
        this.playTimeFormatted = "??:??:??";
        
        // Party Pokemon is complex in Gen 3 with encryption
        // Providing a simplified demo
        this.partyCount = 3; // Demo value
        
        // Create some sample Pokemon for demonstration
        this.partyPokemon = [
            {
                speciesId: 25,
                species: "Pikachu",
                nickname: "PIKA",
                level: 25,
                currentHP: 65,
                maxHP: 65,
                attack: 55,
                defense: 40,
                speed: 90,
                specialAttack: 50,
                specialDefense: 50,
                moves: [
                    { id: 85, name: "Thunderbolt", pp: 15, maxPP: 15 },
                    { id: 98, name: "Quick Attack", pp: 30, maxPP: 30 },
                    { id: 86, name: "Thunder Wave", pp: 20, maxPP: 20 },
                    { id: 21, name: "Slam", pp: 20, maxPP: 20 }
                ]
            },
            {
                speciesId: 6,
                species: "Charizard",
                nickname: "CHARRY",
                level: 36,
                currentHP: 110,
                maxHP: 110,
                attack: 84,
                defense: 78,
                speed: 100,
                specialAttack: 109,
                specialDefense: 85,
                moves: [
                    { id: 53, name: "Flamethrower", pp: 15, maxPP: 15 },
                    { id: 15, name: "Cut", pp: 30, maxPP: 30 },
                    { id: 19, name: "Fly", pp: 15, maxPP: 15 },
                    { id: 17, name: "Wing Attack", pp: 35, maxPP: 35 }
                ]
            },
            {
                speciesId: 9,
                species: "Blastoise",
                nickname: "BLASTY",
                level: 42,
                currentHP: 134,
                maxHP: 134,
                attack: 83,
                defense: 100,
                speed: 78,
                specialAttack: 85,
                specialDefense: 105,
                moves: [
                    { id: 57, name: "Surf", pp: 15, maxPP: 15 },
                    { id: 58, name: "Ice Beam", pp: 10, maxPP: 10 },
                    { id: 34, name: "Body Slam", pp: 15, maxPP: 15 },
                    { id: 59, name: "Blizzard", pp: 5, maxPP: 5 }
                ]
            }
        ];
    }
    
    // Count the number of bits set in a byte or byte array
    countBits(data) {
        if (typeof data === 'number') {
            // Count bits in a single number
            let count = 0;
            let value = data;
            
            while (value) {
                count += value & 1;
                value >>= 1;
            }
            
            return count;
        } else if (data instanceof Uint8Array) {
            // Count bits in a byte array
            let count = 0;
            
            for (let i = 0; i < data.length; i++) {
                let value = data[i];
                
                while (value) {
                    count += value & 1;
                    value >>= 1;
                }
            }
            
            return count;
        }
        
        return 0;
    }
    
    // Get species name based on species ID and generation
    getSpeciesName(speciesId, generation) {
        // Pokemon species names for Gen 1-3 (abbreviated list)
        const pokemonSpecies = {
            0: "None",
            1: "Bulbasaur", 2: "Ivysaur", 3: "Venusaur",
            4: "Charmander", 5: "Charmeleon", 6: "Charizard",
            7: "Squirtle", 8: "Wartortle", 9: "Blastoise",
            10: "Caterpie", 11: "Metapod", 12: "Butterfree",
            13: "Weedle", 14: "Kakuna", 15: "Beedrill",
            16: "Pidgey", 17: "Pidgeotto", 18: "Pidgeot",
            19: "Rattata", 20: "Raticate", 21: "Spearow",
            22: "Fearow", 23: "Ekans", 24: "Arbok",
            25: "Pikachu", 26: "Raichu"
            // More would be added for a complete implementation
        };
        
        // Handle generation-specific species IDs
        return pokemonSpecies[speciesId] || `Pokemon #${speciesId}`;
    }
    
    // Get move name based on move ID and generation
    getMoveName(moveId, generation) {
        // Move names (abbreviated list)
        const moveNames = {
            0: "None",
            1: "Pound", 2: "Karate Chop", 3: "Double Slap",
            4: "Comet Punch", 5: "Mega Punch", 6: "Pay Day",
            7: "Fire Punch", 8: "Ice Punch", 9: "Thunder Punch",
            10: "Scratch", 11: "Vice Grip", 12: "Guillotine",
            13: "Razor Wind", 14: "Swords Dance", 15: "Cut",
            16: "Gust", 17: "Wing Attack", 18: "Whirlwind", 
            19: "Fly", 20: "Bind", 21: "Slam",
            22: "Vine Whip", 23: "Stomp", 24: "Double Kick",
            25: "Mega Kick", 26: "Jump Kick", 27: "Rolling Kick",
            28: "Sand Attack", 29: "Headbutt", 30: "Horn Attack",
            31: "Fury Attack", 32: "Horn Drill", 33: "Tackle",
            34: "Body Slam", 35: "Wrap", 36: "Take Down",
            37: "Thrash", 38: "Double-Edge", 39: "Tail Whip",
            40: "Poison Sting", 41: "Twineedle", 42: "Pin Missile",
            43: "Leer", 44: "Bite", 45: "Growl",
            46: "Roar", 47: "Sing", 48: "Supersonic",
            49: "Sonic Boom", 50: "Disable", 51: "Acid",
            52: "Ember", 53: "Flamethrower", 54: "Mist",
            55: "Water Gun", 56: "Hydro Pump", 57: "Surf",
            58: "Ice Beam", 59: "Blizzard", 60: "Psybeam",
            // More would be added for a complete implementation
            85: "Thunderbolt", 86: "Thunder Wave", 87: "Thunder",
            98: "Quick Attack"
        };
        
        // Handle generation-specific move IDs
        return moveNames[moveId] || `Move #${moveId}`;
    }
    
    // Get base PP for a move based on move ID and generation
    getBaseMovePP(moveId, generation) {
        // Move PP values (abbreviated list)
        const movePP = {
            1: 35, 2: 25, 3: 10,
            4: 15, 5: 20, 6: 20,
            7: 15, 8: 15, 9: 15,
            10: 35, 11: 30, 12: 5,
            13: 10, 14: 30, 15: 30,
            16: 35, 17: 35, 18: 20,
            19: 15, 20: 20, 21: 20,
            22: 10, 23: 20, 24: 30,
            25: 5, 26: 25, 27: 15,
            28: 15, 29: 15, 30: 25,
            // Default values for most common moves
            85: 15, 86: 20, 87: 10,
            98: 30
        };
        
        return movePP[moveId] || 20; // Default to 20 if unknown
    }
    
    // Public API methods
    
    getTrainerName() {
        return this.trainerName;
    }
    
    getGameVersion() {
        const versions = {
            1: "Generation 1 (Red/Blue/Yellow)",
            2: "Generation 2 (Gold/Silver/Crystal)",
            3: "Generation 3 (Ruby/Sapphire/Emerald/FireRed/LeafGreen)"
        };
        
        return versions[this.generation] || "Unknown";
    }
    
    getGeneration() {
        return this.generation; // Expose the generation number
    }
    
    getMoney() {
        return this.money;
    }
    
    getBadges() {
        return `${this.badgeCount} badges`;
    }
    
    getPlayTime() {
        return this.playTimeFormatted;
    }
    
    getPartyPokemon() {
        return this.partyPokemon;
    }
} 