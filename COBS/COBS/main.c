/*
 ============================================================================
 Name        : COBS.c
 Author      : Oleg Shipitko
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

uint8_t encodeCOBS(const char* incomingData, int numberOfBytes, char* encodedData)
{
    uint8_t read_index  = 0;
    uint8_t write_index = 1;
    uint8_t code_index  = 0;
    uint8_t code       = 1;
    
    while(read_index < numberOfBytes)
    {
        if(incomingData[read_index] == 0)
        {
            encodedData[code_index] = code;
            code = 1;
            code_index = write_index++;
            read_index++;
        }
        else
        {
            encodedData[write_index++] = incomingData[read_index++];
            code++;
            
            if(code == 0xFF)
            {
                encodedData[code_index] = code;
                code = 1;
                code_index = write_index++;
            }
        }
    }
    
    encodedData[code_index] = code;
    
    return write_index;
}


uint8_t decodeCOBS(const char* incomingData, int numberOfBytes, char* decodedData)
{
    uint8_t read_index  = 0;
    uint8_t write_index = 0;
    uint8_t code;
    uint8_t i;
    
    while (read_index < numberOfBytes)
    {
        code = incomingData[read_index];
        
        if (read_index + code > numberOfBytes && code != 1)
        {
            return 0;
        }
        
        read_index++;
        
        for (i = 1; i < code; i++)
        {
            decodedData[write_index++] = incomingData[read_index++];
        }
        
        if (code != 0xFF && read_index != numberOfBytes)
        {
            decodedData[write_index++] = '\0';
        }
    }
    return write_index;
}

int main(void) {
    // your code goes here
    char data[8];
    char encoded[20] = {0};
    char decoded[20] = {0};
    
    data[0] = 0x10;
    data[1] = 0x01;
    data[2] = 0x01;
    data[3] = 0x31;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0xFF;
    data[7] = 0xFF;
    
    printf("Initial packet:\n");
    int i = 0;
    for(; i < sizeof(data); i++)
    {
        printf("%X ", data[i]);
    }
    printf("\n");

    int numEncoded = encodeCOBS(data, sizeof(data), encoded);
    printf("Number of encoded bytes = %d\n", numEncoded);
    printf("Encoded packet:\n");
    for(i = 0; i <= numEncoded; i++)
    {
        printf("%X ", encoded[i]);
    }
    printf("\n");
    
    int numDecoded = decodeCOBS(encoded, 10, decoded);
    printf("Number of decoded bytes = %d\n", numDecoded);
    printf("Decoded packet:\n");
    for(i = 0; i < numDecoded - 1; i++)
    {
        printf("%X ", decoded[i]);
    }
    printf("\n");
}
