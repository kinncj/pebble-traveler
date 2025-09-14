# Testing Guide

## Prerequisites
```bash
export PATH="/Users/$USER/.local/bin:$PATH"
conda activate pebble-env
```

## Emulator Testing

### Launch Emulator
```bash
cd watch-face
pebble install --emulator basalt    # Pebble Time
pebble install --emulator chalk     # Pebble Time Steel
pebble install --emulator diorite   # Pebble Time 2
pebble install --emulator emery     # Pebble 2 HR
```

### Test Unified Tap Control
When multiple emulators are running, specify the target:
```bash
# Test unified tap (turns on backlight + cycles timezone)
pebble emu-tap --emulator basalt                    # Turn on backlight and cycle to next timezone
pebble emu-tap --emulator basalt                    # Turn on backlight and cycle to next timezone
pebble emu-tap --emulator basalt                    # Turn on backlight and cycle to next timezone

# Test with different directions (all work the same)
pebble emu-tap --emulator basalt --direction y+     # Unified action
pebble emu-tap --emulator basalt --direction x+     # Unified action
```

**Expected**: Each tap turns on backlight via `light_enable(true)` and cycles to the next configured timezone. Simple and consistent behavior.

### Test Configuration
```bash
# Open configuration page (known timeout issue)
pebble emu-app-config --emulator basalt
```

**Note**: Configuration page currently experiences timeout issues. This is documented in DEVELOPMENT.md troubleshooting section.

### Test AppMessage Communication
The configuration page should:
1. Load 540+ timezone options
2. Allow selection of Home + 4 configurable timezones
3. Include "Always Display Home Timezone?" toggle
4. Send selected configurations to watch via AppMessage

### Expected Behavior
1. **Initial State**: Shows local time (slot 0)
2. **Unified Tap**: Turn on backlight and cycle to next timezone
3. **Simple Control**: One action does both backlight and navigation
4. **Home Display**: If enabled, shows home timezone below main time
5. **Persistence**: Settings survive app restart

## Manual Testing Checklist

- [ ] Local timezone displays correctly
- [ ] Each tap turns on backlight and cycles timezone
- [ ] Simple and consistent tap behavior
- [ ] Configuration page loads with timezone options
- [ ] Selected timezones sync to watch
- [ ] Home timezone display toggle works
- [ ] Time calculations are accurate for non-hour offsets
- [ ] AppMessage communication is reliable
- [ ] Settings persist after restart

## Build Verification

Ensure builds complete successfully for target platforms:
- [ ] Basalt (Pebble Time) - 64KB RAM
- [ ] Emery (Pebble 2 HR) - 128KB RAM

Aplite is excluded due to APP region overflow (4332 bytes over limit).
