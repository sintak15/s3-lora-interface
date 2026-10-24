# Heltec restarts during messaging — October 24, 2026

The user reported that sending two direct messages to unfamiliar nodes and one
public message during a walk appeared to restart the Heltec. A direct message to
the user's home node, step-base, succeeded. The user confirmed seeing the
Heltec's own startup/logo sequence, rather than only a CYD connection warning.

## Evidence recovered over Wi-Fi

The authenticated CYD `/status`, `/diagnostics/report` and `/sd/events` routes
were reachable. The persistent event log retained the walk after the devices
were subsequently power-cycled. Its relevant session contained the two direct
attempts, two attempts to step-base (including a retry), and the public attempt.
Each unfamiliar-node attempt and the public attempt was followed by a new
Heltec configuration/connection sequence. The two step-base attempts had no
such sequence between them. There were also several configuration/connection
sequences between send attempts; the problem was not confined to the instant
the send button was pressed. CYD GPS logging continued through those events,
without a new CYD boot marker during that walk session.

Independent Heltec TCP readback identified HELTEC_V3 running
`2.7.15.567b8ea`. All three direct-message destinations had public keys in its
node database. The unfamiliar nodes were recorded three and five hops away;
step-base was zero hops away. These are recorded observations, not a guarantee
of current reachability or a confirmed cause of the resets.

The Heltec HTTP `/json/report` route exposed its current uptime and cumulative
reboot counter. The CYD field report's `resetReason` describes the **CYD**;
it does not identify why the separate Heltec restarted. Detailed Heltec logging
was disabled. Inspection of the installed firmware's logging source showed
that `security.debug_log_api_enabled` sends logs through the main serial
console/Bluetooth paths, not the TCP API. A subsequent temporary syslog configuration enabled a UDP trace to this PC,
then restored the original syslog target.

## Controlled public test

The user explicitly approved an initial public message, then requested a
battery-powered comparison and one additional message. Three
`diagnostic test` messages were submitted through the CYD's existing `/send`
handler, using its existing channel and initial transmit settings. Each new request
required a separate user instruction; no additional chat tests were sent. No direct messages
were sent to the unfamiliar nodes. No automatic test retransmission was made
by the diagnostic script; normal Heltec mesh reliability behavior remained in
effect.

With the PC USB connection present, that message received a **mesh
acknowledgement**. Approximately 100 seconds of Wi-Fi observation showed the
Heltec reboot counter unchanged at 886 and the CYD uptime advancing normally.
The CYD recorded no new protobuf decode errors, framing timeouts or invalid
frame lengths. Reported Heltec configuration, excluding cache ages, and channel
summaries were unchanged. A mesh acknowledgement confirms a reported mesh
relay/implicit acknowledgement, not receipt or reading by every participant.

The first battery-powered message also received a mesh acknowledgement, with
no reboot during its approximately 100-second observation. Before the third
message, however, the Heltec's counter had advanced from 886 to 889. The user
confirmed those were unprompted restarts. The third message was acknowledged,
then the counter increased to 890 and 891 during observation. The CYD remained
running. The failure therefore was not confined to message-button presses.

## Syslog and reduced-power trial

Detailed logging was captured by temporarily configuring only the Heltec's
`network.rsyslog_server` target. The listener received 315 records. The trace
showed an automatic 94-byte node-information transmission starting, followed
by a new startup sequence without a recorded transmit completion. This
transmission did not require an operator-generated message. No panic/brownout
reason was captured, so the exact reset mechanism remains unproven.

The transmit-power setting was reduced from its configured 30 dBm to 17 dBm.
The installed SX126x driver clamps the original request to the chip's 22 dBm
maximum; the configured 30 dBm does not mean the Heltec V3 was producing 1 W.
Only `lora.tx_power` was changed. The trace then showed the same-size automatic
node-information packet and a relayed 50-byte packet completing transmission.
The deliberate configuration reboots were distinguished from unprompted ones.

The original syslog target was restored. Independent TCP readback confirmed
that network settings, security configuration, module settings and all channels
matched the pre-investigation capture; the sole retained configuration change
was 17 dBm transmit power. The Heltec firmware remained `2.7.15.567b8ea`.
A longer read-only Wi-Fi observation was started at reboot counter 898.
The user separately requested one public apology message for the diagnostic
traffic. That fourth message was sent at 17 dBm and received a mesh
acknowledgement. No extra chat messages were sent. The counter remained 898
through more than 25 minutes of follow-up, including automatic radio traffic.

Reducing power is a diagnostic mitigation and reduces the radio link budget.
Successful transmissions at lower power support a supply/RF-related failure;
they do not independently prove a brownout. The shared boosted supply, its
wiring/ground path, cell connections and antenna placement remain hardware
inspection targets before restoring maximum power.

## Battery indication correction

During the battery comparison, the CYD still reported charging despite the
user removing the PC cable and separate charger. CRATE remained positive after
a large cell-voltage drop. The old charging decision returned true for any
rate of at least +1%/hour before checking its voltage-drop condition. In
addition, a rising modeled SOC can reflect a rebound rather than charge current.

v0.1.98 was installed over authenticated Wi-Fi with stable external USB power.
Its SOC/voltage classifier remained too conservative while the user was actually
charging through the separate 5 V circuit. At the user's direction, v0.1.99
removes that classifier and percentage smoothing: SOC/VCELL come directly from
the MAX, and SOC trend comes solely from signed CRATE. It does not claim to
measure physical charger presence. Remaining time is a labeled software estimate.
v0.1.100 uses the user-requested Battery: Increasing/Decreasing/Steady wording.
The added SD recorder keeps readable voltage/connection/audio diagnostics and
optional raw-register CSV records. See [battery statistics](BATTERY_STATS.md).

## Orange LED and separate power measurements

The exact Heltec V3.2 schematic identifies U3 as LGS4056HDA. Orange LED1 is
connected from VDD_5V through R18 (330 ohms) to the charger's CHRG output. It is
not a firmware TX/reset LED. The charger and processor regulator share the 5 V
input path, so a common supply disturbance could affect both the LED and radio;
this possible correlation must not be dismissed, but was not directly measured.
The user's flashes continued while the reboot counter remained stable at 898,
so a flash alone cannot establish a reboot.

The Heltec onboard BAT/JST/charger circuit is separate from the main cell measured
by the CYD MAX17048 and charged by the external 5 V charging circuit. Its HTTP
battery/USB/charging fields cannot substitute for MAX cell readings. An unused
JST and onboard charger behavior are possible explanations for the LED/voltage
readings, pending a physical wiring check. Do not identify a specific reset
mechanism or charger fault from those fields alone.

Copper shielding may reduce radiated interference but cannot repair supply sag.
Insulate conductors/boards before and after applying a grounded shield, maintain
antenna clearances, and avoid enclosing a regulator or charger in heat-trapping
foil. A short, reliable shield ground matters at RF. Measure the actual boosted
5 V and regulated 3.3 V rails during transmit before restoring maximum power.

References: [Heltec V3.2 schematic](https://resource.heltec.cn/download/WiFi_LoRa_32_V3/WiFi_LoRa_32_V3.2_Schematic_Diagram.pdf),
[MAX17048 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17048-MAX17049.pdf),
[Espressif placement and antenna clearance](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/pcb-layout-design.html).

## Interpretation and follow-up

A successful individual send does not establish that the walking failure is
repaired. Power stability under RF load, operation away from the configured
Wi-Fi network, and routing/retry behavior remain possible factors. The current
evidence does not distinguish a brownout, watchdog reset or firmware panic.
The short chat packets use the ordinary text-message send path; no reboot or
configuration command is issued by those send functions.

Further assistant-generated messages require separate explicit authorization.
A Heltec console/panic trace or measurement of its supply rail during transmit
is needed to assign a definitive root cause; Wi-Fi syslog can capture the last
normal operation but may be lost during a reset.

Private event logs, chat history, node/configuration snapshots and test captures
are kept under the ignored `build/message-investigation-20261024/` directory.
They contain private data and must not be committed.

Source references for the installed firmware:
[message ingestion](https://github.com/meshtastic/firmware/blob/567b8ea1c2b2d100c24b0d6cbc437ec89fae0a56/src/mesh/PhoneAPI.cpp),
[message delivery](https://github.com/meshtastic/firmware/blob/567b8ea1c2b2d100c24b0d6cbc437ec89fae0a56/src/mesh/MeshService.cpp),
[logging paths](https://github.com/meshtastic/firmware/blob/567b8ea1c2b2d100c24b0d6cbc437ec89fae0a56/src/RedirectablePrint.cpp),
[serial console logging](https://github.com/meshtastic/firmware/blob/567b8ea1c2b2d100c24b0d6cbc437ec89fae0a56/src/SerialConsole.cpp),
[radio power clamp](https://github.com/meshtastic/firmware/blob/567b8ea1c2b2d100c24b0d6cbc437ec89fae0a56/src/mesh/SX126xInterface.cpp).

## Installation and validation — October 24, 2026

v0.1.100 was installed over authenticated Wi-Fi to partition `app1`. The radio
link returned responsive, SD was mounted, and the requested Battery: Increasing
wording matched a positive MAX CRATE reading. Reported CYD preferences, cached
Heltec configuration and channels matched their pre-update snapshots. The
pre-existing GPS-sharing setup requirement was preserved.

Firmware compilation, compile-time register/statistics checks, JavaScript
regressions and 390/1280 px browser checks passed. Live SD checks verified start,
duplicate-start idempotence, manual charge annotation, periodic 10-sample windows,
stop, append, downloads, decoded/raw register agreement and authentication/error
responses. A subsequent firmware restart retained the session, manual phase and
all previous log bytes, wrote a resume record with a new boot identifier, and
left recording enabled with zero actual SD/log-write failures. v0.1.99 had
reported two recording delays in its combined counter; v0.1.100 separates busy/
unavailable SD delays from actual write failures. No multi-hour physical charge/
discharge cycle or instrument measurement of high-frequency noise was performed.

A live 10-second window included 103 mV cell-voltage span and 24.35 mV sampled
standard deviation, with zero MAX read, UART or audio errors in that observation.
This is variation in reported gauge samples, not proof of EMI or measured
switching ripple. Direct Heltec readback confirmed its sole retained change
was 17 dBm TX power; other configuration, modules and channels were preserved.
The reboot counter remained 898 through more than 45 minutes of follow-up.

Installed application: `s3-lora-interface-battery-max-v0.1.100-20261024.bin`,
1,884,640 bytes; SHA-256
`fb94d1fb3dadd3e98ff46808e1971a5a89ed8b1e0c60425a6cf6242c389eb406`.
