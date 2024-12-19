Deepgram Streaming API
The goal of your SDK should minimally be:

Manage the Connection Lifecycle: Implement robust connection management to handle opening, error handling, message sending, receiving, and closing of the WebSocket connection.
Concurrency and Threading: Depending on the SDK's target language, manage concurrency appropriately to handle the asynchronous nature of WebSocket communication without blocking the main thread.
Error Handling and Reconnection: Implement error handling and automatic reconnection logic. Transient network issues should not result in lost data or service interruptions.
Implement KeepAlives: Deepgram's API may require keepalive messages to maintain the connection. Implement a mechanism to send periodic pings or other suitable messages to prevent timeouts.
High-Level Pseudo-Code for Deepgram Streaming API
It's essential that you encapsulate your WebSocket connection in a class or similar representation. This will reduce undesired, highly coupled WebSocket code with your application's code. In the industry, this has often been referred to as minimizing "Spaghetti code". If you have WebSocket code or you need to import the above WebSocket libraries into your func main(), this is undesirable unless your application is trivially small.

To implement the WebSocket Client correctly, you must implement based on the WebSocket protocol defined in RFC-6455. Please refer to section 4.1 Client Requirements in RFC-6455.

You want first to declare a WebSocket class of some sort specific to your implementation language:

Text

// This class could simply be called WebSocketClient
// However, since this is specifically for Deepgram, it could be called DeepgramClient
class WebSocketClient {
  private url: String
  private apiKey: String
  private websocket: WebSocket

  // other class properties

  // other class methods
}
NOTE: Depending on the programming language of choice, you might either need to implement async/await and threaded classes to support both threading models. These concepts occur in languages like Javascript, Python, and others. You can implement one or both based on your user's needs.

You will then need to implement the following class methods.

Function: Connect

class WebSocketClient {
  ...
  function Connect() {
    // Implement the websocket connection here
  }
  ...
}
This function should:

Initialize the WebSocket connection using the URL and API Key.
Set up event listener threads for connection events (message, metadata, error).
Start the keep alive timer based on the Keepalive Interval.
Thread: Receive and Process Messages

class WebSocketClient {
  ...
  function ThreadProcessMessages() {
    // Implement the thread handler to process messages
  }
  ...
}
This thread should:

When a message arrives, check if it's a transcription result or a system message.
For transcription messages, process or handle the transcription data.
Handle system messages accordingly (may include error messages or status updates).
Function: Send

class WebSocketClient {
  ...
  function SendBinary([]bytes) {
    // Implements a send function to transport audio to the Deepgram server
  }

  function SendMessage([]byte) {
    // Implements a send function to transport control messages to the Deepgram server
  }
  ...
}
The SendBinary() function should:

Accept audio data as input.
Send the audio data over the WebSocket connection to Deepgram for processing.
The SendMessage() function should:

Accept JSON data as input.
Send the JSON over the WebSocket connection to Deepgram for handling control or connection management type functions. A KeepAlive or CloseStreammessages are examples of these types of messages.
If you need more information on the difference, please refer to RFC-6455.

(Optional) Thread: KeepAlive

class WebSocketClient {
  ...
  function ThreadKeepAlive() {
    // Implement the thread handler to process messages
  }
  ...
}
This thread is optional providing that audio data is constantly streaming to through the WebSocket; otherwise, it should:

Regularly send a keepalive message (such as a ping) to Deepgram based on the Keepalive Interval to maintain the connection.
Notice this thread is independent of the Receive/Process Message Thread above.

Function: Close

class WebSocketClient {
  ...
  function Close() {
    // Implement shutting down the websocket
  }
  ...
}
This function should:

Send a command to close the WebSocket connection.
Stop the keepalive timer to clean up resources.
Deepgram API Specifics
Now that you have a basic client, you must handle the Deepgram API specifics. Refer to this documentation for more information .

Function: Connect
When establishing a connection, you must pass the required parameters defined by the Deepgram Query Parameters.

Thread: Receive and Process Messages
If successfully connected, you should start receiving transcription messages (albeit empty) in the Response Schema defined below.

JSON

{
  "metadata": {
    "transaction_key": "string",
    "request_id": "uuid",
    "sha256": "string",
    "created": "string",
    "duration": 0,
    "channels": 0,
    "models": [
      "string"
    ],
  },
  "type": "Results",
  "channel_index": [
    0,
    0
  ],
  "duration": 0.0,
  "start": 0.0,
  "is_final": boolean,
  "speech_final": boolean,
  "channel": {
    "alternatives": [
      {
        "transcript": "string",
        "confidence": 0,
        "words": [
          {
            "word": "string",
            "start": 0,
            "end": 0,
            "confidence": 0
          }
        ]
      }
    ],
    "search": [
      {
        "query": "string",
        "hits": [
          {
            "confidence": 0,
            "start": 0,
            "end": 0,
            "snippet": "string"
          }
        ]
      }
    ]
  }
}
For convenience, you will need to marshal these JSON representations into usable objects/classes to give your users an easier time using your SDK.

(Optional) Thread: KeepAlive
If you do implement the KeepAlive message, you will need to follow the guidelines defined here.

Function: Close
When you are ready to close your WebSocket client, you will need to follow the shutdown guidelines defined here.

Special Considerations: Errors
You must be able to handle any protocol-level defined in RFC-6455 and application-level (i.e., messages from Deepgram) you will need to follow the guidelines defined here.

Troubleshooting
Here are some common implementation mistakes.

My WebSocket Connection Immediately Disconnects
There are usually a few reasons why the Deepgram Platform will terminate the connection:

No audio data is making it through the WebSocket to the Deepgram Platform. The platform will terminate the connection if no audio data is received in roughly 10 seconds.
A variation on the above... you have muted the audio source and are no longer sending an audio stream or data.
The audio encoding is not supported OR the encoding parameter does not match the encoding in the audio stream.
Invalid connection options via the query parameters are being used. This could be things like misspelling an option or using an incorrect value.
My WebSocket Connection Disconnects in the Middle of My Conversation
There are usually a few reasons why the Deepgram Platform will terminate the connection (similar to the above):

You have muted the audio source and are no longer sending an audio stream or data.
If no audio data is being sent, you must implement the KeepAlive protocol message.
My Transcription Messages Are Getting Delayed
There are usually a few reasons why the Deepgram Platform delays sending transcription messages:

You inadvertently send the KeepAlive protocol message as a Data or Stream message. This will cause the audio processing to choke or hiccup, thus causing the delay. Please refer to RFC-6455 to learn more about the difference between data and control messages.
Network connectivity issues. Please ensure your connection to the Deepgram domain/IP is good. Use ping and traceroute or tracert to map the network path from source to destination.
