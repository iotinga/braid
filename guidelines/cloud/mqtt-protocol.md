![Finanziato dall'Unione europea | Ministero dell'Università e della Ricerca | Italia domani PNRR | iNEST ](../../assets/HEADER_INEST.png)

# MQTT Protocol

>This protocol is based upon MQTTv5 features and the device shadowing paradigm. Globally, BRAID is designed around a single MQTT Backbone. Which means that when we talk about "the MQTT Broker" it stands for the MQTT Backbone.
>In other words any MQTT Agent is a standard MQTTv5 client talking with a unique MQTT Broker - The Backbone.
>Despite the protocol is mainly meant to handle data collection from remote devices (AGENT), we identify two additional players: ETL and HMI. The first one, ETL, is a cloud software acting as a MQTT client meant to write all the incoming data to a timeseries database. 
>The second one, HMI, is a generic software intended to send configurations and settings to AGENTs. HMI is out-of-the-scope of this work, but we include it in the protocol definition for completeness and as entry point for further works.

# Stateless protocols basic definitions

To define actions and status codes we take wide inspiration from HTTP definitions. HTTP is a widely know format and its actions / statuses model is appreciated for completeness and clearness.

## Generic actions

The full list of possible actions is defined in the following table.

| ACTION  | VALUE | DESCRIPTION                                                                                                                                     | AVAILABILITY |
|---------|-------|-------------------------------------------------------------------------------------------------------------------------------------------------|--------------|
| OPTIONS | 1     | Requests information about the communication options available                                                                                  | N.A.         |
| GET     | 2     | Means retrieve whatever information is identified by the topic                                                                                  | AGENT        |
| HEAD    | 3     | Is identical to GET, except that the provider must not return a body in the response                                                            | N.A.         |
| POST    | 4     | Is used to request that the receiver accepts the body enclosed in the message as a new subordinate for the information identified by the topic. | AGENT        |
| PUT     | 5     | Is used to request that the receiver stores the entire body enclosed in the message for the information identified by the topic.                | AGENT        |
| DELETE  | 6     | Requests that the receiver delete the data associated to the topic                                                                              | N.A.         |
| TRACE   | 7     | Is used to invoke an application-layer loopback. The final receiver should reflect the message received back to the client                      | N.A.         |

## Generic status codes

The first digit of the status code defines the class of response. The last two digits do not have any categorization role. There are six possible values for the first digit.

| VALUE | NAME          | DESCRIPTION                                                     |
|-------|---------------|-----------------------------------------------------------------|
| 0     | Request       | Identifies a request                                            |
| 1xx   | Informational | The request was received, continuing process                    |
| 2xx   | Successful    | The request was successfully received, understood, and accepted |
| 3xx   | Redirection   | Further action needs to be taken to complete the request        |
| 4xx   | Client Error  | The request contains bad syntax or cannot be fulfilled          |
| 5xx   | Server Error  | The receiver failed to fulfil an apparently valid request       |

## Used status codes

| STATUS             | CODE | DESCRIPTION                                                                                                                                                                                |
|--------------------|------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| REQUEST            | 0    | Identifies a request                                                                                                                                                                       |
| ACCEPTED           | 202  | The request has been accepted                                                                                                                                                              |
| BAD REQUEST        | 400  | Indicates that the receiver cannot or will not process the request due to something that is perceived to be a client error                                                                 |
| FORBIDDEN          | 403  | Indicates that the sender is tot allowed to perform the specified action on the specified resource                                                                                         |
| ACTION NOT ALLOWED | 405  | The specified action is not allowed on the specified resource                                                                                                                              |
| CONFLICT           | 409  | The version in the request doesn't match the one present on the device. The requester needs to do a GET request first to align its state to the one of the device, then repeat the request |
| INTERNAL ERROR     | 500  | Indicates that the server encountered an unexpected condition that prevented it from fulfilling the request                                                                                |
| NOT IMPLEMENTED    | 501  | The specified action is not currently implemented (but it may be in the future)                                                                                                            |

# MQTT level definitions

In this section we define how BRAID shall use the MQTT protocol. As MQTT is a very simple protocol, we only need to define our Topic definition model and the format of the payload.

## Topic model

The ID of the agent is a string unique for the particular agent. A good ID candidate is the device mac address.
The role is an identifier used from HMI's. We supposed that HMI users are classified in roles and the broker will allow or deny write actions for certains roles on certains topics.

| # | TOPIC                                        | PUB   | SUB        | ACTIONS        |
|---|----------------------------------------------|-------|------------|----------------|
| 1 | braid/agent/{ID}/shadow/reported/            | AGENT | HMI, ETL   | POST           |
| 2 | braid/agent/{ID}/shadow/desired/{role}       | HMI   | HMI, AGENT | GET, POST, PUT |

## Payload datagram

Using the MQTTv5 feature which allows to specify the content-type of the payload, the datagram of the message payload is very simple, as we assume that all the data is encoded using the content-type format. The default encoding is CBOR, so this documentation defines payload types as CBOR types. For more information about CBOR see [https://cbor.io/](https://cbor.io/).

| LABEL              | TYPE        | SOURCE    | DESCRIPTION                                                                                       |
|--------------------|-------------|-----------|---------------------------------------------------------------------------------------------------|
| TS                 | NUMBER      | AGENT     | Timestamp                                                                                         |
| VER                | NUMBER      | AGENT     | Incremental sequence number of packets                                                            |
| PROT               | NUMBER      | AGENT     | Defines the protocol version: fixed value to 1                                                    |
| ACTION             | NUMBER      | AGENT     | Defines the action performed from the Publisher                                                   |
| STATUS             | NUMBER      | AGENT     | Defines the status (0 for REQUEST)                                                                |
| BODY               | OBJECT      | AGENT     | For key definition each system instance sholud define its specific document                       |
| SIGN               | BYTE STRING | AGENT     | Signature from the agent on all the previous data. Can be used to verify data source              |
| INGESTION_TIME     | NUMBER      | BROKER(*) | Timestamp of the broker at the MQTT message arrival                                               |
| VERIFICATION_TOKEN | BYTE STRING | BROKER(*) | Signature from the broker on all the previous data. Can be used to verify previous data integrity |

> (*) BROKER keys are added only in CA scenarios, not in blockchain scenarios

### BODY section definition

The BODY object keys set is custom for each system instance. In our experiment it carries all our sensors reads. We also have only one writable field that allows the server to change update frequency.

### Shadow "Full state" definition

When we say “full state” we must specify what “full” means, because publishing a message to the desired topic "full state" means that the publisher (an HMI) shall send all the writable keys for its role. In the reported state topic instead, "full state" means "all the defined keys" (read and read/write).