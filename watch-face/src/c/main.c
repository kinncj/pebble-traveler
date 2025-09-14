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
// s_hint_layer removed

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
    // Find last Sunday of March (25th-31st)
    int last_sunday = 31 - ((wday + 31 - day) % 7);
    return day >= last_sunday;
  }
  
  // October - DST ends on last Sunday
  if (month == 10) {
    // Find last Sunday of October (25th-31st)
    int last_sunday = 31 - ((wday + 31 - day) % 7);
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
    // Find third Sunday of October (15th-21st)
    int third_sunday = 15 + ((7 - wday + day) % 7);
    if (third_sunday > 21) third_sunday -= 7;
    return day >= third_sunday;
  }
  
  // February - DST ends on third Sunday
  if (month == 2) {
    // Find third Sunday of February (15th-21st)
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
    // Find last Sunday of September (25th-30th)
    int last_sunday = 30 - ((wday + 30 - day) % 7);
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
      strcmp(timezone_id, "America/Toronto") == 0 ||
      strcmp(timezone_id, "America/Montreal") == 0 ||
      strcmp(timezone_id, "America/Halifax") == 0 ||
      strcmp(timezone_id, "Canada/Eastern") == 0 ||
      strcmp(timezone_id, "Canada/Atlantic") == 0 ||
      strcmp(timezone_id, "US/Eastern") == 0) {
    if (is_dst_active_us(current_time)) {
      return base_offset_minutes + 60;  // EDT: -4 hours instead of EST: -5 hours
    }
    return base_offset_minutes;  // EST: -5 hours
  }
  
  // US & Canadian Central Time zones
  if (strcmp(timezone_id, "America/Chicago") == 0 ||
      strcmp(timezone_id, "America/Winnipeg") == 0 ||
      strcmp(timezone_id, "Canada/Central") == 0 ||
      strcmp(timezone_id, "US/Central") == 0) {
    if (is_dst_active_us(current_time)) {
      return base_offset_minutes + 60;  // CDT: -5 hours instead of CST: -6 hours
    }
    return base_offset_minutes;  // CST: -6 hours
  }
  
  // US & Canadian Mountain Time zones
  if (strcmp(timezone_id, "America/Denver") == 0 ||
      strcmp(timezone_id, "Canada/Mountain") == 0 ||
      strcmp(timezone_id, "US/Mountain") == 0) {
    if (is_dst_active_us(current_time)) {
      return base_offset_minutes + 60;  // MDT: -6 hours instead of MST: -7 hours
    }
    return base_offset_minutes;  // MST: -7 hours
  }
  
  // US & Canadian Pacific Time zones
  if (strcmp(timezone_id, "America/Los_Angeles") == 0 ||
      strcmp(timezone_id, "America/Vancouver") == 0 ||
      strcmp(timezone_id, "Canada/Pacific") == 0 ||
      strcmp(timezone_id, "US/Pacific") == 0) {
    if (is_dst_active_us(current_time)) {
      return base_offset_minutes + 60;  // PDT: -7 hours instead of PST: -8 hours
    }
    return base_offset_minutes;  // PST: -8 hours
  }
  
  // Canadian Newfoundland Time (special case: UTC-3:30/-2:30)
  if (strcmp(timezone_id, "Canada/Newfoundland") == 0) {
    if (is_dst_active_us(current_time)) {
      return base_offset_minutes + 60;  // NDT: -2:30 instead of NST: -3:30
    }
    return base_offset_minutes;  // NST: -3:30
  }
  
  // Australian Eastern Time zones
  if (strcmp(timezone_id, "Australia/Sydney") == 0 ||
      strcmp(timezone_id, "Australia/Melbourne") == 0 ||
      strcmp(timezone_id, "Australia/Canberra") == 0 ||
      strcmp(timezone_id, "Australia/ACT") == 0 ||
      strcmp(timezone_id, "Australia/Tasmania") == 0 ||
      strcmp(timezone_id, "Australia/Hobart") == 0) {
    if (is_dst_active_au(current_time)) {
      return base_offset_minutes + 60;  // AEDT: +11 hours instead of AEST: +10 hours
    }
    return base_offset_minutes;  // AEST: +10 hours
  }
  
  // Australian Central Time zones
  if (strcmp(timezone_id, "Australia/Adelaide") == 0 ||
      strcmp(timezone_id, "Australia/Broken_Hill") == 0) {
    if (is_dst_active_au(current_time)) {
      return base_offset_minutes + 60;  // ACDT: +10:30 instead of ACST: +9:30
    }
    return base_offset_minutes;  // ACST: +9:30
  }
  
  // Australian Western Time (no DST since 2009)
  if (strcmp(timezone_id, "Australia/Perth") == 0 ||
      strcmp(timezone_id, "Australia/West") == 0) {
    return base_offset_minutes;  // AWST: +8 hours (no DST)
  }
  
  // Brazilian timezones (DST suspended since 2019, but keeping for historical accuracy)
  if (strcmp(timezone_id, "America/Sao_Paulo") == 0 ||
      strcmp(timezone_id, "Brazil/East") == 0) {
    // Note: Brazil suspended DST in 2019, so always return standard time
    return base_offset_minutes;  // BRT: -3 hours (no DST since 2019)
  }
  
  // Chilean timezone
  if (strcmp(timezone_id, "America/Santiago") == 0 ||
      strcmp(timezone_id, "Chile/Continental") == 0) {
    if (is_dst_active_cl(current_time)) {
      return base_offset_minutes + 60;  // CLST: -3 hours instead of CLT: -4 hours
    }
    return base_offset_minutes;  // CLT: -4 hours
  }
  
  // New Zealand timezone
  if (strcmp(timezone_id, "Pacific/Auckland") == 0 ||
      strcmp(timezone_id, "NZ") == 0) {
    if (is_dst_active_nz(current_time)) {
      return base_offset_minutes + 60;  // NZDT: +13 hours instead of NZST: +12 hours
    }
    return base_offset_minutes;  // NZST: +12 hours
  }
  
  // European Time zones
  if (strncmp(timezone_id, "Europe/", 7) == 0) {
    // Russian timezones don't observe DST (abolished in 2014)
    if (strncmp(timezone_id, "Europe/Moscow", 13) == 0 ||
        strncmp(timezone_id, "Europe/Kaliningrad", 18) == 0 ||
        strncmp(timezone_id, "Europe/Volgograd", 16) == 0 ||
        strncmp(timezone_id, "Europe/Samara", 13) == 0) {
      return base_offset_minutes;  // Russian zones use permanent standard time
    }
    
    // Belarus doesn't observe DST (abolished in 2018)
    if (strncmp(timezone_id, "Europe/Minsk", 12) == 0) {
      return base_offset_minutes;  // Belarus uses permanent standard time
    }
    
    // Iceland doesn't observe DST
    if (strncmp(timezone_id, "Europe/Reykjavik", 16) == 0) {
      return base_offset_minutes;  // Iceland uses permanent GMT
    }
    
    // Most other European countries follow EU DST rules
    if (is_dst_active_eu(current_time)) {
      return base_offset_minutes + 60;  // Summer time: +1 hour
    }
    return base_offset_minutes;  // Standard time
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
}

static void update_active_timezone_count() {
  active_timezone_count = 0;
  for (int i = 0; i < MAX_TIMEZONES; i++) {
    if (timezones[i].enabled) {
      active_timezone_count++;
    }
  }
  
  // Ensure current index is valid
  if (current_timezone_index >= active_timezone_count) {
    current_timezone_index = 0;
  }
}

static int get_active_timezone_index(int display_index) {
  int count = 0;
  for (int i = 0; i < MAX_TIMEZONES; i++) {
    if (timezones[i].enabled) {
      if (count == display_index) {
        return i;
      }
      count++;
    }
  }
  return 0;  // Fallback to first timezone
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

static void switch_timezone() {
  current_timezone_index = (current_timezone_index + 1) % active_timezone_count;
  update_time_display();
}

// Accelerometer tap handler
static void tap_handler(AccelAxisType axis, int32_t direction) {
  // Single tap: turn on backlight AND cycle timezone
  light_enable_interaction();  
  // Cycle to next timezone
  switch_timezone();
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "Tap detected: Backlight on + cycling to next timezone");
}

// Time tick handler
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time_display();
}

// AppMessage handlers
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // APP_LOG(APP_LOG_LEVEL_INFO, "Configuration message received");
  
  // Read configuration from phone
  Tuple *home_tuple = dict_find(iterator, MESSAGE_KEY_HOME);
  Tuple *timezone_1_tuple = dict_find(iterator, MESSAGE_KEY_TIMEZONE_1);
  Tuple *timezone_2_tuple = dict_find(iterator, MESSAGE_KEY_TIMEZONE_2);
  Tuple *timezone_3_tuple = dict_find(iterator, MESSAGE_KEY_TIMEZONE_3);
  Tuple *timezone_4_tuple = dict_find(iterator, MESSAGE_KEY_TIMEZONE_4);
  Tuple *always_show_home_tuple = dict_find(iterator, MESSAGE_KEY_ALWAYS_SHOW_HOME);
  Tuple *background_color_tuple = dict_find(iterator, MESSAGE_KEY_BACKGROUND_COLOR);
  Tuple *time_color_tuple = dict_find(iterator, MESSAGE_KEY_TIME_COLOR);
  Tuple *timezone_label_color_tuple = dict_find(iterator, MESSAGE_KEY_TIMEZONE_LABEL_COLOR);
  Tuple *home_time_color_tuple = dict_find(iterator, MESSAGE_KEY_HOME_TIME_COLOR);
  Tuple *show_seconds_tuple = dict_find(iterator, MESSAGE_KEY_SHOW_SECONDS);
  Tuple *show_home_seconds_tuple = dict_find(iterator, MESSAGE_KEY_SHOW_HOME_SECONDS);
  
  if (home_tuple) {
    load_timezone_config(1, home_tuple->value->cstring);  // Slot 1 = Home
    persist_write_string(MESSAGE_KEY_HOME, home_tuple->value->cstring);
  }
  
  if (timezone_1_tuple) {
    load_timezone_config(2, timezone_1_tuple->value->cstring);  // Slot 2 = Timezone 3
    persist_write_string(MESSAGE_KEY_TIMEZONE_1, timezone_1_tuple->value->cstring);
  }
  
  if (timezone_2_tuple) {
    load_timezone_config(3, timezone_2_tuple->value->cstring);  // Slot 3 = Timezone 4
    persist_write_string(MESSAGE_KEY_TIMEZONE_2, timezone_2_tuple->value->cstring);
  }
  
  if (timezone_3_tuple) {
    load_timezone_config(4, timezone_3_tuple->value->cstring);  // Slot 4 = Timezone 5
    persist_write_string(MESSAGE_KEY_TIMEZONE_3, timezone_3_tuple->value->cstring);
  }
  
  if (timezone_4_tuple) {
    load_timezone_config(5, timezone_4_tuple->value->cstring);  // Slot 5 = Timezone 6
    persist_write_string(MESSAGE_KEY_TIMEZONE_4, timezone_4_tuple->value->cstring);
  }
  
  if (always_show_home_tuple) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "ALWAYS_SHOW_HOME tuple found, value: %d", (int)always_show_home_tuple->value->int32);
    always_show_home = (always_show_home_tuple->value->int32 == 1);
    persist_write_bool(MESSAGE_KEY_ALWAYS_SHOW_HOME, always_show_home);
    // APP_LOG(APP_LOG_LEVEL_INFO, "Always show home timezone: %s", always_show_home ? "true" : "false");
  } else {
    // APP_LOG(APP_LOG_LEVEL_INFO, "ALWAYS_SHOW_HOME tuple not found in message");
  }
  
  // Handle color settings
  if (background_color_tuple) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Setting BACKGROUND_COLOR: %d", (int)background_color_tuple->value->int32);
    background_color = hex_to_gcolor(background_color_tuple->value->int32);
    persist_write_int(MESSAGE_KEY_BACKGROUND_COLOR, background_color_tuple->value->int32);
    window_set_background_color(s_window, background_color);
  }
  
  if (time_color_tuple) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Setting TIME_COLOR: %d for s_time_layer", (int)time_color_tuple->value->int32);
    time_color = hex_to_gcolor(time_color_tuple->value->int32);
    persist_write_int(MESSAGE_KEY_TIME_COLOR, time_color_tuple->value->int32);
    text_layer_set_text_color(s_time_layer, time_color);
  }
  
  if (timezone_label_color_tuple) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Setting TIMEZONE_LABEL_COLOR: %d for s_timezone_layer", (int)timezone_label_color_tuple->value->int32);
    timezone_label_color = hex_to_gcolor(timezone_label_color_tuple->value->int32);
    persist_write_int(MESSAGE_KEY_TIMEZONE_LABEL_COLOR, timezone_label_color_tuple->value->int32);
    text_layer_set_text_color(s_timezone_layer, timezone_label_color);
  }
  
  if (home_time_color_tuple) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Setting HOME_TIME_COLOR: %d for s_home_time_layer", (int)home_time_color_tuple->value->int32);
    home_time_color = hex_to_gcolor(home_time_color_tuple->value->int32);
    persist_write_int(MESSAGE_KEY_HOME_TIME_COLOR, home_time_color_tuple->value->int32);
    text_layer_set_text_color(s_home_time_layer, home_time_color);
  }
  
  // Handle display options
  if (show_seconds_tuple) {
    show_seconds = (show_seconds_tuple->value->int32 == 1);
    persist_write_bool(MESSAGE_KEY_SHOW_SECONDS, show_seconds);
    // Update tick timer subscription based on seconds display
    tick_timer_service_unsubscribe();
    if (show_seconds || show_home_seconds) {
      tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    } else {
      tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    }
  }
  
  if (show_home_seconds_tuple) {
    show_home_seconds = (show_home_seconds_tuple->value->int32 == 1);
    persist_write_bool(MESSAGE_KEY_SHOW_HOME_SECONDS, show_home_seconds);
    // Update tick timer subscription based on seconds display
    tick_timer_service_unsubscribe();
    if (show_seconds || show_home_seconds) {
      tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    } else {
      tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    }
  }
  
  update_active_timezone_count();
  update_time_display();
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "Configuration updated. Active timezones: %d", active_timezone_count);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped! Reason: %d", (int)reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! Reason: %d", (int)reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  // APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

// Load saved configuration
static void load_saved_config() {
  // APP_LOG(APP_LOG_LEVEL_INFO, "Loading saved configuration...");
  char buffer[30];
  bool any_config_loaded = false;
  
  if (persist_read_string(MESSAGE_KEY_HOME, buffer, sizeof(buffer)) > 0) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Loading HOME timezone: %s", buffer);
    load_timezone_config(1, buffer);  // Slot 1 = Home
    any_config_loaded = true;
  }
  
  if (persist_read_string(MESSAGE_KEY_TIMEZONE_1, buffer, sizeof(buffer)) > 0) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Loading TIMEZONE_1: %s", buffer);
    load_timezone_config(2, buffer);  // Slot 2 = Timezone 3
    any_config_loaded = true;
  }
  
  if (persist_read_string(MESSAGE_KEY_TIMEZONE_2, buffer, sizeof(buffer)) > 0) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Loading TIMEZONE_2: %s", buffer);
    load_timezone_config(3, buffer);  // Slot 3 = Timezone 4
    any_config_loaded = true;
  }
  
  if (persist_read_string(MESSAGE_KEY_TIMEZONE_3, buffer, sizeof(buffer)) > 0) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Loading TIMEZONE_3: %s", buffer);
    load_timezone_config(4, buffer);  // Slot 4 = Timezone 5
    any_config_loaded = true;
  }
  
  if (persist_read_string(MESSAGE_KEY_TIMEZONE_4, buffer, sizeof(buffer)) > 0) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "Loading TIMEZONE_4: %s", buffer);
    load_timezone_config(5, buffer);  // Slot 5 = Timezone 6
    any_config_loaded = true;
  }
  
  // Load always show home setting
  if (persist_exists(MESSAGE_KEY_ALWAYS_SHOW_HOME)) {
    always_show_home = persist_read_bool(MESSAGE_KEY_ALWAYS_SHOW_HOME);
    // APP_LOG(APP_LOG_LEVEL_INFO, "Loaded ALWAYS_SHOW_HOME: %s", always_show_home ? "true" : "false");
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
  
  // Load display options
  if (persist_exists(MESSAGE_KEY_SHOW_SECONDS)) {
    show_seconds = persist_read_bool(MESSAGE_KEY_SHOW_SECONDS);
  }
  if (persist_exists(MESSAGE_KEY_SHOW_HOME_SECONDS)) {
    show_home_seconds = persist_read_bool(MESSAGE_KEY_SHOW_HOME_SECONDS);
  }
  
  // If no configuration was loaded, set up some test timezones
  if (!any_config_loaded) {
    // APP_LOG(APP_LOG_LEVEL_INFO, "No saved config found, loading test timezones");
    load_timezone_config(1, "GMT");                  // Home = GMT
    load_timezone_config(2, "America/Sao_Paulo");   // Timezone 3 = São Paulo
    load_timezone_config(3, "Asia/Kolkata");        // Timezone 4 = Mumbai  
    load_timezone_config(4, "Europe/Moscow");       // Timezone 5 = Moscow
    load_timezone_config(5, "Asia/Tokyo");          // Timezone 6 = Tokyo
  }
  
  update_active_timezone_count();
  // APP_LOG(APP_LOG_LEVEL_INFO, "Configuration loaded. Active timezones: %d", active_timezone_count);
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
  text_layer_destroy(s_timezone_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_home_time_layer);
  // s_hint_layer removed
}

static void prv_init(void) {
  // APP_LOG(APP_LOG_LEVEL_INFO, "=== WATCH FACE STARTING ===");
  
  // Initialize default colors
  init_default_colors();
  
  s_window = window_create();
  window_set_background_color(s_window, background_color);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  
  // Load saved configuration
  load_saved_config();
  
  // Subscribe to time updates (seconds if needed, otherwise minutes)
  if (show_seconds || show_home_seconds) {
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  }
  
  // Subscribe to accelerometer tap service
  accel_tap_service_subscribe(tap_handler);
  
  // Initialize AppMessage
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  const int inbox_size = 256;
  const int outbox_size = 256;
  app_message_open(inbox_size, outbox_size);
  // APP_LOG(APP_LOG_LEVEL_INFO, "AppMessage opened with inbox_size=%d, outbox_size=%d", inbox_size, outbox_size);
  
  const bool animated = true;
  window_stack_push(s_window, animated);
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "Watch face initialization complete");
}

static void prv_deinit(void) {
  // Unsubscribe from services
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();
  
  window_destroy(s_window);
}

int main(void) {
  prv_init();

  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
