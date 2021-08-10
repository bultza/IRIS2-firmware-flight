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
 * ...Time:
 * uptime --> Returns elapsed seconds since boot
 * unixtime --> Returns actual UNIX time
 * datetime --> Returns actual date and time
 * ...Sensors:
 * temperature --> Returns temperature in hundredths of deg C
 * pressure --> Returns atmospheric pressure in hundredths of mbar
 * altitude --> Returns altitude in cm
 * voltage --> Returns INA voltage from batteries
 * current --> Returns INA current consumed
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

uint8_t beginFlag = 0;
char strToPrint[100];

// Command buffer
uint8_t numIssuedCommands = 0;
char lastIssuedCommand[CMD_MAX_LEN] = {0};

int8_t terminal_start(void)
{
    beginFlag = 1;

    uint32_t uptime = seconds_uptime();

    uart_print(UART_DEBUG, "IRIS 2 (Image Recorder Instrument for Sunrise) terminal.\r\n");
    uart_print(UART_DEBUG, "Made with passion and hard work by:\r\n");
    uart_print(UART_DEBUG, "...Aitor Conde <aitorconde@gmail.com>. Senior Engineer. Electronics. Software.\r\n");
    uart_print(UART_DEBUG, "...Ramon Garcia <ramongarciaalarcia@gmail.com>. Project Management. Software.\r\n");
    uart_print(UART_DEBUG, "...David Mayo <mayo@sondasespaciales.com>. Electronics.\r\n");
    uart_print(UART_DEBUG, "...Miguel Angel Gomez <haplo@sondasespaciales.com>. Structure.\r\n");
    sprintf(strToPrint, "IRIS 2 last booted %ld seconds ago.\r\n", uptime);
    uart_print(UART_DEBUG, strToPrint);
    uart_print(UART_DEBUG,"IRIS:/# ");

    return 0;
}

uint8_t bufferSizeNow = 0;
uint8_t bufferSizeTotal = 0;
uint8_t command[CMD_MAX_LEN] = {0};

/**
 *
 */
int8_t terminal_readAndProcessCommands(void)
{
    uint8_t commandArrived = 0;

    // If there is something in the buffer
    bufferSizeNow = uart_available(UART_DEBUG);
    if (bufferSizeNow > 0)
    {
        // Let's retrieve char by char
        uint8_t charRead = uart_read(UART_DEBUG);

        // Filter out for only ASCII chars
        if ((uint8_t) charRead >= 32 && (uint8_t) charRead < 127)
        {
            // Build command
            command[bufferSizeTotal] = charRead;
            bufferSizeTotal++;
            uart_write(UART_DEBUG, &charRead, 1); //print echo
        }
        else if(charRead == 0x08 || charRead == 127)   //Backspace!!
        {
            //Delete the last character
            if(bufferSizeTotal != 0)
            {
                bufferSizeTotal--;
                uart_write(UART_DEBUG, &charRead, 1); //print echo
            }
        }
        else if(charRead == '\n' || charRead == '\r')
        {
            commandArrived = 1;
            command[bufferSizeTotal] = '\0';  //End of string
            strToPrint[0] = '\r';
            strToPrint[1] = '\n';
            strToPrint[2] = '\0';
            uart_print(UART_DEBUG, strToPrint); //print echo
        }
    }

    // If a command arrived and was composed
    if (commandArrived && beginFlag == 1)
    {
        //sprintf(strToPrint, "%s\r\n", command);
        //uart_print(UART_DEBUG, strToPrint);

        // Interpret command
        if (command[0] == 0)
        {
            //Empty command, nothing to do, print again the commandline
        }
        else if (strcmp("reboot", command) == 0)
        {
            strcpy(strToPrint, "System will reboot...\r\n");
            uart_print(UART_DEBUG, strToPrint);
            sleep_ms(500);
            //Perform a PUC reboot
            WDTCTL = 0xDEAD;
        }
        else if (strcmp("uptime", command) == 0)
        {
            uint32_t uptime = seconds_uptime();
            sprintf(strToPrint, "%ld s\r\n", uptime);
            uart_print(UART_DEBUG, strToPrint);
        }
        else if (strcmp("unixtime", command) == 0)
        {
            uint32_t unixtTimeNow = i2c_RTC_unixTime_now();
            sprintf(strToPrint, "%ld\r\n", unixtTimeNow);
            uart_print(UART_DEBUG, strToPrint);
        }
        else if (strcmp("date", command) == 0)
        {
            uint32_t unixtTimeNow = i2c_RTC_unixTime_now();
            struct RTCDateTime dateTime;
            convert_from_unixTime(unixtTimeNow, &dateTime);
            sprintf(strToPrint, "Date is: 20%.2d/%.2d/%.2d %.2d:%.2d:%.2d\r\n",
                    dateTime.year,
                    dateTime.month,
                    dateTime.date,
                    dateTime.hours,
                    dateTime.minutes,
                    dateTime.seconds);
            uart_print(UART_DEBUG, strToPrint);
        }
        else if (strncmp("date", command, 4) == 0)
        {
            //datetime YYYY/MM/DD HH:mm:ss
            if(strlen(command) != 24
                    || command[9] != '/'
                    || command[12] != '/'
                    || command[15] != ' '
                    || command[18] != ':'
                    || command[21] != ':')
            {
                strcpy(strToPrint, "Incorrect command, usage is: date YYYY/MM/DD HH:mm:ss\r\n");
            }
            else
            {
                struct RTCDateTime dateTime;
                uint8_t *pointer;
                pointer = &command[7];
                command[9] = '\0';     //End of word
                command[12] = '\0';     //End of word
                command[15] = '\0';     //End of word
                command[18] = '\0';     //End of word
                command[21] = '\0';     //End of word
                dateTime.year = atoi(pointer);
                pointer = &command[10];
                dateTime.month = atoi(pointer);
                pointer = &command[13];
                dateTime.date = atoi(pointer);
                pointer = &command[16];
                dateTime.hours = atoi(pointer);
                pointer = &command[19];
                dateTime.minutes = atoi(pointer);
                pointer = &command[22];
                dateTime.seconds = atoi(pointer);
                uint32_t unixTime = convert_to_unixTime(dateTime);
                i2c_RTC_set_unixTime(unixTime);

                sprintf(strToPrint, "Date changed to: 20%.2d/%.2d/%.2d %.2d:%.2d:%.2d\r\n",
                                    dateTime.year,
                                    dateTime.month,
                                    dateTime.date,
                                    dateTime.hours,
                                    dateTime.minutes,
                                    dateTime.seconds);
            }
            uart_print(UART_DEBUG, strToPrint);
        }
        else if (strcmp("RTCdate", command) == 0)
        {
            struct RTCDateTime dateTime;
            i2c_RTC_getClockData(&dateTime);
            sprintf(strToPrint, "Date from RTC is: 20%.2d/%.2d/%.2d %.2d:%.2d:%.2d\r\n",
                    dateTime.year,
                    dateTime.month,
                    dateTime.date,
                    dateTime.hours,
                    dateTime.minutes,
                    dateTime.seconds);
            uart_print(UART_DEBUG, strToPrint);
        }

        else if (strcmp("temperature", command) == 0)
        {
            int16_t temperatures[6];
            i2c_TMP75_getTemperatures(temperatures);
            sprintf(strToPrint, "%d\r\n", temperatures[0]);
            uart_print(UART_DEBUG, strToPrint);
        }
        else if (strcmp("pressure", command) == 0)
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
        }
        else if (strcmp("voltage", command) == 0)
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
        }
        // This is a camera control command
        else if (strncmp("camera", command, 6) == 0)
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
            }

            // Initialised UART comms with camera
            gopros_cameraInit(selectedCamera);

            // Extract the subcommand from camera control command
            char subcommand[CMD_MAX_LEN] = {0};
            uint8_t i;
            for (i = 9; i < CMD_MAX_LEN; i++)
            {
                subcommand[i-9] = command[i];
            }

            if (strcmp("on", subcommand) == 0)
            {
                gopros_cameraRawPowerOn(selectedCamera);
                sprintf(strToPrint, "Camera %c booting...\r\n", command[7]);
            }
            else if (strcmp("pic", subcommand) == 0)
            {
                gopros_cameraRawTakePicture(selectedCamera);
                sprintf(strToPrint, "Camera %c took a picture.\r\n", command[7]);
            }
            else if (strcmp("video_start", subcommand) == 0)
            {
                gopros_cameraRawStartRecordingVideo(selectedCamera);
                sprintf(strToPrint, "Camera %c started recording video.\r\n", command[7]);
            }
            else if (strcmp("video_end", subcommand) == 0)
            {
                gopros_cameraRawStopRecordingVideo(selectedCamera);
                sprintf(strToPrint, "Camera %c stopped recording video.\r\n", command[7]);
            }
            else if (strncmp("send_cmd", subcommand, 8) == 0)
            {
                uint8_t i;
                uint8_t ended = 0;
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
                sprintf(strToPrint, "Command %s sent to camera %c.\r\n", goProCommandNEOL, command[7]);
            }
            else if (strcmp("off", subcommand) == 0)
            {
                gopros_cameraRawSafePowerOff(selectedCamera);
                sprintf(strToPrint, "Camera %c powered off.\r\n", command[7]);
            }
            else
                sprintf(strToPrint, "Camera subcommand %s not recognised...\r\n", subcommand);

            uart_print(UART_DEBUG, strToPrint);
        }
        else if (strcmp("terminal count", command) == 0)
        {
            sprintf(strToPrint, "%d commands issued in this session.\r\n", numIssuedCommands);
            uart_print(UART_DEBUG, strToPrint);
        }
        else if (strcmp("terminal last", command) == 0)
        {
            sprintf(strToPrint, "Last issued command: %s\r\n", lastIssuedCommand);
            uart_print(UART_DEBUG, strToPrint);
        }
        else if (strcmp("terminal end", command) == 0)
        {
            beginFlag = 0;
            numIssuedCommands = 0;
            uint8_t i;
            for (i = 0; i < CMD_MAX_LEN; i++)
            {
                lastIssuedCommand[i] = 0;
            }
            sprintf(strToPrint, "Terminal session was ended by user.\r\n");
            uart_print(UART_DEBUG, strToPrint);
        }
        else
        {
            sprintf(strToPrint, "Command %s is not recognised.\r\n", command);
            uart_print(UART_DEBUG, strToPrint);
        }

        if (beginFlag != 0)
        {
            uart_print(UART_DEBUG,"IRIS:/# ");
            numIssuedCommands++;

            uint8_t i;
            for (i = 0; i < CMD_MAX_LEN; i++)
            {
                lastIssuedCommand[i] = command[i];
            }
        }
        bufferSizeNow = 0;
        bufferSizeTotal = 0;
    }
    else if(commandArrived && beginFlag == 0)
    {
        if(strcmp("terminal begin", command) == 0)
            terminal_start();
        else
        {
            strcpy(strToPrint, "Incorrect password...\r\n");
            uart_print(UART_DEBUG, strToPrint);
        }
        bufferSizeNow = 0;
        bufferSizeTotal = 0;
    }

    return 0;
}
