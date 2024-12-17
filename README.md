# Dungeon Adventure Game

## Overview
The Dungeon Adventure Game is a text-based RPG where players explore a dungeon, collect items, fight creatures, and try to win by collecting all awards and returning to the starting room. This game features a procedurally generated dungeon with various rooms, items, and creatures.

---

## Game Features
- **Exploration:** Discover different rooms with unique descriptions.
- **Inventory Management:** Collect and manage items with attack and shield bonuses.
- **Combat System:** Engage in battles with creatures that have health and strength attributes.
- **Game Persistence:** Save and load games with file-based storage.
- **Room Map:** View the dungeon layout with an interactive map.

---

## How to Play
### Starting the Game
- On launch, choose:
  - `1 - New Game`: Start a new game by entering a unique nickname.
  - `2 - Load Game`: Load a previously saved game by providing a filename.

### Game Commands
- **Movement:**
  - `move <direction>` - Move in one of the directions: `up`, `down`, `left`, `right`.
- **Exploration:**
  - `look` - Examine the current room.
  - `map` - Display the dungeon map.
- **Inventory:**
  - `inventory` - List collected items.
  - `pickup <item>` - Pick up an item from the room.
- **Combat:**
  - `attack` - Engage in combat with the room's creature.
- **Game Management:**
  - `save <filename>` - Save the current game.
  - `load <filename>` - Load a saved game.
  - `list` - Show all saved games.
  - `delete <filename>` - Delete a saved game.
  - `status` - View player stats.
  - `help` - Display the help menu.
  - `exit` - Exit the game.

---

## Compilation and Execution
### Compilation
#### Windows (using MinGW)
```
mingw32-make
```

#### Linux
```
make
```

### Running the Game
#### Windows
```
Dungeon_Adventure_Game.exe
```

#### Linux
```
./Dungeon_Adventure_Game
```

---

## Gameplay Mechanics
### Rooms
- Each room has:
  - A unique description.
  - Randomly placed items.
  - A creature (optional).
- Starting Room (2,2):
  - Acts as a safe zone.
  - Restores player health when revisited.

### Items
- **Attack Bonus:** Increases player's attack power.
- **Shield Bonus:** Reduces damage taken.

### Creatures
- **Health:** Determines how much damage they can take.
- **Strength:** Determines how much damage they can inflict.

### Combat
- Players attack creatures based on their total attack power.
- Creatures counterattack based on their strength.
- Victory: Creature defeated; possible item drop.
- Loss: Game over.

### Winning Condition
- Collect all award items.
- Defeat all creatures.
- Return to the starting room.

---

## Game Save & Load
- **Save File Format:** Text file storing player stats, inventory, rooms, items, and discovered rooms.
- **Loading Validation:** Ensures file integrity during load.

---

## Technical Details
### Key Constants
- `MAP_SIZE`: Defines the dungeon's size (5x5 grid).
- `MAX_ROOMS`: Maximum number of rooms (10).
- `MAX_ITEMS`: Maximum number of items per room (10).
- `FIXED_CREATURE_COUNT`: Total number of creatures (5).

### Core Data Structures
- `Player`: Holds player stats, inventory, and position.
- `Room`: Contains room description, items, creatures, and position.
- `Item`: Represents in-game items with attack and shield bonuses.
- `Creature`: Describes hostile creatures.

### Memory Management
- All dynamically allocated memory for rooms, items, and creatures is freed at game termination.

---

## Future Improvements
- Add more interactive elements like puzzles and traps.
- Implement multiple save slots with timestamps.
- Add experience points and leveling.
- Enable multiplayer gameplay.

---

## Credits
- **Developer:** Cihan
- **Language:** C
- **Frameworks:** Standard C Libraries

---

**Enjoy the Dungeon Adventure Game!**

