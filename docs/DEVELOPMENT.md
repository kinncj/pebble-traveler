# Development Notes

## Project Structure

```
pebble-traveler/
├── watch-face/          # Watch face with tap controls (v4.0.0)
│   ├── src/c/main.c     # Watch face implementation
│   ├── src/pkjs/        # Configuration interface
│   └── package.json     # Watch face metadata
├── watch-app/           # Standalone app with buttons (v2.0.0)  
│   ├── src/c/main.c     # Watch app implementation
│   ├── src/pkjs/        # Configuration interface
│   └── package.json     # Watch app metadata
├── docs/                # Documentation
├── screenshots/         # App store assets
└── environment.yml      # Conda environment
```

## Environment Setup

### Conda Environment (Recommended)
```bash
# Create isolated Python environment
conda create -n pebble-env python=3.9
conda activate pebble-env

# Install Pebble SDK dependencies
pip install virtualenv
```

### Development Workflow

#### Watch Face Development
```bash
export PATH="/Users/$USER/.local/bin:$PATH"
conda activate pebble-env
cd watch-face
pebble build
pebble install --emulator basalt
pebble emu-tap  # Test unified tap (backlight + timezone cycle)
pebble logs --emulator basalt
```

#### Watch App Development  
```bash
export PATH="/Users/$USER/.local/bin:$PATH"
conda activate pebble-env
cd watch-app
pebble build
pebble install --emulator basalt
# Test button navigation in emulator
pebble logs --emulator basalt
```

## Architecture

### Shared Components
- `src/shared/timezones.h`: Timezone struct definitions and extern declarations
- `src/shared/timezones.canonical.json`: 540 timezone definitions (source of truth)
- Identical timezone calculation logic with UTC + offset_minutes
- Same Clay-based configuration system
- Unified AppMessage protocol (12 message keys)

### Watch Face (Tap Controls)
- `watch-face/src/c/main.c`: Watch face implementation with accelerometer tap handler
- Unified tap control: single tap turns on backlight and cycles timezone
- Direct backlight control via `light_enable_interaction()` 
- Always-on display as primary watch face
- Memory footprint: 30.5KB

### Watch App (Button Controls)
- `watch-app/src/c/main.c`: Standalone app implementation with button handlers
- UP/DOWN buttons for timezone navigation
- SELECT button for backlight control
- BACK button with exit protection (hold to exit)
- Launch-when-needed functionality
- Memory footprint: 21.9KB (optimized)

### Phone-Side (JavaScript) - Shared
- `src/pkjs/index.js`: Clay configuration page
- `src/pkjs/config.js`: Configuration UI definitions  
- Builds configuration UI with timezone autocomplete
- Sends compact timezone data via AppMessage
- Version-specific success messages

## Memory Constraints

### Platform Limits
- **Aplite**: 24KB APP region - **EXCLUDED** (timezone data causes overflow)
- **Basalt** (Pebble Time): 64KB RAM - ✅ Both versions supported
  - Watch Face: 30.5KB footprint, 35KB+ free heap
  - Watch App: 21.9KB footprint, 43KB+ free heap
- **Chalk** (Pebble Time Round): 64KB RAM - ✅ Both versions supported
  - Watch Face: 30.5KB footprint, 35KB+ free heap  
  - Watch App: 21.9KB footprint, 43KB+ free heap
- **Diorite** (Pebble Time Steel): 64KB RAM - ✅ Both versions supported
  - Watch Face: 30.5KB footprint, 35KB+ free heap
  - Watch App: 21.9KB footprint, 43KB+ free heap
- **Emery** (Pebble 2 HR): 128KB RAM - ✅ Both versions supported
  - Watch Face: 30.5KB footprint, 100KB+ free heap
  - Watch App: 21.9KB footprint, 108KB+ free heap

### Optimization Strategies Applied
1. **Compact timezone data**: Use offset_minutes instead of complex structs
2. **Shared definitions**: Single source of truth between phone and watch
3. **Minimal embedment**: Watch stores only selected timezone configurations
4. **Efficient calculations**: UTC + offset arithmetic instead of full timezone libraries
5. **Adaptive timing**: Minute-based updates normally, second-based when seconds display enabled
6. **Color optimization**: Platform-specific color handling with B&W fallbackstecture

### Watch-Side (C)
- `src/c/main.c`: Main watch face implementation
- `src/shared/timezones.h`: Timezone struct definitions and extern declarations
- Uses UTC + offset_minutes for timezone calculations
- Implements Y-axis accelerometer tap handling
- Manages persistent storage of timezone configurations

### Phone-Side (JavaScript)
- `src/pkjs/index.js`: Clay configuration page
- `src/shared/timezones.canonical.json`: 540 timezone definitions (source of truth)
- Builds configuration UI with timezone autocomplete
- Sends compact timezone data via AppMessage

## Memory Constraints

### Platform Limits
- **Aplite**: 24KB APP region - **EXCLUDED** (timezone data causes 4332 byte overflow)
- **Basalt**: 64KB RAM - ✅ Supported (28.9KB footprint, 36.6KB free heap)
- **Emery**: 128KB RAM - ✅ Supported (28.9KB footprint, 102KB free heap)

### Optimization Strategies Applied
1. **Compact timezone data**: Use offset_minutes instead of complex structs
2. **Shared definitions**: Single source of truth between phone and watch
3. **Minimal embedment**: Watch stores only selected timezone configurations
4. **Efficient calculations**: UTC + offset arithmetic instead of full timezone libraries

## AppMessage Protocol

### Phone → Watch
```c
MESSAGE_KEY_HOME: "timezone_identifier"
MESSAGE_KEY_TIMEZONE_1: "timezone_identifier" 
MESSAGE_KEY_TIMEZONE_2: "timezone_identifier"
MESSAGE_KEY_TIMEZONE_3: "timezone_identifier"
MESSAGE_KEY_TIMEZONE_4: "timezone_identifier"
MESSAGE_KEY_ALWAYS_SHOW_HOME: bool
MESSAGE_KEY_BACKGROUND_COLOR: int32 (hex color)
MESSAGE_KEY_TIME_COLOR: int32 (hex color)
MESSAGE_KEY_TIMEZONE_LABEL_COLOR: int32 (hex color)
MESSAGE_KEY_HOME_TIME_COLOR: int32 (hex color)
MESSAGE_KEY_SHOW_SECONDS: bool
MESSAGE_KEY_SHOW_HOME_SECONDS: bool
```

### Color Handling
```c
// Platform-specific color conversion
static GColor hex_to_gcolor(uint32_t hex) {
#ifdef PBL_COLOR
  return GColorFromHEX(hex);
#else
  // Grayscale conversion for B&W displays
  uint8_t gray = ((hex >> 16) + (hex >> 8) + hex) / 3;
  return gray > 128 ? GColorWhite : GColorBlack;
#endif
}
```

### Timezone Data Format
```c
typedef struct {
  int id;
  const char *identifier;      // "America/New_York"
  const char *display_name;    // "New York"
  const char *abbreviation;    // "EST"
  const char *offset_str;      // "-05:00"
  int offset_minutes;          // -300
} SharedTimezone;
```

## Build System

### Utilities
- `utility/compact_timezones.py`: Canonicalizes timezone JSON
- `utility/timezone_tool.py`: Generates headers from JSON
- `utility/generate_timezones_c.py`: Creates C source files

### Target Platforms
```json
"targetPlatforms": ["basalt", "chalk", "diorite", "emery"]
```

All four color Pebble platforms are supported with full feature parity.

## Known Issues

1. **Aplite Memory**: Timezone database too large for Aplite's 24KB APP region
2. **Timezone Updates**: Manual process to update timezone definitions
3. **Network Dependency**: Configuration requires phone connectivity

## Future Enhancements

1. **Compression**: Implement timezone data compression for Aplite support
2. **Caching**: Smart timezone caching based on user location
3. **Dynamic Loading**: Load timezone data on-demand from phone
4. **DST Handling**: More sophisticated daylight saving time transitions
5. **Custom Themes**: Predefined color themes for quick selection
6. **Weather Integration**: Optional weather display for current timezone
7. **Animation**: Smooth transitions between timezone switches
8. **Configuration Fix**: Resolve emulator configuration timeout issues

## CI/CD Pipeline

### GitHub Actions Workflows

#### Build & Test (`.github/workflows/build.yml`)
- **Triggers**: PRs and pushes to main branch
- **Environment**: Ubuntu with conda setup
- **Steps**:
  1. Checkout code
  2. Setup Node.js 18
  3. Install Pebble SDK 4.5
  4. Install ARM toolchain
  5. Build for both basalt and emery
  6. Upload build artifacts

#### Release (`.github/workflows/release.yml`)
- **Triggers**: Git tags matching `v*`
- **Environment**: Ubuntu with conda setup
- **Steps**:
  1. Build watch face packages
  2. Create GitHub release
  3. Upload .pbw files as release assets
  4. Include build artifacts archive

### Local Testing
```bash
export PATH="/Users/$USER/.local/bin:$PATH"
conda activate pebble-env
cd watch-face

# Build for all platforms
pebble build

# Install to specific emulator
pebble install --emulator basalt   # Pebble Time
pebble install --emulator chalk    # Pebble Time Steel
pebble install --emulator diorite  # Pebble Time 2
pebble install --emulator emery    # Pebble 2 HR

# Test unified tap control
pebble emu-tap                      # Turns on backlight and cycles timezone
pebble emu-tap                      # Turns on backlight and cycles to next timezone
pebble emu-tap                      # Turns on backlight and cycles to next timezone

# Test with different directions (all work the same)
pebble emu-tap --direction y+       # Unified action
pebble emu-tap --direction x+       # Unified action

# View logs
pebble logs --emulator basalt

# Open configuration (troubleshooting required)
pebble emu-app-config
```

## Known Issues

1. **Aplite Memory**: Timezone database too large for Aplite's 24KB APP region
2. **Configuration Timeout**: `pebble emu-app-config` experiences timeouts (needs investigation)
3. **Timezone Updates**: Manual process to update timezone definitions
4. **Network Dependency**: Configuration requires phone connectivity

## Troubleshooting

### Configuration Page Timeout
If `pebble emu-app-config` times out:
1. Ensure emulator is running and watch face is installed
2. Try restarting the emulator
3. Check that AppMessage is properly initialized
4. Verify Clay configuration syntax

### Memory Issues
Monitor memory usage during builds:
- Basalt: Should stay under 64KB total
- Emery: Should stay under 128KB total
- Check for memory leaks in timezone handling
