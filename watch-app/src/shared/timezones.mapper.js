module.exports = function(data) {
  // filter any timezone where "offset_str": ""
  data = data.filter(tz => tz.offset_str !== '');

  var map = data.map(tz => ({
    value: tz.identifier,
    label: `${tz.identifier} (GMT ${tz.offset_str})`
  }));

  return map;

  // return map sorted alphabetically by value (identifier)
  return map.sort((a, b) => {
    // Use localeCompare with options for consistent, case-insensitive sorting
    return a.value.localeCompare(b.value, undefined, {
      sensitivity: 'base',
      numeric: true
    });
  });
}