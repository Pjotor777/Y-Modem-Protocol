# Y-Modem-Protocol
ENSC 351 YMODEM Protocol Project
This repository contains a multipart implementation of the YMODEM Batch file transfer protocol in C++, designed as part of the ENSC 351: Real-Time and Embedded Systems course at SFU. The project progresses through increasing levels of protocol complexity, medium behavior, and system realism. In addition, part 5: uartAxi contains the kernel module source code responsible for enabling UART communication on the Diligent Zedboard, allowing serial data transmission between the virtual and host system via the PMOD interface.

Overview
The project simulates the YMODEM protocol used for file transfer between two terminals over a potentially unreliable communication medium. It is split across several parts, each building upon the previous with added functionality and robustness:

## Part 1: Basic YMODEM Sender Simulation
Simulated transmission of file blocks using 128-byte data blocks and CRC-16.

Output is written to a file (ymodemSenderData.dat) instead of using a real serial connection.

Assumes perfect receiver behavior (always sends C, ACK, or NAK as expected).

Key file: SenderY.cpp

## Part 2: Bidirectional Communication and the "Kind" Medium
Introduces a simulated "kind medium" using POSIX socketpairs and multithreading.

A receiver (ReceiverY) is implemented to respond realistically to YMODEM blocks.

Sender and receiver interact via a simulated medium which may complement or alter certain bytes.

Core files: SenderY.cpp, ReceiverY.cpp, Medium.cpp, myIO.cpp

## Part 3: Glitch Handling and Simulated Drain
Introduces glitch byte injection by the medium.

Implements myTcdrain() and enhances myReadcond() to dump incoming glitches before the final byte of each block is sent.

Adds robustness in the presence of unexpected ACKs or data corruption.

## Part 4: Designing a State Chart
Designed a receiver state chart that takes care of all possibilities that can arise with the YMODEM protocol
Was a visual statechart, and thus excluded from the project as of now

## Part 5: Real Serial Device Integration (Zedboard)
Configures a Xilinx Zedboard for live communication over UART (via /dev/ttyS* devices).

Uses TeraTerm/Minicom to verify hardware-level UART output and signal integrity.

Integrates the kernel module uartAxi for bidirectional serial communication.

## Part 6: Full Protocol + "Evil" Medium
Final integration of full YMODEM statechart logic.

Handles medium behavior that includes:

Complementing, dropping, and injecting bytes.

Glitch byte sequences.

Extra unsolicited ACKs.

Implements command console and KVM thread to switch keyboard input between terminals and aggregate output.

Adds support for &s, &r, and &c commands at runtime.

## Key Concepts and Features
POSIX system programming (e.g., read(), write(), select(), socketpair()).

Multithreading with real-time scheduling priorities.

Custom I/O wrappers (myIO.cpp) for simulating QNX-like behavior on Linux.

YMODEM protocol structure, including block formats, handshakes, error checking (CRC16), and retransmissions.

Fault injection in the medium and error-handling in the sender/receiver logic.

## Credits
Developed as part of ENSC 351 – Real-Time and Embedded Systems, Fall 2024.
Instructor: Craig Scratchley

Zedboard setup and documentation contributions: Eton Kan, Maryamsadat RasouliDanesh

## License
This project is intended for academic use only. Unauthorized code sharing or copying is strictly prohibited as per SFU policy.
