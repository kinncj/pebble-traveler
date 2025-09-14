// Dynamically generated config will be produced at runtime by index.js
// This file exports a function that accepts a list of timezone option objects
module.exports = function(build_options) {
  // build_options is an array of {label, value}
  return [
    { "type": "heading", "defaultValue": "Timezone Traveler Settings" },
    { "type": "text", "defaultValue": "Configure your 6 timezones and appearance settings.\nAuthor: kinncj (https://github.com/kinncj/pebble-traveler)" },
    { "type": "section", "items": [
      { "type": "heading", "defaultValue": "Home Timezone" },
      { "type": "select", "messageKey": "HOME", "defaultValue": "", "label": "Home Timezone", "options": [{ "label": "None", "value": "" }].concat(build_options) },
      { "type": "heading", "defaultValue": "Additional Timezones" },
      { "type": "select", "messageKey": "TIMEZONE_1", "defaultValue": "", "label": "Timezone 3", "options": [{ "label": "None", "value": "" }].concat(build_options) },
      { "type": "select", "messageKey": "TIMEZONE_2", "defaultValue": "", "label": "Timezone 4", "options": [{ "label": "None", "value": "" }].concat(build_options) },
      { "type": "select", "messageKey": "TIMEZONE_3", "defaultValue": "", "label": "Timezone 5", "options": [{ "label": "None", "value": "" }].concat(build_options) },
      { "type": "select", "messageKey": "TIMEZONE_4", "defaultValue": "", "label": "Timezone 6", "options": [{ "label": "None", "value": "" }].concat(build_options) }
    ]},
    { "type": "section", "items": [
      { "type": "heading", "defaultValue": "Display Options" },
      { "type": "toggle", "messageKey": "ALWAYS_SHOW_HOME", "defaultValue": false, "label": "Always Display Home Timezone?", "description": "When enabled, the home timezone will always be shown on the watch face regardless of navigation" },
      { "type": "toggle", "messageKey": "SHOW_SECONDS", "defaultValue": false, "label": "Show Seconds (Main Time)", "description": "Display seconds for the current timezone" },
      { "type": "toggle", "messageKey": "SHOW_HOME_SECONDS", "defaultValue": false, "label": "Show Seconds (Home Time)", "description": "Display seconds for the home timezone when visible" }
    ] },
    { "type": "section", "items": [
      { "type": "heading", "defaultValue": "Color Settings" },
      { "type": "text", "defaultValue": "Choose colors for different elements. Colors work on Pebble Time and later models." },
      { "type": "color", "messageKey": "BACKGROUND_COLOR", "defaultValue": "000000", "label": "Background Color" },
      { "type": "color", "messageKey": "TIME_COLOR", "defaultValue": "FFFFFF", "label": "Main Time Color" },
      { "type": "color", "messageKey": "TIMEZONE_LABEL_COLOR", "defaultValue": "AAAAAA", "label": "Timezone Label Color" },
      { "type": "color", "messageKey": "HOME_TIME_COLOR", "defaultValue": "AAAAAA", "label": "Home Time Color" }
    ] },
    { "type": "text", "defaultValue": "Navigation:\n• Y+ (tilt up): Previous timezone\n• Y- (tilt down): Next timezone\n• Tap screen: Next timezone" },
    { "type": "submit", "defaultValue": "Save Settings" }
  ];
};