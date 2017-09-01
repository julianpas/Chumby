#pragma once

int read_acc_reg(unsigned char reg);
int write_acc_reg(unsigned char reg, unsigned char value);
int set_acc_bits(unsigned char reg, unsigned char value);
int clear_acc_bits(unsigned char reg, unsigned char value);
int read_acc_xyz(signed char xyz[3]);
int dump_registers();
int init_acc();
void close_acc();
