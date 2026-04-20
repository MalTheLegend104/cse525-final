# IoT Device Communication Protocol

## Overview

This document describes the communication protocol between IoT client devices and the Flask server.
Clients register via a handshake, then send sensor updates and/or poll for control commands using the UUIDs issued during registration.

---

## Endpoints

### `POST /handshake`

Registers a new device with the server and receives UUIDs for use in all future communication.

**Request**

```json
{
    "hw_id": "<MAC ADDR>",
    "functions": [
        "temperature",
        "control",
        "motion",
        "open_close"
    ]
}
```

| Field       | Type             | Description                                          |
|-------------|------------------|------------------------------------------------------|
| `hw_id`     | string           | The hardware identifier ( MAC address) of the device |
| `functions` | array of strings | List of capabilities this device supports            |

**Response**

```json
{
    "uuid": "<generated uuid>",
    "functions": {
        "temperature": "<temp uuid>",
        "control": "<control uuid>",
        "open/close": "<open/close uuid>"
    }
}
```

| Field       | Type   | Description                                                  |
|-------------|--------|--------------------------------------------------------------|
| `uuid`      | string | The device-level UUID to use as `hw_uuid` in future requests |
| `functions` | object | Map of function name to its assigned `func_uuid`             |

The client **must** use these UUIDs in all subsequent communication.
If the server ever returns `401`, the client is expected to re-run the handshake to obtain fresh credentials.

---

### `POST /update`

Sends sensor data to the server.
If a device supports multiple functions, each function's data is sent as a **separate packet**, allowing each function to report as required.

> This allows sensors to change only on interrupts for example, instead of relying on specific timing.

**Request**

```json
{
    "hw_uuid": "<hw_uuid>",
    "func_uuid": "<func_uuid>",
    "data": {
        "temperature_c": 25.1,
        "temperature_f": 77.18
    }
}
```

| Field       | Type   | Description                                |
|-------------|--------|--------------------------------------------|
| `hw_uuid`   | string | Device UUID issued at handshake            |
| `func_uuid` | string | Function UUID issued at handshake          |
| `data`      | object | Arbitrary sensor payload for this function |

**Responses**

| Code  | Meaning                                                             |
|-------|---------------------------------------------------------------------|
| `200` | Data received and accepted                                          |
| `400` | Malformed or invalid data payload                                   |
| `401` | `hw_uuid` or `func_uuid` not found. The client should re-handshake. |

The server must verify that func_uuid is associated with the provided hw_uuid.
If not, it returns HTTP 401.
---

### `POST /control`

Polling endpoint for devices that support controllable functions.
The client sends its **current state**; the server responds with the **desired state**.
If there is no change, the server echoes the request back unchanged.

The client is expected to **blindly follow** the state returned by the server.
The client must apply the returned state unless it is invalid or physically impossible

**Request**

```json
{
    "hw_uuid": "<hw_uuid>",
    "func_uuid": "<func_uuid>",
    "state": {
        "led": "off"
    }
}
```

**Response**

```json
{
    "hw_uuid": "<hw_uuid>",
    "func_uuid": "<func_uuid>",
    "state": {
        "led": "on"
    }
}
```

| Field       | Type   | Description                       |
|-------------|--------|-----------------------------------|
| `hw_uuid`   | string | Device UUID (echoed)              |
| `func_uuid` | string | Function UUID (echoed)            |
| `state`     | object | The state the client should apply |

If the response state is **identical** to what was sent, the client holds its current state unchanged.

If the server **does not respond**, the client holds its current state.

The server must verify that func_uuid is associated with the provided hw_uuid.
If not, it returns HTTP 401.

**Responses (from client back to caller)**

| Code  | Meaning                                  |
|-------|------------------------------------------|
| `200` | State accepted and will be applied       |
| `400` | Requested state is invalid or impossible |
| `401` | UUIDs do not match the client's registry |

---

### `POST /internal/set_state`

Internal endpoint used by the webserver/dashboard to queue a new desired state for a device function.
Sets the desired state for a function. This state will be returned on the next `/control` poll.
Subsequent updates overwrite the previous state.

> This endpoint is only allowed on localhost. It will return HTTP 403 if accessed from anywhere else.

**Request**

```json
{
    "func_uuid": "<func_uuid>",
    "state": {
        "led": "on"
    }
}
```

**Responses**

| Code  | Meaning                           |
|-------|-----------------------------------|
| `200` | State queued successfully         |
| `400` | Missing or invalid `state` field  |
| `404` | `func_uuid` not found in registry |

---

## Error Reference

| Code  | Endpoint(s)                                  | Meaning                                                  |
|-------|----------------------------------------------|----------------------------------------------------------|
| `200` | All                                          | Success                                                  |
| `400` | `/update`, `/control`, `/internal/set_state` | Malformed or unparseable payload                         |
| `401` | `/update`, `/control`                        | Unknown `hw_uuid` or `func_uuid` — re-handshake required |
| `404` | `/internal/set_state`                        | `func_uuid` not found                                    |