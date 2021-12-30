/*
 * main.c
 * Sept 11th, 2018
 *
 * Author : William Lau
 * This program establishes serial communication with any Arduino Uno running DS18B20_temp_logger.ino,
 * sends desired sampling rate to Uno, creates a .csv file, and prints the time/temperature data received from the Uno.
 *
 */

#include<stdio.h>
#include<string.h>
#include<windows.h>
#include<time.h>
#include "rs232.h"
#include <conio.h>
#include <stdlib.h>

#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

// RS232 declarations
int baud = 19200;
char mode[] = {'8','N','1',0};

unsigned char buf[4096], bufHandshake[128];	// buf holds data when Uno is logging; bufHandshake holds handshake when Uno is connecting to COM port
int port = 0, flag = 0;
char datetime[64];
double sampleRate;
char handshake = 'W';	// Uno is sending 'W' at 200Hz

FILE *fp;

// Function Declarations
void createCSVFile();
void portNumConnect();

//============================== main() ======================================
int main(){
    char exitChar;

    // Prints bootup header
    printf("DS18B20 Temperature Data Logger V1.0 \tAug 2018\nProgrammed by William Lau\n");
    printf("\n--------------------------------------------------------------------------\n");
    printf("Establishing connection to logger...\n");
    Sleep(1000);

    // Listens to COM port for Arduino
    portNumConnect();

    // Query user for sampleRate
    Sleep(2000); // Hold from user query until Arduino is ready to accept data rate value
    printf("Enter number of seconds per samples between 1s to 127s: ");
    scanf("%lf",&sampleRate);
    while(sampleRate < 1 || sampleRate > 127){
        printf("That is an invalid input, please enter number of seconds per samples between 1s to 127s: ");
        fflush(stdin);  // Flush rejected values out of stdin buffer
        scanf("%lf",&sampleRate);	// Read user value again
    }

	// Send sampleRate value to Uno
    RS232_SendByte(port, sampleRate);

	// Create the .csv file
    createCSVFile();

    printf("Initializing logging...\n");
    Sleep(500);

    RS232_PollComport(port,buf,4096);   // polling serial buffer will clear serial buffer when Uno boots up
    memset(buf,0,sizeof(buf));	// set values of buf to 0
    printf("\n--------------------------------------------------------------------------\n");
    printf("\nPress 'e' on the keyboard at any time to exit data logging.\n\n");
    printf("HH:MM:SS|Secs| T1(C)| T2(C)| T3(C)| T4(C)\n");

    // Begin printing char's from serial buffer
    while(exitChar != 'e'){		// Print from serial buffer until the 'e' key is hit
        if(kbhit()){
            exitChar = getch();
        }
        memset(buf,0,sizeof(buf));  //Clear buffer
        RS232_PollComport(port,buf,2);  // Dump 2 char from serial buffer to buf
        // if handshake is received when logging, terminate program
        /*if(buf[0] == 'W'){
            exit(EXIT_FAILURE);
        }
        */
        fprintf(fp, "%s", buf);	// print to .csv
        printf("%s", buf);	// print to console
    }

    fclose(fp);
    RS232_CloseComport(port);

    printf("\n\nProgram has finished executing.\n");
    getch();
}

//=========================== createCSVFile ===============================
//===== This function creates a .csv file with the filename in the    =====
//===== format "Temperature_Dataset_date-month-day_hour.min.sec"      =====
//===== and prints data headers into the file                         =====
//=====                                                               =====
//===== Parameter: None							                      =====
//===== Return: None                                                  =====
//=========================================================================
void createCSVFile(){

    char filename[256] = "Temperature_Dataset_";
    char datetime[64];
    char temp[64];

    // write date/time to filename
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);


    // append header to filename
    sprintf(temp,"%d-", (tm.tm_year + 1900));
    strcat(datetime, temp);
    sprintf(temp,"%d-", (tm.tm_mon + 1));
    strcat(datetime, temp);
    sprintf(temp,"%d_", (tm.tm_mday));
    strcat(datetime, temp);
    sprintf(temp,"%d.", (tm.tm_hour));
    strcat(datetime, temp);
    sprintf(temp,"%d.", (tm.tm_min));
    strcat(datetime, temp);
    sprintf(temp,"%d", (tm.tm_sec));
    strcat(datetime, temp);

    strcat(filename, datetime);

    // Check for duplicate files. Appending 'V2' to filename if duplicate exists
    fp = fopen(filename, "r");
    if(fp){
        strcat(filename, "_V2");
    }
    strcat(filename, ".csv");

    // Create a new .csv file
    fp = fopen(filename, "w+");

    fprintf(fp, "Date & Time:,%s\nSecond per Sample:,%.0f\n\n", datetime, sampleRate);
    fprintf(fp, "Time (H:M:S),Delta Time(Sec),Sensor1 (Cel),Sensor2 (Cel),Sensor3 (Cel),Sensor4 (Cel)\n");

    printf("\nPrinting data to: %s\n", filename);
}

//=========================== portNumConnect() ============================
//===== This function listens to port 0 - 19 for the handshake char   =====
//===== and opens port if handshake is available                      =====
//=====                                                               =====
//===== Parameter: None                  						      =====
//===== Return: None                                                  =====
//=========================================================================
void portNumConnect(){
    int timeout;
    while(flag == 0 && port < 20){		// check port 0-19 if it is available
        if(RS232_OpenComport(port, baud, mode)){
          RS232_CloseComport(port);
          port++;
        }else{		// if port is available...
        timeout = 100;		// limits number of times a port is checked for the handshake
        Sleep(1700);    // Wait for Uno to bootup
          while(timeout != 0 && bufHandshake[0] != handshake){	// check for timeout & if handshake is on the port
              Sleep(5);
              RS232_PollComport(port, bufHandshake, 16);
              //printf("Buf = %s\n", bufHandshake); // uncomment for troubleshooting

              timeout--;
            }

            if (bufHandshake[0] != handshake){
              //printf("Port %d is not responding.\n", port);   // uncomment for troubleshooting
              RS232_CloseComport(port);
              port++;
            }
            else{
                port++;
                printf("Port %d was successfully established.\n\n", port);
                flag = 1;
                port--;
                return;
            }
        }
	}
	printf("No data loggers were detected. Please check your connections & try again.\n\nPress any key to exit.");
	while(!kbhit()){}
	exit(EXIT_FAILURE);
}
