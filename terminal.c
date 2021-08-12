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
 * ...Time:
 * uptime --> Returns elapsed seconds since boot
 * unixtime --> Returns actual UNIX time
 * date --> Returns actual date and time from IRIS 2 CLK or sets it
 *          Format for setting the date: YYYY/MM/DD HH:mm:ss
 * i2c_rtc --> Returns actual date and time from RTC
 * ...Sensors:
 * temperature --> Returns temperature in hundredths of deg C
 * i2c_baro --> Returns atmospheric pressure and Altitude
 * i2c_ina --> Returns the INA voltage and currents
 * ...Telemetry:
 * tm_nor --> Returns current Telemetry Line to be saved in NOR memory.
 * tm_fram --> Returns current Telemetry Line to be saved in FRAM memory.
 * ...Cameras:
 * camera x on --> Powers on and boots camera x (where x in [1,2,3,4])
 * camera x pic --> Takes a picture with camera x using default configuration
 * camera x video_start --> Starts recording video with camera x
 * camera x video_off --> Stops video recording with camera x
 * camera x send_cmd y --> Sends command y (do not include \n!!!) to camera x
 * camera x off --> Safely powers off camera x
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
 * Return flight state.
 * Return flight substate.
 */


#include "terminal.h"

uint8_t beginFlag_ = 0;
char strToPrint_[100];

// Command buffer
uint8_t numIssuedCommands_ = 0;
char lastIssuedCommand_[CMD_MAX_LEN] = {0};

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
 *
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
            //print last command:
            uart_print(UART_DEBUG, lastIssuedCommand_);
            //Copy the command:
            strcpy((char *)command_, lastIssuedCommand_);
            bufferSizeTotal_ = strlen((char *)command_);
        }
        //Ignore any others for the moment
    }

    // If a command arrived and was composed
    if (commandArrived && beginFlag_ == 1)
    {
        //sprintf(strToPrint, "%s\r\n", command);
        //uart_print(UART_DEBUG, strToPrint);

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
            sprintf(strToPrint_, "Uptime is %lds\r\n", uptime);
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
            {
                strcpy(strToPrint_, "Incorrect command, usage is: date YYYY/MM/DD HH:mm:ss\r\n");
            }
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
        else if (strcmp("i2c_rtc", (char *)command_) == 0)
        {
            struct RTCDateTime dateTime;
            i2c_RTC_getClockData(&dateTime);
            sprintf(strToPrint_, "Date from i2c RTC is: 20%.2d/%.2d/%.2d %.2d:%.2d:%.2d\r\n",
                    dateTime.year,
                    dateTime.month,
                    dateTime.date,
                    dateTime.hours,
                    dateTime.minutes,
                    dateTime.seconds);
            uart_print(UART_DEBUG, strToPrint_);
        }

        else if (strcmp("temperature", (char *)command_) == 0)
        {
            int16_t temperatures[6];
            i2c_TMP75_getTemperatures(temperatures);
            sprintf(strToPrint_, "%d\r\n", temperatures[0]);
            uart_print(UART_DEBUG, strToPrint_);
        }
        /*else if (strcmp("pressure", command) == 0)
        {
            int32_t pressure;
            i2c_MS5611_getPressure(&pressure);
            sprintf(strToPrint, "%ld\r\n", pressure);
            uart_print(UART_DEBUG, strToPrint);
        }
        else if (strcmp("altitude", command) == 0)
        {
            int32_t pressure;
            int32_t altitude;
            i2c_MS5611_getPressure(&pressure);
            i2c_MS5611_getAltitude(&pressure, &altitude);
            sprintf(strToPrint, "%ld\r\n", altitude);
            uart_print(UART_DEBUG, strToPrint);
        }*/
        else if (strcmp("i2c_baro", (char *)command_) == 0)
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
            uart_print(UART_DEBUG, strToPrint_);
        }
        /*else if (strcmp("voltage", command) == 0)
        {
            struct INAData inaData;
            i2c_INA_read(&inaData);
            sprintf(strToPrint, "%d\r\n", inaData.voltage);
            uart_print(UART_DEBUG, strToPrint);
        }
        else if (strcmp("current", command) == 0)
        {
            struct INAData inaData;
            i2c_INA_read(&inaData);
            sprintf(strToPrint, "%d\r\n", inaData.current);
            uart_print(UART_DEBUG, strToPrint);
        }*/
        else if (strcmp("i2c_ina", (char *)command_) == 0)
        {
            struct INAData inaData;
            int8_t error = i2c_INA_read(&inaData);
            if(error != 0)
                sprintf(strToPrint_, "I2C INA: Error code %d!\r\n", error);
            else
                sprintf(strToPrint_, "I2C INA: %fV, %dmA\r\n",
                        ((float)inaData.voltage/10.0),
                        inaData.current);

            uart_print(UART_DEBUG, strToPrint_);
        }
        else if (strcmp("tm_nor", (char *)command_) == 0)
        {
            struct TelemetryLine tmLines[2];
            returnCurrentTMLines(tmLines);

            sprintf(strToPrint_, "Unix Time: %ld\r\n", tmLines[0].unixTime);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Up Time: %ld\r\n", tmLines[0].upTime);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Pressure: %ld\r\n", tmLines[0].pressure);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Altitude: %ld\r\n", tmLines[0].altitude);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Vertical Speed AVG: %d\r\n", tmLines[0].verticalSpeed[0]);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Vertical Speed MAX: %d\r\n", tmLines[0].verticalSpeed[1]);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Vertical Speed MIN: %d\r\n", tmLines[0].verticalSpeed[2]);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Temperature PCB: %d\r\n", tmLines[0].temperatures[0]);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Voltage AVG: %d\r\n", tmLines[0].voltages[0]);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Voltage MAX: %d\r\n", tmLines[0].voltages[1]);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Voltage MIN: %d\r\n", tmLines[0].voltages[2]);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Current AVG: %d\r\n", tmLines[0].currents[0]);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Current MAX: %d\r\n", tmLines[0].currents[1]);
            uart_print(UART_DEBUG, strToPrint_);
            sprintf(strToPrint_, "Current MIN: %d\r\n", tmLines[0].currents[2]);
            uart_print(UART_DEBUG, strToPrint_);
        }
        else if (strcmp("tm_fram", (char *)command_) == 0)
        {
            //TODO
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
            char subcommand[CMD_MAX_LEN] = {0};
            uint8_t i;
            for (i = 9; i < CMD_MAX_LEN; i++)
            {
                subcommand[i-9] = command_[i];
                if(command_[i] == '\0')
                    break;  //end of command detected
            }

            if (strcmp("on", subcommand) == 0)
            {
                gopros_cameraRawPowerOn(selectedCamera);
                sprintf(strToPrint_, "Camera %c booting...\r\n", command_[7]);
            }
            else if (strcmp("pic", subcommand) == 0)
            {
                gopros_cameraRawTakePicture(selectedCamera);
                sprintf(strToPrint_, "Camera %c took a picture.\r\n", command_[7]);
            }
            else if (strcmp("video_start", subcommand) == 0)
            {
                gopros_cameraRawStartRecordingVideo(selectedCamera);
                sprintf(strToPrint_, "Camera %c started recording video.\r\n", command_[7]);
            }
            else if (strcmp("video_end", subcommand) == 0)
            {
                gopros_cameraRawStopRecordingVideo(selectedCamera);
                sprintf(strToPrint_, "Camera %c stopped recording video.\r\n", command_[7]);
            }
            else if (strncmp("send_cmd", subcommand, 8) == 0)
            {
                uint8_t i;
                char goProCommand[CMD_MAX_LEN] = {0};
                char goProCommandNEOL[CMD_MAX_LEN] = {0};
                for (i = 9; i < CMD_MAX_LEN; i++)
                {
                    if (subcommand[i] != 0)
                    {
                        goProCommandNEOL[i-9] = subcommand[i];
                        goProCommand[i-9] = subcommand[i];
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
            else if (strcmp("off", subcommand) == 0)
            {
                gopros_cameraRawSafePowerOff(selectedCamera);
                sprintf(strToPrint_, "Camera %c powered off.\r\n", command_[7]);
            }
            else
                sprintf(strToPrint_, "Camera subcommand %s not recognised...\r\n", subcommand);

            uart_print(UART_DEBUG, strToPrint_);
        }
        else if (strcmp("terminal count", (char *)command_) == 0)
        {
            sprintf(strToPrint_, "%d commands issued in this session.\r\n", numIssuedCommands_);
            uart_print(UART_DEBUG, strToPrint_);
        }
        else if (strcmp("terminal last", (char *)command_) == 0)
        {
            sprintf(strToPrint_, "Last issued command: %s\r\n", lastIssuedCommand_);
            uart_print(UART_DEBUG, strToPrint_);
        }
        else if (strcmp("terminal end", (char *)command_) == 0)
        {
            beginFlag_ = 0;
            numIssuedCommands_ = 0;
            uint8_t i;
            for (i = 0; i < CMD_MAX_LEN; i++)
            {
                lastIssuedCommand_[i] = 0;
            }
            sprintf(strToPrint_, "Terminal session was ended by user.\r\n");
            uart_print(UART_DEBUG, strToPrint_);
        }
        else
        {
            sprintf(strToPrint_, "Command %s is not recognised.\r\n", (char *)command_);
            uart_print(UART_DEBUG, strToPrint_);
        }

        if (beginFlag_ != 0)
        {
            uart_print(UART_DEBUG,"IRIS:/# ");
            numIssuedCommands_++;

            uint8_t i;
            for (i = 0; i < CMD_MAX_LEN; i++)
            {
                lastIssuedCommand_[i] = command_[i];
            }
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
