#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <unistd.h>

int I2C_Read_nbyte(int i2c_file, 
		   unsigned char address, unsigned char reg, 
		   unsigned char *inbuf, int length) {

  unsigned char outbuf[1]; // A buffer containing data written to the bus
  struct i2c_rdwr_ioctl_data packets;
  struct i2c_msg messages[2];

  outbuf[0] = reg;

  // This first message is for data written to the I2C device. It specifies
  // the address on the I2C bus and the register to read.
  messages[0].addr    = address;
  messages[0].flags   = 0;
  messages[0].len     = sizeof(outbuf);
  messages[0].buf     = outbuf;

  // This second message is for data read from the I2C device. It specifies
  // how many bytes to read and the pointer to the buffer to store the
  // data that was read.
  messages[1].addr    = address;
  messages[1].flags   = I2C_M_RD;
  messages[1].len     = length;
  messages[1].buf     = inbuf;

  packets.msgs = messages;
  packets.nmsgs = 2;

  if(ioctl(i2c_file, I2C_RDWR, &packets) < 0) {
    perror("Unable to write/read data");
    return 1;
  }

  return 0;
}

int I2C_Write_nbyte(int i2c_file, 
		    unsigned char address, unsigned char reg, 
		    unsigned char *outbuf, int length) {
  char buf[length+1];

  if (ioctl(i2c_file, I2C_SLAVE, address) < 0) {
    perror("Unable to assign slave address");
    return 1;
  }

  // Create buffer containing both the register and the data to write
  // to the register.
  buf[0] = reg;
  memcpy(&(buf[1]), outbuf, length);
  
  if(write(i2c_file, buf, length+1) != length+1) {
    perror("Unable to write value");
    return 1;
  }

  return 0;
}