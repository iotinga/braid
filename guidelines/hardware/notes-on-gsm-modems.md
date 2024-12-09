![Finanziato dall'Unione europea | Ministero dell'Università e della Ricerca | Italia domani PNRR | iNEST ](../../assets/HEADER_INEST.png)

# Notes on GSM modems
This document contains an overview of the hurdles which surfaced while developing firmware for the master MCU, focusing on the GSM modules used and the experiments ran to find out how the modules were supposed to be set up to actually be usable by the firmware. 

Through the use of the GSM modem, the firmware is expected to:
- Attach to a bearer network with a SIM card
- Connect to a MQTTS server, publish to topics and subscribe to other topics
- Synchronize system time using SNTP and/or GSM network time
- Acquire the device's geographic position, heading and speed using GPS

### SIM7080G
The first implementation of the master device firmware used to run on a Lilygo T-SIM7080G development board. While the board itself presented an adequate collection of hardware, the modem itself had several practical limitations, including but not limited to:
- **A single MUX** between the modem power line and the GNSS/GSM module power lines. This feature made it impossible to output data from one module while the other was active, forcing the firmware to lose either the LTE connection or the GPS lock when querying position or connecting to the internet.
- **Slow network attachment** procedure. Together with the feature listed above, it made every switching from the GNSS module to the GSM module take up to a minute in case of poor signal.
- **No support for PPP mode**. All implementations of raw TCP clients relied on writing data encoded in ASCII hexadecimal strings through the UART, in the AT command stream. As such, networking would be fragile and prone to breaking when other AT commands returned errors, requiring careful parsing of the modem's output at the hand of a random Arduino library with unconfirmed testing standards.
- It **does not support standard LTE**. Only Cat-M (widely available) and NB-IoT (which is not generally available in Europe) can be used for bearer connection.

Overall, the 7080G module was working as intended using its internally provided "apps" through the AT command interface. Using it as a raw network interface, however, was not possible. In the end, the module was replaced with a SIMCOM SIM7000G on top of a Lilygo T-SIM7000G development kit.

#### Network attachment procedure
The following ordered AT commands document how to connect a ThingsMobile SIM to the bearer network. Sof of these commands were taken from [this article](https://wiki.dfrobot.com/SIM7000_Arduino_NB-IoT_LTE_GPRS_Expansion_Shield_SKU__DFR0505_DFR0572).

| Step | Command             | When             | Description                                                                                                                                                                                      |
| ---- | ------------------- | ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 0    | `AT+CPIN=<pin>`     | On modem boot    | Unlocks the SIM. Not required if the SIM does not have a PIN set                                                                                                                                 |
| 1    | `AT+CNMP=38`        | On modem boot    | Selects the LTE networking mode                                                                                                                                                                  |
| 2    | `AT+CMNB=1`         | On modem boot    | Selects LTE over Cat-M only                                                                                                                                                                      |
| 3    | `AT+CGNSPWR=0`      | Every connection | Disables the GPS/GNSS module. Without this, the signal lock will never succeed                                                                                                                   |
| 4    | `AT+CSQ`            | Every connection | Returns signal strength in the form `<RSSI>,<BER>`. RSSI will be 99 if there is no signal. Subsequent commands will fail if the antenna hasn't locked onto a cell first. Can be called in a loop |
| 5    | `AT+CNCFG=0,1,"TM"` | Every connection | Configures PDP context number 0 to use `TM` as APN over IPv4                                                                                                                                     |
| 6    | `AT+CGATT=0,1`      | Every connection | Requests a connection to the bearer network using context 0                                                                                                                                      |

#### GPS position retrieval
The following ordered AT commands document how to query the GPS for positioning, heading and other information.

| Step | Command                 | When          | Description                                                                                                                                               |
| ---- | ----------------------- | ------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1    | `AT+CGNSMOD=1,0,0,1,0`  | On modem boot | Configures the GNSS module to use GPS and GALILEO constellations (gets the fastest lock time in Europe)                                                   |
| 2    | `AT+SGNSCMD=2,1000,0,1` | On modem boot | Configures the GNSS module to use periodic triangulation (once every 1000 milliseconds), without checking elapsed distance, with low precision acceptable |
| 3    | `AT+CNACT=0,0`          | Every query   | Drops all connections (TCP, MQTT etc.)                                                                                                                    |
| 4    | `AT+CGATT=0`            | Every query   | Disables the GPS/GNSS module. Without this, the signal lock will never succeed                                                                            |
| 5    | `AT+CGNSINF`            | Every query   | Queries the GPS network, without waiting for signal lock                                                                                                  |

The 7080 modem supports a special "accelerated locking" mode called "Qualcomm Xtra" which uses a file from the internet to precalculate satellite position at a given hour (much like a GPS almanac) for the various constellations. The following is an example procedure which downloads the Xtra file and loads it.

> [!NOTE]
>
> A working network connection is required to download the Xtra file from the internet

| Step | Command                                                                         | When             | Description                                                                                     |
| ---- | ------------------------------------------------------------------------------- | ---------------- | ----------------------------------------------------------------------------------------------- |
| 0    | `AT+CLTS=1`                                                                     | Every connection | Synchronizes modem time with the network                                                        |
| 1    | `AT+CFSGFIS=3,"Xtra3.bin"`                                                      | Every connection | Can be used to check existence of downloaded Xtra file (`/customer/Xtra3.bin`)                  |
| 2    | `AT+CGNSXTRA`                                                                   | Every connection | Can be used to check whether the currently loaded Xtra file is valid (expires every 72 hours)   |
| 3    | `AT+HTTPTOFS="http://iot2.xtracloud.net/xtra3ge_72h.bin","/customer/Xtra3.bin"` | Every connection | Downloads the Xtra file from the official server into the correct location for loading it later |
| 4    | `AT+HTTPTOFS?`                                                                  | Every connection | Checks download status                                                                          |
| 5    | `AT+CGNSCPY?`                                                                   | Every connection | Loads the Xtra file from the modem's flash into the GNSS module's space                         |
| 6    | `AT+CGNSXTRA`                                                                   | Every connection | Checks the newly loaded file                                                                    |
| 7    | `AT+CGNSXTRA=1`                                                                 | Every connection | Enables using the Xtra system for satellite discovery upon query                                |

### SIM7000G
The final networking stack of the master device is implemented using the `esp_modem` component controlling a SIM7000G modem. Over the SIM7080G, the modem has:
- **No power muxing**, allowing both GNSS and LTE modules to work at the same time.
- **Faster network attachment times**, at the expense of a few more milliamperes upon bearer search.
- **Support for raw PPP/data mode**, which lets the `esp_modem` library register the modem as standard network interface with better integration in the existing network stack of the ESP32.

However, the 7000G still present some issues which need careful considerations:
- **No CMUX mode**. This mode would let the modem receive and process AT commands while the raw data mode is active, without interrupting TCP connections or data streams.
- **Different AT commands** syntax and different vendor-specific commands from the 7080G.
- **Larger hardware footprint**: the 7000G package is more than double the size than that of the 7080G.
- It **does not support standard LTE**. Only Cat-M (widely available) and NB-IoT (which is not generally available in Europe) can be used for bearer connection.
- Attaching the modem to a 2G/EDGE network draws periodic peaks of upwards to 1 ampere of current, making it basically unusable without incurring in a modem brownout/reboot.

#### Network attachment procedure - Cat-M
The following ordered AT commands document how to connect a ThingsMobile SIM to the bearer network.

| Step | Command           | When             | Description                                                                                                                                                                                      |
| ---- | ----------------- | ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 0    | `AT+CPIN=<pin>`   | On modem boot    | Unlocks the SIM. Not required if the SIM does not have a PIN set                                                                                                                                 |
| 1    | `AT+CNMP=38`      | On modem boot    | Selects the LTE networking mode                                                                                                                                                                  |
| 2    | `AT+CMNB=1`       | On modem boot    | Selects LTE over Cat-M only                                                                                                                                                                      |
| 3    | `AT+CSQ`          | Every connection | Returns signal strength in the form `<RSSI>,<BER>`. RSSI will be 99 if there is no signal. Subsequent commands will fail if the antenna hasn't locked onto a cell first. Can be called in a loop |
| 4    | `AT+CNCFG=1,"TM"` | Every connection | Configures the PDP context to use `TM` as APN over IPv4                                                                                                                                          |
| 5    | `AT+CGATT=1`      | Every connection | Requests a connection to the bearer network                                                                                                                                                      |

After the procedure outlined above, the underlying `esp_netif_t` should receive an IP address. However, it has been observed that the `IP_EVENT_PPP_GOT_IP` event (which signals that the network interface has received an IP address through DHCP) is fired correctly only ~50% of times upon first attaching to the network. The second connection/attachment generally works every time. Because of this, the code retries toggling the network attachment on an exponential backoff to receive a new IP if the previous attempt timed out.

It has also been observed that the modem may spontaneously exit from data-only mode. This a very rare occurrence which seems related to connection timeouts in cases of poor signal lock. In such cases, it may be wise to have a periodic procedure which monitors the network connection for dropouts/timeouts and consequently:
1. Forcibly sets the modem in command mode
2. Issues a new network reconfiguration/attachment
3. Sets the modem to PPP mode and lets the network interface receive a new IP

PPP status changes can be intercepted with something similar to:

```c
static void on_ppp_changed(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    switch (event_id) {
        // Handle events
    }
}

void setup_netif() {
    esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &on_ppp_changed, NULL);
}
```

#### Network attachment procedure - 2G EDGE
The following ordered AT commands document how to connect a ThingsMobile SIM to the bearer network over 2G EDGE.

| Step | Command                                        | When             | Description                                                                                                                                                                                                                                 |
| ---- | ---------------------------------------------- | ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0    | `AT+CPIN=<pin>`                                | On modem boot    | Unlocks the SIM. Not required if the SIM does not have a PIN set                                                                                                                                                                            |
| 1    | `AT+CGPIO=0,48,1,0`                            | Every connection | On the Lilygo development board, this command turns off power to the GPS antenna. Otherwise, the antenna goes onto a high power mode                                                                                                        |
| 2    | `AT+CPIN=?`                                    | Every connection | Polls the modem. It is recommended to introduce a half-second delay afterwards. This is only based on empiric evidence                                                                                                                      |
| 3    | `AT+CFUN=0`                                    | Every connection | Puts the GPRS stack into reduced functionality mode, disabling the radio                                                                                                                                                                    |
| 4    | `AT+CNMP=13`                                   | Every connection | Selects the GSM networking mode                                                                                                                                                                                                             |
| 5    | `AT+CFUN=1`                                    | Every connection | Puts the GPRS stack into full functionality mode, enabling the radio                                                                                                                                                                        |
| 6    | `AT+CGDCONT=1,"IP","<apn>","0.0.0.0",0,0,0,0`  | Every connection | Configures PDP context 1, resetting the IP and setting the APN                                                                                                                                                                              |
| 7    | `AT+CGDCONT=13,"IP","<apn>","0.0.0.0",0,0,0,0` | Every connection | Configures PDP context 13, resetting the IP and setting the APN                                                                                                                                                                             |
| 8    | `AT+CSQ`                                       | Every connection | Returns signal strength in the form `+CSQ: <RSSI>,<BER>`. RSSI will be 99 if there is no signal. Subsequent commands will fail if the antenna hasn't locked onto a cell first. Should be called in a loop until the desired RSSI is reached |
| 9    | `AT+CGREG?`                                    | Every connection | Returns bearer registration status in the form `+CGREG: <_ignored>,<STATUS>`. STATUS will be either 1 (registered on home network) or 5 (registered on affiliated network/roaming) when the modem is registered. Should be called in a loop |
| 10   | `AT+CIPSHUT`                                   | Every connection | Drops all open TCP sockets                                                                                                                                                                                                                  |
| 11   | `AT+CGATT=0`                                   | Every connection | Drops connection to the bearer network                                                                                                                                                                                                      |
| 12   | `AT+SAPBR=3,1,"Contype","GPRS"`                | Every connection | Sets all internal apps to use the 2G connection mode                                                                                                                                                                                        |
| 13   | `AT+SAPBR=3,1,"APN","<apn>"`                   | Every connection | Sets all internal apps to use the given APN                                                                                                                                                                                                 |
| 14   | `AT+CGATT=1`                                   | Every connection | Requests a connection to the bearer network                                                                                                                                                                                                 |

This will generally result in the `esp_netif_t` receiving an IP address every time a connection is set up.

#### GPS position retrieval
The following ordered AT commands document how to query the GPS for positioning, heading and other information.

| Step | Command              | When          | Description                                                                                                                        |
| ---- | -------------------- | ------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| 1    | `AT+CGNSMOD=1,0,0,1` | On modem boot | Configures the GNSS module to use GPS and GALILEO constellations (gets the fastest lock time in Europe)                            |
| 2    | `AT+CGPIO=0,48,1,1`  | Every query   | On the Lilygo development board, this command turns on power to the GPS antenna. Otherwise, the antenna goes onto a low power mode |
| 3    | `AT+CGNSINF`         | Every query   | Queries the GPS network, without waiting for signal lock                                                                           |

THe 7000G is also equipped with Xtra almanac functionality. The commands are the same as those on the 7080G, documented above.

### Experiments timeline - The Cat-M saga
This section is less of a technical overview and more of a rough event recollection of the experimentation and material used to produce the final working code documented in this file.

1. Development started on the Lilygo T-SIM7080G. The Arduino framework was chosen because it was used in [the official code examples](https://github.com/Xinyuan-LilyGO/LilyGo-T-SIM7080G/tree/e34401b62e72fdb09956a29142cb2207282dd8b5/examples). In particular, the [MinimalModemNBIOTExample](https://github.com/Xinyuan-LilyGO/LilyGo-T-SIM7080G/tree/e34401b62e72fdb09956a29142cb2207282dd8b5/examples/MinimalModemNBIOTExample) was chosen as a starting point.
2. The GPS querying functionality was implemented first, without any particular issue. It was observed that the antenna was quite weak and would take a decent amount of time to receive signal lock even outside (around 30 seconds with Xtra enabled). It would never lock indoors, even close to windows.
3. A ThingsMobile SIM was chosen as data connectivity provider for development. Running the example code, the device would never succeed in registering to the bearer network. It was discovered that NBIoT, at the time of writing, is generally unavailable in Europe.
4. The modem was configured to use LTE Cat-M, but still would not register to the network. More troubleshooting required connecting to the modem via UART from a terminal and issuing raw AT commands. Through such a method, it was discovered that the SIM required an APN to be set manually and forcing IPv4, contrary to what was documented on ThingsMobile's website (the SIM was supposed to automatically select its own APN). This configuration had to be issued in the form of a "PDP context", a set of configurations used by the modem to define bearer transport and credentials.
5. The modem would now register to the network but would not be able to send any data with timeouts occurring. By using the modem's serial again, a working set of commands was documented, mainly discovering that the `AT+CGACT` and `AT+CGATT` commands were needed to attach to the bearer network. At this point, both board code examples from Lilygo and SIMCOM (the modem's manufacturer) were to be considered only partially working due to the highly customized set of configurations applied to the modem upon boot.

    ```c
    // Example function which configures a network connection over Cat-M using the TinyGSM library
    bool configureNetwork(TinyGsm &modem, uint32_t timeout_ms) {
        SIM70xxRegStatus s;
        ESP_LOGI(TAG, "Network registration in progress");
        modem.sendAT("+CREG=1");
        if (modem.waitResponse(timeout_ms) != 1) {
            ESP_LOGE(TAG, "Network registration failure");
            return false;
        }

        do {
            s = modem.getRegistrationStatus();
            if (s != REG_OK_HOME && s != REG_OK_ROAMING) {
                delay(2000);
            }
        } while (s != REG_OK_HOME && s != REG_OK_ROAMING);
        ESP_LOGI(TAG, "Network register info: %s", register_info[s]);

        // Activate network bearer, APN can not be configured by default,
        // if the SIM card is locked, please configure the correct APN and user password, use the gprsConnect() method
        modem.sendAT("+CNACT=0,1");
        if (modem.waitResponse(timeout_ms) != 1) {
            ESP_LOGE(TAG, "Activate network bearer failed");
            return false;
        }

        // Activate PDP context
        modem.sendAT("+SNPDPID=0");
        if (modem.waitResponse(timeout_ms) != 1) {
            ESP_LOGE(TAG, "PDP context activation failed");
            return false;
        }

        return true;
    }
    ```

6. An MQTT client was implemented on top of the modem's internal MQTT 3.1.1 client, using AT commands over UART. This approach, although being the only possible one, was cumbersome and prone to error, requiring the modem's serial output to be parsed continuously for responses and errors. An existing Arduino library called PubSubClient was supposed to handle the procedure but was frequently running into internal timeouts or just plain refuse to parse message outputs so it was discarded for a more manual approach.

    ```c
    // Example of publishing messages using the TinyGSM library to issue AT commands to the modem
    esp_err_t Mqtt_Pub(TinyGsm &modem, const char *topic, const void *buf, size_t bufSize) {
        static char cmd_buf[512];
        esp_err_t status = ESP_OK;

        ESP_LOGD(TAG, "Publishing buffer of size %d on '%s'", bufSize, topic);

        snprintf(cmd_buf, sizeof(cmd_buf), "+SMPUB=\"%s\",%d,1,1", topic, bufSize);
        modem.sendAT(cmd_buf);
        if (modem.waitResponse(">") != 1) {
            ESP_LOGD(TAG, "Timeout while waiting for modem to read data");
            status = ESP_FAIL;
        }

        if (status == ESP_OK) {
            modem.stream.write((const char *)buf, bufSize);
            if (modem.waitResponse()) {
                ESP_LOGD(TAG, "Buffer of size %d published on '%s'", bufSize, topic);
            } else {
                ESP_LOGD(TAG, "Buffer of size %d failed to publish on '%s'", bufSize, topic);
                status = ESP_FAIL;
            }
        }

        return status;
    }
    ```

7. Although the implementation was generally reliable, it was not capable of:
   - doing MQTTS without first copying a textual certificate inside the modem's flash and a slew of other boilerplate
   - using MQTT 5 subscriptions
   Because of this, it was decided to port the existing code over to the Lilygo T-SIM7000G board, which bundled a modem "officially" supported by the `esp_modem` library developed by Espressif. At the same time, since it wasn't needed anymore, the Arduino framework was removed from the project's dependencies.
8. The new 7000G modem had several differences from the 7080G, which are documented in the previous sections of this document. To compensate for all the unknowns introduced by the hardware change, it was decided to first deploy some example code the development kit, [`pppos_example`](https://github.com/espressif/esp-protocols/tree/5964eadbf5591e8c12ffa5c724c7ace083423239/components/esp_modem/examples/pppos_client). The example code did not work, failing to register to the bearer network. Fortunately, the [`modem_console` example](https://github.com/espressif/esp-protocols/tree/5964eadbf5591e8c12ffa5c724c7ace083423239/components/esp_modem/examples/modem_console) could be used to poke around the modem and AT commands.
9.  It was observed that some commands used to configure the network on the 7000G accepted different parameters from the old modem. By adjusting the same set of commands, the 7000G would successfully register to the bearer network using 2G/EDGE. Some of the functions in the `esp_modem` library returned `ESP_ERR_TIMEOUT` because of underlying timeouts being way too strict. Relaxing the timeouts and using raw AT commands in some placed mostly solved the issue. Additionally, the library itself did not handle modem powerup or checks for existing connection status, which had to be implemented in firmware.

    ```c
    // Example of polling for network attachment state while ignoring timeouts
    int state = 0;
    esp_err_t err = ESP_OK;
    esp_modem_set_mode(modem, ESP_MODEM_MODE_COMMAND);
    vTaskDelay(pdMS_TO_TICKS(500));
    do {
        // This command may time out (takes more than 500ms) so it's fine to check a couple of times
        err = esp_modem_get_network_attachment_state(modem, &state);
        vTaskDelay(1);
    } while (err == ESP_ERR_TIMEOUT);
    if (state == 1) {
        ESP_LOGD(TAG, "Network already attached");
    }
    ```

10. The MQTT client was refactored to use Espressif's MQTT client for ESP32, which uses the modem in raw data mode by implementing an `esp-netif` interface. After successfully connecting to the (2G) network with a fresh Iliad SIM, the client would randomly timeout either during connection or after the first packet sent. After a lot of troubleshooting, it was discovered that sending data through the 2G network drew peaks of ~1A from the power circuit. This would sometimes brown out the modem at random, manifesting the former problems.
11. The SIM was replaced with the ThingsMobile one used on the old 7080G modem and the network was switched over to LTE Cat-M. It was discovered that network registration over LTE would sometimes fail because of the signal lock taking randomly too long before the firmware proceeded with network registration (which was the reason the Iliad SIM was tried). This was solved by polling RSSI repeatedly before attempting bearer registration.

    ```c
    int rssi = 99, ber;
    while (rssi >= 99) {
        esp_modem_get_signal_quality(modem, &rssi, &ber);
    }
    ```

12. At this point the network connection required a PPP connection to the transition over to raw TCP after receiving a static IP from the bearer. However, it was observed that the underlying network interface object fired the aptly named `IP_EVENT_PPP_GOT_IP` event only ~50% of the times after having a successful and verified PPP connection and receiving a valid IP from the network. The following code snippets implement the PPP connection timeout mechanism used to refresh the IP address in case the event isn't fired before a set timeout:

    ```c
    #define IP_TIMEOUT_INITIAL    pdMS_TO_TICKS(SEC_TO_MS(10))
    #define IP_TIMEOUT_FACTOR     (2)
    #define IP_TIMEOUT_FACTOR_MAX (6)

    static EventGroupHandle_t event_group = NULL;
    static const int GOT_IP_BIT = BIT0;

    static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        ESP_LOGD(TAG, "IP event! %" PRIu32, event_id);
        switch (event_id) {
        case IP_EVENT_PPP_GOT_IP: {
            xEventGroupSetBits(event_group, GOT_IP_BIT);
            break;
        }
        default:
            break;
        }
    }

    // Example function which implements the IP event timeout with exponential backoff
    void networkAttach() {
        ESP_ERROR_CHECK(esp_modem_set_network_attachment_state(modem, 1));
        vTaskDelay(pdMS_TO_TICKS(1000));

        esp_err_t err = esp_modem_set_mode(modem, ESP_MODEM_MODE_DATA);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_modem_set_mode(ESP_MODEM_MODE_DATA) failed with err %d", err);
            return err;
        }

        // Wait for IP address
        ESP_LOGD(TAG, "Waiting for IP address");
        EventBits_t bits;
        TickType_t timeout = IP_TIMEOUT_INITIAL;
        do {
            if (timeout > IP_TIMEOUT_INITIAL) {
                ESP_LOGD(TAG, "Timed out while waiting for IP address, reattaching to the network");
                esp_modem_set_mode(modem, ESP_MODEM_MODE_COMMAND);
                vTaskDelay(pdMS_TO_TICKS(1000));

                ESP_ERROR_CHECK(esp_modem_set_network_attachment_state(modem, 0));
                vTaskDelay(pdMS_TO_TICKS(500));

                ESP_ERROR_CHECK(esp_modem_set_network_attachment_state(modem, 1));
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            bits = xEventGroupWaitBits(event_group, GOT_IP_BIT, pdFALSE, pdFALSE, timeout);

            // Cap the timeout at 60 seconds, incrementing  it only if lower
            if (timeout < (IP_TIMEOUT_INITIAL * IP_TIMEOUT_FACTOR_MAX)) {
                timeout *= IP_TIMEOUT_FACTOR;
            }
        } while ((bits & GOT_IP_BIT) == 0);
    }
    ```

13. After successfully connecting to the network every time, a single issue remained: the MQTT client would not be able to connect to any server behind a DNS. After in-depth debugging, it was discovered that the DNS requests always timed out for unknown reasons. No solution was found in the alloted time for that specific problem, so the firmware explicitly does not support DNS queries at the time of writing.

### Experiments timeline - The 2G saga
After the initial Cat-M saga, the GPRS function was still too unreliable, presenting timeouts, IP failing to be assigned (the event was never fired) and general flakiness of the connection. For this reason, it was decided to try to run base hardware examples on a 2G connection.

1. All certificates for the server and a new root CA were created. They were then used to provision a fresh ATECC board. In doing this, it was discovered that the ATECC requires a root certificate, not bound to a CA or signed by any other certificate.
2. A basic example running on WiFi was made functional using the newly provisioned ATECC and a correctly configured instance of Mosquitto. The modified example code can be found in the examples directory.
3. An example taken from the esp-modem library was modified to connect to MQTT using the modem over 2G. It was observed that the modem was generally barely functional again.
4. AT commands used for initialization by the TinyGSM Arduino library were copied, resulting in the command set contained in the current code. Moreover, the GSM EDGE network is used. It was observed that using 2G resulted in near flawless connection stability, albeit sacrificing speed (taking into account the overhead of TLS).