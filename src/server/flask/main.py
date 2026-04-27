from flask import Flask, request, jsonify, render_template_string
import uuid
import json
import os

app = Flask(__name__)

# This is where we store the UUID mappings for consistent UUIDs across runs
REGISTRY_FILE = "device_registry.json"

# { hw_uuid: { "hw_id": str, "functions": { func_name: func_uuid } } }
devices = {}

# { func_uuid: hw_uuid }
func_to_device = {}

# { func_uuid: state_dict }
desired_states = {}

# { func_uuid: state_dict }
sensor_data = {}
hw_id_registry = {}

# loads the registry from the file if it exists
def load_registry():
    if os.path.exists(REGISTRY_FILE):
        with open(REGISTRY_FILE, "r") as f:
            hw_id_registry = json.load(f)

# dumps the registry to the file
def save_registry():
    with open(REGISTRY_FILE, "w") as f:
        json.dump(hw_id_registry, f, indent=2)

@app.route("/handshake", methods=["POST"])
def handshake():
    """
    Register a device and assign UUIDs.

    Request JSON:
    {
        "hw_id": "<MAC ADDR>",
        "functions": ["temperature", "control", ...]
    }

    Response JSON:
    {
        "uuid": "<generated uuid>",
        "functions": {
            "<function_name>": "<func_uuid>",
            ...
        }
    }

    Returns:
        200 on success
        400 on malformed request
    """
    # make sure we have the hw_id and the functions the node provides
    data = request.get_json(silent=True)
    if not data or "hw_id" not in data or "functions" not in data:
        return jsonify({"error": "malformed request"}), 400

    hw_id = data["hw_id"]
    functions = data["functions"]

    if not isinstance(functions, list) or not all(isinstance(f, str) for f in functions):
        return jsonify({"error": "malformed request"}), 400

    # Reuse existing hw_uuid for this hw_id if we've seen it before
    if hw_id in hw_id_registry:
        hw_uuid = hw_id_registry[hw_id]
    else:
        hw_uuid = str(uuid.uuid4())
        hw_id_registry[hw_id] = hw_uuid
        save_registry()

    # Give each node "function" it's own UUID
    func_map = {}
    for func_name in functions:
        func_uuid = str(uuid.uuid4())
        func_map[func_name] = func_uuid
        func_to_device[func_uuid] = hw_uuid

    devices[hw_uuid] = {
        "hw_id": hw_id,
        "functions": func_map,
    }

    return jsonify({
        "uuid": hw_uuid,
        "functions": func_map,
    }), 200


@app.route("/update", methods=["POST"])
def update():
    """
    Receive sensor data for a specific device function.

    Request JSON:
    {
        "hw_uuid": "<hw_uuid>",
        "func_uuid": "<func_uuid>",
        "data": { ... }
    }

    Behavior:
        - Validates hw_uuid and func_uuid
        - Ensures func_uuid belongs to hw_uuid
        - Processes or stores sensor data

    Returns:
        200 if accepted
        400 if payload is malformed
        401 if UUIDs are invalid (client should re-handshake)
    """
    # Make sure we actually got data from the endpoint
    data = request.get_json(silent=True)
    if not data or "hw_uuid" not in data or "func_uuid" not in data or "data" not in data:
        return jsonify({"error": "malformed request"}), 400

    hw_uuid = data["hw_uuid"]
    func_uuid = data["func_uuid"]
    payload = data["data"]

    if hw_uuid not in devices:
        return jsonify({"error": "unknown uuid"}), 401

    if func_uuid not in func_to_device or func_to_device[func_uuid] != hw_uuid:
        return jsonify({"error": "unknown uuid"}), 401

    # Internal store of the data
    # This will be changed once I see what the frontend expects
    sensor_data[func_uuid] = payload

    return jsonify({}), 200


@app.route("/control", methods=["POST"])
def control():
    """
    Poll and synchronize control state for a device function.

    Request JSON:
    {
        "hw_uuid": "<hw_uuid>",
        "func_uuid": "<func_uuid>",
        "state": { ... }
    }

    Behavior:
        - Validates hw_uuid and func_uuid
        - Ensures func_uuid belongs to hw_uuid
        - Compares client state with server desired state
        - Returns desired state (or echoes if unchanged)

    Response JSON:
    {
        "hw_uuid": "<hw_uuid>",
        "func_uuid": "<func_uuid>",
        "state": { ... }
    }

    Returns:
        200 on success
        400 if payload is malformed
        401 if UUIDs are invalid
    """
    # Make sure all information required is here
    data = request.get_json(silent=True)
    if not data or "hw_uuid" not in data or "func_uuid" not in data or "state" not in data:
        return jsonify({"error": "malformed request"}), 400

    hw_uuid = data["hw_uuid"]
    func_uuid = data["func_uuid"]
    client_state = data["state"]

    if hw_uuid not in devices:
        return jsonify({"error": "unknown uuid"}), 401

    if func_uuid not in func_to_device or func_to_device[func_uuid] != hw_uuid:
        return jsonify({"error": "unknown uuid"}), 401

    # Get the currently stored desired state as stored
    target_state = desired_states.get(func_uuid, client_state)

    return jsonify({
        "hw_uuid": hw_uuid,
        "func_uuid": func_uuid,
        "state": target_state,
    }), 200


@app.route("/internal/set_state", methods=["POST"])
def set_state():
    """
    Internal endpoint to set desired state for a function.

    Request JSON:
    {
        "func_uuid": "<func_uuid>",
        "state": { ... }
    }

    Behavior:
        - Validates func_uuid
        - Stores/overwrites desired state
        - Intended for localhost use only

    Returns:
        200 if state set successfully
        400 if payload is invalid
        403 if request is not from localhost
        404 if func_uuid is not found
    """
    # Make sure it's localhost
    if request.remote_addr != "127.0.0.1":
        return jsonify({"error": "forbidden"}), 403

    data = request.get_json(silent=True)
    if not data or "func_uuid" not in data or "state" not in data:
        return jsonify({"error": "missing or invalid fields"}), 400

    func_uuid = data["func_uuid"]
    state = data["state"]

    if not isinstance(state, dict):
        return jsonify({"error": "missing or invalid fields"}), 400

    if func_uuid not in func_to_device:
        return jsonify({"error": "func_uuid not found"}), 404

    # Store the provided state
    desired_states[func_uuid] = state

    return jsonify({}), 200


if __name__ == "__main__":
    app.run(debug=True)