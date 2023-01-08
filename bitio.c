#include "RAII.h"
#include "bitio.h"
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
// [DESIGN: BitIO File Format] specification
//  BitIO files have simple structure, which can be divided into three parts:
//   1. The file header, which contains exactly 5 bytes, namely BitIO, or 42 69 74 49 4F in hexadecimal
//   2. The content which may contains any number of bytes. Bits are stored from the most significant bit
//    to the least significant bit, therefore if a specific bit is effective, all bits in this byte that is
//    more significant than this bit is also effective
//   3. The size indicator which is exactly the last one byte of the file, indicating the number of effective
//    bits in the last byte of content. As a special case, if the content is completely empty, the size
//    indicator shall report CHAR_BIT bits instead of 0 bits effective. Therefore, A minimal BitIO file whose
//    content is empty shall form as follows in hexadecimal format given CHAR_BIT extends to 8:
//     42 69 74 49 4F 08
// Note: this provider is tricky. If problems are encountered, it is likely that they root here. However, you
//  should try to make sure the rest part of your code is correct before digging into this provider as which
//  can cost a lot of time.
//
// ===========================================================================================================
//
//  [DESIGN: Buffer Usage]
//   to handle the non-effective bits in the last byte in content, BitIO holds an extra 2 bytes for redundancy
//   to avoid reading ineffective bits unintentionally. This two bytes is reversed for the end of file,
//   which according to the file format specification may contain ineffective bits: the padding or the size
//   indicator. Therefore, the buffer is combined by two logical partitions, the lower BitIO_BufferSize
//   bytes, which is named as the effective buffer, and the upper BitIO_BufferRedundancy bytes, which is
//   named as the redundancy buffer. Bytes in the redundancy buffer must not be treated as effective or be
//   read.
//
//   Initialize: The following procedures assumes that the file header has been read and verified
//    Read 2 bytes from the stream
//     If this operation failed with no byte read, the file is corrupted
//     If this operation reads only one byte, this byte must be the size indicator which must be 0x08,
//      indicating this is a empty BitIO with no effective content, otherwise the file is corrupted
//     If this operation reads 2 bytes, save them to the redundancy buffer
//   Pulling extra data: The following procedures assumes that the BitIO provider has been initialized
//    If the end of the underlying stream has been reached, no data can be pulled, just give up
//    Move the redundancy buffer to the beginning of effective buffer
//    Read BitIO_BufferSize bytes from the underlying stream, saving instantly after the moved bytes
//     If this operation reads exactly BitIO_BufferSize bytes, all bits currently in the effective buffer are
//      effective now
//     If this operation reads less than BitIO_BufferSize bytes, the end of the underlying stream should be
//      reached and the last byte available in current buffer should be the size indicator. The number of
//      effective bits may be calculated based on the value of the size indicator and the number of bytes
//      actually read by this operation. Note that it is possible for this operation to read 0 bytes, in
//      which case the last read operation on the underlying stream in face reached the end of the stream,
//      in which case the size indicator is the last byte moved from the redundancy buffer
//
//                     buffer
//      /-----------------^------------------\
//     +--------------------------------------+
//     | effective buffer | redundancy buffer |
//     +------------------+-------------------+
//      \--------v-------/ \--------v--------/
//               |                  |
//               |                  +--- BitIO_BufferRedundancy(2)
//               +--- BitIO_BufferSize
//
// ===========================================================================================================
//
// [DESIGN: Writing Bits]
//  bits are first written to the byte referenced as current byte, filling from the least significant bit to
//   the most significant bit, which in case of writing a single bit the current byte is first shifted left
//   then set the least significant set with respect of the bit to be written. Number of bits written to the
//   current byte is maintained
//  number of bits written to current byte shall never reach CHAR_BIT when a call to write any number of bits
//   to BitIO returns. Whenever CHAR_BIT bits have written to current byte, it shall be commited to buffer,
//   which in effect advances the pointer indicates current byte. This may be achieved by a method provided
//   who will take care of flushing the buffer whenever needed.
//
// ===========================================================================================================
//
// [Design: Writing Multiple Bits] when there are uncommitted bits in current byte i.e. not align
//  here, use `*bits' to refer to the current byte to be written, and `current byte' to refer to the byte
//   in the buffer to which bits are being writing into. There are bits_written bits uncommitted in
//   current byte and padding_bits bits ineffective in it. Following [DESIGN: Writing Bits], uncommitted
//   bits are stored in the lower bits in current byte. The whole procedure breaks down into 2 stages and
//   as 3 options detailed below
//
//  Stage 1: there is at least CHAR_BIT bits left to write
//   In this stage, all bits in *bits are effective, which are also divided into the higher padding_bits
//    bits and the lower bits_written bits.
//    a. Shift the effective bits in current byte and concat which by the higher part of *bits constructs
//     a fully effective byte in proper order stored in current byte
//    b. Commit current byte
//    c. Copy *bits to current byte. Note that the higher part of *bits has been committed therefore is
//     ineffective while the lower part is still effective, whose length is bits_written, which follows
//     the store pattern and counter representation of current byte specified in [DESIGN: Writing Bits]
//    d. Decrease the number of bits to write by CHAR_BIT and advance the pointer to source of bits
//   Major procedure of this stage is illustrated as follows
//
//       padding_bits      bits_written                    bits_written   padding_bits
//   /--------^--------\ /------^-----\                  /------^------\ /------^-----\
//   +------------------+--------------+   shift left    +--------------+--------------+
//   | Ineffective Bits | Written Bits |  ------------>  | Written Bits |     Zeros    |
//   +------------------+--------------+                 +--------------+--------------+
//                    ^                                                 ^
//                    |                                                 |
//               current byte                                           | OR -----------------+
//                                                                      v                     |
//    +---------------+----------------+   shift right   +--------------+--------------+      |
//    |  Upper Bits   |   Lower Bits   |  ------------>  |     Zeros    |  Upper Bits  |      |
//    +---------------+----------------+                 +--------------+--------------+      |
//                    ^                                                      (USED)           |
//                    |                                                                       |
//                  *bits                                +--------------+--------------+      |
//                    |                                  | Written Bits |  Upper Bits  |  <---+
//                    |                                  +--------------+--------------+
//                    +----> copy to current byte                       |
//                                         |                            |
//     (Ineffective)                       |                            +----> commit
//    +--------------+--------------+      |
//    |  Upper Bits  |  Lower Bits  |  <---+
//    +--------------+--------------+
//     \------v-----/ \-----v------/
//      padding_bits   bits_written
//
//
//  Stage 2: there is less than CHAR_BIT bits to write
//   In this stage, *bits is the last byte to operate on as source which contains ineffective bits as
//    lower bits. This stage has two options:
//   Stage 2a: there is at least padding_bits bits to write
//    In this case, the general operation is similar to Stage 1 since current byte can be filled up and
//     committed, with the exception in step c and d that:
//      1. the bits_written counter is updated since it does not match the actual number of bits available
//      2. the current buffer is shifted right after having data copied from *bits to follow store
//          pattern specified in [DESIGN: Writing Bits]
//      3. number of bits left to write is set to zero
//   Stage 2b: there is less than padding_bits bits to write
//    In this case, current byte cannot be filled up, therefore just
//     1. make space for bits to concat by shifting bits incurrent to left
//     2. concat effective bits from *bits in the space made
//     3. update bits_written counter properly
//     4. set number of bits left to write to zero
//
// ===========================================================================================================
//
// [DESIGN: Reading Bits]
//  the first bit read by any call to any function for reading is acquired from the byte referenced by current
//   byte, which, more specifically, is the most significant bit of current byte after refilling that is done
//   in effect by advancing the reading pointer if needed
//  effective bits in current bits is always located in the higher bits, from the most significant bit to the
//   least significant bit


enum BitIO_Constant{
    BitIO_BufferSize = 1024,        // major buffer size
    BitIO_BufferRedundancy = 2,     // extra buffer space for handling last-byte-padding
};
// modes of BitIO. The value of BitIOStatus_Read and BitIOStatus_Write MUST NOT be changed since which is
//  converted from and to integer value directly
enum BitIOStatus{ BitIOStatus_Read = 0, BitIOStatus_Write = 1, BitIOStatus_Closed };

struct BitIO_{
    RAII _;
    // read/write buffer
    unsigned char buffer_[BitIO_BufferSize + BitIO_BufferRedundancy];
    // pointer to current byte reading/writing in buffer
    unsigned char* current_byte_;
    // locators within current buffer
    union{
        // values used in reading mode
        struct{
            // if the BitIO has reached the end
            bool eof;
            // bits available in current byte
            unsigned char bits_available;
            // total bits available in buffer, including bits in current byte
            unsigned short bits_read;
        }read;
        // values used in writing mode
        struct{
            // bits written to current byte
            unsigned char bits_written;
            // bytes written to current buffer, excluding the byte currently writing to
            unsigned short bytes_written;
        }write;
        // helper label for clearing this field with a single assignment
        unsigned int clean_helper;
    }size_;
    // underlying file stream, should be opened in binary mode
    FILE* stream_;
    // if the underlying stream is managed, in which case the stream shall be closed when closing BitIO
    bool managed_;
    // if the underlying stream in plain, in which case the size indicator shall not be write
    bool plain_;
    // current status of BitIO
    enum BitIOStatus status_;
    // operation on the stream, pushing or pulling data in proper way
    bool (*streamOperation_)(struct BitIO_* io);
};

// initialize a BitIO provider
static inline void BitIO_initialize_(BitIO* io){
    io->size_.clean_helper = 0;
    io->stream_ = NULL;
    io->managed_ = true;
    io->plain_ = false;
    io->status_ = BitIOStatus_Closed;
    io->current_byte_ = io->buffer_;
    io->streamOperation_ = NULL;
    RAII_set_deleter(io, (void(*)(void*))BitIO_close);
}

BitIO* BitIO_create(){
    BitIO* io = (BitIO*)malloc(sizeof(BitIO));
    BitIO_initialize_(io);
    return io;
}

// check that if the operation requested is proper with respect to current mode and stream status
//  return if the requested operation should be terminated
static inline bool BitIO_check_(BitIO* io, bool write){
    const char names[][7] = {"input", "output"};
    if(io->status_ == BitIOStatus_Closed){
        fprintf(stderr, "[BitIO]: The underlying stream is not yet opened or has already been closed\n");
        return true;
    }
    if(write != io->status_){
        fprintf(stderr, "[BitIO]: Invalid %s operation on a %s stream\n", names[io->status_], names[write]);
        return true;
    }
    return false;
}

// pull data from a BitIO file, return if effective data retrieved
static bool BitIO_pull_BitIO_(BitIO* io){
    if(io->size_.read.eof) return false;
    if(feof(io->stream_)){
        io->size_.read.eof = true;
        return false;
    }
    memcpy(io->buffer_, io->buffer_ + BitIO_BufferSize, BitIO_BufferRedundancy);
    unsigned int read_size = fread(io->buffer_ + BitIO_BufferRedundancy, 1, BitIO_BufferSize, io->stream_);
    if(read_size == BitIO_BufferSize){
        io->size_.read.bits_read = BitIO_BufferSize * CHAR_BIT;
    }else{
        io->size_.read.bits_read = CHAR_BIT * read_size + io->buffer_[BitIO_BufferRedundancy + read_size - 1];
    }
    io->current_byte_ = io->buffer_;
    io->size_.read.bits_available = io->size_.read.bits_read > CHAR_BIT ? CHAR_BIT : io->size_.read.bits_read;
    return io->size_.read.bits_read > 0;
}

// pull data from a plain binary file
static bool BitIO_pull_regular_(BitIO* io){
    if(io->size_.read.eof) return false;
    if(feof(io->stream_)){
        io->size_.read.eof = true;
        return false;
    }
    unsigned int read_size = fread(io->buffer_, 1, BitIO_BufferSize, io->stream_);
    io->size_.read.bits_read = read_size * CHAR_BIT;
    io->current_byte_ = io->buffer_;
    io->size_.read.bits_available = read_size > 0 ? CHAR_BIT : 0;
    return read_size > 0;
}

// push data to a BitIO file
//  note that this method pushes only data in the content part
static bool BitIO_push_(BitIO* io){
    fwrite(io->buffer_, io->size_.write.bytes_written, 1, io->stream_);
    io->size_.write.bytes_written = 0;
    io->current_byte_ = io->buffer_;
    return true;
}

void BitIO_close(BitIO* io){
    if(io->status_ == BitIOStatus_Closed) return;
    // if current status is not closed, underlying stream must not be NULL
    assert(io->stream_ != NULL);
    if(io->status_ == BitIOStatus_Write){   // only for writing mode
        // flush bytes in buffer to underlying stream, write the size indicator
        if(io->size_.write.bits_written != 0){
            *io->current_byte_ <<= (CHAR_BIT - io->size_.write.bits_written);
            io->size_.write.bytes_written += 1;
            io->streamOperation_(io);
            if(io->plain_){
                #ifndef NDEBUG
                fprintf(stderr, "Ineffective bits exists but not recorded due to plain mode!\n");
                #endif
            }else{
                fputc(io->size_.write.bits_written, io->stream_);
            }
        }else{
            io->streamOperation_(io);
            if(!io->plain_){
                fputc(CHAR_BIT, io->stream_);
            }
        }
    }
    // close the underlying stream
    if(io->managed_){
        fclose(io->stream_);
    }
    // reset BitIO
    BitIO_initialize_(io);
}


bool BitIO_open(BitIO* io, void* source, unsigned int modes){
    static const unsigned char BitIO_Signature[] = "BitIO";
    static const unsigned int BitIO_SignatureLength = sizeof(BitIO_Signature) - 1;
    // a BitIO must be currently closed to be opened
    assert(io->status_ == BitIOStatus_Closed);
    // set default mode
    if(modes == 0){
        modes = BitIOOpen_Read;
    }
    // begin checking requested mode
    if(modes & (~BitIOOpen_Mask)){
        #ifndef NDEBUG
        fprintf(stderr, "[BitIO]: Invalid open mode, reversed bits set!\n");
        #endif
        return false;
    }
    if(!(((modes & BitIOOpen_Read) != 0) ^ ((modes & BitIOOpen_Write) != 0))){
        #ifndef NDEBUG
        fprintf(stderr, "[BitIO]: Must specify exactly one of BitIOOpen_Read and BitIOOpen_Write!\n");
        #endif
        return false;
    }
    if(modes & BitIOOpen_Plain && modes & BitIOOpen_BitIO){
        #ifndef NDEBUG
        fprintf(stderr, "[BitIO]: Plain structure and BitIO structure are in conflict!\n");
        #endif
        return false;
    }
    if(modes & BitIOOpen_Unmanaged && modes & modes & BitIOOpen_ByPath){
        #ifndef NDEBUG
        fprintf(stderr, "[BitIO]: Open by path in unmanaged mode may cause resource leakage!\n");
        #endif
        return false;
    }
    // end checking requested mode

    // open underlying stream
    if(source == NULL){
        return false;
    }
    if(modes & BitIOOpen_ByPath){
        char* path = (char*)source;
        io->stream_ = fopen(path, modes & BitIOOpen_Read ? "rb" : "wb");
        if(io->stream_ == NULL){
            #ifndef NDEBUG
            fprintf(stderr, "[BitIO]: Failed to open file with fopen call!\n %s\n", strerror(errno));
            #endif
            return false;
        }
    }else{
        io->stream_ = (FILE*)source;
    }
    // manage underlying stream if needed
    if(modes & BitIOOpen_Unmanaged){
        io->managed_ = false;
    }else{
        io->managed_ = true;
    }
    if(modes & BitIOOpen_Write){  // only for writing mode
        if(modes & BitIOOpen_Plain){
            io->plain_ = true;
        }else{
            // write the file header
            fwrite(BitIO_Signature, BitIO_SignatureLength, 1, io->stream_);
            io->plain_ = false;
        }
        // set stream operation and BitIO status
        io->streamOperation_ = BitIO_push_;
        io->status_ = BitIOStatus_Write;
    }else{  // only for reading mode
        io->status_ = BitIOStatus_Read;
        // first, try to read the file header
        unsigned int read_size = fread(io->buffer_, 1, BitIO_SignatureLength, io->stream_);
        if(!(modes & BitIOOpen_Plain)){ // skip the signature check if assumed as plain
            if(read_size == BitIO_SignatureLength){ // if the expected number of bytes is read
                // then, check the header matches the signature of BitIO files
                if(memcmp(BitIO_Signature, io->buffer_, BitIO_SignatureLength) == 0){
                    // the signature matches, assume that the file is a BitIO file
                    // initialize the reading buffer and counters, refer to comment on stream pulling method
                    //  for more information about the procedure above
                    read_size = fread(io->buffer_ + BitIO_BufferSize, 1, BitIO_BufferRedundancy, io->stream_);
                    assert(read_size != 0);
                    if(read_size == 1){
                        assert(io->buffer_[BitIO_BufferSize] == CHAR_BIT);
                        assert(feof(io->stream_));
                    }
                    io->size_.read.bits_read = 0;
                    io->size_.read.bits_available = 0;
                    io->streamOperation_ = BitIO_pull_BitIO_;
                }
            }
        }
        if(io->streamOperation_ == NULL){
            // stream operation is not set, which happens iff the file is not (considered as) a BitIO file
            if(modes & BitIOOpen_BitIO){
                // plain binary file is not allowed
                #ifndef NDEBUG
                fprintf(stderr, "[BitIO]: Corrupted BitIO stream to read!\n");
                #endif
                BitIO_close(io);
                return false;
            }
            // treat as a plain binary file which contains only effective bits
            io->streamOperation_ = BitIO_pull_regular_;
            io->size_.read.bits_read = read_size * CHAR_BIT;
            io->size_.read.bits_available = read_size > 0 ? CHAR_BIT : 0;
        }
    }
    return true;
}

bool BitIO_eof(BitIO* io){
    if(io->status_ == BitIOStatus_Closed) return true;
    if(io->status_ == BitIOStatus_Write) return false;
    return io->size_.read.eof;
}

// write full bytes to the buffer, pushes if needed
//  length is treated as number of bytes to write
static void BitIO_writeBuffer_(BitIO* io, const unsigned char* data, unsigned int length){
    while(length){
        const unsigned int buffer_available = BitIO_BufferSize - io->size_.write.bytes_written;
        const unsigned int write_size = buffer_available < length ? buffer_available : length;
        memcpy(io->current_byte_, data, write_size);
        io->current_byte_ += write_size;
        io->size_.write.bytes_written += write_size;
        length -= write_size;
        data += write_size;
        if(write_size == buffer_available) io->streamOperation_(io);
    }
}

// move the pointer to current byte forward, which, in effect, commits the current writing byte to the buffer
//  pushes if needed
static void BitIO_advanceWriteBufferPointer_(BitIO* io){
    io->current_byte_ += 1;
    io->size_.write.bytes_written += 1;
    if(io->size_.write.bytes_written == BitIO_BufferSize){
        io->streamOperation_(io);
    }
}

void BitIO_put(BitIO* io, bool bit){
    if(BitIO_check_(io, true)) return;
    *io->current_byte_ = (*io->current_byte_ << 1) | bit;
    io->size_.write.bits_written += 1;
    if(io->size_.write.bits_written == CHAR_BIT){
        io->size_.write.bits_written = 0;
        BitIO_advanceWriteBufferPointer_(io);
    }
}

void BitIO_write(BitIO* io, const unsigned char* bits, unsigned int bit_length){
    if(BitIO_check_(io, true)) return;
    if(io->size_.write.bits_written == 0){
        // bytes in the buffer contains either entirely effective or ineffective bits, which is aligned
        //  this write request may be done in a faster way
        unsigned int byte_length = bit_length / CHAR_BIT;
        unsigned int bits_left = bit_length - byte_length * CHAR_BIT;
        // copy the full bytes part of bits
        BitIO_writeBuffer_(io, bits, byte_length);
        // then handle the rest part which has no more than a single byte
        *io->current_byte_ = bits[byte_length] >> (CHAR_BIT - bits_left);
        io->size_.write.bits_written = bits_left;
        return;
    }
    const unsigned char padding_bits = CHAR_BIT - io->size_.write.bits_written;
    while(bit_length){  // write until all bits have been written
        if(bit_length < padding_bits){  // Stage 2b
            // simply put them in current byte
            *io->current_byte_ = (*io->current_byte_ << bit_length) | ((*bits) >> (CHAR_BIT - bit_length));
            io->size_.write.bits_written += bit_length;
            bit_length = 0;
        }else{  // Stage 1 or Stage 2a
            // first, write first padding_bits bits to current byte and commit current byte
            *io->current_byte_ = (*io->current_byte_ << padding_bits)
                                 | ((*bits) >> (io->size_.write.bits_written));
            BitIO_advanceWriteBufferPointer_(io);
            // note that higher padding_bits bits of *bits has been used i.e. written and the lower
            //  bits_written bits are not, therefore coping it directly to current byte follows the design
            //  as long as bits_written is maintained properly and *bits contains only effective bits
            *io->current_byte_ = *bits;
            if(bit_length < CHAR_BIT){  // Stage 2a
                // current byte, which is copied from *bits, contains ineffective bits from the least
                //  significant bit, therefore there is no more bytes from bits, update counters and shift
                //  current byte according to the design
                io->size_.write.bits_written = bit_length - padding_bits;
                *io->current_byte_ >>= CHAR_BIT - bit_length;
                bit_length = 0;
            }else{  // Stage 1
                // current byte follows the design and a byte has been consumed from the bits to write
                //  continue to next loop
                bit_length -= CHAR_BIT;
                bits += 1;
            }
        }
    }
}

// read full bytes from the buffer, pulls if needed
//  length is treated as number of bytes to read
//  return number of *bits* actually read
//  If there is less than length * CHAT_BIT bits available all bits are read and assigned to data
static size_t BitIO_readBuffer_(BitIO* io, unsigned char* data, unsigned int length){
    size_t bits_read = 0;
    while(length){
        // if the buffer is currently empty, pull bytes from underlying stream
        if(io->size_.read.bits_available == 0){
            if(!io->streamOperation_(io)){
                // no data can be pulled, stop reading
                break;
            }
        }
        // calculate the bytes available, including the tailing byte which contains ineffective bits
        //  note that the existence of such byte indicated the end of input has reached
        const unsigned int bytes_available = io->size_.read.bits_read / CHAR_BIT
                                             + (io->size_.read.bits_read % CHAR_BIT != 0);
        // number of bytes to copy from buffer to destination
        const unsigned int bytes_read = length > bytes_available ? bytes_available : length;
        // number of bits to copy from buffer to destination
        const unsigned int bits_in_read_byte = bytes_read * CHAR_BIT;
        // number of effective bits to copy from buffer to destination
        const unsigned int actual_bits_read = bits_in_read_byte > io->size_.read.bits_read
                                              ? io->size_.read.bits_read : bits_in_read_byte;
        // copy bytes to destination
        memcpy(data, io->current_byte_, bytes_read);
        // update counters and pointers
        length -= bytes_read;
        io->size_.read.bits_read -= actual_bits_read;
        io->current_byte_ += bytes_read;
        data += bytes_read;
        bits_read += actual_bits_read;
    }
    return bits_read;
}

// move the pointer to current byte forward, pulls if needed
//  return if there are still bits available
static bool BitIO_advanceReadBufferPointer_(BitIO* io){
    if(io->size_.read.bits_read == 0){
        // note that pulling operations configure the counters and pointers for us, therefore there is no need
        //  to alter them in this case
        if(io->streamOperation_(io) == false) return false;
    }else{
        io->current_byte_ += 1;
        io->size_.read.bits_available = io->size_.read.bits_read > CHAR_BIT
                                        ? CHAR_BIT : io->size_.read.bits_read;
    }
    return true;
}

unsigned char BitIO_get(BitIO* io){
    if(BitIO_check_(io, false)) return EOF;
    if(BitIO_eof(io)) return EOF;
    if(io->size_.read.bits_available == 0 && BitIO_advanceReadBufferPointer_(io) == false){
        return EOF;
    }
    unsigned char result = *io->current_byte_ >> (CHAR_BIT - 1);
    *io->current_byte_ <<= 1;
    io->size_.read.bits_read -= 1;
    io->size_.read.bits_available -= 1;
    return result;
}

size_t BitIO_read(BitIO* io, unsigned char* bits, unsigned int bit_length){
    if(BitIO_check_(io, false)) return 0;
    // either 8 bits in current byte or 0 bit in current byte implies aligned buffer
    //  advance the pointer first if there is 0 bit in current byte to deal with these two conditions jointly
    if(io->size_.read.bits_available == 0 && BitIO_advanceReadBufferPointer_(io) == false) return 0;
    size_t bits_read = 0;
    if(io->size_.read.bits_available == 8){
        // buffer is aligned, read the full bytes part with a faster manner
        const unsigned int byte_length = bit_length / CHAR_BIT;
        const unsigned int bits_left = bit_length - byte_length * CHAR_BIT;
        bits_read = BitIO_readBuffer_(io, bits, byte_length);
        if(bits_read != bit_length - bits_left){
            // less bits read then expected, end of stream reached
            return bits_read;
        }
        // otherwise, update the counters and pointers
        bits += byte_length;
        bit_length = bits_left;
    }
    unsigned char helper;
    const unsigned char bits_available = io->size_.read.bits_available;
    const unsigned char padding_bits = CHAR_BIT - bits_available;
    while(bit_length){
        if(bit_length <= io->size_.read.bits_available){
            // there is enough bits in current byte
            io->size_.read.bits_available -= bit_length;
            io->size_.read.bits_read -= bit_length;
            *bits = *io->current_byte_;
            *io->current_byte_ <<= bit_length;
            bit_length = 0;
        }else{
            // number of bits in current byte is not enough
            //  bits in current byte will be consumed in either case
            helper = *io->current_byte_;
            if(BitIO_advanceReadBufferPointer_(io) == false){
                // no enough bits read and end of underlying stream have been reached
                *bits = helper;
                bits_read += bits_available;
                break;
            }
            helper = (helper & (((unsigned char)-1) << padding_bits))
                     | (*io->current_byte_ >> bits_available);
            *bits = helper;
            const unsigned char effective_bits_in_helper = io->size_.read.bits_available >= padding_bits
                                                           ? CHAR_BIT
                                                           : bits_available + io->size_.read.bits_available;
            const unsigned char actual_bits_read = effective_bits_in_helper > bit_length
                                                   ? bit_length : effective_bits_in_helper;
            const unsigned char current_byte_consumed_bits = actual_bits_read - bits_available;
            bits_read += actual_bits_read;
            bit_length -= actual_bits_read;
            *io->current_byte_ <<= current_byte_consumed_bits;
            io->size_.read.bits_read -= actual_bits_read;
            io->size_.read.bits_available -= current_byte_consumed_bits;
        }
    }
    return bits_read;
}