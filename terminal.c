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
 * camera x off --> Safely powers off camera x
 */

/*
 * FUTURE COMMANDS (TODO):
 * Acceleration X,Y,Z axes.
 * Camera 1,2,3,4 power off.
 * Camera 1,2,3,4 take a picture.
 * Camera 1,2,3,4 start recording video.
 * Camera 1,2,3,4 stop recording video.
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

    uart_print(UART_DEBUG, "\rIRIS 2 (Image Recorder Instrument for Sunrise) terminal.\r\n");
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

int8_t terminal_readAndProcessCommands(void)
{
    uint8_t commandArrived = 0;

    // Check the size of the UART Debug buffer
    uint8_t bufferSizeNow = 0;
    uint8_t bufferSizeTotal = 0;
    char command[CMD_MAX_LEN] = {0};

    // If there is something in the buffer
    while (bufferSizeNow = uart_available(UART_DEBUG) > 0)
    {
        // Let's retrieve char by char
        uint8_t i;
        char charRead;

        for (i = bufferSizeTotal; i < bufferSizeTotal+bufferSizeNow; i++)
        {
            charRead = uart_read(UART_DEBUG);

            // Filter out for only ASCII chars
            if ((uint8_t) charRead >= 32 && (uint8_t) charRead <= 127)
            {
                commandArrived = 1;

                // Build command
                command[i] = charRead;
            }
        }

        bufferSizeTotal += bufferSizeNow;
    }

    // If a command arrived and was composed
    if (commandArrived && beginFlag == 1)
    {
        sprintf(strToPrint, "%s\r\n", command);
        uart_print(UART_DEBUG, strToPrint);

        // Interpret command
        if (strcmp("uptime", command) == 0)
        {
            uint32_t uptime = seconds_uptime();
            sprintf(strToPrint, "%ld\r\n", uptime);
            uart_print(UART_DEBUG, strToPrint);
        }
        else if (strcmp("unixtime", command) == 0)
        {
            uint32_t unixtTimeNow = i2c_RTC_unixTime_now();
            sprintf(strToPrint, "%ld\r\n", unixtTimeNow);
            uart_print(UART_DEBUG, strToPrint);
        }
        else if (strcmp("datetime", command) == 0)
        {
            struct RTCDateTime dateTime;
            i2c_RTC_getClockData(&dateTime);
            sprintf(strToPrint, "%d/%d/%d %d:%d:%d\r\n", dateTime.date, dateTime.month, dateTime.year,
                    dateTime.hours, dateTime.minutes, dateTime.seconds);
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
                gopros_cameraRawSafePowerOff(selectedCamera);
                sprintf(strToPrint, "Camera %c stopped recording video.\r\n", command[7]);
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
            sprintf(strToPrint, "Terminal session was ended by user.\r\n", command);
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
    }
    else if(commandArrived && beginFlag == 0 && strcmp("terminal begin", command) == 0)
    {
        terminal_start();
    }

    return 0;
}
