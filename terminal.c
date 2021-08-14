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
 * ...Telemetry:
 * tm nor --> Returns current Telemetry Line to be saved in NOR memory
 * tm fram --> Returns current Telemetry Line to be saved in FRAM memory
 * ...Cameras:
 * camera x on --> Powers on and boots camera x (where x in [1,2,3,4])
 * camera x pic --> Takes a picture with camera x using default configuration
 * camera x video_start --> Starts recording video with camera x
 * camera x video_off --> Stops video recording with camera x
 * camera x send_cmd y --> Sends command y (do not include \n!!!) to camera x
 * camera x off --> Safely powers off camera x
 * ...Memory:
 * dump [tlm/events] [nor/fram] [hex/csv] --> Dump Telemetry Lines or Event Lines
 *                                            from selected memory (NOR/FRAM) in
 *                                            HEX or CSV format.
 */

/*
 * FUTURE COMMANDS (TODO):
 * Acceleration X,Y,Z axes.
 * Camera 1,2,3,4 change resolution.
 * Camera 1,2,3,4 change frames per second.
 * Dump NOR memory contents.
 * Dump FRAM memory contents.
 * Dump telemetry lines.
 * Dump event lines.
 * Read a NOR address.
 * Read a FRAM address.
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
void addCommandToHistory(char * command);

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

// PUBLIC FUNCTIONS

/**
 * Starts a terminal session. From now on, incoming commands are considerd.
 * It also prints out the credits of the IRIS 2 instrument and FSW.
 */
int8_t terminal_start(void)
{
    beginFlag_ = 1;

    uint32_t uptime = seconds_uptime();

    uart_print(UART_DEBUG, "IRIS 2 (Image Recorder Instrument for Sunrise) terminal.\r\n");
    uart_print(UART_DEBUG, "Made with passion and hard work by:\r\n");
    uart_print(UART_DEBUG, "  * Aitor Conde <aitorconde@gmail.com>. Senior Engineer. Electronics. Software.\r\n");
    uart_print(UART_DEBUG, "  * Ramon Garcia <ramongarciaalarcia@gmail.com>. Project Management. Software.\r\n");
    uart_print(UART_DEBUG, "  * David Mayo <mayo@sondasespaciales.com>. Electronics.\r\n");
    uart_print(UART_DEBUG, "  * Miguel Angel Gomez <haplo@sondasespaciales.com>. Structure.\r\n");
    sprintf(strToPrint_, "IRIS 2 last booted %ld seconds ago.\r\n", uptime);
    uart_print(UART_DEBUG, strToPrint_);
    uart_print(UART_DEBUG,"IRIS:/# ");

    return 0;
}

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
            // Extract the subcommand from I2C command
            extractSubcommand(4, (char *) command_);

            if (strcmp("rtc", (char *) subcommand_) == 0)
            {
                struct RTCDateTime dateTime;
                uint8_t error = i2c_RTC_getClockData(&dateTime);

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
                uint8_t error = i2c_TMP75_getTemperatures(temperatures);

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
                    i2c_MS5611_getAltitude(&pressure, &altitude);
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
        // This is a camera control command
        else if (strncmp("camera", (char *)command_, 6) == 0)
        {
            // Process the selected camera
            uint8_t selectedCamera;
            switch (command_[7])
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
            extractSubcommand(9, (char *) command_);

            if (strcmp("on", subcommand_) == 0)
            {
                gopros_cameraRawPowerOn(selectedCamera);
                sprintf(strToPrint_, "Camera %c booting...\r\n", command_[7]);
            }
            else if (strcmp("pic", subcommand_) == 0)
            {
                gopros_cameraRawTakePicture(selectedCamera);
                sprintf(strToPrint_, "Camera %c took a picture.\r\n", command_[7]);
            }
            else if (strcmp("video_start", subcommand_) == 0)
            {
                gopros_cameraRawStartRecordingVideo(selectedCamera);
                sprintf(strToPrint_, "Camera %c started recording video.\r\n", command_[7]);
            }
            else if (strcmp("video_end", subcommand_) == 0)
            {
                gopros_cameraRawStopRecordingVideo(selectedCamera);
                sprintf(strToPrint_, "Camera %c stopped recording video.\r\n", command_[7]);
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
                sprintf(strToPrint_, "Command %s sent to camera %c.\r\n", goProCommandNEOL, command_[7]);
            }
            else if (strcmp("off", subcommand_) == 0)
            {
                gopros_cameraRawSafePowerOff(selectedCamera);
                sprintf(strToPrint_, "Camera %c powered off.\r\n", command_[7]);
            }
            else
                sprintf(strToPrint_, "Camera subcommand %s not recognised...\r\n", subcommand_);

            uart_print(UART_DEBUG, strToPrint_);
        }
        else if (strncmp("terminal", (char *)command_, 8) == 0)
        {
            // Extract the subcommand from terminal command
            extractSubcommand(9, (char *) command_);

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
        // This is a memory dump command
        else if (strncmp("dump", (char *)command_, 4) == 0)
        {
            // Which part of the memory shall we read? Extract tlm/events from command
            extractSubcommand(5, (char *) command_);

            int8_t error = 0;

            uint8_t memoryToRead;
            uint32_t startReadAddress;
            uint32_t endReadAddress;

            char cmdCopy[CMD_MAX_LEN];
            if (strncmp("tlm", (char *)subcommand_, 3) == 0)
            {
                // From which memory shall we read? Extract nor/fram from command
                strcpy(cmdCopy, subcommand_);
                extractSubcommand(4, (char *) cmdCopy);
                if (strncmp("nor", (char *)subcommand_, 3) == 0)
                {
                    memoryToRead = MEMORY_NOR;
                    startReadAddress = NOR_TLM_ADDRESS;
                    endReadAddress = NOR_EVENTS_ADDRESS - 1;
                    strcpy(cmdCopy, subcommand_);
                    extractSubcommand(4, (char *) cmdCopy);
                }
                else if (strncmp("fram", (char *)subcommand_, 4) == 0)
                {
                    memoryToRead = MEMORY_FRAM;
                    startReadAddress = FRAM_TLM_ADDRESS;
                    endReadAddress = FRAM_TLM_ADDRESS + FRAM_TLM_SIZE;
                    strcpy(cmdCopy, subcommand_);
                    extractSubcommand(5, (char *) cmdCopy);
                }
                else
                    error--;
            }
            else if (strncmp("events", (char *)subcommand_, 6) == 0)
            {
                // From which memory shall we read? Extract nor/fram from command
                strcpy(cmdCopy, subcommand_);
                extractSubcommand(7, (char *) cmdCopy);
                if (strncmp("nor", (char *)subcommand_, 3) == 0)
                {
                    memoryToRead = MEMORY_NOR;
                    startReadAddress = NOR_EVENTS_ADDRESS;
                    endReadAddress = NOR_LAST_ADDRESS;
                    strcpy(cmdCopy, subcommand_);
                    extractSubcommand(4, (char *) cmdCopy);
                }
                else if (strncmp("fram", (char *)subcommand_, 4) == 0)
                {
                    memoryToRead = MEMORY_FRAM;
                    startReadAddress = FRAM_EVENTS_ADDRESS;
                    endReadAddress = FRAM_EVENTS_ADDRESS + FRAM_EVENTS_SIZE;
                    strcpy(cmdCopy, subcommand_);
                    extractSubcommand(5, (char *) cmdCopy);
                }
                else
                    error--;
            }
            else
                error--;

            // How should we output the results? Extract hex/csv from command
            uint8_t typeOutput = 0;
            if (strcmp("hex", (char *) subcommand_) == 0)
                typeOutput = 0;
            else if (strcmp("csv", (char *) subcommand_) == 0)
                typeOutput = 1;
            else
                error--;

            // If there are no errors reading the command,
            if (error >= 0)
            {
                if (memoryToRead == MEMORY_NOR)
                {
                    //TODO: Test

                    // Read page by page
                    uint32_t pointer;
                    uint8_t buffer[NOR_BYTES_PAGE];

                    uint8_t i,j;
                    for (i = 0; i < NOR_NUM_PAGES; i++)
                    {
                        pointer = startReadAddress + i * NOR_BYTES_PAGE;
                        if (NOR_BYTES_PAGE < endReadAddress - pointer)
                            spi_NOR_readFromAddress(pointer, buffer, NOR_BYTES_PAGE, CS_FLASH1);
                        else
                            spi_NOR_readFromAddress(pointer, buffer, endReadAddress - pointer, CS_FLASH1);

                        for (j = 0; j < NOR_BYTES_PAGE; j++)
                        {
                            if (typeOutput == 0)
                                sprintf(strToPrint_, "%X ", buffer[j]);
                            else if (typeOutput == 1)
                                sprintf(strToPrint_, "%d,", buffer[j]);

                            uart_print(UART_DEBUG, strToPrint_);
                        }
                        //uart_print(UART_DEBUG, "\r\n"); // New page new line
                    }
                }
                else if (memoryToRead == MEMORY_FRAM)
                {
                    //TODO: Implement + test
                }
            }
            else
            {
                sprintf(strToPrint_, "Improper command format. Use: dump [tlm/events] [nor/fram] [hex/csv].\r\n");
                uart_print(UART_DEBUG, strToPrint_);
            }

        }
        else
        {
            sprintf(strToPrint_, "Command %s is not recognised.\r\n", (char *)command_);
            uart_print(UART_DEBUG, strToPrint_);
        }

        if (beginFlag_ != 0)
        {
            // Add command to CMD history
            addCommandToHistory((char *) command_);
            if (numIssuedCommands_ < CMD_MAX_SAVE)
                cmdSelector_ = numIssuedCommands_;
            else
                cmdSelector_ = CMD_MAX_SAVE - 1;

            numIssuedCommands_++;
            uint8_t i;
            for (i = 0; i < CMD_MAX_LEN; i++)
                lastIssuedCommand_[i] = command_[i];

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
