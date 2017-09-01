#pragma once

int I2C_Read_nbyte(int i2c_file, 
		   unsigned char address, unsigned char reg, 
		   unsigned char *inbuf, int length);
int I2C_Write_nbyte(int i2c_file, 
		    unsigned char address, unsigned char reg, 
		    unsigned char *outbuf, int length);
