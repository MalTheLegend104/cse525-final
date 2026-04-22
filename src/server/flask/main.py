from flask import Flask, request, jsonify, render_template_string
import uuid

app = Flask(__name__)

# This is a basic outline of the API.
#
# The client will send a packet with the following info:
# {
#     "hw_id" : "<MAC ADDR>",
#     "functions" : [
#         "temperature",
#         "control",
#         "motion",
#         "open/close",
#         "etc."
#     ]
# }
#
# The response will be in the form:
# {
#     "uuid" : "<generated uuid>",
#     "functions" : {
#         "temperature" : "<temp uuid>",
#         "control" : "<control uuid>",
#         "etc." : ""
#     }
# }
# The client is expected to use these UUIDs in all future communications.
#
# After successfully "authenticating":
#
# The client will send packets to `/update` with this format:
#
# {
#     "hw_uuid" : "<hw_uuid>",
#     "func_uuid" : "<func_uuid>",
#     "data" : {
#         "any data provided by the sensor"
#     }
# }
# If the node has more than one function, they will be sent as separate packets.
# This lets the update times for each function be different if desired.
#
# If the server cannot find either the hw_uuid or func_uuid in its registry, it will return HTTP 401.
# If the server cannot parse the data contained, it will return HTTP 400.
# If the server got the data and can use it, it will return HTTP 200.
#
# If the response is 401, it is expected that the client go through the /handshake endpoint again to refresh credentials
#
# If the client supports control functions, it is expected to poll the /control endpoint.
# The server will respond with the current control status from the webserver.
# If the server does not respond, it is expected that the client hold it's current state.
# The client control packet to the server should be in this form:
# {
#     "hw_uuid" : "",
#     "func_uuid": "",
#     "state" : {
#         "any relevant data for the specific control function"
#     }
# }
#
# The server will respond with basically the same packet.
# Any changes in this are expected to be changed on the client.
# {
#     "hw_uuid" : "",
#     "func_uuid": "",
#     "state" : {
#         "any relevant data for the specific control function"
#     }
# }
#
# If there is no change, the server is to echo the packet. It will be identical to what was sent.
# The client is expected to blindly follow the state provided by the server.
# It will only discard it if the server request is impossible or incorrect.
#
# The client can respond with HTTP 400 if the request is invalid, HTTP 401 it the UUIDs do not match, or HTTP 200 if accepted.
#
# For example, say we have a client that controls an LED. The client would poll the server with this packet:
# {
#     "hw_uuid" : "<hw_uuid>",
#     "func_uuid" : "<func_uuid>",
#     "state" : {
#         "led" : "off"
#     }
# }
#
# Say the server (via request from the webserver) wants to turn the LED on, it would respond with:
# {
#     "hw_uuid" : "<hw_uuid>",
#     "func_uuid" : "<func_uuid>",
#     "state" : {
#         "led" : "on"
#     }
# }

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
    pass


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
    pass


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
    pass


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
    pass


if __name__ == "__main__":
    app.run(debug=True)