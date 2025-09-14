# Timezone Traveler - Pebble Watch Face

A configurable multi-timezone watch face for Pebble watches supporting six total timezones: Local + Home + 4 configurable slots, with unified tap control (tap turns on backlight and cycles timezone), customizable colors, and flexible display options.

## Features

- **Six Total Timezones**: Local (fixed) + Home + 4 configurable timezone slots with GMT offset display
- **Unified Tap Control**: Single tap turns on backlight and cycles to next timezone
- **Simple Interaction**: One action does both backlight and navigation for convenience
- **Customizable Colors**: Background, main time, timezone labels, and home time colors
- **Display Options**: 
  - Optional seconds display for main time
  - Optional seconds display for home time
  - Optional "Always Display Home Timezone?" toggle
- **Phone Configuration**: Configure timezones and appearance via Clay configuration page
- **Cross-Platform Color Support**: Full color support on Pebble Time+, graceful grayscale on B&W displays
- **GMT Offset Display**: Shows timezone names with their UTC offsets (e.g., "Tokyo (GMT +09:00)", "local (GMT -08:00)")
- **Single Source of Truth**: Shared timezone definitions between phone and watch
- **Compact Design**: Clean time display with timezone labels

## Installation & Development

### Prerequisites
- Pebble SDK 4.5+
- Node.js 18+
- Conda environment (recommended)

### Development Setup
1. Clone the repository:
   ```bash
   git clone https://github.com/kinncj/pebble-traveler.git
   cd pebble-traveler
   ```

2. Set up conda environment:
   ```bash
   conda create -n pebble-env python=3.9
   conda activate pebble-env
   ```

3. Install Pebble SDK and dependencies (follow Pebble SDK installation guide)

4. Build the watch face:
   ```bash
   cd watch-face
   npm install
   pebble build
   ```

5. Install to emulator:
   ```bash
   pebble install --emulator basalt
   # or chalk, diorite, emery
   ```

### CI/CD
The project includes GitHub Actions for:
- **Continuous Integration**: Builds and tests on PRs and main branch commits
- **Automated Releases**: Creates releases with .pbw packages when tags are pushed
- **Cross-platform builds**: Supports both basalt and emery platforms

See `.github/workflows/` for complete CI/CD configuration.

## Supported Platforms

- **Pebble Time** (basalt) - 64KB RAM
- **Pebble Time Steel** (chalk) - 64KB RAM  
- **Pebble Time 2** (diorite) - 64KB RAM
- **Pebble 2 HR** (emery) - 128KB RAM

All platforms support full color customization. Memory usage: ~30.5KB footprint with 35KB+ free heap.

Note: Aplite platform is not supported due to memory constraints with the timezone database.

## Default Behavior

The watch face includes:
- **Slot 0 (Local)**: Your device's local timezone (fixed)
- **Slot 1 (Home)**: Configurable home timezone
- **Slots 2-5**: Four additional configurable timezone slots

## Configuration

1. Open the Pebble app on your phone
2. Go to the watch face settings for "Timezone Traveler"
3. **Timezone Settings**:
   - Configure your Home timezone (slot 1)
   - Configure up to 4 additional timezones (slots 2-5)
   - Available timezones include 540+ options with proper abbreviations and UTC offsets
4. **Display Options**:
   - Toggle "Always Display Home Timezone?" to show home time below main display
   - Enable "Show Seconds (Main Time)" to display seconds for the current timezone
   - Enable "Show Seconds (Home Time)" to display seconds for the home timezone when visible
5. **Color Settings** (Pebble Time and later):
   - Background Color: Choose the watch face background
   - Main Time Color: Color for the primary time display
   - Timezone Label Color: Color for timezone names and GMT offsets
   - Home Time Color: Color for the secondary home timezone display
6. Save settings to sync with the watch

Note: Color settings work on color Pebble models (Time, Time Steel, Time 2, etc.). On black and white models, colors are automatically converted to appropriate grayscale values.

## Navigation

- **Unified Tap Control**: 
  - Single tap: Turn on backlight and cycle to next timezone
- The watch cycles through configured timezone slots only
- Home timezone can optionally be displayed persistently below main time
- Simple and consistent interaction model

## Technical Details

- Uses UTC + offset_minutes for accurate timezone calculations
- Supports non-hour timezone offsets (e.g., +05:30, +09:45)
- AppMessage communication between phone and watch
- Dynamic color adaptation: full color on supported devices, grayscale conversion on B&W displays
- Efficient memory usage: 30.5KB RAM footprint with 35KB+ free heap on basalt, 100KB+ on emery
- Adaptive tick timer: minute updates normally, second updates when seconds display is enabled
- GMT offset display: shows both timezone names and their UTC offsets for clarity

## Author

Created by kinncj - [GitHub](https://github.com/kinncj/pebble-traveler)

## License

MIT License - see [LICENSE](../LICENSE) file for details.
- Persistent storage of timezone configurations
- Compact timezone data representation

## Emulator Testing

When using the Pebble emulator:

### Unified Tap Control Simulation
```bash
# Unified tap (backlight + timezone cycle)
pebble emu-tap

# Test multiple taps
pebble emu-tap    # Turn on backlight + cycle to next timezone
pebble emu-tap    # Turn on backlight + cycle to next timezone
pebble emu-tap    # Turn on backlight + cycle to next timezone

# Test with different directions (all work the same)
pebble emu-tap --direction y+
pebble emu-tap --direction x+
```

### Configuration Page
```bash
pebble emu-app-config
```

Available tap directions: `x+`, `x-`, `y+`, `y-`, `z+`, `z-` (all work identically)

## Building

```bash
cd watch-face
npm install  # Install pebble-clay dependency
pebble build
pebble install --emulator basalt
```

## Project Structure

- `watch-face/` - Main Pebble C application
  - `src/c/main.c` - Watch face logic and timezone handling
  - `src/pkjs/` - PebbleKit JS configuration interface
- `docs/` - Project documentation

## Technical Details

- Built using Pebble SDK C API with PebbleKit JS configuration
- Uses AccelerometerService for tap detection
- Uses AppMessage for configuration sync between phone and watch
- Uses persistent storage to save timezone preferences
- Supports all Pebble platforms (aplite, basalt, chalk, diorite, emery)
- Manual timezone offset calculation (simplified, doesn't handle DST)

## Available Timezones

The configuration page includes these timezone options:
- New York (EST/EDT)
- Los Angeles (PST/PDT) 
- Chicago (CST/CDT)
- Denver (MST/MDT)
- London (GMT/BST)
- Paris (CET/CEST)
- Berlin (CET/CEST)
- Tokyo (JST)
- Shanghai (CST)
- Sydney (AEST/AEDT)
- Mumbai (IST)
- Dubai (GST)
- Moscow (MSK)
- São Paulo (BRT)

## Diagrams

See `docs/DIAGRAMS.md` for architecture and AppMessage flow diagrams (Mermaid format).
