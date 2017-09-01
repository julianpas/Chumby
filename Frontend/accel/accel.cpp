// file: acc.c
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "i2c.h"

#define I2C_FILE_NAME "/dev/i2c-0"

#define ACC_I2C_ADDR 0x3A
#define XOUT8_REG 0x06
#define MCTL_REG 0x16
#define CTL1_REG 0x18
#define CTL2_REG 0x19

#define NUMREGS 32

static int acc_fd;

/*
The following four short functions read and write accelerometer registers.
They are wrappers around the I2C read and write functions. 
*/

/*****************************************************************************/
/*                                                                           */
/* name: read_acc_reg                                                        */
/* This function reads a single register from the accelerometer.             */
/*                                                                           */
/*****************************************************************************/
int read_acc_reg(unsigned char reg) {
   unsigned char buf[1];

   I2C_Read_nbyte(acc_fd, ACC_I2C_ADDR, reg, buf, 1);
   return buf[0];
} 

/*****************************************************************************/
/*                                                                           */
/* name: write_acc_reg                                                       */
/* This function writes a single register to the accelerometer.              */
/*                                                                           */
/*****************************************************************************/
int write_acc_reg(unsigned char reg, unsigned char value) {
   return I2C_Write_nbyte(acc_fd, ACC_I2C_ADDR, reg, &value, 1);
}

/*****************************************************************************/
/*                                                                           */
/* name: set_acc_bits                                                        */
/* This function provides a way to set a single bit in a register. In the    */
/* value passed to this procedure, the bit to be set is 1; all other         */
/* bits must be 0.                                                           */
/*                                                                           */
/* example: The following example sets to 1 the LSB of CTRL1.                */
/*    clear_acc_bits( CTRL1, 0x01 );                                         */
/*                                                                           */
/*****************************************************************************/
int set_acc_bits(unsigned char reg, unsigned char value) {
   unsigned char temp;
   temp = read_acc_reg(reg) | value;
   return I2C_Write_nbyte(acc_fd, ACC_I2C_ADDR, reg, &temp, 1);
}

/*****************************************************************************/
/*                                                                           */
/* name: clear_acc_bits                                                      */
/* This function provides a way to clear a single bit in a register. In the  */
/* value passed to this procedure, the bit to be cleared is 0; all other     */
/* bits must be 1.                                                           */
/*                                                                           */
/* example: The following example clears (ie, sets to 0) the LSB of CTRL1.   */
/*    clear_acc_bits( CTRL1, 0xfe );                                         */
/*                                                                           */
/*****************************************************************************/
int clear_acc_bits(unsigned char reg, unsigned char value) {
   unsigned char temp;
   temp = read_acc_reg(reg) & value;
   return I2C_Write_nbyte(acc_fd, ACC_I2C_ADDR, reg, &temp, 1);
} 

/*****************************************************************************/
/*                                                                           */
/* name: read_acc_xyz                                                        */
/* The normal way to read an acc reg is to use read_acc_reg. However, the    */
/* three axis regs are read frequently, so this procedure was created as a   */
/* shortcut to read all three registers at the same time.                    */
/*                                                                           */
/*****************************************************************************/
int read_acc_xyz(signed char xyz[3]) {
   signed char buf[3];

   // Read all three register values at once. Convert from unsigned to signed.
   if (I2C_Read_nbyte(acc_fd, ACC_I2C_ADDR, XOUT8_REG,
                     (unsigned char *)buf, sizeof(buf))) {
      perror("Unable to read acceleromter data");
      return 1;
   }
   xyz[0] = buf[0];
   xyz[1] = buf[1];
   xyz[2] = buf[2];
   return 0;
}

/*****************************************************************************/
/*                                                                           */
/* name: dump_registers                                                      */
/*                                                                           */
/*****************************************************************************/
int dump_registers() {
   unsigned char tmp_regs[NUMREGS];
   unsigned int reg;

   printf("Register dump:\n");
   for(reg=0; reg<sizeof(tmp_regs); reg++) {
      printf("%02x: 0x%02x\n", reg, read_acc_reg(reg) );
   }
   return 0;
}

/*****************************************************************************/
/*                                                                           */
/* name: init_acc                                                            */
/*                                                                           */
/*****************************************************************************/
int init_acc() {

   // Open a connection to the I2C userspace control file.
   if ((acc_fd = open(I2C_FILE_NAME, O_RDWR)) < 0) {
      perror("Unable to open i2c control file");
      exit(1);
   }

   // disable accelerometer, set 2g range
   if(write_acc_reg(MCTL_REG, 0x04)) {
      fprintf(stderr, "Unable to set accelerometer to standby\n");
      return 1;
   }

   // set digital filter bandwidth to 125 Hz
   write_acc_reg(CTL1_REG, 0xb8);

   // set drive strength to standard
   write_acc_reg(CTL2_REG, 0x00);

   // reenable the accelerometer by setting measurement mode.
   if(set_acc_bits(MCTL_REG, 0x01)) {
      fprintf(stderr, "Unable to enable the accelerometer\n");
      return 1;
   }

   return 0;
}

void close_acc() {
  write_acc_reg(MCTL_REG, 0x04);
  close(acc_fd);
}