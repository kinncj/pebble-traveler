#!/usr/bin/env python3
"""
Utility to generate `src/shared/timezones.h` from the authoritative
`src/shared/timezones.json` file.

Usage:
  python3 utility/timezone_tool.py --gen
  python3 utility/timezone_tool.py --json path/to/timezones.json --out path/to/timezones.h

This script treats the JSON file as the single source of truth and emits a C header
containing a `SharedTimezone` array and `SHARED_TIMEZONE_COUNT`.
"""

import argparse
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_JSON_PATH = os.path.join(ROOT, 'src', 'shared', 'timezones.canonical.json')
DEFAULT_HEADER_PATH = os.path.join(ROOT, 'src', 'shared', 'timezones.h')


def esc(s):
    if s is None:
        s = ''
    return s.replace('\\', '\\\\').replace('"', '\\"')


def generate_header_from_json(json_path, out_path):
    if not os.path.exists(json_path):
        print('JSON not found at', json_path, file=sys.stderr)
        raise SystemExit(2)
    with open(json_path, 'r', encoding='utf-8') as f:
        try:
            tzs = json.load(f)
        except Exception as e:
            print('Failed to parse JSON:', e, file=sys.stderr)
            raise

    # Ensure deterministic order: sort by identifier if id not reliable
    try:
        tzs_sorted = sorted(tzs, key=lambda t: (int(t.get('id', 0)) if str(t.get('id','')).isdigit() else 10**9, t.get('identifier','')))
    except Exception:
        tzs_sorted = tzs

    count = len(tzs_sorted)
    lines = []
    lines.append('#ifndef TIMEZONES_H')
    lines.append('#define TIMEZONES_H')
    lines.append('')
    lines.append('#define SHARED_TIMEZONE_COUNT %d' % count)
    lines.append('')
    lines.append('typedef struct {')
    lines.append('  int id;')
    lines.append('  const char *identifier;')
    lines.append('  const char *display_name;')
    lines.append('  const char *abbreviation;')
    lines.append('  const char *offset_str;')
    lines.append('  int offset_minutes;')
    lines.append('} SharedTimezone;')
    lines.append('')
    lines.append('static const SharedTimezone SHARED_TIMEZONES[SHARED_TIMEZONE_COUNT] = {')

    # Assign deterministic IDs: prefer existing 'id' in JSON, otherwise use the 1-based index
    for idx, t in enumerate(tzs_sorted, start=1):
        identifier = esc(t.get('identifier', ''))
        display = esc(t.get('display_name', t.get('identifier', '')))
        abbr = esc(t.get('abbreviation_sdt', t.get('abbreviation', '')))
        off_str = esc(t.get('offset_sdt', t.get('offset_str', '')))
        try:
            # Try to get offset_minutes first, then convert offset_hours to minutes
            off_min = int(t.get('offset_minutes', 0))
            if off_min == 0 and 'offset_hours' in t:
                off_min = int(t.get('offset_hours', 0)) * 60
        except Exception:
            off_min = 0
        # Use JSON id if present and numeric; otherwise fall back to sequential idx
        try:
            tid = int(t.get('id')) if ('id' in t and str(t.get('id')).isdigit()) else idx
        except Exception:
            tid = idx
        line = '  {%d, "%s", "%s", "%s", "%s", %d},' % (
            tid, identifier, display, abbr, off_str, off_min)
        lines.append(line)

    lines.append('};')
    lines.append('')
    lines.append('#endif // TIMEZONES_H')

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
    print('Wrote', out_path)


def main():
    parser = argparse.ArgumentParser(description='Generate timezones.h from timezones.json')
    parser.add_argument('--json', default=DEFAULT_JSON_PATH, help='Path to timezones.json (default: %(default)s)')
    parser.add_argument('--out', default=DEFAULT_HEADER_PATH, help='Output header path (default: %(default)s)')
    parser.add_argument('--gen', action='store_true', help='Generate header from JSON')
    args = parser.parse_args()

    if not args.gen:
        parser.print_help()
        return

    generate_header_from_json(args.json, args.out)


if __name__ == '__main__':
    main()
