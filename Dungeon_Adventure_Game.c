#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SAVED_GAMES 20
#define MAX_FILENAME_LENGTH 256
#define MAX_INVENTORY 15
#define MAX_COMMAND_LENGTH 256
#define MAX_ITEMS 10
#define MAP_SIZE 5
#define MAX_ROOMS 10
#define FIXED_CREATURE_COUNT 5
int creatures_left = FIXED_CREATURE_COUNT;
int discovered[MAP_SIZE][MAP_SIZE] = {0};

// Array of saved game file names
char saved_games[MAX_SAVED_GAMES][MAX_FILENAME_LENGTH];
int saved_game_count = 0;

// Predefined unique room descriptions
const char *room_descriptions[] = {
    "A dimly lit chamber with moss-covered walls.",
    "A grand hall adorned with ancient tapestries.",
    "A small, cluttered library filled with dusty books.",
    "A cavernous room echoing with dripping water.",
    "A stone corridor lined with flickering torches.",
    "A serene garden blooming with mystical flowers.",
    "A dusty armory housing old weapons.",
    "A mysterious altar glowing with faint light.",
    "A gloomy dungeon cell with iron bars.",
    "A vibrant market room bustling with activity."
};
#define TOTAL_DESCRIPTIONS (sizeof(room_descriptions) / sizeof(room_descriptions[0]))

// Struct Definitions
typedef struct Item {
    char *name;
    int attack_bonus;
    int shield_bonus;
} Item;

typedef struct Creature {
    char *name;
    int health;
    int strength;
} Creature;

typedef struct Room {
    int id;
    char *description;
    Item *items[MAX_ITEMS];
    int item_count;
    Creature *creature;
    int x, y;          // Map position
    int discovered;    // Room discovered?
} Room;

typedef struct Player {
    char nickname[50];  
    int health;
    int base_strength;
    Item *inventory[MAX_INVENTORY];
    int inventory_count;
    int x, y;  
} Player;

// Function Prototypes
void initialize_game(Player *player, Room **rooms, int *room_count);
void display_room(Room *room);
void parse_command(Player *player, Room *rooms[], int room_count, char *command);
void move_player(Player *player, char *direction, Room *rooms[], int room_count);
void pickup_item(Player *player, Room *rooms[], int room_count, char *item_name);
void attack_creature(Player *player, Room *rooms[], int room_count);
void list_inventory(Player *player);
void save_game(Player *player, Room *rooms[], int room_count, const char *filepath);
int load_game(Player *player, Room **rooms, int *room_count, const char *filepath);
void list_saved_games();  
void load_saved_games();
int is_nickname_taken(const char *nickname);
void save_game_to_list(const char *filepath);
void delete_saved_game(const char *filepath);
void free_resources(Room *rooms[], int room_count, Player *player);
int is_item_in_inventory(Player *player, char *item_name);
int has_collected_all_awards(Room *rooms[], int room_count, Player *player);
void display_map(Room *rooms[], int room_count, Player *player);
void display_help();
void display_status(Player *player);
int compute_total_attack(Player *player);
int compute_total_shield(Player *player);
Room* find_room_at_position(Room *rooms[], int room_count, int x, int y);

// Function to shuffle room descriptions
void shuffle_descriptions(const char **descriptions, int count) {
    for (int i = count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        const char *temp = descriptions[i];
        // Note: Casting away constness for shuffling
        ((const char **)descriptions)[i] = descriptions[j];
        ((const char **)descriptions)[j] = temp;
    }
}

int main() {
    Player player = { .health = 100, .base_strength = 10, .inventory_count = 0, .x = 2, .y = 2 };
    Room *rooms[MAX_ROOMS] = { NULL };
    int room_count = 0;
    char command[MAX_COMMAND_LENGTH];
    
    load_saved_games();

    while (1) {
        printf("Game Selection:\n");
        printf("1 - New Game\n");
        printf("2 - Load Game\n");
        printf("Make your choice (1 or 2): ");

        char input[MAX_COMMAND_LENGTH];
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        int choice = atoi(input);  

        if (choice == 1) {
            // New Game
            while (1) {
                printf("Please enter a nickname: ");
                if (fgets(player.nickname, sizeof(player.nickname), stdin) == NULL) break;
                player.nickname[strcspn(player.nickname, "\n")] = '\0';  

                if (is_nickname_taken(player.nickname)) {
                    printf("This nickname is already taken. Please choose another one.\n");
                } else {
                    break;
                }
            }
            srand(time(NULL));
            initialize_game(&player, rooms, &room_count);

            printf("Welcome to the Dungeon Adventure Game, %s!\n", player.nickname);
            printf("Use the 'help' command for assistance.\n");
            break;
        } else if (choice == 2) {
            // Load Game
            printf("Please enter the 'load <filename>' command to load a game: ");
            char input_line[MAX_COMMAND_LENGTH];
            if (fgets(input_line, sizeof(input_line), stdin) == NULL) break;
            input_line[strcspn(input_line, "\n")] = '\0';  // Remove newline character

            // Parse the command
            char *token = strtok(input_line, " ");
            if (!token) {
                printf("No command entered!\n");
                continue;
            }

            // Is the first word 'load'?
            if (strcmp(token, "load") == 0) {
                char *filepath = strtok(NULL, " ");
                if (!filepath) {
                    printf("Usage: load <filepath>\n");
                    continue;
                }

                if (load_game(&player, rooms, &room_count, filepath)) {
                    printf("Game loaded successfully!\n");
                    break;
                } else {
                    printf("Failed to load file! Please enter a valid file or select 'New Game'.\n");
                }
            } else {
                printf("Command not found: %s\n", token);
            }
        } else {
            printf("Invalid input! Please choose 1 or 2.\n");
        }
    }

    Room *current_room = find_room_at_position(rooms, room_count, player.x, player.y);
    display_room(current_room);

    // Game loop
    while (1) {
        printf(">> ");
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) break;
        command[strcspn(command, "\n")] = '\0';  // Remove newline character
        parse_command(&player, rooms, room_count, command);
    }

    free_resources(rooms, room_count, &player);
    return 0;
}

// Function Implementations
void initialize_game(Player *player, Room **rooms, int *room_count) {
    // Structure representing cells on the map
    typedef struct {
        int is_room;  // 1 if a room exists, 0 if empty
        Room *room;   // Pointer to the room
    } Cell; 

    Cell map[MAP_SIZE][MAP_SIZE] = {0}; // The map starts with all cells empty

    // Shuffle the room descriptions before assignment
    shuffle_descriptions(room_descriptions, TOTAL_DESCRIPTIONS);

    // The player will always be at position (2,2)
    map[2][2].is_room = 1;

    // Create the first room with a unique description
    Room *initial_room = (Room *)malloc(sizeof(Room));
    if (!initial_room) {
        perror("Failed to allocate memory for initial room");
        exit(EXIT_FAILURE);
    }
    initial_room->id = (*room_count)++;
    initial_room->description = strdup("Starting room."); // Unique starting description
    initial_room->item_count = 0;
    initial_room->creature = NULL;
    initial_room->x = 2;
    initial_room->y = 2;
    map[2][2].room = initial_room;
    rooms[initial_room->id] = initial_room;
    discovered[2][2] = 1; // Starting room is considered discovered

    // Randomly place the remaining rooms
    while (*room_count < MAX_ROOMS) {
        int random_number = rand() % (MAP_SIZE * MAP_SIZE); // Random number from 0 to 24
        int row = random_number / MAP_SIZE;  // Row
        int col = random_number % MAP_SIZE;  // Column

        // If not the starting room and the room doesn't already exist, create a new room
        if (!(row == 2 && col == 2)) {
            int already_exists = 0;
            for (int i = 0; i < *room_count; i++) {
                if (rooms[i]->x == col && rooms[i]->y == row) {
                    already_exists = 1;
                    break;
                }
            }

            if (!already_exists) {
                // Create a new room
                Room *new_room = (Room *)malloc(sizeof(Room));
                if (!new_room) {
                    perror("Failed to allocate memory for new room");
                    exit(EXIT_FAILURE);
                }
                new_room->id = (*room_count)++;
                new_room->description = strdup(room_descriptions[*room_count - 1 < TOTAL_DESCRIPTIONS ? *room_count - 1 : TOTAL_DESCRIPTIONS - 1]);
                new_room->item_count = 0;
                new_room->creature = NULL;
                new_room->x = col;
                new_room->y = row;
                map[row][col].room = new_room;
                rooms[new_room->id] = new_room;

                // Add random items to the rooms
                if (rand() % 2 == 0) {
                    Item *item = (Item *)malloc(sizeof(Item));
                    if (!item) {
                        perror("Failed to allocate memory for item");
                        exit(EXIT_FAILURE);
                    }
                    item->name = (char *)malloc(32);
                    if (!item->name) {
                        perror("Failed to allocate memory for item name");
                        free(item);
                        exit(EXIT_FAILURE);
                    }
                    sprintf(item->name, "item%d", new_room->id);
                    
                    // Assign random attack or shield bonus
                    if (rand() % 2 == 0) {
                        item->attack_bonus = rand() % 5 + 1; // Attack bonus between 1-5
                        item->shield_bonus = 0;
                    } else {
                        item->attack_bonus = 0;
                        item->shield_bonus = rand() % 5 + 1; // Shield bonus between 1-5
                    }
                    new_room->items[new_room->item_count++] = item;
                }
            }
        }
    }

    // **Place Fixed Number of Creatures**
    int created_creatures = 0;
    while (created_creatures < FIXED_CREATURE_COUNT) {
        int random_number = rand() % (MAP_SIZE * MAP_SIZE);
        int row = random_number / MAP_SIZE;
        int col = random_number % MAP_SIZE;

        if (!(row == 2 && col == 2)) { // Not in the starting room
            Room *room = find_room_at_position(rooms, *room_count, col, row);
            if (room && !room->creature) {
                Creature *creature = (Creature *)malloc(sizeof(Creature));
                if (!creature) {
                    perror("Failed to allocate memory for creature");
                    exit(EXIT_FAILURE);
                }
                creature->name = (char *)malloc(32);
                if (!creature->name) {
                    perror("Failed to allocate memory for creature name");
                    free(creature);
                    exit(EXIT_FAILURE);
                }
                sprintf(creature->name, "Creature_%d", room->id);
                creature->health = rand() % 50 + 50;    // Health between 50-100
                creature->strength = rand() % 10 + 5;  // Strength between 5-15
                room->creature = creature;
                created_creatures++;
            }
        }
    }
}

// Check if a specific item exists in the player's inventory
int is_item_in_inventory(Player *player, char *item_name) {
    for (int i = 0; i < player->inventory_count; i++) {
        if (strcmp(player->inventory[i]->name, item_name) == 0) {
            return 1;  // Item found
        }
    }
    return 0;  // Item not found
}

int has_collected_all_awards(Room *rooms[], int room_count, Player *player) {
    for (int i = 0; i < room_count; i++) {
        Room *room = rooms[i];
        for (int j = 0; j < room->item_count; j++) {
            Item *item = rooms[i]->items[j];
            if (strncmp(item->name, "award", 5) == 0 && !is_item_in_inventory(player, item->name)) {
                return 0;  // Missing any award item
            }
        }
    }
    return 1;  // All awards collected
}

void display_room(Room *room) {
    if (room != NULL) {
        printf("Room Description: %s\n", room->description);
        if (room->item_count > 0) {
            printf("Items in the room:\n");
            for (int i = 0; i < room->item_count; i++) {
                Item *item = room->items[i];
                printf("- %s", item->name);
                if (item->attack_bonus > 0) {
                    printf(" (+%d attack)", item->attack_bonus);
                }
                if (item->shield_bonus > 0) {
                    printf(" (+%d shield)", item->shield_bonus);
                }
                printf("\n");
            }
        }
        if (room->creature != NULL) {
            printf("Creature: %s (Health: %d)\n", room->creature->name, room->creature->health);
        }
    } else {
        printf("You are in an empty area. There is no room here.\n");
    }
}

void parse_command(Player *player, Room *rooms[], int room_count, char *command) {
    char *token = strtok(command, " ");
    if (!token) return;

    if (strcmp(token, "move") == 0) {
        token = strtok(NULL, " ");
        if (token) {
            move_player(player, token, rooms, room_count);
        } else {
            printf("Usage: move <direction>\n");
        }
    } else if (strcmp(token, "look") == 0) {
        Room *current_room = find_room_at_position(rooms, room_count, player->x, player->y);
        display_room(current_room);
    } else if (strcmp(token, "inventory") == 0) {
        list_inventory(player);
    } else if (strcmp(token, "pickup") == 0) {
        token = strtok(NULL, " ");
        if (token) {
            pickup_item(player, rooms, room_count, token);
        } else {
            printf("Usage: pickup <item>\n");
        }
    } else if (strcmp(token, "attack") == 0) {
        attack_creature(player, rooms, room_count);
    } else if (strcmp(token, "save") == 0) {
        token = strtok(NULL, " ");
        if (token) {
            save_game(player, rooms, room_count, token);
            save_game_to_list(token);
        } else {
            printf("Usage: save <filepath>\n");
        }
    } else if (strcmp(token, "load") == 0) {
        token = strtok(NULL, " ");
        if (token) {
            if (load_game(player, rooms, &room_count, token)) {
                printf("Game successfully loaded!\n");
                display_room(find_room_at_position(rooms, room_count, player->x, player->y));
            } else {
                printf("Failed to load file! Please enter a valid file or select 'New Game'.\n");
            }
        } else {
            printf("Usage: load <filepath>\n");
        }
    } else if (strcmp(token, "list") == 0) {
        list_saved_games();
    } else if (strcmp(token, "delete") == 0) {
        token = strtok(NULL, " ");
        if (token) {
            delete_saved_game(token);
        } else {
            printf("Usage: delete <filepath>\n");
        }
    } else if (strcmp(token, "exit") == 0) {
        printf("Exiting the game. Goodbye!\n");
        exit(0);
    } else if (strcmp(token, "map") == 0) {
        display_map(rooms, room_count, player);
    } else if (strcmp(token, "help") == 0) {
        display_help();
    } else if (strcmp(token, "status") == 0) {
        display_status(player);
    } else {
        printf("Unknown command: %s\n", token);
    }
}

void move_player(Player *player, char *direction, Room *rooms[], int room_count) {
    int new_x = player->x;
    int new_y = player->y;

    if (strcmp(direction, "up") == 0) new_y -= 1;
    else if (strcmp(direction, "down") == 0) new_y += 1;
    else if (strcmp(direction, "left") == 0) new_x -= 1;
    else if (strcmp(direction, "right") == 0) new_x += 1;
    else {
        printf("Invalid direction: %s\n", direction);
        return;
    }

    // Check map boundaries
    if (new_x < 0 || new_x >= MAP_SIZE || new_y < 0 || new_y >= MAP_SIZE) {
        printf("You cannot leave the map.\n");
        return;
    }

    // Update player's position
    player->x = new_x;
    player->y = new_y;

    Room *current_room = find_room_at_position(rooms, room_count, player->x, player->y);
    if (current_room) {
        printf("You entered a room:\n");
        display_room(current_room);

        if (current_room->x == 2 && current_room->y == 2) {
            player->health = 100;  // Reset health when returning to the starting room
        }

        // Check Winning Condition (Awards Only)
        if (current_room->x == 2 && current_room->y == 2 && 
            has_collected_all_awards(rooms, room_count, player) && creatures_left == 0) {
            printf("You have collected all awards!\n");
            printf("You returned to the starting room and completed your mission successfully!\n");
            printf("Congratulations! You won the game.\n");
            exit(0);  // End the game
        }
    } else {
        printf("You are in an empty area. There is no room here.\n");
    }
}

Room* find_room_at_position(Room *rooms[], int room_count, int x, int y) {
    for (int i = 0; i < room_count; i++) {
        if (rooms[i]->x == x && rooms[i]->y == y) {
            return rooms[i];
        }
    }
    return NULL;
}

void pickup_item(Player *player, Room *rooms[], int room_count, char *item_name) {
    Room *current_room = find_room_at_position(rooms, room_count, player->x, player->y);
    if (current_room == NULL) {
        printf("There is no room here, you cannot pick up an item.\n");
        return;
    }
    for (int i = 0; i < current_room->item_count; i++) {
        if (strcmp(current_room->items[i]->name, item_name) == 0) {
            if (player->inventory_count < MAX_INVENTORY) {
                player->inventory[player->inventory_count++] = current_room->items[i];
                for (int j = i; j < current_room->item_count - 1; j++) {
                    current_room->items[j] = current_room->items[j + 1];
                }
                current_room->item_count--;
                printf("%s picked up.\n", item_name);
                return;
            } else {
                printf("Inventory is full!\n");
                return;
            }
        }
    }
    printf("Item not found: %s\n", item_name);
}

void attack_creature(Player *player, Room *rooms[], int room_count) {
    Room *current_room = find_room_at_position(rooms, room_count, player->x, player->y);
    if (!current_room || !current_room->creature) {
        printf("There is no creature here.\n");
        return;
    }

    Creature *creature = current_room->creature;
    printf("You started a battle with %s!\n", creature->name);

    while (creature->health > 0 && player->health > 0) {
        int player_damage = rand() % compute_total_attack(player) + 1;
        printf("You dealt %d damage to %s.\n", player_damage, creature->name);
        creature->health -= player_damage;

        if (creature->health <= 0) {
            printf("You defeated %s!\n", creature->name);
            free(creature->name);
            free(creature);
            current_room->creature = NULL;
            creatures_left--;  // Decrease creature count

            // Drop an item from the creature
            Item *dropped_item = (Item *)malloc(sizeof(Item));
            if (!dropped_item) {
                perror("Failed to allocate memory for dropped item");
                exit(EXIT_FAILURE);
            }
            dropped_item->name = (char *)malloc(32);
            if (!dropped_item->name) {
                perror("Failed to allocate memory for dropped item name");
                free(dropped_item);
                exit(EXIT_FAILURE);
            }
            sprintf(dropped_item->name, "award%d", rand() % 100);
            dropped_item->attack_bonus = rand() % 5 + 1;
            dropped_item->shield_bonus = rand() % 5 + 1;
            current_room->items[current_room->item_count++] = dropped_item;

            printf("An item dropped: %s\n", dropped_item->name);
            return;
        }

        int creature_damage = rand() % creature->strength + 1 - compute_total_shield(player);
        if (creature_damage < 0) creature_damage = 0;

        printf("%s dealt %d damage to you.\n", current_room->creature->name, creature_damage);
        player->health -= creature_damage;

        if (player->health <= 0) {
            printf("You lost. Game over.\n");
            exit(0);
        }
    }
}

void list_inventory(Player *player) {
    printf("Inventory:\n");
    for (int i = 0; i < player->inventory_count; i++) {
        Item *item = player->inventory[i];
        printf("- %s", item->name);
        if (item->attack_bonus > 0) {
            printf(" (+%d attack)", item->attack_bonus);
        }
        if (item->shield_bonus > 0) {
            printf(" (+%d shield)", item->shield_bonus);
        }
        printf("\n");
    }
}

// List Saved Games
void list_saved_games() {
    FILE *file = fopen("saved_game.txt", "r");
    if (!file) {
        printf("No saved games found.\n");
        return;
    }

    printf("Saved Games:\n");
    char filepath[MAX_FILENAME_LENGTH];
    while (fscanf(file, "%255s", filepath) == 1) {
        printf("- %s\n", filepath);
    }
    fclose(file);
}

void load_saved_games() {
    FILE *file = fopen("saved_game.txt", "r");
    if (file) {
        while (fscanf(file, "%255s", saved_games[saved_game_count]) == 1) {
            saved_game_count++;
            if (saved_game_count >= MAX_SAVED_GAMES) break;
        }
        fclose(file);
    }
}

void save_game_to_list(const char *filepath) {
    FILE *file = fopen("saved_game.txt", "a");
    if (file) {
        fprintf(file, "%s\n", filepath);
        fclose(file);
    } else {
        perror("Error saving game path");
    }
}

void delete_saved_game(const char *filepath) {
    int file_deleted = remove(filepath);

    if (file_deleted == 0) {
        printf("Successfully deleted %s from the directory.\n", filepath);
    } else {
        perror("Error deleting file from the directory");
    }

    FILE *file = fopen("saved_game.txt", "w");
    if (file) {
        int found = 0;
        for (int i = 0; i < saved_game_count; i++) {
            if (strcmp(saved_games[i], filepath) != 0) {
                fprintf(file, "%s\n", saved_games[i]);
            } else {
                found = 1;
            }
        }
        fclose(file);

        if (found) {
            printf("Removed %s from saved_game.txt.\n", filepath);
            load_saved_games();
        } else {
            printf("File %s not found in saved_game.txt.\n", filepath);
        }
    } else {
        perror("Error updating saved_game.txt");
    }
}

// Save the Game
// Save the Game
void save_game(Player *player, Room *rooms[], int room_count, const char *filepath) {
    FILE *file = fopen(filepath, "w");
    if (!file) {
        perror("Error saving game");
        return;
    }

    // Save player data
    fprintf(file, "Nickname: %s\n", player->nickname);
    fprintf(file, "Health: %d\nBase Strength: %d\nPosition: %d %d\nInventory Count: %d\n",
            player->health, player->base_strength, player->x, player->y, player->inventory_count);
    
    // Save inventory
    fprintf(file, "Inventory:\n");
    for (int i = 0; i < player->inventory_count; i++) {
        Item *item = player->inventory[i];
        fprintf(file, "%s %d %d\n", item->name, item->attack_bonus, item->shield_bonus);
    }

    // Save creatures_left
    fprintf(file, "Creatures Left: %d\n", creatures_left);

    // Save room count
    fprintf(file, "Room Count: %d\n", room_count);

    // Save each room's data
    for (int i = 0; i < room_count; i++) {
        Room *room = rooms[i];
        fprintf(file, "Room %d:\n", room->id);
        fprintf(file, "Description: %s\n", room->description);
        fprintf(file, "Position: %d %d\n", room->x, room->y);
        fprintf(file, "Item Count: %d\n", room->item_count);
        for (int j = 0; j < room->item_count; j++) {
            Item *item = room->items[j];
            fprintf(file, "Item: %s %d %d\n", item->name, item->attack_bonus, item->shield_bonus);
        }
        if (room->creature) {
            fprintf(file, "Creature: %s %d %d\n", room->creature->name, room->creature->health, room->creature->strength);
        } else {
            fprintf(file, "Creature: None\n");
        }
    }

    // Save discovered rooms
    fprintf(file, "Discovered Rooms:\n");
    for (int i = 0; i < room_count; i++) {
        if (discovered[rooms[i]->y][rooms[i]->x]) {
            fprintf(file, "%d %d\n", rooms[i]->x, rooms[i]->y);
        }
    }
    fclose(file);

    // Save the file path to the saved games list
    if (saved_game_count < MAX_SAVED_GAMES) {
        strncpy(saved_games[saved_game_count], filepath, MAX_FILENAME_LENGTH - 1);
        saved_games[saved_game_count][MAX_FILENAME_LENGTH - 1] = '\0';  // Ensure null-termination
        saved_game_count++;
    } else {
        printf("Error: Maximum saved games reached.\n");
    }

    printf("Game saved to %s.\n", filepath);
}


int load_game(Player *player, Room **rooms, int *room_count, const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("Error loading game");
        printf("Details: Could not open file %s. Ensure the file exists and is readable.\n", filepath);
        return 0;
    }

    memset(discovered, 0, sizeof(discovered));

    // Read player data
    if (fscanf(file, "Nickname: %49s\n", player->nickname) != 1) {
        printf("Error: Could not read nickname! File might be corrupted.\n");
        fclose(file);
        return 0;
    }

    if (fscanf(file, "Health: %d\nBase Strength: %d\nPosition: %d %d\nInventory Count: %d\n",
               &player->health, &player->base_strength, &player->x, &player->y, &player->inventory_count) != 5) {
        printf("Error: Player information missing! File might be corrupted.\n");
        fclose(file);
        return 0;
    }

    // Load inventory
    {
        char line[256];
        if (fgets(line, sizeof(line), file) == NULL || strncmp(line, "Inventory:", 10) != 0) {
            printf("Error: Could not read Inventory header!\n");
            fclose(file);
            return 0;
        }

        for (int i = 0; i < player->inventory_count; i++) {
            char item_name[32];
            int attack_bonus, shield_bonus;

            if (fscanf(file, "%31s %d %d\n", item_name, &attack_bonus, &shield_bonus) != 3) {
                printf("Error: Could not read inventory item!\n");
                fclose(file);
                return 0;
            }

            Item *item = (Item *)malloc(sizeof(Item));
            if (!item) {
                printf("Error: Memory allocation failed for item!\n");
                fclose(file);
                return 0;
            }

            item->name = strdup(item_name);
            item->attack_bonus = attack_bonus;
            item->shield_bonus = shield_bonus;
            player->inventory[i] = item;
        }
    }

    // Load creatures_left
    if (fscanf(file, "Creatures Left: %d\n", &creatures_left) != 1) {
        printf("Error: Could not read creatures_left! File might be corrupted.\n");
        fclose(file);
        return 0;
    }

    // Load room count
    if (fscanf(file, "Room Count: %d\n", room_count) != 1) {
        printf("Error: Could not read room count!\n");
        fclose(file);
        return 0;
    }

    // Load each room's data
    for (int i = 0; i < *room_count; i++) {
        Room *room = (Room *)malloc(sizeof(Room));
        if (!room) {
            printf("Error: Memory allocation failed for room!\n");
            fclose(file);
            return 0;
        }

        // Read room header
        if (fscanf(file, "Room %d:\n", &room->id) != 1) {
            printf("Error: Could not read room ID!\n");
            free(room);
            fclose(file);
            return 0;
        }

        // Read description
        char description_buffer[256];
        if (fgets(description_buffer, sizeof(description_buffer), file) == NULL ||
            sscanf(description_buffer, "Description: %[^\n]\n", description_buffer) != 1) {
            printf("Error: Could not read room description!\n");
            free(room);
            fclose(file);
            return 0;
        }
        room->description = strdup(description_buffer);

        // Read position
        if (fscanf(file, "Position: %d %d\n", &room->x, &room->y) != 2) {
            printf("Error: Could not read room position!\n");
            free(room->description);
            free(room);
            fclose(file);
            return 0;
        }

        // Read item count
        if (fscanf(file, "Item Count: %d\n", &room->item_count) != 1) {
            printf("Error: Could not read item count!\n");
            free(room->description);
            free(room);
            fclose(file);
            return 0;
        }

        // Load items
        for (int j = 0; j < room->item_count; j++) {
            char item_label[10];
            char item_name[32];
            int attack_bonus, shield_bonus;

            if (fscanf(file, "Item: %31s %d %d\n", item_name, &attack_bonus, &shield_bonus) != 3) {
                printf("Error: Could not read room item!\n");
                free(room->description);
                free(room);
                fclose(file);
                return 0;
            }

            Item *item = (Item *)malloc(sizeof(Item));
            if (!item) {
                printf("Error: Memory allocation failed for item!\n");
                free(room->description);
                free(room);
                fclose(file);
                return 0;
            }

            item->name = strdup(item_name);
            item->attack_bonus = attack_bonus;
            item->shield_bonus = shield_bonus;
            room->items[j] = item;
        }

        // Read creature
        char creature_label[20];
        char line[256];
        if (fgets(line, sizeof(line), file) == NULL) {
            printf("Error: Could not read creature information!\n");
            // Handle as no creature
            room->creature = NULL;
        } else {
            if (strncmp(line, "Creature: None", 14) == 0) {
                room->creature = NULL;
            } else {
                char creature_name[32];
                int creature_health, creature_strength;
                if (sscanf(line, "Creature: %31s %d %d\n", creature_name, &creature_health, &creature_strength) != 3) {
                    printf("Error: Could not read creature data!\n");
                    // Handle as no creature
                    room->creature = NULL;
                } else {
                    Creature *creature = (Creature *)malloc(sizeof(Creature));
                    if (!creature) {
                        printf("Error: Memory allocation failed for creature!\n");
                        // Handle as no creature
                        room->creature = NULL;
                    } else {
                        creature->name = strdup(creature_name);
                        creature->health = creature_health;
                        creature->strength = creature_strength;
                        room->creature = creature;
                    }
                }
            }
        }

        // Add room to rooms array
        rooms[room->id] = room;
    }

    // Load discovered rooms
    {
        char line[256];
        if (fgets(line, sizeof(line), file) == NULL || strncmp(line, "Discovered Rooms:", 17) != 0) {
            printf("Warning: No discovered rooms found in save file.\n");
        } else {
            int x, y;
            while (fscanf(file, "%d %d\n", &x, &y) == 2) {
                if (y >= 0 && y < MAP_SIZE && x >= 0 && x < MAP_SIZE) {
                    discovered[y][x] = 1;
                }
            }
        }
    }

    fclose(file);

    printf("Game loaded successfully from %s.\n", filepath);
    return 1;
}

int is_nickname_taken(const char *nickname) {
    for (int i = 0; i < saved_game_count; i++) {
        FILE *file = fopen(saved_games[i], "r");
        if (file) {
            char saved_nickname[50];
            if (fscanf(file, "Nickname: %49s\n", saved_nickname) == 1 && strcmp(saved_nickname, nickname) == 0) {
                fclose(file);
                return 1;
            }
            fclose(file);
        }
    }
    return 0;
}

void free_resources(Room *rooms[], int room_count, Player *player) {
    for (int i = 0; i < room_count; i++) {
        if (rooms[i]) {
            free(rooms[i]->description);
            for (int j = 0; j < rooms[i]->item_count; j++) {
                free(rooms[i]->items[j]->name);
                free(rooms[i]->items[j]);
            }
            if (rooms[i]->creature) {
                free(rooms[i]->creature->name);
                free(rooms[i]->creature);
            }
            free(rooms[i]);
        }
    }
    for (int i = 0; i < player->inventory_count; i++) {
        free(player->inventory[i]->name);
        free(player->inventory[i]);
    }
}

void display_map(Room *rooms[], int room_count, Player *player) {
    printf("Map:\n");
    for (int i = 0; i < MAP_SIZE; i++) {
        for (int j = 0; j < MAP_SIZE; j++) {
            if (player->x == j && player->y == i) {
                printf("[P]");  // Player's current position
            } else if (i == 2 && j == 2) {
                printf("[I]");  // Starting room
            } else if (find_room_at_position(rooms, room_count, j, i)) {
                printf("[R]");  // Room exists
            } else {
                printf("[X]");  // No room at this position
            } 
        }
        printf("\n");
    }
}

void display_help() {
    printf("Available commands:\n");
    printf("- move <direction>: Move in a direction (up, down, left, right).\n");
    printf("- look: Examine the current location.\n");
    printf("- inventory: View your inventory.\n");
    printf("- pickup <item>: Pick up an item in the room.\n");
    printf("- attack: Attack the creature in the room.\n");
    printf("- status: Display player status.\n");
    printf("- save <filepath>: Save the game.\n");
    printf("- load <filepath>: Load a saved game.\n");
    printf("- list: List all saved games.\n");
    printf("- delete <filepath>: Delete a saved game.\n");
    printf("- map: Display the map.\n");
    printf("- help: Display this help message.\n");
    printf("- exit: Exit the game.\n");
}

void display_status(Player *player) {
    int total_attack = compute_total_attack(player);
    int total_shield = compute_total_shield(player);
    printf("Player Status:\n");
    printf("Health: %d\n", player->health);
    printf("Attack Power: %d\n", total_attack);
    printf("Shield Power: %d\n", total_shield);
}

int compute_total_attack(Player *player) {
    int total_attack = player->base_strength;
    for (int i = 0; i < player->inventory_count; i++) {
        total_attack += player->inventory[i]->attack_bonus;
    }
    return total_attack;
}

int compute_total_shield(Player *player) {
    int total_shield = 0;  // Initial shield value
    for (int i = 0; i < player->inventory_count; i++) {
        total_shield += player->inventory[i]->shield_bonus;
    }
    return total_shield;
}
