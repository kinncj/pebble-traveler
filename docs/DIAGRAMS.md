# Diagrams (Mermaid)

This file contains architecture and message-flow diagrams in Mermaid format. Use a renderer that supports Mermaid to view them.

## System Architecture

```mermaid
flowchart LR
  PhoneApp["Phone App (Clay)"] -->|AppMessage| Watch["Watch (C App)"]
  Watch -->|Persist| Storage[(Persistent Storage)]
  Watch -->|Tick| TimeLib[(system time / localtime/gmtime)]
  Watch -->|Accel Tap| Accel["Accelerometer Service"]
  Watch -->|Display| Screen[TextLayers]
  subgraph Platforms
    Basalt(Basalt)
    Chalk(Chalk)
    Diorite(Diorite)
    Emery(Emery)
  end
  Watch --> Platforms
```

## AppMessage / Configuration Flow

```mermaid
sequenceDiagram
  participant User
  participant PhoneApp as "Phone (Clay)"
  participant Watch
  participant Storage
  User->>PhoneApp: Select timezone / colors
  PhoneApp->>Watch: send AppMessage (timezone/color keys)
  Watch->>Watch: inbox_received_callback
  Watch->>Storage: persist_write_* for each key
  Watch->>Watch: update_time_display()
  Watch-->>PhoneApp: outbox_sent (optional ack)
```
```
