/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

/*
 * COMMANDS:
 * ...Terminal:
 * terminal begin --> Starts Terminal session
 * terminal count --> Returns number of issued commands
 * terminal last --> Returns last command
 * terminal end --> Ends Terminal session
 * ...CPU:
 * reboot --> Reboots IRIS 2 CPU
 * fsw mode --> Returns Flight Software (FSW) mode
 *              0: flight mode / 1: simulation mode (simulator enabled)
 * fsw state --> Returns FSW state
 * fsw substate --> Returns FSW substate
 * ...Time:
 * uptime --> Returns elapsed seconds since boot
 * unixtime --> Returns actual UNIX time
 * date --> Returns actual date and time from IRIS 2 CLK or sets it
 *          Format for setting the date: YYYY/MM/DD HH:mm:ss
 * i2c rtc --> Returns actual date and time from RTC
 * ...Sensors:
 * i2c temp --> Returns temperature in tenths of deg C.
 * i2c baro --> Returns atmospheric pressure in hundredths of mbar and altitude in cm
 * i2c ina --> Returns the INA voltage in hundredths of V and currents in mA
 * ...Cameras:
 * camera x on --> Powers on and boots camera x (where x in [1,2,3,4])
 * camera x pic --> Takes a picture with camera x using default configuration
 * camera x video_start --> Starts recording video with camera x
 * camera x video_off --> Stops video recording with camera x
 * camera x send_cmd y --> Sends command y (do not include \n!!!) to camera x
 * camera x off --> Safely powers off camera x
 * ...Telemetry:
 * tm nor --> Returns current Telemetry Line to be saved in NOR memory
 * tm fram --> Returns current Telemetry Line to be saved in FRAM memory
 * ...Memory:
 * memory [status/read/dump/write/erase] [nor/fram]
 *  if status --> Returns the status of the NOR or FRAM memory.
 *  if read, add: [tlm/events] [OPTIONAL line_start] [OPTIONAL line_end] --> Reads num bytes starting at address.
 *  if dump, add: [line_start] [num bytes] [hex/bin] --> Reads all Telemetry Lines or Event Lines in HEX/CSV format.
 *  if write, add: [address] [num bytes] [data] --> Writes num bytes of data starting at address.
 *  if erase, add: [sector] [num sector] or [bulk] --> Erases a sector of data (0-255) or whole memory.
 *
 */

/*
 * FUTURE COMMANDS (TODO):
 * Acceleration X,Y,Z axes (i2c acc).
 * Camera 1,2,3,4 change resolution.
 * Camera 1,2,3,4 change frames per second.
 */

#include "terminal.h"

uint8_t beginFlag_ = 0;
char strToPrint_[100];

// Terminal session control data
uint8_t numIssuedCommands_ = 0;
char lastIssuedCommand_[CMD_MAX_LEN] = {0};
char commandHistory_[CMD_MAX_SAVE][CMD_MAX_LEN] = {0};
uint8_t cmdSelector_ = 0;

// PRIVATE FUNCTIONS
char subcommand_[CMD_MAX_LEN] = {0};
void extractSubcommand(uint8_t start, char * command);
void extractCommandPart(char * command, uint8_t desiredPart);
void addCommandToHistory(char * command);
void rebootReasonDecoded(uint16_t code, char * rebootReason);
//Composed (multi-part) commands have their own function
void processTerminalCommand(char * command);
void processI2CCommand(char * commmand);
void processCameraCommand(char * command);
void processMemoryCommand(char * command);

/**
 * DEPRECATED. Use extractCommandPart instead.
 */
void extractSubcommand(uint8_t start, char * command)
{
    uint8_t i;
    for (i = 0; i < CMD_MAX_LEN; i++)
        subcommand_[i] = 0;

    for (i = start; i < CMD_MAX_LEN; i++)
    {
        subcommand_[i-start] = command[i];
        if(command[i] == '\0')
            break;  //end of command detected
    }
}

/**
 * A parted command is separated into multiple parts by means of spaces.
 * This function returns a single desired part of a command.
 */
void extractCommandPart(char * command, uint8_t desiredPart)
{
    uint8_t i;

    // Wipe out contents of subcommand_
    for (i = 0; i < CMD_MAX_LEN; i++)
        subcommand_[i] = 0;

    // Variable to store the number of part that we are iterating
    uint8_t numPart = 0;
    uint8_t whereToWrite = 0;

    for (i = 0; i < CMD_MAX_LEN; i++)
    {
        if ((uint8_t) (numPart == desiredPart))
            subcommand_[i-whereToWrite] = command[i];

        // Update the part that we are iterating
        if (command[i] == ' ')
        {
            numPart++;
            whereToWrite = i + 1;
        }
        // End of command detected
        else if (command[i] == '\0')
            break;
    }
}

/**
 * Adds a sent command to the history of commands.
 * NOTE: The history of commands is wiped out with terminal off command.
 */
void addCommandToHistory(char * command)
{
    if (numIssuedCommands_ < CMD_MAX_SAVE)
    {
        uint8_t i;
        for (i = 0; i < CMD_MAX_LEN; i++)
            commandHistory_[numIssuedCommands_][i] = command[i];
    }
    else
    {
        uint8_t i,j;
        for (i = 0; i < CMD_MAX_SAVE-1; i++)
            for (j = 0; j < CMD_MAX_LEN; j++)
                commandHistory_[i][j] = commandHistory_[i+1][j];

        for (i = 0; i < CMD_MAX_LEN; i++)
            commandHistory_[CMD_MAX_SAVE-1][i] = command[i];
    }
}

/**
 * As described on page 78 of datasheet
 */
void rebootReasonDecoded(uint16_t code, char * rebootReason)
{
    switch (code)
    {
    case 0x00:
        strcpy(rebootReason, "Clean Start");
        break;
    case 0x02:
        strcpy(rebootReason, "Brownout (BOR)");
        break;
    case 0x04:
            strcpy(rebootReason, "RSTIFG RST/NMI (BOR)");
            break;
    case 0x06:
            strcpy(rebootReason, "PMMSWBOR software BOR (BOR)");
            break;
    case 0x0a:
            strcpy(rebootReason, "Security violation (BOR)");
            break;
    case 0x0e:
            strcpy(rebootReason, "SVSHIFG SVSH event (BOR)");
            break;
    case 0x14:
            strcpy(rebootReason, "PMMSWPOR software POR (POR)");
            break;
    case 0x16:
            strcpy(rebootReason, "WDTIFG WDT timeout (PUC)");
            break;
    case 0x18:
            strcpy(rebootReason, "WDTPW password violation (PUC)");
            break;
    case 0x1a:
            strcpy(rebootReason, "FRCTLPW password violation (PUC)");
            break;
    case 0x1c:
            strcpy(rebootReason, "Uncorrectable FRAM bit error detection (PUC)");
            break;
    case 0x1e:
            strcpy(rebootReason, "Peripheral area fetch (PUC)");
            break;
    case 0x20:
            strcpy(rebootReason, "PMMPW PMM password violation (PUC)");
            break;
    case 0x22:
            strcpy(rebootReason, "MPUPW MPU password violation (PUC)");
            break;
    case 0x24:
            strcpy(rebootReason, "CSPW CS password violation (PUC)");
            break;
    case 0x26:
            strcpy(rebootReason, "MPUSEGIPIFG encapsulated IP memory segment violation (PUC)");
            break;
    case 0x28:
            strcpy(rebootReason, "MPUSEGIIFG information memory segment violation (PUC)");
            break;
    case 0x2a:
            strcpy(rebootReason, "MPUSEG1IFG segment 1 memory violation (PUC)");
            break;
    case 0x2c:
            strcpy(rebootReason, "MPUSEG2IFG segment 2 memory violation (PUC)");
            break;
    case 0x2e:
            strcpy(rebootReason, "MPUSEG3IFG segment 3 memory violation (PUC)");
            break;
    default:
        strcpy(rebootReason, "Unknown");
        break;
    }
}

void processTerminalCommand(char * command)
{
    // Extract the subcommand from terminal command
    extractSubcommand(9, (char *) command);

    if (strcmp("count", (char *)subcommand_) == 0)
        sprintf(strToPrint_, "%d commands issued in this session.\r\n", numIssuedCommands_);
    else if (strcmp("last", (char *)subcommand_) == 0)
        sprintf(strToPrint_, "Last issued command: %s\r\n", lastIssuedCommand_);
    else if (strcmp("end", (char *)subcommand_) == 0)
    {
        beginFlag_ = 0;
        numIssuedCommands_ = 0;
        cmdSelector_ = 0;

        uint8_t i,j;
        for (i = 0; i < CMD_MAX_LEN; i++)
            lastIssuedCommand_[i] = 0;

        for (i = 0; i < CMD_MAX_SAVE-1; i++)
            for (j = 0; j < CMD_MAX_LEN; j++)
                commandHistory_[i][j] = 0;

        sprintf(strToPrint_, "Terminal session was ended by user.\r\n");
    }
    else
        sprintf(strToPrint_, "Terminal subcommand %s not recognised...\r\n", subcommand_);

    uart_print(UART_DEBUG, strToPrint_);
}

void processI2CCommand(char * command)
{
    // Extract the subcommand from I2C command
    extractSubcommand(4, (char *) command);

    if (strcmp("rtc", (char *) subcommand_) == 0)
    {
        struct RTCDateTime dateTime;
        int8_t error = i2c_RTC_getClockData(&dateTime);

        if (error != 0)
            sprintf(strToPrint_, "I2C RTC: Error code %d!\r\n", error);
        else
            sprintf(strToPrint_, "Date from I2C RTC is: 20%.2d/%.2d/%.2d %.2d:%.2d:%.2d\r\n",
                    dateTime.year,
                    dateTime.month,
                    dateTime.date,
                    dateTime.hours,
                    dateTime.minutes,
                    dateTime.seconds);
    }
    else if (strcmp("temp", (char*) subcommand_) == 0)
    {
        int16_t temperatures[6];
        int8_t error = i2c_TMP75_getTemperatures(temperatures);

        if (error != 0)
            sprintf(strToPrint_, "I2C TEMP: Error code %d!\r\n", error);
        else
            sprintf(strToPrint_, "I2C TEMP: %.2f deg C\r\n", temperatures[0]/10.0);
    }
    else if (strcmp("baro", (char *) subcommand_) == 0)
    {
        int32_t pressure, altitude;
        int8_t error = i2c_MS5611_getPressure(&pressure);
        if(error != 0)
            sprintf(strToPrint_, "I2C BARO: Error code %d!\r\n", error);
        else
        {
            altitude = calculateAltitude(pressure);
            sprintf(strToPrint_, "I2C BARO: Pressure: %.2f mbar, Altitude: %.2f m\r\n",
                    ((float)pressure/100.0),
                    ((float)altitude/100.0));
        }
    }
    else if (strcmp("ina", (char *) subcommand_) == 0)
    {
        struct INAData inaData;
        int8_t error = i2c_INA_read(&inaData);
        if(error != 0)
            sprintf(strToPrint_, "I2C INA: Error code %d!\r\n", error);
        else
            sprintf(strToPrint_, "I2C INA: %.2f V, %d mA\r\n",
                    ((float)inaData.voltage/100.0),
                    inaData.current);
    }
    else
        sprintf(strToPrint_, "Device or sensor %s not recognised in I2C devices list.\r\n", subcommand_);

    uart_print(UART_DEBUG, strToPrint_);
}

void processCameraCommand(char * command)
{
    // Process the selected camera
    uint8_t selectedCamera;
    switch (command[7])
    {
        case '1':
            selectedCamera = CAMERA01;
            break;

        case '2':
            selectedCamera = CAMERA02;
            break;

        case '3':
            selectedCamera = CAMERA03;
            break;

        case '4':
            selectedCamera = CAMERA04;
            break;

        default:
            sprintf(strToPrint_, "Error, no correct camera was selected...\r\n");
    }

    // Initialised UART comms with camera
    gopros_cameraInit(selectedCamera);

    // Extract the subcommand from camera control command
    extractSubcommand(9, (char *) command);

    if (strcmp("on", subcommand_) == 0)
    {
        gopros_cameraRawPowerOn(selectedCamera);
        sprintf(strToPrint_, "Camera %c booting...\r\n", command[7]);
    }
    else if (strcmp("pic", subcommand_) == 0)
    {
        gopros_cameraRawTakePicture(selectedCamera);
        sprintf(strToPrint_, "Camera %c took a picture.\r\n", command[7]);
    }
    else if (strcmp("video_start", subcommand_) == 0)
    {
        gopros_cameraRawStartRecordingVideo(selectedCamera);
        sprintf(strToPrint_, "Camera %c started recording video.\r\n", command[7]);
    }
    else if (strcmp("video_end", subcommand_) == 0)
    {
        gopros_cameraRawStopRecordingVideo(selectedCamera);
        sprintf(strToPrint_, "Camera %c stopped recording video.\r\n", command[7]);
    }
    else if (strncmp("send_cmd", subcommand_, 8) == 0)
    {
        uint8_t i;
        char goProCommand[CMD_MAX_LEN] = {0};
        char goProCommandNEOL[CMD_MAX_LEN] = {0};
        for (i = 9; i < CMD_MAX_LEN; i++)
        {
            if (subcommand_[i] != 0)
            {
                goProCommandNEOL[i-9] = subcommand_[i];
                goProCommand[i-9] = subcommand_[i];
            }
            else
            {
                goProCommand[i-9] = (char) 0x0A;
                break;
            }
        }

        gopros_cameraRawSendCommand(selectedCamera, goProCommand);
        sprintf(strToPrint_, "Command %s sent to camera %c.\r\n", goProCommandNEOL, command[7]);
    }
    else if (strcmp("off", subcommand_) == 0)
    {
        gopros_cameraRawSafePowerOff(selectedCamera);
        sprintf(strToPrint_, "Camera %c powered off.\r\n", command[7]);
    }
    else
        sprintf(strToPrint_, "Camera subcommand %s not recognised...\r\n", subcommand_);

    uart_print(UART_DEBUG, strToPrint_);
}

void processMemoryCommand(char * command)
{
    int8_t memorySubcommand;
    int8_t memoryType;

    // MEMORY SUBCOMMANDS: status/read/dump/write/erase
    extractCommandPart((char *) command, 1);

    // memory status [nor/fram]
    if (strncmp("status", (char *)subcommand_, 6) == 0)
        memorySubcommand = MEM_CMD_STATUS;
    // memory read [nor/fram] [OPTIONAL start_line] [OPTIONAL end_line]
    else if (strncmp("read", (char *)subcommand_, 4) == 0)
        memorySubcommand = MEM_CMD_READ;
    // memory dump [nor/fram] [address] [num_bytes] [hex/bin]
    else if (strncmp("dump", (char *)subcommand_, 4) == 0)
        memorySubcommand = MEM_CMD_DUMP;
    // memory write [nor/fram] [address] [num_bytes] [byte0 byte1 byte2...]
    else if (strncmp("write", (char *)subcommand_, 5) == 0)
        memorySubcommand = MEM_CMD_WRITE;
    // memory erase [nor/fram] [address] [num_bytes] OR [bulk]
    else if (strncmp("erase", (char *)subcommand_, 5) == 0)
        memorySubcommand = MEM_CMD_ERASE;
    else
    {
        memorySubcommand = -1;
        sprintf(strToPrint_, "Incorrect memory subcommand. Use: memory [status/read/dump/write/erase].\r\n");
        uart_print(UART_DEBUG, strToPrint_);
    }

    if (memorySubcommand != -1)
    {
        // Move on, EXTRACT MEMORY: nor or fram
        extractCommandPart((char *) command, 2);

        if (strncmp("nor", (char *)subcommand_, 3) == 0)
            memoryType = MEM_TYPE_NOR;
        else if (strncmp("fram", (char *)subcommand_, 4) == 0)
            memoryType = MEM_TYPE_FRAM;
        else
        {
            memoryType = -1;
            sprintf(strToPrint_, "Incorrect desired memory. Use: memory [status/read/dump/write/erase] [nor/fram].\r\n");
            uart_print(UART_DEBUG, strToPrint_);
        }

        //Move on, PROCESS each subcommand
        if (memoryType != -1)
        {
            /* * *
             * Memory Status
             * memory status [nor/fram]
             */
            if (memorySubcommand == MEM_CMD_STATUS)
            {
                //if (memoryType == MEM_TYPE_NOR)
                {
                    // Print NOR memory flag status
                    uint8_t busy = spi_NOR_checkWriteInProgress(confRegister_.nor_deviceSelected);
                    uart_print(UART_DEBUG, "NOR memory status: \r\n");
                    if (busy)
                        uart_print(UART_DEBUG, " * NOR memory is busy (write operation in progress)\r\n");
                    else
                        uart_print(UART_DEBUG, " * NOR memory is ready\r\n");

                    // Print NOR memory pointer status
                    uint32_t n_tlmlines = (confRegister_.nor_telemetryAddress - NOR_TLM_ADDRESS) / sizeof(struct TelemetryLine);
                    uint32_t n_tlmlinesTotal = NOR_TLM_SIZE / sizeof(struct TelemetryLine);
                    float percentage_used = (float)n_tlmlines * 100.0 / (float) n_tlmlinesTotal;
                    sprintf(strToPrint_, " * %ld saved telemetry lines. %.2f%% used. Last address is %ld\r\n",
                            n_tlmlines,
                            percentage_used,
                            confRegister_.nor_telemetryAddress);
                    uart_print(UART_DEBUG, strToPrint_);

                    uint32_t n_events = (confRegister_.nor_eventAddress - NOR_EVENTS_ADDRESS) / sizeof(struct EventLine);
                    uint32_t n_eventsTotal = NOR_EVENTS_SIZE / sizeof(struct EventLine);
                    percentage_used = (float)n_events * 100.0 / (float) n_eventsTotal;
                    sprintf(strToPrint_, " * %ld saved events. %.2f%% used. Last address is %ld\r\n",
                            n_events,
                            percentage_used,
                            confRegister_.nor_eventAddress);
                    uart_print(UART_DEBUG, strToPrint_);
                }
                //else if (memoryType == MEM_TYPE_FRAM)
                {
                    //Print FRAM memory status
                    uart_print(UART_DEBUG, "FRAM memory status: \r\n");
                    uart_print(UART_DEBUG, " * FRAM memory is ready\r\n");
                    uint16_t n_events = (confRegister_.fram_eventAddress - FRAM_EVENTS_ADDRESS) / sizeof(struct EventLine);
                    uint16_t n_eventsTotal = FRAM_EVENTS_SIZE / sizeof(struct EventLine);
                    float percentage_used = (float)n_events * 100.0 / (float) n_eventsTotal;
                    sprintf(strToPrint_, " * %d saved events. %.2f%% used. Last address is %ld\r\n",
                            n_events,
                            percentage_used,
                            confRegister_.fram_eventAddress);
                    uart_print(UART_DEBUG, strToPrint_);
                    n_events = (confRegister_.fram_telemetryAddress - FRAM_TLM_ADDRESS) / sizeof(struct TelemetryLine);
                    n_eventsTotal = FRAM_TLM_SIZE / sizeof(struct TelemetryLine);
                    percentage_used = (float)n_events * 100.0 / (float) n_eventsTotal;
                    sprintf(strToPrint_, " * %d saved telemetry. %.2f%% used. Last address is %ld\r\n",
                            n_events,
                            percentage_used,
                            confRegister_.fram_telemetryAddress);
                    uart_print(UART_DEBUG, strToPrint_);
                }
            }
            /* * *
             * Memory Read
             * memory read [nor/fram] [OPTIONAL start_line] [OPTIONAL end_line]
             */
            else if (memorySubcommand == MEM_CMD_READ)
            {
                uint8_t error = 0;
                uint8_t lineType = 0;
                uint32_t startAddress = 0;
                uint8_t lineSize = 0;
                int16_t lineStart = 0;
                int16_t lineEnd = 0;

                // READ TYPE OF LINE to read (telemetry/event)
                extractCommandPart((char *) command, 3);
                if (strncmp("tlm", (char *)subcommand_, 3) == 0)
                {
                    if (memoryType == MEM_TYPE_NOR)
                        startAddress = NOR_TLM_ADDRESS;
                    else if (memoryType == MEM_TYPE_FRAM)
                        startAddress = FRAM_TLM_ADDRESS;
                    lineType = MEM_LINE_TLM;
                    lineSize = sizeof(struct TelemetryLine);
                }
                else if (strncmp("event", (char *)subcommand_, 5) == 0)
                {
                    if (memoryType == MEM_TYPE_NOR)
                        startAddress = NOR_EVENTS_ADDRESS;
                    else if (memoryType == MEM_TYPE_FRAM)
                        startAddress = FRAM_EVENTS_ADDRESS;
                    lineType = MEM_LINE_EVENT;
                    lineSize = sizeof(struct EventLine);
                }
                else
                {
                    error--;
                    sprintf(strToPrint_, "Incorrect desired line. Use: memory read [nor/fram] [tlm/event].\r\n");
                    uart_print(UART_DEBUG, strToPrint_);
                }

                // READ MEMORY START, if specified
                extractCommandPart((char *) command, 4);
                if (subcommand_[0] != '\0')
                    lineStart = atoi(subcommand_);

                // READ MEMORY END, if specified
                extractCommandPart((char *) command, 5);
                // TODO: If memory end not specified, read until there are no more lines
                if (subcommand_[0] != '\0')
                    lineEnd = atoi(subcommand_);

                if (lineStart < 0)
                {
                    error--;
                    sprintf(strToPrint_, "ERROR: Beginning line must be a positive integer.\r\n");
                    uart_print(UART_DEBUG, strToPrint_);
                }

                if (lineEnd < 0)
                {
                    error--;
                    sprintf(strToPrint_, "ERROR: Ending line must be a positive integer.\r\n");
                    uart_print(UART_DEBUG, strToPrint_);
                }

                if (lineEnd != 0 && lineEnd < lineStart)
                {
                    error--;
                    sprintf(strToPrint_, "ERROR: Ending line must be bigger than beginning line.\r\n");
                    uart_print(UART_DEBUG, strToPrint_);
                }

                // No errors, proceed to command execution...
                if (error == 0)
                {
                    // Print line header in CSV format
                    if (lineType == MEM_LINE_TLM)
                    {
                        uart_print(UART_DEBUG, "address, date, unixtime, uptime, pressure, altitude,"
                                " verticalSpeedAVG, verticalSpeedMAX, verticalSpeedMIN,"
                                " temperatures0, temperatures1, temperatures2, temperatures3, temperatures4,"
                                " accXAxisAVG, accXAxisMAX, accXAxisMIN, accYAxisAVG, accYAxisMAX, accYAxisMIN,"
                                " accZAxisAVG, accZAxisMAX, accZAxisMIN, voltagesAVG, voltagesMAX, voltagesMIN,"
                                " currentsAVG, currentsMAX, currentsMIN, state, sub_state,"
                                " switches_status0, switches_status1, switches_status2, switches_status3,"
                                " switches_status4, switches_status5, switches_status6, switches_status7,"
                                " errors0, errors1, errors2, errors3, errors4, errors5, errors6, errors7, errors8\r\n");
                    }
                    else if (lineType == MEM_LINE_EVENT)
                    {
                        uart_print(UART_DEBUG, "address, date, unixtime,"
                                " uptime, state, sub_state, event, payload0,"
                                " payload1, payload2, payload3, payload4\r\n");
                    }

                    // Compute lines to be read
                    uint16_t linesToRead;
                    if (lineEnd == 0)
                        linesToRead = 1;
                    else
                        linesToRead = (lineEnd + 1) - lineStart;

                    // Read line by line
                    struct TelemetryLine readTelemetry;
                    struct EventLine readEvent;
                    uint8_t i;
                    for (i = lineStart; i < lineStart + linesToRead; i++)
                    {
                        //startAddress = startAddress + i * lineSize;

                        // Retrieve line
                        if (memoryType == MEM_TYPE_NOR)
                        {
                            if (lineType == MEM_LINE_TLM)
                                getTelemetryNOR(i, &readTelemetry);
                            else if (lineType == MEM_LINE_EVENT)
                                getEventNOR(i, &readEvent);
                        }
                        else if (memoryType == MEM_TYPE_FRAM)   //FRAM
                        {
                            if (lineType == MEM_LINE_TLM)
                                getTelemetryFRAM(i, &readTelemetry);
                            else if (lineType == MEM_LINE_EVENT)
                                getEventFRAM(i, &readEvent);
                        }

                        if (lineType == MEM_LINE_TLM)
                        {
                            struct RTCDateTime dateTime;
                            convert_from_unixTime(readTelemetry.unixTime, &dateTime);
                            sprintf(strToPrint_, "%d, 20%.2d/%.2d/%.2d %.2d:%.2d:%.2d,",
                                    i,
                                    dateTime.year,
                                    dateTime.month,
                                    dateTime.date,
                                    dateTime.hours,
                                    dateTime.minutes,
                                    dateTime.seconds);
                            uart_print(UART_DEBUG, strToPrint_);
                            sprintf(strToPrint_, " %ld, %ld, %ld, %ld, %d, %d,"
                                    " %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,"
                                    " %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,"
                                    " %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,"
                                    " %d, %d, %d, %d, %d, %d, %d\r\n",
                                    readTelemetry.unixTime,
                                    readTelemetry.upTime,
                                    readTelemetry.pressure,
                                    readTelemetry.altitude,
                                    readTelemetry.verticalSpeed[0],
                                    readTelemetry.verticalSpeed[1],
                                    readTelemetry.verticalSpeed[2],
                                    readTelemetry.temperatures[0],
                                    readTelemetry.temperatures[1],
                                    readTelemetry.temperatures[2],
                                    readTelemetry.temperatures[3],
                                    readTelemetry.temperatures[4],
                                    readTelemetry.accXAxis[0],
                                    readTelemetry.accXAxis[1],
                                    readTelemetry.accXAxis[2],
                                    readTelemetry.accYAxis[0],
                                    readTelemetry.accYAxis[1],
                                    readTelemetry.accYAxis[2],
                                    readTelemetry.accZAxis[0],
                                    readTelemetry.accZAxis[1],
                                    readTelemetry.accZAxis[2],
                                    readTelemetry.voltages[0],
                                    readTelemetry.voltages[1],
                                    readTelemetry.voltages[2],
                                    readTelemetry.currents[0],
                                    readTelemetry.currents[1],
                                    readTelemetry.currents[2],
                                    readTelemetry.state,
                                    readTelemetry.sub_state,
                                    readTelemetry.switches_status & 0x01,
                                    readTelemetry.switches_status & 0x02,
                                    readTelemetry.switches_status & 0x03,
                                    readTelemetry.switches_status & 0x04,
                                    readTelemetry.switches_status & 0x05,
                                    readTelemetry.switches_status & 0x06,
                                    readTelemetry.switches_status & 0x07,
                                    readTelemetry.switches_status & 0x08,
                                    readTelemetry.errors & 0x01,
                                    readTelemetry.errors & 0x02,
                                    readTelemetry.errors & 0x03,
                                    readTelemetry.errors & 0x04,
                                    readTelemetry.errors & 0x05,
                                    readTelemetry.errors & 0x06,
                                    readTelemetry.errors & 0x07,
                                    readTelemetry.errors & 0x08,
                                    readTelemetry.errors & 0x09);
                            uart_print(UART_DEBUG, strToPrint_);
                        }
                        else if (lineType == MEM_LINE_EVENT)
                        {
                            struct RTCDateTime dateTime;
                            convert_from_unixTime(readEvent.unixTime, &dateTime);
                            sprintf(strToPrint_, "%d, 20%.2d/%.2d/%.2d %.2d:%.2d:%.2d, ",
                                    i,
                                    dateTime.year,
                                    dateTime.month,
                                    dateTime.date,
                                    dateTime.hours,
                                    dateTime.minutes,
                                    dateTime.seconds);
                            uart_print(UART_DEBUG, strToPrint_);
                            sprintf(strToPrint_, "%ld, %ld, %d, %d, %d, %d, %d, %d, %d, %d\r\n",
                                       readEvent.unixTime,
                                       readEvent.upTime,
                                       readEvent.state,
                                       readEvent.sub_state,
                                       readEvent.event,
                                       readEvent.payload[0],
                                       readEvent.payload[1],
                                       readEvent.payload[2],
                                       readEvent.payload[3],
                                       readEvent.payload[4]);
                            uart_print(UART_DEBUG, strToPrint_);
                        }
                    }
                }

            }
            /* * *
             * Memory Dump
             * memory dump [nor/fram] [address] [num_bytes] [hex/bin]
             */
            else if (memorySubcommand == MEM_CMD_DUMP)
            {
                uint8_t error = 0;

                // EXTRACT READ ADDRESS
                uint32_t * readAddress;
                extractCommandPart((char *) command, 3);
                if (subcommand_ != '\0')
                    readAddress = (uint32_t *) atoi(subcommand_);
                else
                {
                    sprintf(strToPrint_, "Please specify the reading address. Use: memory dump [nor/fram] [address].");
                    uart_print(UART_DEBUG, strToPrint_);
                    error--;
                }

                // EXTRACT NUMBER OF BYTES to read
                uint16_t numBytes = 0;
                extractCommandPart((char *) command, 4);
                if (subcommand_ != '\0')
                    numBytes = atoi(subcommand_);
                else
                {
                    sprintf(strToPrint_, "Please specify the number of bytes to read. Use: memory dump [nor/fram] [address] [num_bytes].");
                    uart_print(UART_DEBUG, strToPrint_);
                    error--;
                }

                // EXTRACT OUTPUT FORMAT (hex/bin)
                uint8_t outputFormat;
                extractCommandPart((char *) command, 4);
                if (subcommand_ != '\0')
                {
                    if (strncmp("hex", (char*)subcommand_, 3) == 0)
                        outputFormat = MEM_OUTFORMAT_HEX;
                    else if (strncmp("bin", (char*)subcommand_, 3) == 0)
                        outputFormat = MEM_OUTFORMAT_BIN;
                }
                else
                {
                    sprintf(strToPrint_, "Please specify the output format. Use: memory dump [nor/fram] [address] [num_bytes] [hex/bin].");
                    uart_print(UART_DEBUG, strToPrint_);
                    error--;
                }

                // No errors, proceed to serve command
                if (error == 0)
                {
                    uint8_t byteRead;

                    uint8_t i;
                    for (i = 0; i < numBytes; i++)
                    {
                        // Depending on memory type, we read one way or the other
                        if (memoryType == MEM_TYPE_NOR)
                            spi_NOR_readFromAddress(*readAddress, &byteRead, 1, confRegister_.nor_deviceSelected);
                        else if (memoryType == MEM_TYPE_FRAM)
                            byteRead = *readAddress;

                        // Take into account output format
                        if (outputFormat == MEM_OUTFORMAT_HEX)
                            sprintf(strToPrint_, "%X ", byteRead);
                        // TODO: LSB or MSB? Reverse order?
                        else if (outputFormat == MEM_OUTFORMAT_BIN)
                            sprintf(strToPrint_, "%d%d%d%d%d%d%d%d",
                                    byteRead & 0x00,
                                    byteRead & 0x01,
                                    byteRead & 0x02,
                                    byteRead & 0x03,
                                    byteRead & 0x04,
                                    byteRead & 0x05,
                                    byteRead & 0x06,
                                    byteRead & 0x07);

                        uart_print(UART_DEBUG, strToPrint_);
                    }

                    uart_print(UART_DEBUG, "\r\n");
                }
            }
            /* * *
             * Memory Write
             * memory write [nor/fram] [address] [num_bytes] [byte0 byte1 byte2...]
             */
            else if (memorySubcommand == MEM_CMD_WRITE)
            {
                uint8_t error = 0;

                // EXTRACT WRITE ADDRESS
                uint32_t * writeAddress;
                extractCommandPart((char *) command, 3);
                if (subcommand_ != '\0')
                    writeAddress = (uint32_t *) atoi(subcommand_);
                else
                {
                    sprintf(strToPrint_, "Please specify the writing address. Use: memory write [nor/fram] [address].");
                    uart_print(UART_DEBUG, strToPrint_);
                    error--;
                }

                // EXTRACT NUMBER OF BYTES to write
                uint16_t numBytes = 0;
                extractCommandPart((char *) command, 4);
                if (subcommand_ != '\0')
                    numBytes = atoi(subcommand_);
                else
                {
                    sprintf(strToPrint_, "Please specify the number of bytes to write. Use: memory write [nor/fram] [address] [num_bytes].");
                    uart_print(UART_DEBUG, strToPrint_);
                    error--;
                }

                int8_t i;
                for (i = 0; i < numBytes; i++)
                {
                    // Extract i'th byte to write
                    extractCommandPart((char *) command, 5+i);
                    if (subcommand_ != '\0')
                    {
                        // Depending on memory type, we write one way or the other
                        if (memoryType == MEM_TYPE_NOR)
                            spi_NOR_writeToAddress(*writeAddress, (uint8_t*) &subcommand_, 1, confRegister_.nor_deviceSelected);
                        else if (memoryType == MEM_TYPE_FRAM)
                            *writeAddress = *subcommand_;
                    }
                    else
                        break; // We are done here...
                }
            }
            /* * *
             * Memory Erase
             * memory erase [nor/fram] [address] [num_bytes] OR [bulk]
             */
            else if (memorySubcommand == MEM_CMD_ERASE)
            {
                //TODO: Implement FRAM erasing... or not?

                uint8_t error = 0;

                extractCommandPart((char *) command, 3);

                // Check whether bulk erase is requested or if
                // user wants to erase a certain amount of bytes
                if (strncmp("sector", (char *)subcommand_, 6) == 0)
                {
                    // Extract erase sector
                    uint8_t sectorToErase;
                    extractCommandPart((char *) command, 4);
                    if (subcommand_ != '\0')
                        sectorToErase = atoi(subcommand_);
                    else
                    {
                        sprintf(strToPrint_, "Please specify the erasing sector. Use: memory erase [nor/fram] sector [num sector]\r\n.");
                        uart_print(UART_DEBUG, strToPrint_);
                        error--;
                    }

                    if (sectorToErase > 255)
                    {
                        sprintf(strToPrint_, "ERROR: NOR sector number must be between 0 and 255 (included).\r\n.");
                        uart_print(UART_DEBUG, strToPrint_);
                        error--;
                    }

                    // No errors processing command, move on
                    if (error == 0)
                    {
                        // Sector erase
                        int16_t eraseStart = seconds_uptime();
                        sprintf(strToPrint_, "Started erasing sector %d of NOR memory...\r\n", sectorToErase);
                        uart_print(UART_DEBUG, strToPrint_);

                        uint32_t sectorLength = (uint32_t) 0x3FFFF;
                        uint32_t sectorAddress = 0x00 + sectorToErase * (sectorLength+1);
                        spi_NOR_sectorErase(sectorAddress, confRegister_.nor_deviceSelected);

                        int16_t eraseEnd = seconds_uptime();
                        sprintf(strToPrint_, "NOR sector %d has been completely wiped out in %d seconds.\r\n", sectorToErase, eraseEnd - eraseStart);
                        uart_print(UART_DEBUG, strToPrint_);
                    }

                }
                else if (strncmp("bulk", (char *)subcommand_, 4) == 0)
                {
                    // Bulk erase the memory
                    int16_t eraseStart = seconds_uptime();
                    uart_print(UART_DEBUG, "Started bulk-erasing NOR memory...\r\n");

                    spi_NOR_bulkErase(confRegister_.nor_deviceSelected);

                    int16_t eraseEnd = seconds_uptime();
                    sprintf(strToPrint_, "NOR memory has been completely wiped out in %d seconds.\r\n", eraseEnd - eraseStart);
                    uart_print(UART_DEBUG, strToPrint_);
                }
                else
                {
                    sprintf(strToPrint_, "Available erase options are sector or bulk erase. Use: memory erase [nor/fram] [sector/bulk].\r\n");
                    uart_print(UART_DEBUG, strToPrint_);
                }
            }
        }
    }
}

/**
 * Starts a terminal session. From now on, incoming commands are considerd.
 * It also prints out the credits of the IRIS 2 instrument and FSW.
 */
int8_t terminal_start(void)
{
    beginFlag_ = 1;

    uint32_t uptime = seconds_uptime();

    uart_print(UART_DEBUG, "IRIS 2 (Image Recorder Instrument for Sunrise) terminal.\r\n");
    sprintf(strToPrint_, "IRIS 2 Firmware version is '%d', compiled on %s at %s.\r\n", FWVERSION, __DATE__, __TIME__);
    uart_print(UART_DEBUG, strToPrint_);
    uart_print(UART_DEBUG, "Made with passion and hard work by:\r\n");
    uart_print(UART_DEBUG, "  * Aitor Conde <aitorconde@gmail.com>. Senior Engineer. Electronics. Software.\r\n");
    uart_print(UART_DEBUG, "  * Ramon Garcia <ramongarciaalarcia@gmail.com>. Project Management. Software.\r\n");
    uart_print(UART_DEBUG, "  * David Mayo <mayo@sondasespaciales.com>. Electronics.\r\n");
    uart_print(UART_DEBUG, "  * Miguel Angel Gomez <haplo@sondasespaciales.com>. Structure.\r\n");
    sprintf(strToPrint_, "IRIS 2 last booted %ld seconds ago.\r\n", uptime);
    uart_print(UART_DEBUG, strToPrint_);
    sprintf(strToPrint_, "Reboot counter is %d.\r\n", confRegister_.numberReboots);
    uart_print(UART_DEBUG, strToPrint_);
    char rebootReason[70];
    rebootReasonDecoded(confRegister_.hardwareRebootReason, rebootReason);
    sprintf(strToPrint_, "Last Reboot reason was 0x%02X, '%s'.\r\n", confRegister_.hardwareRebootReason, rebootReason);
    uart_print(UART_DEBUG, strToPrint_);
    uart_print(UART_DEBUG,"IRIS:/# ");

    return 0;
}

// PUBLIC FUNCTIONS

uint8_t bufferSizeNow_ = 0;
uint8_t bufferSizeTotal_ = 0;
uint8_t command_[CMD_MAX_LEN] = {0};

/**
 * Reads if there is any awaiting command in the UART DEBUG buffer.
 * If so, processes it and acts accordingly.
 */
int8_t terminal_readAndProcessCommands(void)
{
    uint8_t commandArrived = 0;

    // If there is something in the buffer
    bufferSizeNow_ = uart_available(UART_DEBUG);
    if (bufferSizeNow_ > 0)
    {
        // Let's retrieve char by char
        uint8_t charRead = uart_read(UART_DEBUG);

        // Filter out for only ASCII chars
        if ((uint8_t) charRead >= 32 && (uint8_t) charRead < 127)
        {
            // Build command
            command_[bufferSizeTotal_] = charRead;
            bufferSizeTotal_++;
            uart_write(UART_DEBUG, &charRead, 1); //print echo
        }
        else if(charRead == 0x08 || charRead == 127)   //Backspace!!
        {
            //Delete the last character
            if(bufferSizeTotal_ != 0)
            {
                bufferSizeTotal_--;
                uart_write(UART_DEBUG, &charRead, 1); //print echo
            }
        }
        else if(charRead == '\n' || charRead == '\r')
        {
            commandArrived = 1;
            command_[bufferSizeTotal_] = '\0';  //End of string
            strToPrint_[0] = '\r';
            strToPrint_[1] = '\n';
            strToPrint_[2] = '\0';
            uart_print(UART_DEBUG, strToPrint_); //print echo
        }
    }

    //detecting keystrokes from the arrows:
    if(bufferSizeTotal_ >= 2 &&
            command_[bufferSizeTotal_ - 2] == '[')
    {
        if(command_[bufferSizeTotal_ - 1] == 'A')
        {
            //Up!
            //Clean line:
            strToPrint_[0] = '\r';
            strToPrint_[1] = '\0';
            uart_print(UART_DEBUG, strToPrint_); //put cursor on start
            uart_print(UART_DEBUG, "                              "); //Clean
            uart_print(UART_DEBUG, "                              "); //Clean
            uart_print(UART_DEBUG, strToPrint_); //put cursor on start
            //print terminal:
            uart_print(UART_DEBUG,"IRIS:/# ");
            //print selected command:
            sprintf(strToPrint_, "%s", commandHistory_[cmdSelector_]);
            uart_print(UART_DEBUG, strToPrint_);
            //Copy the command:
            strcpy((char *)command_, commandHistory_[cmdSelector_]);
            bufferSizeTotal_ = strlen((char *)command_);

            if (cmdSelector_ > 0)
                cmdSelector_--;
        }
        else if(command_[bufferSizeTotal_ - 1] == 'B')
        {
            if (cmdSelector_ < numIssuedCommands_-1 && cmdSelector_ < CMD_MAX_SAVE-1)
                cmdSelector_++;

            //Down!
            //Clean line:
            strToPrint_[0] = '\r';
            strToPrint_[1] = '\0';
            uart_print(UART_DEBUG, strToPrint_); //put cursor on start
            uart_print(UART_DEBUG, "                              "); //Clean
            uart_print(UART_DEBUG, "                              "); //Clean
            uart_print(UART_DEBUG, strToPrint_); //put cursor on start
            //print terminal:
            uart_print(UART_DEBUG,"IRIS:/# ");
            //print selected command:
            sprintf(strToPrint_, "%s", commandHistory_[cmdSelector_]);
            uart_print(UART_DEBUG, strToPrint_);
            //Copy the command:
            strcpy((char *)command_, commandHistory_[cmdSelector_]);
            bufferSizeTotal_ = strlen((char *)command_);
        }
        //Ignore any others for the moment
    }

    // If a command arrived and was composed
    if (commandArrived && beginFlag_ == 1)
    {
        // Interpret command
        if (command_[0] == 0)
        {
            //Empty command, nothing to do, print again the commandline
        }
        else if (strncmp("terminal", (char *)command_, 8) == 0)
        {
            processTerminalCommand((char *) command_);
        }
        else if (strcmp("reboot", (char *)command_) == 0)
        {
            strcpy(strToPrint_, "System will reboot...\r\n");
            uart_print(UART_DEBUG, strToPrint_);
            sleep_ms(500);
            //Perform a PUC reboot
            WDTCTL = 0xDEAD;
        }
        else if (strcmp("uptime", (char *)command_) == 0)
        {
            uint32_t uptime = seconds_uptime();
            sprintf(strToPrint_, "Uptime is %ld s\r\n", uptime);
            uart_print(UART_DEBUG, strToPrint_);
        }
        else if (strcmp("unixtime", (char *)command_) == 0)
        {
            uint32_t unixtTimeNow = i2c_RTC_unixTime_now();
            sprintf(strToPrint_, "%ld\r\n", unixtTimeNow);
            uart_print(UART_DEBUG, strToPrint_);
        }
        else if (strcmp("date", (char *)command_) == 0)
        {
            uint32_t unixtTimeNow = i2c_RTC_unixTime_now();
            struct RTCDateTime dateTime;
            convert_from_unixTime(unixtTimeNow, &dateTime);
            sprintf(strToPrint_, "Date is: 20%.2d/%.2d/%.2d %.2d:%.2d:%.2d\r\n",
                    dateTime.year,
                    dateTime.month,
                    dateTime.date,
                    dateTime.hours,
                    dateTime.minutes,
                    dateTime.seconds);
            uart_print(UART_DEBUG, strToPrint_);
        }
        else if (strncmp("date", (char *)command_, 4) == 0)
        {
            //date YYYY/MM/DD HH:mm:ss
            if(strlen((char *)command_) != 24
                    || command_[9] != '/'
                    || command_[12] != '/'
                    || command_[15] != ' '
                    || command_[18] != ':'
                    || command_[21] != ':')
                strcpy(strToPrint_, "Incorrect command, usage is: date YYYY/MM/DD HH:mm:ss\r\n");
            else
            {
                struct RTCDateTime dateTime;
                uint8_t *pointer;
                pointer = &command_[7];
                command_[9] = '\0';     //End of word
                command_[12] = '\0';     //End of word
                command_[15] = '\0';     //End of word
                command_[18] = '\0';     //End of word
                command_[21] = '\0';     //End of word
                dateTime.year = atoi((char *)pointer);
                pointer = &command_[10];
                dateTime.month = atoi((char *)pointer);
                pointer = &command_[13];
                dateTime.date = atoi((char *)pointer);
                pointer = &command_[16];
                dateTime.hours = atoi((char *)pointer);
                pointer = &command_[19];
                dateTime.minutes = atoi((char *)pointer);
                pointer = &command_[22];
                dateTime.seconds = atoi((char *)pointer);
                uint32_t unixTime = convert_to_unixTime(dateTime);
                i2c_RTC_set_unixTime(unixTime);

                sprintf(strToPrint_, "Date changed to: 20%.2d/%.2d/%.2d %.2d:%.2d:%.2d\r\n",
                                    dateTime.year,
                                    dateTime.month,
                                    dateTime.date,
                                    dateTime.hours,
                                    dateTime.minutes,
                                    dateTime.seconds);
            }
            uart_print(UART_DEBUG, strToPrint_);
        }
        // This is an FSW command
        else if (strncmp("fsw", (char *) command_, 3) == 0)
        {
            // Extract the subcommand from FSW command
            extractSubcommand(4, (char *) command_);

            if (strcmp("mode", (char *) subcommand_) == 0)
                sprintf(strToPrint_, "FSW mode: %d\r\n", confRegister_.simulatorEnabled);
            else if (strcmp("state", (char *) subcommand_) == 0)
                sprintf(strToPrint_, "FSW state: %d\r\n", confRegister_.flightState);
            else if (strcmp("substate", (char *) subcommand_) == 0)
                sprintf(strToPrint_, "FSW substate: %d\r\n", confRegister_.flightSubState);
            else
                sprintf(strToPrint_, "FSW subcommand %s not recognised...\r\n", subcommand_);

            uart_print(UART_DEBUG, strToPrint_);
        }
        // This is an I2C command
        else if(strncmp("i2c", (char *) command_, 3) == 0)
        {
            processI2CCommand((char *) command_);
        }
        else if (strncmp("camera", (char *)command_, 6) == 0)
        {
            processCameraCommand((char *) command_);
        }
        // This is a telemetry command
        else if (strncmp("tm", (char *)command_, 2) == 0)
        {
            struct TelemetryLine tmLines[2];
            returnCurrentTMLines(tmLines);

            // Extract the subcommand from telemetry command
            extractSubcommand(3, (char *) command_);

            uint8_t printResults = 1;

            struct TelemetryLine askedTMLine;
            if (strcmp("nor", (char *)subcommand_) == 0)
                askedTMLine = tmLines[0];
            else if (strcmp("fram", (char *)subcommand_) == 0)
                askedTMLine = tmLines[1];
            else
            {
                printResults = 0;
                sprintf(strToPrint_, "%s is an invalid memory for telemetry command.\r\n", subcommand_);
                uart_print(UART_DEBUG, strToPrint_);
            }

            if (printResults)
            {
                sprintf(strToPrint_, "Unix Time: %ld\r\n", askedTMLine.unixTime);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Up Time: %ld\r\n", askedTMLine.upTime);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Pressure: %ld\r\n", askedTMLine.pressure);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Altitude: %ld\r\n", askedTMLine.altitude);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Vertical Speed AVG: %d\r\n", askedTMLine.verticalSpeed[0]);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Vertical Speed MAX: %d\r\n", askedTMLine.verticalSpeed[1]);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Vertical Speed MIN: %d\r\n", askedTMLine.verticalSpeed[2]);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Temperature PCB: %d\r\n", askedTMLine.temperatures[0]);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Voltage AVG: %d\r\n", askedTMLine.voltages[0]);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Voltage MAX: %d\r\n", askedTMLine.voltages[1]);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Voltage MIN: %d\r\n", askedTMLine.voltages[2]);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Current AVG: %d\r\n", askedTMLine.currents[0]);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Current MAX: %d\r\n", askedTMLine.currents[1]);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Current MIN: %d\r\n", askedTMLine.currents[2]);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Flight state: %d\r\n", askedTMLine.state);
                uart_print(UART_DEBUG, strToPrint_);
                sprintf(strToPrint_, "Flight substate: %d\r\n", askedTMLine.sub_state);
                uart_print(UART_DEBUG, strToPrint_);
            }
        }
        else if (strncmp("memory", (char *)command_, 6) == 0)
        {
            processMemoryCommand((char *) command_);
        }
        else
        {
            sprintf(strToPrint_, "Command %s is not recognised.\r\n", (char *)command_);
            uart_print(UART_DEBUG, strToPrint_);
        }

        if (beginFlag_ != 0)
        {
            // Add command to CMD history
            if(command_[0] != 0)
            {
                addCommandToHistory((char *) command_);
                if (numIssuedCommands_ < CMD_MAX_SAVE)
                    cmdSelector_ = numIssuedCommands_;
                else
                    cmdSelector_ = CMD_MAX_SAVE - 1;

                numIssuedCommands_++;
                uint8_t i;
                for (i = 0; i < CMD_MAX_LEN; i++)
                    lastIssuedCommand_[i] = command_[i];
            }
            uart_print(UART_DEBUG,"IRIS:/# ");
        }

        bufferSizeNow_ = 0;
        bufferSizeTotal_ = 0;
    }
    else if(commandArrived && beginFlag_ == 0)
    {
        if(strcmp("terminal begin", (char *)command_) == 0)
            terminal_start();
        else
        {
            strcpy(strToPrint_, "Incorrect password...\r\n");
            uart_print(UART_DEBUG, strToPrint_);
        }

        bufferSizeNow_ = 0;
        bufferSizeTotal_ = 0;
    }

    return 0;
}
