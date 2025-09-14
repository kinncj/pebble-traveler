#!/usr/bin/env python3
import json
import os

ROOT = os.path.dirname(os.path.dirname(__file__))
SRC = os.path.join(ROOT, 'src', 'shared', 'timezones.json')
OUT = os.path.join(ROOT, 'src', 'shared', 'timezones.canonical.json')

with open(SRC, 'r', encoding='utf-8') as f:
    data = json.load(f)

seen = set()
compact = []
for entry in data:
    ident = entry.get('identifier')
    if not ident:
        continue
    if ident in seen:
        continue
    seen.add(ident)
    # pick abbreviation: prefer abbreviation_sdt then abbreviation_dst
    abbr = entry.get('abbreviation_sdt') or entry.get('abbreviation_dst') or ''
    # prefer offset_sdt
    offset_str = entry.get('offset_sdt') or entry.get('offset_dst') or ''
    # compute offset_minutes
    offset_minutes = 0
    if offset_str:
        t = offset_str
        sign = 1
        if t[0] == '+': t = t[1:]
        elif t[0] == '-': sign = -1; t = t[1:]
        parts = t.split(':')
        try:
            h = int(parts[0])
            m = int(parts[1]) if len(parts) > 1 else 0
            offset_minutes = sign * (h * 60 + m)
        except Exception:
            offset_minutes = 0
    # preserve original numeric id if present
    cid = entry.get('id') if isinstance(entry.get('id'), int) else None
    obj = {
        'identifier': ident,
        'display_name': entry.get('display_name',''),
        'abbr': abbr,
        'offset_str': offset_str,
        'offset_minutes': offset_minutes
    }
    if cid is not None:
        obj['id'] = cid
    compact.append(obj)

# sort by identifier for determinism
compact.sort(key=lambda x: x['identifier'])

with open(OUT, 'w', encoding='utf-8') as f:
    json.dump(compact, f, indent=2, ensure_ascii=False)

print('Wrote', OUT, 'entries=', len(compact))
