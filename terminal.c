/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

/*
 * COMMANDS:
 * help --> Returns this list of commands
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
 * conf --> If no parameter is passed, returns actual FSW configuration
 *      If [get] is passed, returns only desired parameter
 *      If [set] is passed, sets value to desired parameter
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
 *  if read, add: [tlm/event] [OPTIONAL line_start] [OPTIONAL line_end] --> Reads num bytes starting at address.
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
char strToPrint_[200];

// Terminal session control data
uint8_t numIssuedCommands_ = 0;
char lastIssuedCommand_[CMD_MAX_LEN] = {0};
char commandHistory_[CMD_MAX_SAVE][CMD_MAX_LEN] = {0};
uint8_t cmdSelector_ = 0;

// PRIVATE FUNCTIONS
//char subcommand_[CMD_MAX_LEN] = {0};
void extractCommandPart(char * command, uint8_t desiredPart, char * commandPart);
void addCommandToHistory(char * command);
void rebootReasonDecoded(uint16_t code, char * rebootReason);
//Composed (multi-part) commands have their own function
void processTerminalCommand(char * command);
void processFSWCommand(char * command);
void processConfCommand(char * command);
void processI2CCommand(char * command);
void processCameraCommand(char * command);
void processTMCommand(char * command);
void processMemoryCommand(char * command);

/**
 * A parted command is separated into multiple parts by means of spaces.
 * This function returns a single desired part of a command.
 */
void extractCommandPart(char * command, uint8_t desiredPart, char * commandPart)
{
    uint8_t i;

    // Wipe out contents of commandPart
    //for (i = 0; i < CMD_MAX_LEN; i++)
    //    commandPart[i] = 0;

    commandPart[0] = '\0';

    // Variable to store the number of part that we are iterating
    uint8_t numPart = 0;
    uint8_t whereToWrite = 0;

    for (i = 0; i < CMD_MAX_LEN; i++)
    {
        if ((uint8_t) (numPart == desiredPart))
            commandPart[i-whereToWrite] = command[i];

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
    char terminalSubcommand[CMD_MAX_LEN] = {0};
    extractCommandPart((char *) command, 1, (char *) terminalSubcommand);

    if (strcmp("count", (char *)terminalSubcommand) == 0)
        sprintf(strToPrint_, "%d commands issued in this session.\r\n", numIssuedCommands_);
    else if (strcmp("last", (char *)terminalSubcommand) == 0)
        sprintf(strToPrint_, "Last issued command: %s\r\n", lastIssuedCommand_);
    else if (strcmp("end", (char *)terminalSubcommand) == 0)
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
        sprintf(strToPrint_, "Terminal subcommand %s not recognised...\r\n", terminalSubcommand);

    uart_print(UART_DEBUG, strToPrint_);
}

/**
 * TODO please complete
 */
void processFSWCommand(char * command)
{
    // Extract the subcommand from FSW command
    char fswSubcommand[CMD_MAX_LEN] = {0};
    extractCommandPart((char *) command, 1, (char *) fswSubcommand);

    if (strcmp("mode", (char *)fswSubcommand) == 0)
        sprintf(strToPrint_, "FSW mode: %d\r\n", confRegister_.simulatorEnabled);
    else if (strcmp("state", (char *)fswSubcommand) == 0)
        sprintf(strToPrint_, "FSW state: %d\r\n", confRegister_.flightState);
    else if (strcmp("substate", (char *)fswSubcommand) == 0)
        sprintf(strToPrint_, "FSW substate: %d\r\n", confRegister_.flightSubState);
    else
        sprintf(strToPrint_, "FSW subcommand %s not recognised...\r\n", fswSubcommand);

    uart_print(UART_DEBUG, strToPrint_);
}

/**
 * TODO please complete
 */
void processConfCommand(char * command)
{
    //Extract the subcommand (get/set) from conf command
    char confSubcommand[CMD_MAX_LEN] = {0};
    extractCommandPart((char *) command, 1, (char *) confSubcommand);
    int8_t typeOfOperation = 0;

    if (confSubcommand[0] == '\0')
    {
        typeOfOperation = 0;
        sprintf(strToPrint_, "magicWord = %X\r\n", confRegister_.magicWord);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "swVersion = %d\r\n", confRegister_.swVersion);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "simulatorEnabled = %d\r\n", confRegister_.simulatorEnabled);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "flightState = %d\r\n", confRegister_.flightState);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "flightSubState = %d\r\n", confRegister_.flightSubState);
        uart_print(UART_DEBUG, strToPrint_);

        uart_print(UART_DEBUG, "\r\n");

        sprintf(strToPrint_, "fram_telemetryAddress = %ld\r\n", confRegister_.fram_telemetryAddress);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "fram_eventAddress = %ld\r\n", confRegister_.fram_eventAddress);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "fram_tlmSavePeriod = %d\r\n", confRegister_.fram_tlmSavePeriod);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "nor_deviceSelected = %d\r\n", confRegister_.nor_deviceSelected);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "nor_telemetryAddress = %ld\r\n", confRegister_.nor_telemetryAddress);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "nor_eventAddress = %ld\r\n", confRegister_.nor_eventAddress);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "nor_tlmSavePeriod = %d\r\n", confRegister_.nor_tlmSavePeriod);
        uart_print(UART_DEBUG, strToPrint_);

        uart_print(UART_DEBUG, "\r\n");

        sprintf(strToPrint_, "baro_readPeriod = %d\r\n", confRegister_.baro_readPeriod);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "ina_readPeriod = %d\r\n", confRegister_.ina_readPeriod);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "acc_readPeriod = %d\r\n", confRegister_.acc_readPeriod);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "temp_readPeriod = %d\r\n", confRegister_.temp_readPeriod);
        uart_print(UART_DEBUG, strToPrint_);
    }
    else if (strncmp("get", (char *)confSubcommand, 3) == 0)
        typeOfOperation = 1;
    else if (strncmp("set", (char *)confSubcommand, 3) == 0)
        typeOfOperation = 2;
    else
        uart_print(UART_DEBUG, "Invalid command parameter. Use: conf or conf [get/set] for a particular value.\r\n");

    // GET OR SET OPERATION
    if (typeOfOperation > 0)
    {
        // Extract parameter to get/set
        char selectedParameter[CMD_MAX_LEN] = {0};
        extractCommandPart((char *) command, 2, (char *) selectedParameter);

        // SET OPERATION
        uint32_t valueToSet;
        if (typeOfOperation == 2)
        {
            // Extract value to set
            char valueToSetChar[CMD_MAX_LEN] = {0};
            extractCommandPart((char *) command, 3, (char *) valueToSetChar);
            if (valueToSetChar[0] != '\0')
                valueToSet = atol(valueToSetChar);
            else
            {
                uart_print(UART_DEBUG, "Invalid command format. Use: conf set [parameter] [value].\r\n");
            }
        }

        // Evaluate parameter to get/set
        uint8_t parameterOk = 1;
        if (strncmp("magicWord", (char *)selectedParameter, 9) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "magicWord = %X\r\n", confRegister_.magicWord);
            else if (typeOfOperation == 2)
                confRegister_.magicWord = valueToSet;
        }
        else if (strncmp("swVersion", (char *)selectedParameter, 9) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "swVersion = %d\r\n", confRegister_.swVersion);
            else if (typeOfOperation == 2)
                confRegister_.swVersion = valueToSet;
        }
        else if (strncmp("simulatorEnabled", (char *)selectedParameter, 16) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "simulatorEnabled = %d\r\n", confRegister_.simulatorEnabled);
            else if (typeOfOperation == 2)
                confRegister_.simulatorEnabled = valueToSet;
        }
        else if (strncmp("flightState", (char *)selectedParameter, 11) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "flightState = %d\r\n", confRegister_.flightState);
            else if (typeOfOperation == 2)
                confRegister_.flightState = valueToSet;
        }
        else if (strncmp("flightSubState", (char *)selectedParameter, 14) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "flightSubState = %d\r\n", confRegister_.flightSubState);
            else if (typeOfOperation == 2)
                confRegister_.flightSubState = valueToSet;
        }
        else if (strncmp("fram_telemetryAddress", (char *)selectedParameter, 21) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "fram_telemetryAddress = %ld\r\n", confRegister_.fram_telemetryAddress);
            else if (typeOfOperation == 2)
                confRegister_.fram_telemetryAddress = valueToSet;
        }
        else if (strncmp("fram_eventAddress", (char *)selectedParameter, 17) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "fram_eventAddress = %ld\r\n", confRegister_.fram_eventAddress);
            else if (typeOfOperation == 2)
                confRegister_.fram_eventAddress = valueToSet;
        }
        else if (strncmp("fram_tlmSavePeriod", (char *)selectedParameter, 18) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "fram_tlmSavePeriod = %d\r\n", confRegister_.fram_tlmSavePeriod);
            else if (typeOfOperation == 2)
                confRegister_.fram_tlmSavePeriod = valueToSet;
        }
        else if (strncmp("nor_deviceSelected", (char *)selectedParameter, 18) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "nor_deviceSelected = %d\r\n", confRegister_.nor_deviceSelected);
            else if (typeOfOperation == 2)
                confRegister_.nor_deviceSelected = valueToSet;
        }
        else if (strncmp("nor_telemetryAddress", (char *)selectedParameter, 20) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "nor_telemetryAddress = %ld\r\n", confRegister_.nor_telemetryAddress);
            else if (typeOfOperation == 2)
                confRegister_.nor_telemetryAddress = valueToSet;
        }
        else if (strncmp("nor_eventAddress", (char *)selectedParameter, 16) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "nor_eventAddress = %ld\r\n", confRegister_.nor_eventAddress);
            else if (typeOfOperation == 2)
                confRegister_.nor_eventAddress = valueToSet;
        }
        else if (strncmp("nor_tlmSavePeriod", (char *)selectedParameter, 17) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "nor_tlmSavePeriod = %d\r\n", confRegister_.nor_tlmSavePeriod);
            else if (typeOfOperation == 2)
                confRegister_.nor_tlmSavePeriod = valueToSet;
        }
        else if (strncmp("baro_readPeriod", (char *)selectedParameter, 15) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "baro_readPeriod = %d\r\n", confRegister_.baro_readPeriod);
            else if (typeOfOperation == 2)
                confRegister_.baro_readPeriod = valueToSet;
        }
        else if (strncmp("ina_readPeriod", (char *)selectedParameter, 14) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "ina_readPeriod = %d\r\n", confRegister_.ina_readPeriod);
            else if (typeOfOperation == 2)
                confRegister_.ina_readPeriod = valueToSet;
        }
        else if (strncmp("acc_readPeriod", (char *)selectedParameter, 14) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "acc_readPeriod = %d\r\n", confRegister_.acc_readPeriod);
            else if (typeOfOperation == 2)
                confRegister_.acc_readPeriod = valueToSet;
        }
        else if (strncmp("temp_readPeriod", (char *)selectedParameter, 15) == 0)
        {
            if (typeOfOperation == 1)
                sprintf(strToPrint_, "temp_readPeriod = %d\r\n", confRegister_.temp_readPeriod);
            else if (typeOfOperation == 2)
                confRegister_.temp_readPeriod = valueToSet;
        }
        else
        {
            parameterOk = 0;
            uart_print(UART_DEBUG, "Invalid command format. Use: conf [get/set] [parameter].\r\n");
        }

        if (parameterOk != 0)
        {
            if (typeOfOperation == 2)
            {
                sprintf(strToPrint_, "Selected parameter %s has been set to %ld.\r\n", selectedParameter, valueToSet);
            }
            uart_print(UART_DEBUG, strToPrint_);
        }
    }

}

/**
 * TODO please complete
 */
void processI2CCommand(char * command)
{
    // Extract the subcommand from I2C command
    char i2cSubcommand[CMD_MAX_LEN] = {0};
    extractCommandPart((char *) command, 1, (char *) i2cSubcommand);

    if (strcmp("rtc", (char *) i2cSubcommand) == 0)
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
    else if (strcmp("temp", (char*) i2cSubcommand) == 0)
    {
        int16_t temperatures[6];
        int8_t error = i2c_TMP75_getTemperatures(temperatures);

        if (error != 0)
            sprintf(strToPrint_, "I2C TEMP: Error code %d!\r\n", error);
        else
            sprintf(strToPrint_, "I2C TEMP: %.2f deg C\r\n", temperatures[0]/10.0);
    }
    else if (strcmp("baro", (char *) i2cSubcommand) == 0)
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
    else if (strcmp("ina", (char *) i2cSubcommand) == 0)
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
        sprintf(strToPrint_, "Device or sensor %s not recognised in I2C devices list.\r\n", i2cSubcommand);

    uart_print(UART_DEBUG, strToPrint_);
}

/**
 * TODO please complete
 */
void processCameraCommand(char * command)
{
    // Process the selected camera
    char selectedCameraChar[CMD_MAX_LEN] = {0};
    extractCommandPart((char *) command, 1, (char *) selectedCameraChar);
    uint8_t selectedCamera;

    switch (selectedCameraChar[0])
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
    char cameraSubcommand[CMD_MAX_LEN] = {0};
    extractCommandPart((char *) command, 2, (char *) cameraSubcommand);

    if (strcmp("on", cameraSubcommand) == 0)
    {
        //gopros_cameraRawPowerOn(selectedCamera);
        cameraPowerOn(selectedCamera);
        sprintf(strToPrint_, "Camera %c booting...\r\n", command[7]);
    }
    else if (strcmp("rawon", cameraSubcommand) == 0)
    {
        gopros_cameraRawPowerOn(selectedCamera);
        sprintf(strToPrint_, "Camera %c RAW booting...\r\n", command[7]);
    }
    else if (strcmp("pic", cameraSubcommand) == 0)
    {
        gopros_cameraRawTakePicture(selectedCamera);
        sprintf(strToPrint_, "Camera %c took a picture.\r\n", command[7]);
    }
    else if (strcmp("video_start", cameraSubcommand) == 0)
    {
        gopros_cameraRawStartRecordingVideo(selectedCamera);
        sprintf(strToPrint_, "Camera %c started recording video.\r\n", command[7]);
    }
    else if (strcmp("video_end", cameraSubcommand) == 0)
    {
        gopros_cameraRawStopRecordingVideo(selectedCamera);
        sprintf(strToPrint_, "Camera %c stopped recording video.\r\n", command[7]);
    }
    else if (strncmp("send_cmd", cameraSubcommand, 8) == 0)
    {
        uint8_t i;
        char goProCommand[CMD_MAX_LEN] = {0};
        char goProCommandNEOL[CMD_MAX_LEN] = {0};
        for (i = 9; i < CMD_MAX_LEN; i++)
        {
            if (cameraSubcommand[i] != 0)
            {
                goProCommandNEOL[i-9] = cameraSubcommand[i];
                goProCommand[i-9] = cameraSubcommand[i];
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
    else if (strcmp("off", cameraSubcommand) == 0)
    {
        cameraPowerOff(selectedCamera);
        sprintf(strToPrint_, "Camera %c powered off.\r\n", command[7]);
    }
    else
        sprintf(strToPrint_, "Camera subcommand %s not recognised...\r\n", cameraSubcommand);

    uart_print(UART_DEBUG, strToPrint_);
}

/**
 * Print telemetry lines data.
 */
void processTMCommand(char * command)
{
    struct TelemetryLine tmLines[2];
    returnCurrentTMLines(tmLines);

    // Extract the subcommand from telemetry command
    char telemetrySubcommand[CMD_MAX_LEN] = {0};
    extractCommandPart((char *) command, 1, (char *) telemetrySubcommand);

    uint8_t printResults = 1;

    struct TelemetryLine askedTMLine;
    if (strcmp("nor", (char *)telemetrySubcommand) == 0)
        askedTMLine = tmLines[1];
    else if (strcmp("fram", (char *)telemetrySubcommand) == 0)
        askedTMLine = tmLines[0];
    else
    {
        printResults = 0;
        uart_print(UART_DEBUG, "Invalid memory for telemetry command. Use: tm [nor/fram]\r\n");
    }

    if (printResults)
    {
        sprintf(strToPrint_, "Pressure:\t%.2fmbar\r\n", askedTMLine.pressure/100.0);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Altitude:\t%.2fm\r\n", askedTMLine.altitude/100.0);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Speed AVG:\t%.2fm/s\r\n", askedTMLine.verticalSpeed[0]/100.0);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Speed MAX:\t%.2fm/s\r\n", askedTMLine.verticalSpeed[1]/100.0);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Speed MIN:\t%.2fm/s\r\n", askedTMLine.verticalSpeed[2]/100.0);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Temp CPU:\t%.1fºC\r\n", askedTMLine.temperatures[0]/10.0);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Temp [1]:\t%.1fºC\r\n", askedTMLine.temperatures[1]/10.0);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Temp [2]:\t%.1fºC\r\n", askedTMLine.temperatures[2]/10.0);
        uart_print(UART_DEBUG, strToPrint_);
        //TODO Acc adata
        sprintf(strToPrint_, "Voltage AVG:\t%.2fV\r\n", askedTMLine.voltage[0]/100.0);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Voltage MAX:\t%.2fV\r\n", askedTMLine.voltage[1]/100.0);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Voltage MIN:\t%.2fV\r\n", askedTMLine.voltage[2]/100.0);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Current AVG:\t%dmA\r\n", askedTMLine.current[0]);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Current MAX:\t%dmA\r\n", askedTMLine.current[1]);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Current MIN:\t%dmA\r\n", askedTMLine.current[2]);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "State:\t\t%d\r\n", askedTMLine.state);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Substate:\t%d\r\n", askedTMLine.sub_state);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Switches:\t0x%02X\r\n", askedTMLine.switches_status);
        uart_print(UART_DEBUG, strToPrint_);
        sprintf(strToPrint_, "Errors:\t\t0x%04X\r\n", askedTMLine.errors);
        uart_print(UART_DEBUG, strToPrint_);
    }
}

/**
 * Prints current status of the CPU
 */
void printStatus()
{
    //Get current FRAM telemetry to get the general statistics of the sensors
    struct TelemetryLine tmLines[2];
    returnCurrentTMLines(tmLines);
    struct TelemetryLine askedTMLine = tmLines[0];

    uint64_t uptime = millis_uptime();
    uint32_t elapsedSeconds = seconds_uptime();
    uint32_t unixtTimeNow = i2c_RTC_unixTime_now();

    //Put first the general status of the board
    sprintf(strToPrint_, "Uptime:\t\t%.3fs\r\n", uptime/1000.0);
    uart_print(UART_DEBUG, strToPrint_);
    struct RTCDateTime dateTime;
    convert_from_unixTime(unixtTimeNow, &dateTime);
    sprintf(strToPrint_, "Time:\t\t20%.2d/%.2d/%.2d %.2d:%.2d:%.2d\r\n",
            dateTime.year,
            dateTime.month,
            dateTime.date,
            dateTime.hours,
            dateTime.minutes,
            dateTime.seconds);
    uart_print(UART_DEBUG, strToPrint_);

    /*{
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
    }*/

    sprintf(strToPrint_, "State:\t\t%d\r\n", askedTMLine.state);
    uart_print(UART_DEBUG, strToPrint_);
    sprintf(strToPrint_, "Substate:\t%d\r\n", askedTMLine.sub_state);
    uart_print(UART_DEBUG, strToPrint_);
    sprintf(strToPrint_, "Switches:\t0x%02X\r\n", askedTMLine.switches_status);
    uart_print(UART_DEBUG, strToPrint_);
    sprintf(strToPrint_, "Errors:\t\t0x%04X\r\n", askedTMLine.errors);
    uart_print(UART_DEBUG, strToPrint_);
    sprintf(strToPrint_, "CPU Temp:\t%.1fºC\r\n", askedTMLine.temperatures[0]/10.0);
    uart_print(UART_DEBUG, strToPrint_);
    //Then start with the sensors
    sprintf(strToPrint_, "Pressure:\t%.2fmbar\r\n", askedTMLine.pressure/100.0);
    uart_print(UART_DEBUG, strToPrint_);
    sprintf(strToPrint_, "Altitude:\t%.2fm\r\n", askedTMLine.altitude/100.0);
    uart_print(UART_DEBUG, strToPrint_);
    int32_t speed = getVerticalSpeed();
    sprintf(strToPrint_, "Current Speed:\t%.3fm/s\r\n", (float)speed / 100.0);
    uart_print(UART_DEBUG, strToPrint_);
    //Sunrise GPIO
    sprintf(strToPrint_, "Sunrise GPIO:\t");
    uart_print(UART_DEBUG, strToPrint_);
    if(askedTMLine.switches_status & BIT6)
        uart_print(UART_DEBUG, "HIGH\r\n");
    else
        uart_print(UART_DEBUG, "LOW\r\n");
    sprintf(strToPrint_, "Temp CPU:\t%.1fºC\r\n", askedTMLine.temperatures[0]/10.0);
    uart_print(UART_DEBUG, strToPrint_);
    sprintf(strToPrint_, "Temp [1]:\t%.1fºC\r\n", askedTMLine.temperatures[1]/10.0);
    uart_print(UART_DEBUG, strToPrint_);
    sprintf(strToPrint_, "Temp [2]:\t%.1fºC\r\n", askedTMLine.temperatures[2]/10.0);
    uart_print(UART_DEBUG, strToPrint_);
    uart_print(UART_DEBUG, "Statistics:\r\n\t\tMin\tAvg\tMax\r\n");
    sprintf(strToPrint_, "Speed(m/s):\t%.2f\t%.2f\t%.2f\r\n",
            askedTMLine.verticalSpeed[2]/100.0,
            askedTMLine.verticalSpeed[0]/100.0,
            askedTMLine.verticalSpeed[1]/100.0);
    uart_print(UART_DEBUG, strToPrint_);
    sprintf(strToPrint_, "Battery(V):\t%.2f\t%.2f\t%.2f\r\n",
            askedTMLine.voltage[2]/100.0,
            askedTMLine.voltage[0]/100.0,
            askedTMLine.voltage[1]/100.0);
    uart_print(UART_DEBUG, strToPrint_);
    sprintf(strToPrint_, "Battery(mA):\t%d\t%d\t%d\r\n",
            askedTMLine.current[2],
            askedTMLine.current[0],
            askedTMLine.current[1]);
    uart_print(UART_DEBUG, strToPrint_);

    sprintf(strToPrint_, "AccX(g):\t%.2f\t%.2f\t%.2f\r\n",
            askedTMLine.accXAxis[2]/100.0,
            askedTMLine.accXAxis[0]/100.0,
            askedTMLine.accXAxis[1]/100.0);
    uart_print(UART_DEBUG, strToPrint_);
    sprintf(strToPrint_, "AccY(g):\t%.2f\t%.2f\t%.2f\r\n",
            askedTMLine.accYAxis[2]/100.0,
            askedTMLine.accYAxis[0]/100.0,
            askedTMLine.accYAxis[1]/100.0);
    uart_print(UART_DEBUG, strToPrint_);
    sprintf(strToPrint_, "AccZ(g):\t%.2f\t%.2f\t%.2f\r\n",
            askedTMLine.accZAxis[2]/100.0,
            askedTMLine.accZAxis[0]/100.0,
            askedTMLine.accZAxis[1]/100.0);
    uart_print(UART_DEBUG, strToPrint_);

    //Now print altitude history
    printAltitudeHistory();
}

/**
 * TODO please complete
 */
void processMemoryCommand(char * command)
{
    int8_t memorySubcommand;
    int8_t memoryType;

    // MEMORY SUBCOMMANDS: status/read/dump/write/erase
    char memorySubcommandStr[CMD_MAX_LEN] = {0};
    extractCommandPart((char *) command, 1, (char *) memorySubcommandStr);

    // memory status [nor/fram]
    if (strncmp("status", (char *)memorySubcommandStr, 6) == 0)
        memorySubcommand = MEM_CMD_STATUS;
    // memory read [nor/fram] [tlm/event] [OPTIONAL start_line] [OPTIONAL end_line]
    else if (strncmp("read", (char *)memorySubcommandStr, 4) == 0)
        memorySubcommand = MEM_CMD_READ;
    // memory dump [nor/fram] [address] [num_bytes] [hex/bin]
    else if (strncmp("dump", (char *)memorySubcommandStr, 4) == 0)
        memorySubcommand = MEM_CMD_DUMP;
    // memory write [nor/fram] [address] [num_bytes] [byte0 byte1 byte2...]
    else if (strncmp("write", (char *)memorySubcommandStr, 5) == 0)
        memorySubcommand = MEM_CMD_WRITE;
    // memory erase [nor/fram] [address] [num_bytes] OR [bulk]
    else if (strncmp("erase", (char *)memorySubcommandStr, 5) == 0)
        memorySubcommand = MEM_CMD_ERASE;
    else
    {
        memorySubcommand = -1;
        uart_print(UART_DEBUG, "Incorrect memory subcommand. Use: memory [status/read/dump/write/erase].\r\n");
    }

    if (memorySubcommand != -1)
    {
        // Move on, EXTRACT MEMORY: nor or fram
        char memoryTypeStr[CMD_MAX_LEN] = {0};
        extractCommandPart((char *) command, 2, (char *) memoryTypeStr);

        if (strncmp("nor", (char *)memoryTypeStr, 3) == 0)
            memoryType = MEM_TYPE_NOR;
        else if (strncmp("fram", (char *)memoryTypeStr, 4) == 0)
            memoryType = MEM_TYPE_FRAM;
        else
        {
            memoryType = -1;
            uart_print(UART_DEBUG, "Incorrect desired memory. Use: memory [status/read/dump/write/erase] [nor/fram].\r\n");
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
             * memory read [nor/fram] [tlm/event] [OPTIONAL start_line] [OPTIONAL end_line]
             */
            else if (memorySubcommand == MEM_CMD_READ)
            {
                uint8_t readCmdError = 0;
                uint8_t lineType = 0;
                int32_t lineStart = 0;
                int32_t lineEnd = 0;

                // READ TYPE OF LINE to read (telemetry/event)
                char lineTypeStr[CMD_MAX_LEN] = {0};
                extractCommandPart((char *) command, 3, (char *) lineTypeStr);

                if (strncmp("tlm", (char *)lineTypeStr, 3) == 0)
                    lineType = MEM_LINE_TLM;
                else if (strncmp("event", (char *)lineTypeStr, 5) == 0)
                    lineType = MEM_LINE_EVENT;
                else
                {
                    readCmdError--;
                    uart_print(UART_DEBUG, "Incorrect desired line. Use: memory read [nor/fram] [tlm/event].\r\n");
                }

                // READ MEMORY START, if specified
                char lineStartStr[CMD_MAX_LEN] = {0};
                extractCommandPart((char *) command, 4, (char *) lineStartStr);

                if (lineStartStr[0] != '\0')
                    lineStart = atol(lineStartStr);
                else
                    lineStart = 0;

                // READ MEMORY END, if specified
                char lineEndStr[CMD_MAX_LEN] = {0};
                extractCommandPart((char *) command, 5, (char *) lineEndStr);

                if (lineEndStr[0] != '\0')
                    lineEnd = atol(lineEndStr);
                else
                {
                    if (lineType == MEM_LINE_TLM)
                        lineEnd = (uint16_t) confRegister_.fram_telemetryAddress / NOR_TLM_SIZE;
                    else if (lineType == MEM_LINE_EVENT)
                        lineEnd = (uint16_t) confRegister_.fram_eventAddress / NOR_EVENTS_SIZE;
                }

                if (lineStart < 0)
                {
                    readCmdError--;
                    uart_print(UART_DEBUG, "ERROR: Beginning line must be a positive integer.\r\n");
                }

                if (lineEnd < 0)
                {
                    readCmdError--;
                    uart_print(UART_DEBUG, "ERROR: Ending line must be a positive integer.\r\n");
                }

                if (lineEnd != 0 && lineEnd < lineStart)
                {
                    readCmdError--;
                    uart_print(UART_DEBUG, "ERROR: Ending line must be bigger than beginning line.\r\n");
                }

                // No errors, proceed to command execution...
                if (readCmdError == 0)
                {
                    // Print line header in CSV format
                    if (lineType == MEM_LINE_TLM)
                    {
                        uart_print(UART_DEBUG, "address,date,unixtime,uptime,pressure,altitude,"
                                "verticalSpeedAVG,verticalSpeedMAX,verticalSpeedMIN,"
                                "temperatures0,temperatures1,temperatures2,temperatures3,temperatures4,"
                                "accXAxisAVG,accXAxisMAX,accXAxisMIN,accYAxisAVG,accYAxisMAX,accYAxisMIN,"
                                "accZAxisAVG,accZAxisMAX,accZAxisMIN,voltagesAVG,voltagesMAX,voltagesMIN,"
                                "currentsAVG,currentsMAX,currentsMIN,state,sub_state,"
                                "switches_status,errors\r\n");
                    }
                    else if (lineType == MEM_LINE_EVENT)
                    {
                        uart_print(UART_DEBUG, "address,date,unixtime,"
                                "uptime,state,sub_state,event,payload0,"
                                "payload1,payload2,payload3,payload4\r\n");
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
                    uint32_t i;
                    for (i = lineStart; i < lineStart + linesToRead; i++)
                    {
                        // Retrieve line
                        if (memoryType == MEM_TYPE_NOR)
                        {
                            int8_t readCmdError;
                            if (lineType == MEM_LINE_TLM)
                                readCmdError = getTelemetryNOR(i, &readTelemetry);
                            else if (lineType == MEM_LINE_EVENT)
                                readCmdError = getEventNOR(i, &readEvent);

                            if (readCmdError != 0)
                            {
                                sprintf(strToPrint_, "Error while trying to read line %ld from NOR memory.\r\n", i);
                                uart_print(UART_DEBUG, strToPrint_);
                            }
                        }
                        else if (memoryType == MEM_TYPE_FRAM)
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
                            sprintf(strToPrint_, "%ld,20%.2d/%.2d/%.2d %.2d:%.2d:%.2d,",
                                    i,
                                    dateTime.year,
                                    dateTime.month,
                                    dateTime.date,
                                    dateTime.hours,
                                    dateTime.minutes,
                                    dateTime.seconds);
                            uart_print(UART_DEBUG, strToPrint_);
                            sprintf(strToPrint_, "%ld,%ld,%ld,%ld,%d,%d,"
                                    "%d,%d,%d,%d,",
                                    readTelemetry.unixTime,
                                    readTelemetry.upTime,
                                    readTelemetry.pressure,
                                    readTelemetry.altitude,
                                    readTelemetry.verticalSpeed[0],
                                    readTelemetry.verticalSpeed[1],
                                    readTelemetry.verticalSpeed[2],
                                    readTelemetry.temperatures[0],
                                    readTelemetry.temperatures[1],
                                    readTelemetry.temperatures[2]/*,
                                    readTelemetry.temperatures[3],
                                    readTelemetry.temperatures[4]*/);
                            uart_print(UART_DEBUG, strToPrint_);
                            sprintf(strToPrint_, "%d,%d,%d,%d,%d,"
                                    "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
                                    "%d,0x%02X,0x%04X\r\n",
                                    readTelemetry.accXAxis[0],
                                    readTelemetry.accXAxis[1],
                                    readTelemetry.accXAxis[2],
                                    readTelemetry.accYAxis[0],
                                    readTelemetry.accYAxis[1],
                                    readTelemetry.accYAxis[2],
                                    readTelemetry.accZAxis[0],
                                    readTelemetry.accZAxis[1],
                                    readTelemetry.accZAxis[2],
                                    readTelemetry.voltage[0],
                                    readTelemetry.voltage[1],
                                    readTelemetry.voltage[2],
                                    readTelemetry.current[0],
                                    readTelemetry.current[1],
                                    readTelemetry.current[2],
                                    readTelemetry.state,
                                    readTelemetry.sub_state,
                                    readTelemetry.switches_status,
                                    readTelemetry.errors);
                            uart_print(UART_DEBUG, strToPrint_);
                        }
                        else if (lineType == MEM_LINE_EVENT)
                        {
                            struct RTCDateTime dateTime;
                            convert_from_unixTime(readEvent.unixTime, &dateTime);
                            sprintf(strToPrint_, "%ld,20%.2d/%.2d/%.2d %.2d:%.2d:%.2d,",
                                    i,
                                    dateTime.year,
                                    dateTime.month,
                                    dateTime.date,
                                    dateTime.hours,
                                    dateTime.minutes,
                                    dateTime.seconds);
                            uart_print(UART_DEBUG, strToPrint_);
                            sprintf(strToPrint_, "%ld,%ld,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
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
                char readAddressStr[CMD_MAX_LEN] = {0};
                uint8_t *readAddressPointer;
                uint32_t readAddress;
                extractCommandPart((char *) command, 3, (char *) readAddressStr);

                if (readAddressStr[0] != '\0')
                {
                    readAddress = atol(readAddressStr);
                    readAddressPointer = (uint8_t *)readAddress;
                }
                else
                {
                    uart_print(UART_DEBUG, "Please specify the reading address. Use: memory dump [nor/fram] [address].\r\n");
                    error--;
                }

                // EXTRACT NUMBER OF BYTES to read
                char numBytesStr[CMD_MAX_LEN] = {0};
                uint16_t numBytes = 0;
                extractCommandPart((char *) command, 4, (char *) numBytesStr);

                if (numBytesStr[0] != '\0')
                    numBytes = atol(numBytesStr);
                else
                {
                    uart_print(UART_DEBUG, "Please specify the number of bytes to read. Use: memory dump [nor/fram] [address] [num_bytes].\r\n");
                    error--;
                }

                // EXTRACT OUTPUT FORMAT (hex/bin)
                char outputFormatStr[CMD_MAX_LEN] = {0};
                uint8_t outputFormat;
                extractCommandPart((char *) command, 5, (char *) outputFormatStr);

                if (outputFormatStr[0] != '\0')
                {
                    if (strncmp("hex", (char*)outputFormatStr, 3) == 0)
                        outputFormat = MEM_OUTFORMAT_HEX;
                    else if (strncmp("bin", (char*)outputFormatStr, 3) == 0)
                        outputFormat = MEM_OUTFORMAT_BIN;
                    else
                    {
                        uart_print(UART_DEBUG, "Incorrect output format. Use: memory dump [nor/fram] [address] [num_bytes] [hex/bin].\r\n");
                        error--;
                    }
                }
                else
                {
                    uart_print(UART_DEBUG, "Please specify the output format. Use: memory dump [nor/fram] [address] [num_bytes] [hex/bin].\r\n");
                    error--;
                }

                // No errors, proceed to serve command
                if (error == 0)
                {
                    uint8_t byteRead;

                    uint32_t i;
                    for (i = 0; i < numBytes; i++)
                    {
                        // Depending on memory type, we read one way or the other
                        if (memoryType == MEM_TYPE_NOR)
                        {
                            int8_t errorRead = spi_NOR_readFromAddress(readAddress + i, &byteRead, 1, confRegister_.nor_deviceSelected);
                            if (errorRead != 0)
                            {
                                sprintf(strToPrint_, "Error while trying to read from address %ld from NOR memory.\r\n", readAddress + i);
                                uart_print(UART_DEBUG, strToPrint_);
                            }
                        }
                        else if (memoryType == MEM_TYPE_FRAM)
                        {
                            byteRead = *readAddressPointer;
                        }

                        // Take into account output format
                        if (outputFormat == MEM_OUTFORMAT_HEX)
                        {
                            if(i % 16 == 0)
                            {
                                if (i == 0)
                                    sprintf(strToPrint_, "%ld ", (i + readAddress));
                                else
                                    sprintf(strToPrint_, "\r\n%ld ", (i + readAddress));

                                uart_print(UART_DEBUG, strToPrint_);
                            }
                            sprintf(strToPrint_, "0x%02X ", byteRead);
                            uart_print(UART_DEBUG, strToPrint_);
                            readAddressPointer++;
                        }
                        else if (outputFormat == MEM_OUTFORMAT_BIN)
                        {
                            //Just print the binary raw data into the console
                            uart_write(UART_DEBUG, &byteRead, 1);
                            readAddressPointer++;
                        }
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
                //Code disabled until not tested
                /*uint8_t error = 0;

                // EXTRACT WRITE ADDRESS
                char writeAddressStr[CMD_MAX_LEN] = {0};
                uint32_t * writeAddress;
                extractCommandPart((char *) command, 3, (char *) writeAddressStr);

                if (writeAddressStr[0] != '\0')
                    writeAddress = (uint32_t *) atol(writeAddressStr);
                else
                {
                    uart_print(UART_DEBUG, "Please specify the writing address. Use: memory write [nor/fram] [address].\r\n");
                    error--;
                }

                // EXTRACT NUMBER OF BYTES to write
                char numBytesStr[CMD_MAX_LEN] = {0};
                uint16_t numBytes = 0;
                extractCommandPart((char *) command, 4, (char *) numBytesStr);

                if (numBytesStr[0] != '\0')
                    numBytes = atoi(numBytesStr);
                else
                {
                    uart_print(UART_DEBUG, "Please specify the number of bytes to write. Use: memory write [nor/fram] [address] [num_bytes].\r\n");
                    error--;
                }

                //TODO: Implement Write protection to make sure that nothing is overwritten

                int8_t i;
                for (i = 0; i < numBytes; i++)
                {
                    // Extract i'th byte to write
                    char byteToWrite[CMD_MAX_LEN] = {0};
                    extractCommandPart((char *) command, 5+i, (char *) byteToWrite);

                    if (byteToWrite[0] != '\0')
                    {
                        // Depending on memory type, we write one way or the other
                        if (memoryType == MEM_TYPE_NOR)
                        {
                            int8_t errorWrite = spi_NOR_writeToAddress(*writeAddress, (uint8_t*) &byteToWrite, 1, confRegister_.nor_deviceSelected);
                            if (errorWrite != 0)
                            {
                                sprintf(strToPrint_, "Error while trying to write to address %ld from NOR memory.\r\n", *writeAddress);
                                uart_print(UART_DEBUG, strToPrint_);
                            }
                        }
                        else if (memoryType == MEM_TYPE_FRAM)
                            *writeAddress = *byteToWrite;
                    }
                    else
                        break; // We are done here...
                }*/
            }
            /* * *
             * Memory Erase
             * memory erase [nor/fram] [address] [num_bytes] OR [bulk]
             */
            else if (memorySubcommand == MEM_CMD_ERASE)
            {
                //TODO: Implement FRAM erasing... or not?

                uint8_t error = 0;
                char eraseType[CMD_MAX_LEN] = {0};
                extractCommandPart((char *) command, 3, (char *) eraseType);

                // Check whether bulk erase is requested or if
                // user wants to erase a certain amount of bytes
                if (strncmp("sector", (char *)eraseType, 6) == 0)
                {
                    // Extract erase sector
                    char sectorToEraseStr[CMD_MAX_LEN] = {0};
                    uint8_t sectorToErase;
                    extractCommandPart((char *) command, 4, (char *) sectorToEraseStr);
                    if (sectorToEraseStr[0] != '\0')
                        sectorToErase = atoi(sectorToEraseStr);
                    else
                    {
                        uart_print(UART_DEBUG, "Please specify the erasing sector. Use: memory erase [nor/fram] sector [num sector]\r\n.");
                        error--;
                    }

                    if (sectorToErase > 255)
                    {
                        uart_print(UART_DEBUG, "ERROR: NOR sector number must be between 0 and 255 (included).\r\n.");
                        error--;
                    }

                    // No errors processing command, move on
                    if (error == 0)
                    {
                        if (memoryType == MEM_TYPE_NOR)
                        {
                            // Sector erase the NOR memory
                            int16_t eraseStart = seconds_uptime();
                            sprintf(strToPrint_, "Started erasing sector %d of NOR memory...\r\n", sectorToErase);
                            uart_print(UART_DEBUG, strToPrint_);

                            uint32_t sectorLength = (uint32_t) 0x3FFFF;
                            uint32_t sectorAddress = 0x00 + sectorToErase * (sectorLength+1);
                            int8_t eraseCmdError = spi_NOR_sectorErase(sectorAddress, confRegister_.nor_deviceSelected);

                            if (eraseCmdError == 0)
                            {
                                int16_t eraseEnd = seconds_uptime();
                                sprintf(strToPrint_, "NOR sector %d has been completely wiped out in %d seconds.\r\n", sectorToErase, eraseEnd - eraseStart);
                                uart_print(UART_DEBUG, strToPrint_);
                            }
                            else
                            {
                                sprintf(strToPrint_, "Error while trying to erase sector %d of NOR memory.\r\n", sectorToErase);
                                uart_print(UART_DEBUG, strToPrint_);
                            }
                        }
                        else if (memoryType == MEM_TYPE_FRAM)
                        {
                            // TODO: Sector erase the FRAM memory
                            uart_print(UART_DEBUG, "FRAM memory sector erasing not implemented.\r\n");
                        }
                    }

                }
                else if (strncmp("bulk", (char *)eraseType, 4) == 0)
                {
                    if (memoryType == MEM_TYPE_NOR)
                    {
                        // Bulk erase the NOR memory
                        int16_t eraseStart = seconds_uptime();
                        uart_print(UART_DEBUG, "Started bulk-erasing NOR memory...\r\n");

                        int8_t eraseCmdError = spi_NOR_bulkErase(confRegister_.nor_deviceSelected);

                        if (eraseCmdError == 0)
                        {
                            // Reset write addresses
                            confRegister_.nor_telemetryAddress = NOR_TLM_ADDRESS;
                            confRegister_.nor_eventAddress = NOR_EVENTS_ADDRESS;

                            // We are done here
                            int16_t eraseEnd = seconds_uptime();
                            sprintf(strToPrint_, "NOR memory has been completely wiped out in %d seconds.\r\n", eraseEnd - eraseStart);
                            uart_print(UART_DEBUG, strToPrint_);
                        }
                        else
                        {
                            uart_print(UART_DEBUG, "Error while trying to bulk erase the NOR memory.\r\n");
                        }
                    }
                    else if (memoryType == MEM_TYPE_FRAM)
                    {
                        // TODO: Bulk erase the FRAM memory
                        uart_print(UART_DEBUG, "FRAM memory bulk erasing not implemented.\r\n");
                    }
                }
                else
                {
                    uart_print(UART_DEBUG, "Available erase options are sector or bulk erase. Use: memory erase [nor/fram] [sector/bulk].\r\n");
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
    //uart_print(UART_DEBUG,"IRIS:/# ");
    uart_print(UART_DEBUG,"# ");

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
            //uart_print(UART_DEBUG,"IRIS:/# ");
            uart_print(UART_DEBUG,"# ");
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
            uart_print(UART_DEBUG,"# ");;
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
        else if (strcmp("help", (char *)command_) == 0)
        {
            uart_print(UART_DEBUG, "help --> Returns this list of commands.\r\n");
            uart_print(UART_DEBUG, "...Terminal:\r\n");
            uart_print(UART_DEBUG, "terminal begin --> Starts Terminal session.\r\n");
            uart_print(UART_DEBUG, "terminal count --> Returns number of issued commands.\r\n");
            uart_print(UART_DEBUG, "terminal last --> Returns last command.\r\n");
            uart_print(UART_DEBUG, "terminal end --> Ends Terminal session.\r\n");
            uart_print(UART_DEBUG, "...CPU:\r\n");
            uart_print(UART_DEBUG, "reboot --> Reboots IRIS 2 CPU.\r\n");
            uart_print(UART_DEBUG, "fsw mode --> Returns Flight Software (FSW) mode\r\n");
            uart_print(UART_DEBUG, "\t 0: flight mode / 1: simulation mode (simulator enabled).\r\n");
            uart_print(UART_DEBUG, "fsw state --> Returns FSW state.\r\n");
            uart_print(UART_DEBUG, "fsw substate --> Returns FSW substate.\r\n");
            uart_print(UART_DEBUG, "conf --> If no parameter is passed, returns actual FSW configuration\r\n");
            uart_print(UART_DEBUG, "\t If [get] is passed, returns only desired parameter.\r\n");
            uart_print(UART_DEBUG, "\t If [set] is passed, sets value to desired parameter.\r\n");
            uart_print(UART_DEBUG, "...Time:\r\n");
            uart_print(UART_DEBUG, "uptime --> Returns elapsed seconds since boot.\r\n");
            uart_print(UART_DEBUG, "unixtime --> Returns actual UNIX time.\r\n");
            uart_print(UART_DEBUG, "date --> Returns actual date and time from IRIS 2 CLK or sets it\r\n");
            uart_print(UART_DEBUG, "\t Format for setting the date: YYYY/MM/DD HH:mm:ss.\r\n");
            uart_print(UART_DEBUG, "i2c rtc --> Returns actual date and time from RTC.\r\n");
            uart_print(UART_DEBUG, "...Sensors:\r\n");
            uart_print(UART_DEBUG, "i2c temp --> Returns temperature in tenths of deg C.\r\n");
            uart_print(UART_DEBUG, "i2c baro --> Returns atmospheric pressure in hundredths of mbar and altitude in cm.\r\n");
            uart_print(UART_DEBUG, "i2c ina --> Returns the INA voltage in hundredths of V and currents in mA.\r\n");
            uart_print(UART_DEBUG, "...Cameras:\r\n");
            uart_print(UART_DEBUG, "camera x on --> Powers on and boots camera x (where x in [1,2,3,4]).\r\n");
            uart_print(UART_DEBUG, "camera x pic --> Takes a picture with camera x using default configuration.\r\n");
            uart_print(UART_DEBUG, "camera x video_start --> Starts recording video with camera x.\r\n");
            uart_print(UART_DEBUG, "camera x video_off --> Stops video recording with camera x.\r\n");
            uart_print(UART_DEBUG, "camera x send_cmd y --> Sends command y (do not include line feed at the end!) to camera x.\r\n");
            uart_print(UART_DEBUG, "camera x off --> Safely powers off camera x.\r\n");
            uart_print(UART_DEBUG, "...Telemetry:\r\n");
            uart_print(UART_DEBUG, "tm nor --> Returns current Telemetry Line to be saved in NOR memory.\r\n");
            uart_print(UART_DEBUG, "tm fram --> Returns current Telemetry Line to be saved in FRAM memory.\r\n");
            uart_print(UART_DEBUG, "...Memory:\r\n");
            uart_print(UART_DEBUG, "memory [status/read/dump/write/erase] [nor/fram]\r\n");
            uart_print(UART_DEBUG, "\t if status --> Returns the status of the NOR or FRAM memory.\r\n");
            uart_print(UART_DEBUG, "\t if read, add: [tlm/event] [OPTIONAL line_start] [OPTIONAL line_end] --> Reads num bytes starting at address.\r\n");
            uart_print(UART_DEBUG, "\t if dump, add: [line_start] [num bytes] [hex/bin] --> Reads all Telemetry Lines or Event Lines in HEX/CSV format.\r\n");
            uart_print(UART_DEBUG, "\t if write, add: [address] [num bytes] [data] --> Writes num bytes of data starting at address.\r\n");
            uart_print(UART_DEBUG, "\t if erase, add: [sector] [num sector] or [bulk] --> Erases a sector of data (0-255) or whole memory.\r\n");
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
        else if (strncmp("uartdebug", (char *)command_, 9) == 0)
        {
            if(command_[10] == '1')
            {
                confRegister_.debugUART = 1;
                uart_init(1, BR_57600);
            }
            else if(command_[10] == '2')
            {
                confRegister_.debugUART = 2;
                uart_init(2, BR_57600);
            }
            else if(command_[10] == '3')
            {
                confRegister_.debugUART = 3;
                uart_init(3, BR_57600);
            }
            else if(command_[10] == '4')
            {
                confRegister_.debugUART = 4;
                uart_init(4, BR_57600);
            }
            else
                confRegister_.debugUART = 0;

            sprintf(strToPrint_, "UART debug changed to camera: %d\r\n",
                    confRegister_.debugUART);
            uart_print(UART_DEBUG, strToPrint_);
        }
        else if (strncmp("u ", (char *)command_, 2) == 0)
        {
            uint8_t *pointer;
            pointer = &command_[2];
            uart_print(confRegister_.debugUART, (char *)pointer);
            uart_print(confRegister_.debugUART, "\n");
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
        else if (strncmp("fsw", (char *) command_, 3) == 0)
        {
            processFSWCommand((char *) command_);
        }
        else if (strncmp("conf", (char *) command_, 4) == 0)
        {
            processConfCommand((char *) command_);
        }
        else if(strncmp("i2c", (char *) command_, 3) == 0)
        {
            processI2CCommand((char *) command_);
        }
        else if (strncmp("camera", (char *)command_, 6) == 0)
        {
            processCameraCommand((char *) command_);
        }
        else if (strncmp("tm", (char *)command_, 2) == 0)
        {
            processTMCommand((char *) command_);
        }
        else if (strcmp("status", (char *)command_) == 0)
        {
            printStatus();
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
            //uart_print(UART_DEBUG,"IRIS:/# ");
            uart_print(UART_DEBUG,"# ");
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
