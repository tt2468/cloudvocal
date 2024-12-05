# WebSocket protocol

## API endpoint
All connections to Rev AI's Streaming Speech-to-Text API start as a WebSocket handshake HTTP request to wss://api.rev.ai/speechtotext/v1/stream. On successful authorization, the client can start sending binary WebSocket messages containing audio data in one of the supported formats. As speech is detected, Rev AI returns hypotheses of the recognized speech content.

warning
The base URL is different from the base URL for the Asynchronous Speech-to-Text API.

Example
wss://api.rev.ai/speechtotext/v1/stream?access_token=<REVAI_ACCESS_TOKEN>&content_type=audio/x-raw;layout=interleaved;rate=16000;format=S16LE;channels=1&metadata=<METADATA>

## Responses
All transcript responses from the Streaming Speech-to-Text API are text messages and are returned as serialized JSON. The transcript response has two states: partial hypothesis and final hypothesis.

The JSON will contain a type property which indicates what kind of response the message is. Valid values for this type property are:

"connected"
"partial"
"final"
The "connected" type is only returned once during the initial handshake when opening a WebSocket connection. All other responses should be of the type "partial" or "final".

### Response Object
Here is a brief description of the response object and its properties:

Property Name	Type	Description
type	string	Either "partial" or "final"
ts	double	The start time of the hypothesis in seconds
end_ts	double	The end time of the hypothesis in seconds
elements	array of Elements	Only present if final property is true. A list of Rev AI transcript element properties. See Transcript object for details that are all the recognized words up to current point in audio

## Request
A WebSocket request to the Streaming Speech-to-Text API consists of the following parts:

Request parameter	Required	Default
Base URL (WebSocket) or read_url URL (RTMP)		Yes
Access Token	access_token	Yes	None
Content Type	content_type	Yes	None
Language	language	No	en
Metadata	metadata	No	None
Custom Vocabulary	custom_vocabulary_id	No	None
Profanity Filter	filter_profanity	No	false
Disfluencies	remove_disfluencies	No	false
Delete After Seconds	delete_after_seconds	No	None
Detailed Partials	detailed_partials	No	false
Start Timestamp	start_ts	No	None
Maximum segment duration seconds	max_segment_duration_seconds	No	None
Transcriber	transcriber	No	See transcriber section
Speaker switch detection	enable_speaker_switch	No	false
Skip Post-processing	skip_postprocessing	No	false
Priority	priority	No	speed
Maximum wait time for connection	max_connection_wait_seconds	No	60
Access token
Clients must authenticate by including their Rev AI access token as a query parameter in their requests. If access_token is invalid or the query parameter is not present, the WebSocket connection will be closed with code 4001.

Example
wss://api.rev.ai/speechtotext/v1/stream?access_token=<REVAI_ACCESS_TOKEN>&content_type=audio/x-raw;layout=interleaved;rate=16000;format=S16LE;channels=1
Content type

All requests must also contain a content_type query parameter. The content type describes the format of audio data being sent. If you are submitting raw audio, Rev AI requires extra parameters as shown below. If the content type is invalid or not set, the WebSocket connection is closed with a 4002 close code.

Rev AI officially supports these content types:

audio/x-raw (has additional requirements )
audio/x-flac
audio/x-wav
RAW file content type
You are required to provide additional information in content_type when content_type is audio/x-raw.

Parameter (type)	Description	Allowed Values	Required
layout (string)	The layout of channels within a buffer. Possible values are "interleaved" (for LRLRLRLR) and "non-interleaved" (LLLLRRRR). Not case-sensitive	interleaved,non-interleaved	audio/x-raw only
rate (int)	Sample rate of the audio bytes	Inclusive Range from 8000-48000Hz	audio/x-raw only
format (string)	Format of the audio samples. Case-sensitive. See Allowed Values column for valid values	List of valid formats	audio/x-raw only
channels (int)	Number of audio channels that the audio samples contain	Inclusive range from 1-10 channels	audio/x-raw only
These parameters follow the content_type, delimited by semi-colons (;). Each parameter should be specified in the format parameter_name=parameter_value.

Example
wss://api.rev.ai/speechtotext/v1/stream?access_token=<REVAI_ACCESS_TOKEN>&content_type=audio/x-raw;layout=interleaved;rate=16000;format=S16LE;channels=1&metadata=<METADATA>
Language
attention
Custom Prices (other than the default) are set independently by language. Please refer to your contract for pricing information. If you are not a contract customer the pricing is found here

Specify the transcription language with the language query parameter. When the language is not provided, transcription will default to English. The language query parameter cannot be used along with the following options: filter_profanity, remove_disfluencies, and custom_vocabulary_id.

Language	Language Code
English	en
French	fr
German	de
Italian	it
Japanese	ja
Korean	ko
Mandarin	cmn
Portuguese	pt
Spanish	es
Additional requirements for content type:

content_type must be audio/x-raw or audio/x-flac
when providing raw audio, it must be formatted as S16LE
rate must be included, regardless of content_type

### Request stages

#### Initial connection
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

#### Audio submission
WebSocket messages sent to Rev AI must be of one of these two WebSocket message types:

Message type	Message requirements	Notes
Binary	Audio data is transmitted as binary data and should be sent in chunks of 250ms or more.

Streams sending audio chunks that are less than 250ms in size may experience increased transcription latency.	The format of the audio must match that specified in the content_type parameter.
Text	The client should send an End-Of-Stream("EOS") text message to signal the end of audio data, and thus gracefully close the WebSocket connection.

On an EOS message, Rev AI will return a final hypothesis along with a WebSocket close message.	Currently, only this one text message type is supported.

WebSocket close type messages are explicitly not supported as a message type and will abruptly close the socket connection with a 1007 Invalid Payload error. Clients will not receive their final hypothesis in this case.

Any other text messages, including incorrectly capitalized messages such as "eos" and "Eos", are invalid and will also close the socket connection with a 1007 Invalid Payload error.
