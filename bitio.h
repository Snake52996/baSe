#ifndef CxKANOAXDP_bitio_H_
#define CxKANOAXDP_bitio_H_
#include <stddef.h>
#include <stdbool.h>
// open requests of BitIO
enum BitIOOpen: unsigned int{
    // open BitIO for read
    BitIOOpen_Read      = 0x01u,
    // open BitIO for write
    BitIOOpen_Write     = 0x02u,
    // open BitIO in plain mode
    //  for read, this will make BitIO assume the opened stream is always plain regardless the signature
    //  for write, this will silent the signature and size indicator output, which will emit a warning in case
    //   running in debug mode and the last byte of content contains ineffective bits
    BitIOOpen_Plain     = 0x04u,
    // open BitIO in BitIO mode
    //  for read, this will restrict the opened stream to be a BitIO file, cause the call to fail if an
    //   incorrect structure is detected
    //  for write, this is the default behaviour that BitIO will generate output stream that follows
    //   [DESIGN: BitIO File Format]
    BitIOOpen_BitIO     = 0x08u,
    // open BitIO in unmanaged mode
    //  this will skip the step to close the underlying stream, leave it open
    BitIOOpen_Unmanaged = 0x10u,
    // open BitIO by path, not a existing file stream
    BitIOOpen_ByPath    = 0x20u,
    BitIOOpen_UpperBound_,
    BitIOOpen_Mask = (BitIOOpen_UpperBound_ << 1) - 3
};

// I/O provider to read/write data in unit of bits
//  this I/O provider may read from `plain binary files' as well as files previously written by the provider
//  itself. The difference between these two kinds is that all bits in a `plain binary files' are effective,
//  while bits in the last data byte in a BitIO generated files may be effective or not.
typedef struct BitIO_ BitIO;

// create an empty BitIO element
BitIO* BitIO_create();

// close BitIO
void BitIO_close(BitIO* io);

// open BitIO on specified stream
//  the source shall be either a FILE* points to the stream to wrap BitIO around, or a char* points to the
//   buffer that stores the path to the file to open
//  the mode shall be constructed by constant from enumeration BitIOOpen by add or bit-wise or attributes
//   together. Passing 0 will cause the default mode selected, which is wrapping around the stream provided
//   in source as read mode, auto detect the stream structure and close the stream when BitIO is closed
//  return if BitIO is opened successfully
bool BitIO_open(BitIO* io, void* source, unsigned int modes);

// return if the BitIO reached the end of file
//  note that *last* call to this BitIO instance *did not* get effective data if and only if a call to this
//  method immediately before the read call returns false and a call to this method immediately after the
//  read call returns true
//  for BitIO in writing mode, this method never returns true
inline bool BitIO_eof(BitIO* io);

// put a single bit to output
void BitIO_put(BitIO* io, bool bit);

// write multiple bits to output, the bits must be stored from most significant bit to least significant bit
void BitIO_write(BitIO* io, const unsigned char* bits, unsigned int bit_length);

// read one bit
//  return EOF if there is no bits available, otherwise return the bit read (at the least significant bit)
unsigned char BitIO_get(BitIO* io);

// read multiple bits from output, the bits will be stored from most significant bit to least significant bit
//  return number of bits actually read
size_t BitIO_read(BitIO* io, unsigned char* bits, unsigned int bit_length);
#endif