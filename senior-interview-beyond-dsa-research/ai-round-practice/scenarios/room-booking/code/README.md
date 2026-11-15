# roombook

`roombook` manages meeting rooms for one office day. Today it can check a file of manual
bookings for conflicts. Next it needs to assign rooms to booking requests automatically.

## Files

Rooms:

```
room R101 cap=6 features=tv,whiteboard
room R201 cap=12 features=vc,tv
room R005 cap=4 features=-
```

Booking requests:

```
request q17 09:30-10:15 size=5 needs=vc
request q18 13:00-14:00 size=2 needs=-
```

Manual bookings (for conflict checks):

```
booking R101 09:00-10:00 standup
```

- Times are `HH:MM` within one day. `24:00` is allowed as an end time.
- A booking covers `[start, end)`: a meeting that ends at 10:00 and one that starts at 10:00 do not overlap.
- Features: `tv`, `vc`, `whiteboard`, `accessible`. `-` means none.

## Layout

| Path | What it does |
| --- | --- |
| `src/clock.*` | Parse and format `HH:MM` |
| `src/catalog.*` | Parse rooms, requests and manual bookings; feature bitmasks |
| `src/calendar.*` | Per-room bookings, overlap checks, conflict report |
| `src/allocator.*` | Automatic room assignment (not implemented yet) |
| `app/main.cpp` | `roombook check <rooms> <bookings>` and `roombook assign <rooms> <requests>` |
| `tests/` | Tests |
| `data/` | Sample files |
