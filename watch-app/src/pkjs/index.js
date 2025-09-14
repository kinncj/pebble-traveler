// Configuration page for Timezone Traveler

var Clay = require('pebble-clay');
var clayConfigurator = require('./config')
var timeZoneOptions = require('../shared/timezones.canonical');
var timeZoneMapper = require('../shared/timezones.mapper');
// Inline message keys to avoid webpack path issues
var messageKeys = {
    "ALWAYS_SHOW_HOME": 10005,
    "BACKGROUND_COLOR": 10006,
    "HOME": 10000,
    "HOME_TIME_COLOR": 10009,
    "SHOW_HOME_SECONDS": 10011,
    "SHOW_SECONDS": 10010,
    "TIMEZONE_1": 10001,
    "TIMEZONE_2": 10002,
    "TIMEZONE_3": 10003,
    "TIMEZONE_4": 10004,
    "TIMEZONE_LABEL_COLOR": 10008,
    "TIME_COLOR": 10007
};

// Create configuration directly instead of using external config.js
var clayConfig = clayConfigurator(timeZoneMapper(timeZoneOptions));

var clay = new Clay(clayConfig, function(minified) {
  // Return custom success page HTML
  return '<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Settings Saved</title><style>body{font-family:Arial,sans-serif;text-align:center;padding:40px 20px;background:#f0f0f0}.success{background:#4CAF50;color:white;padding:20px;border-radius:10px;margin:20px 0;font-size:18px}.instructions{margin:20px 0;color:#666;line-height:1.5}.version{margin-top:30px;color:#999;font-size:14px}.btn{background:#2196F3;color:white;border:none;padding:15px 30px;font-size:16px;border-radius:5px;cursor:pointer;margin-top:20px}</style></head><body><div class="success">✓ Configuration Saved</div><div class="instructions">Your timezone settings have been updated!<br><br><strong>Button Controls v2.0.0:</strong><br>• UP button: Previous timezone<br>• DOWN button: Next timezone<br>• SELECT button: Toggle backlight<br>• BACK button: Single press ignored<br>• BACK button: Hold to exit app</div><div class="version">Timezone Traveler App v2.0.0</div><button class="btn" onclick="document.location=\'pebblejs://close\'">Close</button></body></html>';
}, {
  autoHandleEvents: false
});

// Configuration event listeners
Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e.response) {
    // Parse configuration
    var config = clay.getSettings(e.response);
    
    // Build AppMessage
    var message = {};
    
    // Map message key names to IDs using the inlined mapping
    function getCfg(name) {
      var id = messageKeys[name];
      if (typeof id === 'undefined') return undefined;
      return config[String(id)];
    }

    if (getCfg('HOME')) {
      message.HOME = getCfg('HOME');
    }
    if (getCfg('TIMEZONE_1')) {
      message.TIMEZONE_1 = getCfg('TIMEZONE_1');
    }
    if (getCfg('TIMEZONE_2')) {
      message.TIMEZONE_2 = getCfg('TIMEZONE_2');
    }
    if (getCfg('TIMEZONE_3')) {
      message.TIMEZONE_3 = getCfg('TIMEZONE_3');
    }
    if (getCfg('TIMEZONE_4')) {
      message.TIMEZONE_4 = getCfg('TIMEZONE_4');
    }
    if (typeof getCfg('ALWAYS_SHOW_HOME') !== 'undefined') {
      message.ALWAYS_SHOW_HOME = getCfg('ALWAYS_SHOW_HOME') ? 1 : 0;
    }
    // Color processing - Clay provides colors as integers already
    if (getCfg('BACKGROUND_COLOR') !== null && getCfg('BACKGROUND_COLOR') !== undefined) {
      var bgColor = getCfg('BACKGROUND_COLOR');
      message.BACKGROUND_COLOR = bgColor; // Use directly as integer
    }
    if (getCfg('TIME_COLOR') !== null && getCfg('TIME_COLOR') !== undefined) {
      var timeColor = getCfg('TIME_COLOR');
      message.TIME_COLOR = timeColor;
    }
    if (getCfg('TIMEZONE_LABEL_COLOR') !== null && getCfg('TIMEZONE_LABEL_COLOR') !== undefined) {
      var tzLabelColor = getCfg('TIMEZONE_LABEL_COLOR');
      message.TIMEZONE_LABEL_COLOR = tzLabelColor;
    }
    if (getCfg('HOME_TIME_COLOR') !== null && getCfg('HOME_TIME_COLOR') !== undefined) {
      var homeTimeColor = getCfg('HOME_TIME_COLOR');
      message.HOME_TIME_COLOR = homeTimeColor;
    }
    if (typeof getCfg('SHOW_SECONDS') !== 'undefined') {
      message.SHOW_SECONDS = getCfg('SHOW_SECONDS') ? 1 : 0;
    }
    if (typeof getCfg('SHOW_HOME_SECONDS') !== 'undefined') {
      message.SHOW_HOME_SECONDS = getCfg('SHOW_HOME_SECONDS') ? 1 : 0;
    }    
    // Send to watch
    Pebble.sendAppMessage(message,
      function() {
        console.log('Configuration sent successfully!');
      },
      function(e) {
        console.error('Failed to send configuration:', e.error.message);
      }
    );
  } else {
    console.log('Configuration closed without changes');
  }
});