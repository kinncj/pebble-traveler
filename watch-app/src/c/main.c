#include <pebble.h>
#include "../shared/timezones.h"

// Ensure time_t is available for platforms that don't include it properly
#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

#define MAX_TIMEZONES 6  // Local + GMT + 4 configurable
#define TZ_NAME_LENGTH 20

static Window *s_window;
static TextLayer *s_timezone_layer;
static TextLayer *s_time_layer;
static TextLayer *s_home_time_layer;  // For displaying home timezone

// Timezone data structure
typedef struct {
  char name[TZ_NAME_LENGTH];
  char display_name[TZ_NAME_LENGTH];
  int offset_minutes; // offset from UTC in minutes
  bool enabled;
} Timezone;

// Default timezones: Local is always enabled, Home and 4 additional slots
static Timezone timezones[MAX_TIMEZONES] = {
  {"Local", "Local", 0, true},        // 1. Local (GPS-based)
  {"", "Home", 0, false},             // 2. Home (user configurable)
  {"", "", 0, false},                 // 3. Timezone 3 (user configurable)
  {"", "", 0, false},                 // 4. Timezone 4 (user configurable)
  {"", "", 0, false},                 // 5. Timezone 5 (user configurable)
  {"", "", 0, false}                  // 6. Timezone 6 (user configurable)
};

static int current_timezone_index = 0;
static int active_timezone_count = 1;  // Start with just Local
static bool always_show_home = false;  // Always display home timezone option
static int num_timezone_configs = SHARED_TIMEZONE_COUNT;

// Appearance settings
static GColor background_color;
static GColor time_color;
static GColor timezone_label_color;
static GColor home_time_color;
static bool show_seconds = false;
static bool show_home_seconds = false;

// Helper function to determine if DST is active for US timezones
static bool is_dst_active_us(struct tm *tm) {
  // DST in US: Second Sunday in March to First Sunday in November
  // This is a simplified calculation for 2024 onwards
  
  int month = tm->tm_mon + 1;  // tm_mon is 0-11, we need 1-12
  int day = tm->tm_mday;
  int wday = tm->tm_wday;  // 0=Sunday, 1=Monday, ..., 6=Saturday
  
  // Before March or after November - no DST
  if (month < 3 || month > 11) {
    return false;
  }
  
  // April to October - always DST
  if (month > 3 && month < 11) {
    return true;
  }
  
  // March - DST starts on second Sunday
  if (month == 3) {
    // Find second Sunday of March
    // First Sunday is between 1st and 7th
    // Second Sunday is between 8th and 14th
    int second_sunday = 8 + ((7 - wday + day) % 7);
    if (second_sunday > 14) second_sunday -= 7;
    return day >= second_sunday;
  }
  
  // November - DST ends on first Sunday
  if (month == 11) {
    // Find first Sunday of November
    int first_sunday = 1 + ((7 - wday + day) % 7);
    if (first_sunday > 7) first_sunday -= 7;
    return day < first_sunday;
  }
  
  return false;
}

// Helper function to determine if DST is active for EU timezones
static bool is_dst_active_eu(struct tm *tm) {
  // DST in EU: Last Sunday in March to Last Sunday in October
  
  int month = tm->tm_mon + 1;  // tm_mon is 0-11, we need 1-12
  int day = tm->tm_mday;
  int wday = tm->tm_wday;  // 0=Sunday, 1=Monday, ..., 6=Saturday
  
  // Before March or after October - no DST
  if (month < 3 || month > 10) {
    return false;
  }
  
  // April to September - always DST
  if (month > 3 && month < 10) {
    return true;
  }
  
  // March - DST starts on last Sunday
  if (month == 3) {
    // Find last Sunday of March
    int last_sunday = 31;
    while ((last_sunday - day + wday) % 7 != 0) {
      last_sunday--;
    }
    return day >= last_sunday;
  }
  
  // October - DST ends on last Sunday
  if (month == 10) {
    // Find last Sunday of October
    int last_sunday = 31;
    while ((last_sunday - day + wday) % 7 != 0) {
      last_sunday--;
    }
    return day < last_sunday;
  }
  
  return false;
}

// Helper function to determine if DST is active for Australian timezones
static bool is_dst_active_au(struct tm *tm) {
  // DST in Australia: First Sunday in October to First Sunday in April (next year)
  
  int month = tm->tm_mon + 1;  // tm_mon is 0-11, we need 1-12
  int day = tm->tm_mday;
  int wday = tm->tm_wday;  // 0=Sunday, 1=Monday, ..., 6=Saturday
  
  // May to September - no DST
  if (month >= 5 && month <= 9) {
    return false;
  }
  
  // November to March - always DST
  if (month >= 11 || month <= 3) {
    return true;
  }
  
  // October - DST starts on first Sunday
  if (month == 10) {
    // Find first Sunday of October
    int first_sunday = 1 + ((7 - wday + day) % 7);
    if (first_sunday > 7) first_sunday -= 7;
    return day >= first_sunday;
  }
  
  // April - DST ends on first Sunday
  if (month == 4) {
    // Find first Sunday of April
    int first_sunday = 1 + ((7 - wday + day) % 7);
    if (first_sunday > 7) first_sunday -= 7;
    return day < first_sunday;
  }
  
  return false;
}

// Helper function to determine if DST is active for Brazilian timezones
static bool is_dst_active_br(struct tm *tm) {
  // DST in Brazil: Third Sunday in October to Third Sunday in February (next year)
  // Note: Brazil suspended DST in 2019, but keeping this for historical accuracy
  
  int month = tm->tm_mon + 1;  // tm_mon is 0-11, we need 1-12
  int day = tm->tm_mday;
  int wday = tm->tm_wday;  // 0=Sunday, 1=Monday, ..., 6=Saturday
  
  // March to September - no DST
  if (month >= 3 && month <= 9) {
    return false;
  }
  
  // November to January - always DST
  if (month >= 11 || month == 1) {
    return true;
  }
  
  // October - DST starts on third Sunday
  if (month == 10) {
    // Find third Sunday of October
    int third_sunday = 15 + ((7 - wday + day) % 7);
    if (third_sunday > 21) third_sunday -= 7;
    return day >= third_sunday;
  }
  
  // February - DST ends on third Sunday
  if (month == 2) {
    // Find third Sunday of February
    int third_sunday = 15 + ((7 - wday + day) % 7);
    if (third_sunday > 21) third_sunday -= 7;
    return day < third_sunday;
  }
  
  return false;
}

// Helper function to determine if DST is active for Chilean timezones
static bool is_dst_active_cl(struct tm *tm) {
  // DST in Chile: First Sunday in September to First Sunday in April (next year)
  
  int month = tm->tm_mon + 1;  // tm_mon is 0-11, we need 1-12
  int day = tm->tm_mday;
  int wday = tm->tm_wday;  // 0=Sunday, 1=Monday, ..., 6=Saturday
  
  // May to August - no DST
  if (month >= 5 && month <= 8) {
    return false;
  }
  
  // October to March - always DST
  if (month >= 10 || month <= 3) {
    return true;
  }
  
  // September - DST starts on first Sunday
  if (month == 9) {
    // Find first Sunday of September
    int first_sunday = 1 + ((7 - wday + day) % 7);
    if (first_sunday > 7) first_sunday -= 7;
    return day >= first_sunday;
  }
  
  // April - DST ends on first Sunday
  if (month == 4) {
    // Find first Sunday of April
    int first_sunday = 1 + ((7 - wday + day) % 7);
    if (first_sunday > 7) first_sunday -= 7;
    return day < first_sunday;
  }
  
  return false;
}

// Helper function to determine if DST is active for New Zealand
static bool is_dst_active_nz(struct tm *tm) {
  // DST in New Zealand: Last Sunday in September to First Sunday in April (next year)
  
  int month = tm->tm_mon + 1;  // tm_mon is 0-11, we need 1-12
  int day = tm->tm_mday;
  int wday = tm->tm_wday;  // 0=Sunday, 1=Monday, ..., 6=Saturday
  
  // May to August - no DST
  if (month >= 5 && month <= 8) {
    return false;
  }
  
  // October to March - always DST
  if (month >= 10 || month <= 3) {
    return true;
  }
  
  // September - DST starts on last Sunday
  if (month == 9) {
    // Find last Sunday of September
    int last_sunday = 30;
    while ((last_sunday - day + wday) % 7 != 0) {
      last_sunday--;
    }
    return day >= last_sunday;
  }
  
  // April - DST ends on first Sunday
  if (month == 4) {
    // Find first Sunday of April
    int first_sunday = 1 + ((7 - wday + day) % 7);
    if (first_sunday > 7) first_sunday -= 7;
    return day < first_sunday;
  }
  
  return false;
}

// Helper function to get DST-adjusted offset for a timezone
static int get_dst_adjusted_offset(const char* timezone_id, int base_offset_minutes, struct tm *current_time) {
  if (!timezone_id || !current_time) {
    return base_offset_minutes;
  }
  
  // US & Canadian Eastern Time zones
  if (strcmp(timezone_id, "America/New_York") == 0 || 
      strcmp(timezone_id, "America/Detroit") == 0 ||
      strcmp(timezone_id, "America/Montreal") == 0 ||
      strcmp(timezone_id, "America/Toronto") == 0) {
    return is_dst_active_us(current_time) ? base_offset_minutes + 60 : base_offset_minutes;
  }
  
  // US & Canadian Central Time zones
  if (strcmp(timezone_id, "America/Chicago") == 0 ||
      strcmp(timezone_id, "America/Winnipeg") == 0 ||
      strcmp(timezone_id, "America/Mexico_City") == 0) {
    return is_dst_active_us(current_time) ? base_offset_minutes + 60 : base_offset_minutes;
  }
  
  // US & Canadian Mountain Time zones
  if (strcmp(timezone_id, "America/Denver") == 0 ||
      strcmp(timezone_id, "America/Edmonton") == 0 ||
      strcmp(timezone_id, "America/Boise") == 0) {
    return is_dst_active_us(current_time) ? base_offset_minutes + 60 : base_offset_minutes;
  }
  
  // US & Canadian Pacific Time zones
  if (strcmp(timezone_id, "America/Los_Angeles") == 0 ||
      strcmp(timezone_id, "America/Vancouver") == 0 ||
      strcmp(timezone_id, "America/Tijuana") == 0) {
    return is_dst_active_us(current_time) ? base_offset_minutes + 60 : base_offset_minutes;
  }
  
  // Canadian Newfoundland Time (special case: UTC-3:30/-2:30)
  if (strcmp(timezone_id, "Canada/Newfoundland") == 0) {
    return is_dst_active_us(current_time) ? -150 : -210; // -2:30 DST, -3:30 standard
  }
  
  // Australian Eastern Time zones
  if (strcmp(timezone_id, "Australia/Sydney") == 0 ||
      strcmp(timezone_id, "Australia/Melbourne") == 0 ||
      strcmp(timezone_id, "Australia/Hobart") == 0) {
    return is_dst_active_au(current_time) ? base_offset_minutes + 60 : base_offset_minutes;
  }
  
  // Australian Central Time zones
  if (strcmp(timezone_id, "Australia/Adelaide") == 0 ||
      strcmp(timezone_id, "Australia/Darwin") == 0) {
    // Darwin doesn't observe DST, Adelaide does
    if (strcmp(timezone_id, "Australia/Darwin") == 0) {
      return base_offset_minutes; // No DST
    }
    return is_dst_active_au(current_time) ? base_offset_minutes + 60 : base_offset_minutes;
  }
  
  // Australian Western Time (no DST since 2009)
  if (strcmp(timezone_id, "Australia/Perth") == 0 ||
      strcmp(timezone_id, "Australia/Eucla") == 0) {
    return base_offset_minutes; // No DST
  }
  
  // Brazilian timezones (DST suspended since 2019, but keeping for historical accuracy)
  if (strcmp(timezone_id, "America/Sao_Paulo") == 0 ||
      strcmp(timezone_id, "America/Rio_Branco") == 0) {
    return base_offset_minutes; // DST suspended
  }
  
  // Chilean timezone
  if (strcmp(timezone_id, "America/Santiago") == 0 ||
      strcmp(timezone_id, "Pacific/Easter") == 0) {
    return is_dst_active_cl(current_time) ? base_offset_minutes + 60 : base_offset_minutes;
  }
  
  // New Zealand timezone
  if (strcmp(timezone_id, "Pacific/Auckland") == 0 ||
      strcmp(timezone_id, "Pacific/Chatham") == 0) {
    return is_dst_active_nz(current_time) ? base_offset_minutes + 60 : base_offset_minutes;
  }
  
  // European Time zones
  if (strncmp(timezone_id, "Europe/", 7) == 0) {
    return is_dst_active_eu(current_time) ? base_offset_minutes + 60 : base_offset_minutes;
  }
  
  // For other timezones, return base offset (no DST handling implemented yet)
  return base_offset_minutes;
}

// Helper function to convert hex color to GColor
static GColor hex_to_gcolor(uint32_t hex) {
#ifdef PBL_COLOR
  return GColorFromHEX(hex);
#else
  // For black and white platforms, map to appropriate grayscale
  uint8_t r = (hex >> 16) & 0xFF;
  uint8_t g = (hex >> 8) & 0xFF;
  uint8_t b = hex & 0xFF;
  uint8_t gray = (r + g + b) / 3;
  return gray > 128 ? GColorWhite : GColorBlack;
#endif
}

// Initialize default colors
static void init_default_colors() {
  background_color = GColorBlack;
  time_color = GColorWhite;
#ifdef PBL_COLOR
  timezone_label_color = GColorLightGray;
  home_time_color = GColorLightGray;
#else
  timezone_label_color = GColorWhite;
  home_time_color = GColorWhite;
#endif
}

// Function declarations
static void update_active_timezone_count();

// Configuration functions
static void load_timezone_config(int slot, const char* timezone_id) {
  // APP_LOG(APP_LOG_LEVEL_INFO, "load_timezone_config: slot=%d, timezone_id='%s'", slot, timezone_id);
  
  if (slot < 1 || slot >= MAX_TIMEZONES) {
    // APP_LOG(APP_LOG_LEVEL_WARNING, "Invalid slot %d, ignoring", slot);
    return;  // Don't modify Local (slot 0)
  }
  
  if (strlen(timezone_id) == 0) {
    // Disable this timezone slot
    timezones[slot].enabled = false;
    strcpy(timezones[slot].name, "");
    if (slot == 1) {
      strcpy(timezones[slot].display_name, "Home");  // Keep "Home" label for slot 1
    } else {
      strcpy(timezones[slot].display_name, "");
    }
  timezones[slot].offset_minutes = 0;
    // APP_LOG(APP_LOG_LEVEL_INFO, "Disabled timezone slot %d", slot);
  } else {
    // Find the timezone configuration in the shared array
    for (int i = 0; i < num_timezone_configs; i++) {
      if (strcmp(SHARED_TIMEZONES[i].identifier, timezone_id) == 0) {
        timezones[slot].enabled = true;
        strncpy(timezones[slot].name, SHARED_TIMEZONES[i].identifier, sizeof(timezones[slot].name));
        timezones[slot].name[sizeof(timezones[slot].name)-1] = '\0';
        if (slot == 1) {
          strncpy(timezones[slot].display_name, "Home", sizeof(timezones[slot].display_name));  // Always show "Home" for slot 1
        } else {
          strncpy(timezones[slot].display_name, SHARED_TIMEZONES[i].display_name, sizeof(timezones[slot].display_name));
        }
        timezones[slot].display_name[sizeof(timezones[slot].display_name)-1] = '\0';
        timezones[slot].offset_minutes = SHARED_TIMEZONES[i].offset_minutes;
        //APP_LOG(APP_LOG_LEVEL_INFO, "Configured slot %d: %s (%s), offset_minutes=%d", 
        // slot, timezones[slot].display_name, timezones[slot].name, timezones[slot].offset_minutes);
        break;
      }
    }
  }
  
  update_active_timezone_count();
}

static void update_active_timezone_count() {
  active_timezone_count = 0;
  for (int i = 0; i < MAX_TIMEZONES; i++) {
    if (timezones[i].enabled) {
      active_timezone_count++;
    }
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "Active timezone count: %d", active_timezone_count);
  
  // Ensure current index is valid
  if (current_timezone_index >= active_timezone_count) {
    current_timezone_index = 0;
  }
}

static int get_active_timezone_index(int display_index) {
  int active_count = 0;
  for (int i = 0; i < MAX_TIMEZONES; i++) {
    if (timezones[i].enabled) {
      if (active_count == display_index) {
        return i;
      }
      active_count++;
    }
  }
  return 0; // fallback to first timezone
}

static void update_time_display() {
  // Get current time and UTC
  time_t temp = time(NULL);
  struct tm *local_tm = localtime(&temp);
  struct tm *utc_tm = gmtime(&temp);

  // Get the actual timezone index for the current display position
  int actual_tz_index = get_active_timezone_index(current_timezone_index);
  Timezone current_tz = timezones[actual_tz_index];

  // Calculate minutes-since-midnight for current timezone
  int display_hour = 0;
  int display_min = 0;
  if (actual_tz_index == 0) {
    // Local time: use localtm directly
    display_hour = local_tm->tm_hour;
    display_min = local_tm->tm_min;
  } else {
    int utc_minutes = utc_tm->tm_hour * 60 + utc_tm->tm_min;
    // Get DST-adjusted offset for this timezone
    int adjusted_offset = get_dst_adjusted_offset(current_tz.name, current_tz.offset_minutes, local_tm);
    int tz_minutes = utc_minutes + adjusted_offset;
    // Normalize
    while (tz_minutes < 0) tz_minutes += 24 * 60;
    tz_minutes %= 24 * 60;
    display_hour = tz_minutes / 60;
    display_min = tz_minutes % 60;
  }
  
  // Format time string for current timezone
  static char time_buffer[32];
  static char home_time_buffer[32];
  
  // Current timezone time
  if (clock_is_24h_style()) {
    if (show_seconds) {
      snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d:%02d", display_hour, display_min, local_tm->tm_sec);
    } else {
      snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d", display_hour, display_min);
    }
  } else {
    int disp = display_hour;
    if (disp == 0) disp = 12;
    if (disp > 12) disp -= 12;
    if (show_seconds) {
      snprintf(time_buffer, sizeof(time_buffer), "%d:%02d:%02d %s", disp, display_min, local_tm->tm_sec, (display_hour >= 12) ? "PM" : "AM");
    } else {
      snprintf(time_buffer, sizeof(time_buffer), "%d:%02d %s", disp, display_min, (display_hour >= 12) ? "PM" : "AM");
    }
  }
  
  // Update main display - always show the current timezone name and time
  // Prepare timezone label with GMT offset
  static char tz_label[64];  // Increased size to accommodate timezone name + GMT offset
  
  if (strcmp(current_tz.display_name, "Local") == 0) {
    // For local timezone, calculate the actual GMT offset
    time_t temp_time = time(NULL);
    struct tm *local_tm = localtime(&temp_time);
    struct tm *utc_tm = gmtime(&temp_time);
    
    // Calculate offset in minutes
    int local_offset_minutes = (local_tm->tm_hour - utc_tm->tm_hour) * 60 + (local_tm->tm_min - utc_tm->tm_min);
    
    // Handle day boundary crossings
    if (local_offset_minutes > 12 * 60) {
      local_offset_minutes -= 24 * 60;
    } else if (local_offset_minutes < -12 * 60) {
      local_offset_minutes += 24 * 60;
    }
    
    // Format GMT offset
    int offset_hours = local_offset_minutes / 60;
    int offset_mins = abs(local_offset_minutes % 60);
    
    if (local_offset_minutes >= 0) {
      snprintf(tz_label, sizeof(tz_label), "local (GMT +%02d:%02d)", offset_hours, offset_mins);
    } else {
      snprintf(tz_label, sizeof(tz_label), "local (GMT -%02d:%02d)", abs(offset_hours), offset_mins);
    }
  } else {
    // For configured timezones, calculate DST-adjusted offset and display it
    int adjusted_offset = get_dst_adjusted_offset(current_tz.name, current_tz.offset_minutes, local_tm);
    int offset_hours = adjusted_offset / 60;
    int offset_mins = abs(adjusted_offset % 60);
    
    if (adjusted_offset >= 0) {
      snprintf(tz_label, sizeof(tz_label), "%s (GMT +%02d:%02d)", current_tz.display_name, offset_hours, offset_mins);
    } else {
      snprintf(tz_label, sizeof(tz_label), "%s (GMT -%02d:%02d)", current_tz.display_name, abs(offset_hours), offset_mins);
    }
  }
  
  text_layer_set_text(s_timezone_layer, tz_label);
  text_layer_set_text(s_time_layer, time_buffer);

  // Handle home timezone display (always show when enabled and not already showing home)
  if (always_show_home && timezones[1].enabled && actual_tz_index != 1) {
    // Calculate home timezone time (slot 1 is always home)
    if (timezones[1].enabled) {
      int utc_minutes = utc_tm->tm_hour * 60 + utc_tm->tm_min;
      // Get DST-adjusted offset for home timezone
      int home_adjusted_offset = get_dst_adjusted_offset(timezones[1].name, timezones[1].offset_minutes, local_tm);
      int home_minutes = utc_minutes + home_adjusted_offset;
      while (home_minutes < 0) home_minutes += 24 * 60;
      home_minutes %= 24 * 60;
      int home_hour = home_minutes / 60;
      int home_min = home_minutes % 60;
      
      // Format home time without GMT offset (simplified)
      if (clock_is_24h_style()) {
        if (show_home_seconds) {
          snprintf(home_time_buffer, sizeof(home_time_buffer), "%s: %02d:%02d:%02d", 
                  timezones[1].display_name, home_hour, home_min, local_tm->tm_sec);
        } else {
          snprintf(home_time_buffer, sizeof(home_time_buffer), "%s: %02d:%02d", 
                  timezones[1].display_name, home_hour, home_min);
        }
      } else {
        int home_display_hour = home_hour;
        if (home_display_hour == 0) home_display_hour = 12;
        if (home_display_hour > 12) home_display_hour -= 12;
        if (show_home_seconds) {
          snprintf(home_time_buffer, sizeof(home_time_buffer), "%s: %d:%02d:%02d %s", 
                  timezones[1].display_name, home_display_hour, home_min, local_tm->tm_sec, (home_hour >= 12) ? "PM" : "AM");
        } else {
          snprintf(home_time_buffer, sizeof(home_time_buffer), "%s: %d:%02d %s", 
                  timezones[1].display_name, home_display_hour, home_min, (home_hour >= 12) ? "PM" : "AM");
        }
      }
      text_layer_set_text(s_home_time_layer, home_time_buffer);
      layer_set_hidden(text_layer_get_layer(s_home_time_layer), false);
      // APP_LOG(APP_LOG_LEVEL_INFO, "Showing home timezone: %s", home_time_buffer);
    }
    
    text_layer_set_text(s_home_time_layer, home_time_buffer);
    layer_set_hidden(text_layer_get_layer(s_home_time_layer), false);
    // APP_LOG(APP_LOG_LEVEL_INFO, "Showing home timezone: %s", home_time_buffer);
  } else {
    // Hide home timezone display
    layer_set_hidden(text_layer_get_layer(s_home_time_layer), true);
    // APP_LOG(APP_LOG_LEVEL_INFO, "Hiding home timezone display");
  }
}

static void switch_timezone_next() {
  if (active_timezone_count <= 1) return;
  
  current_timezone_index = (current_timezone_index + 1) % active_timezone_count;
  update_time_display();
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Switched to timezone index: %d", current_timezone_index);
}

static void switch_timezone_prev() {
  if (active_timezone_count <= 1) return;
  
  current_timezone_index = (current_timezone_index - 1 + active_timezone_count) % active_timezone_count;
  update_time_display();
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Switched to timezone index: %d", current_timezone_index);
}

// Button click handlers
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  // UP button: Previous timezone
  switch_timezone_prev();
  light_enable_interaction();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // DOWN button: Next timezone
  switch_timezone_next();
  light_enable_interaction();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // SELECT button: Toggle backlight
  light_enable_interaction();
}

static void back_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  // BACK button long press: Exit app
  window_stack_pop(true);
}

static void back_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  // BACK button single press: Show brief message instead of exiting
  APP_LOG(APP_LOG_LEVEL_INFO, "Hold BACK button to exit app");
  // Optionally add a brief vibration to indicate the action was captured
  vibes_short_pulse();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  
  // Handle back button: single click does nothing, long click exits
  window_single_click_subscribe(BUTTON_ID_BACK, back_single_click_handler);
  window_long_click_subscribe(BUTTON_ID_BACK, 1000, back_long_click_handler, NULL);
}

// Time tick handler
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time_display();
}

// AppMessage handlers
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "AppMessage received");
  
  // Read Home timezone
  Tuple *home_tuple = dict_find(iterator, MESSAGE_KEY_HOME);
  if (home_tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Home timezone: %s", home_tuple->value->cstring);
    load_timezone_config(1, home_tuple->value->cstring);
    persist_write_string(MESSAGE_KEY_HOME, home_tuple->value->cstring);
  }
  
  // Read Timezone slots 1-4 (indices 2-5 in our array)
  for (int i = 0; i < 4; i++) {
    uint32_t key = MESSAGE_KEY_TIMEZONE_1 + i;
    Tuple *tz_tuple = dict_find(iterator, key);
    if (tz_tuple && strlen(tz_tuple->value->cstring) > 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Timezone %d: %s", i+1, tz_tuple->value->cstring);
      load_timezone_config(i + 2, tz_tuple->value->cstring);
      persist_write_string(key, tz_tuple->value->cstring);
    } else {
      // Clear the timezone slot
      timezones[i + 2].enabled = false;
      persist_delete(key);
    }
  }
  
  // Read display options
  Tuple *always_show_home_tuple = dict_find(iterator, MESSAGE_KEY_ALWAYS_SHOW_HOME);
  if (always_show_home_tuple) {
    always_show_home = always_show_home_tuple->value->int32 == 1;
    persist_write_bool(MESSAGE_KEY_ALWAYS_SHOW_HOME, always_show_home);
    APP_LOG(APP_LOG_LEVEL_INFO, "Always show home: %s", always_show_home ? "true" : "false");
  }
  
  Tuple *show_seconds_tuple = dict_find(iterator, MESSAGE_KEY_SHOW_SECONDS);
  if (show_seconds_tuple) {
    show_seconds = show_seconds_tuple->value->int32 == 1;
    persist_write_bool(MESSAGE_KEY_SHOW_SECONDS, show_seconds);
    APP_LOG(APP_LOG_LEVEL_INFO, "Show seconds: %s", show_seconds ? "true" : "false");
    
    // Update tick timer subscription based on seconds display
    tick_timer_service_unsubscribe();
    if (show_seconds) {
      tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    } else {
      tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    }
  }
  
  Tuple *show_home_seconds_tuple = dict_find(iterator, MESSAGE_KEY_SHOW_HOME_SECONDS);
  if (show_home_seconds_tuple) {
    show_home_seconds = show_home_seconds_tuple->value->int32 == 1;
    persist_write_bool(MESSAGE_KEY_SHOW_HOME_SECONDS, show_home_seconds);
    APP_LOG(APP_LOG_LEVEL_INFO, "Show home seconds: %s", show_home_seconds ? "true" : "false");
  }
  
  // Read color settings
  Tuple *bg_color_tuple = dict_find(iterator, MESSAGE_KEY_BACKGROUND_COLOR);
  if (bg_color_tuple) {
    background_color = hex_to_gcolor(bg_color_tuple->value->int32);
    window_set_background_color(s_window, background_color);
    persist_write_int(MESSAGE_KEY_BACKGROUND_COLOR, bg_color_tuple->value->int32);
    APP_LOG(APP_LOG_LEVEL_INFO, "Background color: 0x%08X", (unsigned int)bg_color_tuple->value->int32);
  }
  
  Tuple *time_color_tuple = dict_find(iterator, MESSAGE_KEY_TIME_COLOR);
  if (time_color_tuple) {
    time_color = hex_to_gcolor(time_color_tuple->value->int32);
    text_layer_set_text_color(s_time_layer, time_color);
    persist_write_int(MESSAGE_KEY_TIME_COLOR, time_color_tuple->value->int32);
    APP_LOG(APP_LOG_LEVEL_INFO, "Time color: 0x%08X", (unsigned int)time_color_tuple->value->int32);
  }
  
  Tuple *tz_label_color_tuple = dict_find(iterator, MESSAGE_KEY_TIMEZONE_LABEL_COLOR);
  if (tz_label_color_tuple) {
    timezone_label_color = hex_to_gcolor(tz_label_color_tuple->value->int32);
    text_layer_set_text_color(s_timezone_layer, timezone_label_color);
    persist_write_int(MESSAGE_KEY_TIMEZONE_LABEL_COLOR, tz_label_color_tuple->value->int32);
    APP_LOG(APP_LOG_LEVEL_INFO, "Timezone label color: 0x%08X", (unsigned int)tz_label_color_tuple->value->int32);
  }
  
  Tuple *home_time_color_tuple = dict_find(iterator, MESSAGE_KEY_HOME_TIME_COLOR);
  if (home_time_color_tuple) {
    home_time_color = hex_to_gcolor(home_time_color_tuple->value->int32);
    text_layer_set_text_color(s_home_time_layer, home_time_color);
    persist_write_int(MESSAGE_KEY_HOME_TIME_COLOR, home_time_color_tuple->value->int32);
    APP_LOG(APP_LOG_LEVEL_INFO, "Home time color: 0x%08X", (unsigned int)home_time_color_tuple->value->int32);
  }
  
  update_active_timezone_count();
  update_time_display();
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed: %d", reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

// Load saved configuration
static void load_saved_config() {
  // Load timezone configurations
  if (persist_exists(MESSAGE_KEY_HOME)) {
    char home_tz[64];
    persist_read_string(MESSAGE_KEY_HOME, home_tz, sizeof(home_tz));
    load_timezone_config(1, home_tz);
  }
  
  for (int i = 0; i < 4; i++) {
    uint32_t key = MESSAGE_KEY_TIMEZONE_1 + i;
    if (persist_exists(key)) {
      char tz[64];
      persist_read_string(key, tz, sizeof(tz));
      load_timezone_config(i + 2, tz);
    }
  }
  
  // Load display options
  if (persist_exists(MESSAGE_KEY_ALWAYS_SHOW_HOME)) {
    always_show_home = persist_read_bool(MESSAGE_KEY_ALWAYS_SHOW_HOME);
  }
  
  if (persist_exists(MESSAGE_KEY_SHOW_SECONDS)) {
    show_seconds = persist_read_bool(MESSAGE_KEY_SHOW_SECONDS);
  }
  
  if (persist_exists(MESSAGE_KEY_SHOW_HOME_SECONDS)) {
    show_home_seconds = persist_read_bool(MESSAGE_KEY_SHOW_HOME_SECONDS);
  }
  
  // Load color settings
  if (persist_exists(MESSAGE_KEY_BACKGROUND_COLOR)) {
    background_color = hex_to_gcolor(persist_read_int(MESSAGE_KEY_BACKGROUND_COLOR));
  }
  
  if (persist_exists(MESSAGE_KEY_TIME_COLOR)) {
    time_color = hex_to_gcolor(persist_read_int(MESSAGE_KEY_TIME_COLOR));
  }
  
  if (persist_exists(MESSAGE_KEY_TIMEZONE_LABEL_COLOR)) {
    timezone_label_color = hex_to_gcolor(persist_read_int(MESSAGE_KEY_TIMEZONE_LABEL_COLOR));
  }
  
  if (persist_exists(MESSAGE_KEY_HOME_TIME_COLOR)) {
    home_time_color = hex_to_gcolor(persist_read_int(MESSAGE_KEY_HOME_TIME_COLOR));
  }
  
  update_active_timezone_count();
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Apply background color
  window_set_background_color(window, background_color);

  // Create timezone name layer (larger to accommodate GMT offset)
  s_timezone_layer = text_layer_create(GRect(0, 35, bounds.size.w, 30));
  text_layer_set_text_alignment(s_timezone_layer, GTextAlignmentCenter);
  text_layer_set_font(s_timezone_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(s_timezone_layer, timezone_label_color);
  text_layer_set_background_color(s_timezone_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_timezone_layer));

  // Create time layer (adjusted position for larger timezone layer)
  s_time_layer = text_layer_create(GRect(0, 75, bounds.size.w, 40));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM));
  text_layer_set_text_color(s_time_layer, time_color);
  text_layer_set_background_color(s_time_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Create home time layer (smaller font, positioned below main time)
  s_home_time_layer = text_layer_create(GRect(0, 125, bounds.size.w, 25));
  text_layer_set_text_alignment(s_home_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_home_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(s_home_time_layer, home_time_color);
  text_layer_set_background_color(s_home_time_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_home_time_layer));
  layer_set_hidden(text_layer_get_layer(s_home_time_layer), true); // Initially hidden

  // Initialize display
  update_time_display();
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_timezone_layer);
  text_layer_destroy(s_home_time_layer);
}

static void prv_init(void) {
  // Initialize colors
  init_default_colors();
  
  // Load saved configuration
  load_saved_config();
  
  // Create main Window element and assign to pointer
  s_window = window_create();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  
  // Set click configuration provider
  window_set_click_config_provider(s_window, click_config_provider);
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_window, true);
  
  // Register with TickTimerService
  if (show_seconds) {
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  }
  
  // Register callbacks for AppMessage
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  const int inbox_size = 512;
  const int outbox_size = 512;
  app_message_open(inbox_size, outbox_size);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);
  
  app_event_loop();
  prv_deinit();
}