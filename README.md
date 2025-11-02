# ESP32 Quest System Documentation

## Overview
A gamified productivity tracker with matrix-style animations, level progression, and quest timers on an OLED display.

---

## Hardware Requirements
- ESP32 microcontroller
- 128x64 OLED display (SSD1306, I2C address: 0x3C)
- Push button connected to GPIO pin 4
- Pull-up resistor for button (or use internal pull-up)

---

## How It Works

### **Three Screens**
1. **Main Screen** - Matrix rain animation with motivational messages
2. **Status Screen** - Shows player stats (level, XP, quest count)
3. **Quest Screen** - Start a timed quest

**Navigate**: Short press button to cycle through screens  
**Start Quest**: Long press (1 second) on Quest Screen

---

## Core Systems

### **1. Player Progression**
- Start at Level 1 with 0 XP
- Complete quests to gain XP (10-15 + level bonus)
- Level up when XP reaches threshold
- XP requirement increases by 1.5x each level
- Progress saved to persistent storage

### **2. Quest System**
- Duration: 2 minutes (120 seconds)
- Shows countdown timer with progress bar
- Press button during quest to cancel
- Completion: Animated cube → claim reward → gain XP
- Tracks total quests completed

### **3. Animations**
- **Matrix Rain**: Digital rain effect on Main/Quest screens
- **Cube Bounce**: 3D rotating diamond after quest completion
- **XP Glitch**: Cyberpunk-style glitch effect when gaining XP
- **Level Up**: Flash and bounce animation

---

## Code Structure

### **Main Sections**
```
INCLUDES & SETUP          - Libraries and hardware config
PLAYER SYSTEM             - Level, XP, quest tracking
BUTTON HANDLING           - Input detection and debouncing
SCREEN SYSTEM             - Screen state management
MATRIX ANIMATION          - Digital rain effect
3D GRAPHICS               - Rotating cube/diamond
STORAGE                   - Save/load player data
VISUAL EFFECTS            - Progress bars, fade effects
SCREEN RENDERING          - Display functions for each screen
XP ANIMATIONS             - Reward and level up effects
QUEST LOGIC               - Timer and completion handling
INPUT PROCESSING          - Button press handlers
SETUP & LOOP              - Initialization and main loop
```

---

## Key Functions

### **Screen Management**
- `showMain()` - Display matrix rain with messages
- `showStatus()` - Display player stats
- `showQuest()` - Display quest entry screen

### **Quest Functions**
- `startQuest()` - Run 2-minute quest timer
- `showCubeAnimationUntilButton()` - Reward animation

### **Progression**
- `showXPAdded()` - Glitch animation when gaining XP
- `showLevelUp()` - Celebration animation for leveling

### **Storage**
- `savePlayerData()` - Persist progress to flash memory
- `loadPlayerData()` - Restore progress on boot

### **Input**
- `processButtonInput()` - Handle button presses with debouncing
- `handleShortPress()` - Cycle screens
- `handleLongPress()` - Start quest

---

## Button Controls

| Action | Function |
|--------|----------|
| **Short Press** | Cycle: Main → Status → Quest → Main |
| **Long Press (1s)** | Start quest (only on Quest Screen) |
| **Press During Quest** | Cancel quest |
| **Press During Cube** | Claim reward and continue |

---

## Customization

### **Change Quest Duration**
```cpp
unsigned long questDuration = 120000; // milliseconds (120s = 2min)
```

### **Adjust XP Rewards**
```cpp
xpReward = random(10, 15) + (level * 1.5); // Min, Max, Level bonus
```

### **Modify Messages**
```cpp
const char* messages[] = {
  "  - STAY FOCUSED -  ",
  "  - YOU GOT THIS! -  ",
  // Add your messages here
};
```

### **Change Player Name**
```cpp
display.print("PLAYER: JULIUS"); // Edit here
```

---

## Storage Keys
Data saved to ESP32 flash memory:
- `level` - Current player level
- `xp` - Current experience points
- `xpToNext` - XP needed for next level
- `screen` - Last active screen
- `totalQuests` - Total quests completed

---

## Troubleshooting

**Display not working?**
- Check I2C address (default: 0x3C)
- Verify wiring: SDA and SCL connections
- Check power supply

**Button not responding?**
- Verify GPIO 4 connection
- Check pull-up resistor
- Try adjusting `debounceDelay` value

**Progress not saving?**
- Ensure stable power supply
- Check Serial Monitor for errors
- Verify flash memory is not full

---

## Serial Monitor Output
Monitor quest completion and XP gains:
```
Quest complete!
Gained 12 XP
```

Set baud rate to **115200** in Serial Monitor.

---

## Adding New Features

### **Add a New Screen**
1. Add enum value: `SCREEN_NEW` to `Screen` enum
2. Create function: `void showNew() { }`
3. Add case in `handleShortPress()` navigation
4. Implement rendering logic

### **Add New Animation**
1. Create animation function in Visual Effects section
2. Call from appropriate screen function
3. Use `display.clearDisplay()` and `display.display()`

### **Change Progression Formula**
Modify in `startQuest()`:
```cpp
xpToNext = (int)(xpToNext * 1.5); // Change multiplier
```

---

## Tips
- Matrix rain automatically plays on Main and Quest screens
- All progress auto-saves after XP gain
- Quest can be cancelled anytime by pressing button
- Cube animation is interactive - press to continue
- Long conversations update message every 10 seconds
