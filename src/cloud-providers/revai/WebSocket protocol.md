WebSocket protocol
Initial connection
All requests begin as an HTTP GET request. A WebSocket request is declared by including the header value Upgrade: websocket and Connection: Upgrade.

Client --> Rev AI
GET /speechtotext/v1/stream HTTP/1.1
Host: api.rev.ai
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: Chxzu/uTUCmjkFH9d/8NTg==
Sec-WebSocket-Version: 13
Origin: http://api.rev.ai
If authorization is successful, the request is upgraded to a WebSocket connection.

Client <-- Rev AI
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: z0pcAwXZZRVlMcca8lmHCPzvrKU=
After the connection has been upgraded, the servers will return a "connected" message. You must wait for this connected message before sending binary audio data. The response includes an id, which is the corresponding job identifier, as shown in the example below:

{
    "type": "connected",
    "id": s1d24ax2fd21
}
warning
If Rev AI currently does not have the capacity to handle the request, a WebSocket close message is returned with status code of 4013. A HTTP/1.1 400 Bad Request response indicates that the request is not a WebSocket upgrade request.

Audio submission
WebSocket messages sent to Rev AI must be of one of these two WebSocket message types:

Message type	Message requirements	Notes
Binary	Audio data is transmitted as binary data and should be sent in chunks of 250ms or more.

Streams sending audio chunks that are less than 250ms in size may experience increased transcription latency.	The format of the audio must match that specified in the content_type parameter.
Text	The client should send an End-Of-Stream("EOS") text message to signal the end of audio data, and thus gracefully close the WebSocket connection.

On an EOS message, Rev AI will return a final hypothesis along with a WebSocket close message.	Currently, only this one text message type is supported.

WebSocket close type messages are explicitly not supported as a message type and will abruptly close the socket connection with a 1007 Invalid Payload error. Clients will not receive their final hypothesis in this case.

Any other text messages, including incorrectly capitalized messages such as "eos" and "Eos", are invalid and will also close the socket connection with a 1007 Invalid Payload error.