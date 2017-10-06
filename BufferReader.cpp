#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <ios>
#include "BufferReader.h"
#include "BufferWindowReader.h"

using std::vector;
using std::string;
using std::cout;
using std::endl;


/*
 * ___________________________________________________________________________
 */
BufferReader::BufferReader() {
}


/*
 * ___________________________________________________________________________
 */
uint8_t BufferReader::getBytePadded(BC bc, bool padLeft) const {

    uint8_t byte = 0;
    uint8_t mask = padLeft ? 0x01 : 0x80;

    for (BC len = this->getLength(); bc < len; bc <<= 1) {
        if (this->getBit(bc)) {
            byte += mask;
        }
        if (padLeft) {
            mask <<= 1;
        } else {
            mask >>= 1;
        }
    }

    return byte;
}


/*
 * ___________________________________________________________________________
 */
BC BufferReader::copyTo(uint8_t* const bytes, size_t max) const {

    BC len = this->getLength();

    if (bytes == 0) {
        len = 0;
    } else if (len > max) {
        len = max;
    }

    BC bc = 0;
    for (; bc < len; ++bc) {
        bytes[bc.byte()] = this->getByte(bc);
    }

    return bc;
}


/*
 * ___________________________________________________________________________
 */
BufferWindowReader BufferReader::getWindow(BC offset, BC length) const {

    return BufferWindowReader(*this, offset, length);
}


/*
 * ___________________________________________________________________________
 */
string BufferReader::toBitString(bool split, BC from, BC len) const {

    string stream;

    if (from.isUndef()) {
        from = 0;
    }
    BC to = this->getLength();
    if (!len.isUndef() && from + len <= to) {
        to = from + len;
    }

    size_t i = 0;
    for (BC bc = from; bc < to; bc <<= 1) {

        if (this->getBit(bc)) {
            stream.append("1");
        } else {
            stream.append("0");
        }

        if (split && bc < (to << 1)) {
            if ((i % 8) == 7) {
                stream.append(" ");
            } else if ((i % 4) == 3) {
                stream.append(".");
            }
        }
        i++;
    }

    return stream;
}


/*
 * ___________________________________________________________________________
 */
string BufferReader::toHexString(bool split, BC from, BC len) const {

    string stream;

    if (from.isUndef()) {
        from = 0;
    }
    BC to = this->getLength();
    if (!len.isUndef() && from + len <= to) {
        to = from + len;
    }

    // Buffer to hold a two-digit hex number (plus string termination)
    char hexbuf[3];

    for (BC bc = from; bc < to; bc++) {

        if ((bc + 1) > to) {
            stream.append("~");
        } else {
            if (split && bc > from && (bc + 1) <= to) {
                stream.append(" ");
            }
            snprintf(hexbuf, sizeof(hexbuf), "%.2X", this->getByte(bc));
            stream.append(hexbuf);
        }
    }

    return stream;
}


/*
 * ___________________________________________________________________________
 */
string BufferReader::toBase64String() const {

    /* this reader's length */
    BC len = this->getLength();

    // The base64 string to be constructed
    string base64;

    /* the digits used for base64 encoding ordered by value */
    string digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz0123456789+/";

    /* require the length to be a multiple of 8 bits */
    if (!len.isMultipleOf(1)) {
        throw std::runtime_error(
                "BufferReader::toBase64String(...): Bad alignment");
    }

    /* iterate over data */
    BC pos = 0;
    while (pos < len) {
        size_t i = 0;
        /* process in groups of 6 bits */
        for (size_t j = 0; j < 6; ++j) {
            /* overrun bits are treated as zero */
            if ((pos < len) && this->getBit(pos)) {
                i += 1 << (5 - j);
            }
            pos <<= 1;
        }
        base64.append(1, digits[i]);
    }

    /* now add padding characters */
    len = len.ceil(3);
    while (pos < len) {
        base64.append(1, '=');
        pos <<= 6;
    }

    return base64;
}


/*
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 0b00101
 * ___________________________________________________________________________
 */
string BufferReader::toRawString(size_t cols, BC from, BC len) const {

    string stream;

    if (from.isUndef()) {
        from = 0;
    }
    BC to = this->getLength();
    if (!len.isUndef() && from + len <= to) {
        to = from + len;
    }

    BC bc = from;
    while (bc + 1 <= to) {
        BC left = to - bc;
        size_t bytes = left.byte();
        if (bytes > cols) {
            bytes = cols;
        }
        if (bytes > 0) {
            stream.append(this->toHexString(true, bc, BC(bytes, 0)));
            stream.append("\n");
            bc += bytes;
        } else {
            break;
        }
    }
    if (bc < to) {
        stream.append("1b");
        stream.append(this->toBitString(false, bc, to));
        stream.append("\n");
    }

    return stream;
}


/*
 * ___________________________________________________________________________
 */
string BufferReader::toCString(size_t cols, BC from, BC len) const {

    string stream;

    if (from.isUndef()) {
        from = 0;
    }
    BC to = this->getLength();
    if (!len.isUndef() && from + len <= to) {
        to = from + len;
    }

    // Buffer to hold "0x" plus a two-digit hex number (plus string termination)
    char hexbuf[5];

    size_t i = 0;
    for (BC bc = from; bc < to; ++bc) {
        snprintf(hexbuf, sizeof(hexbuf), "0x%.2X", this->getByte(bc));
        stream.append(hexbuf);
        if (bc + 1 < to) {
            /* another byte will follow */
            stream.append(",");
            if (++i % cols == 0) {
                stream.append("\n");
            } else {
                stream.append(" ");
            }
        }
    }

    return stream;
}


/*
 * 0x0000 | 0000.0000 0000.0000 - 0000.0000 0000.0000 |
 * ___________________________________________________________________________
 */
size_t BufferReader::printBitDump(BC from, BC len) const {

    size_t nLines = 0;

    if (from.isUndef()) {
        from = 0;
    }
    BC to = this->getLength();
    if (!len.isUndef() && from + len <= to) {
        to = from + len;
    }

    // Buffer for use of snprintf
    char hexbuf[32];

    string line;
    BC n;
    while (from < to) {

        // Each line starts with base address
        snprintf(hexbuf, sizeof(hexbuf),
                "0x%.4X.%.1X | ", from.byte(), from.bit8());

        /* make sure we don't print beyond what is
         * available and what has been requested */
        if (from + BC(0, 16) <= to) {
            n = BC(0, 16);
        } else {
            n = to - from;
        }
        line.append(this->toBitString(true, from, n));
        // TODO: Fix width
        line.append(" - ");

        /* make sure we don't print beyond what is
         * available and what has been requested */
        if ((from << 32) <= to) {
            n = BC(0, 16);
        } else if ((from << 16) < to) {
            n = (to - from) >> 16;
        } else {
            n = 0;
        }
        line.append(this->toBitString(true, from << 16, n));
        // TODO: Fix width
        line.append(" |");

        from <<= 32;

        cout << line << endl;
        nLines++;
    }

    return nLines;
}


/*
 * ___________________________________________________________________________
 */
void BufferReader::printHexDump(size_t nCols) const {

    /* hex dump comes with its own newline */
    cout << this->getHexDump(nCols);
}


/*
 * ___________________________________________________________________________
 */
string BufferReader::getHexDump(size_t nCols) const {

    /* data pointers */
    BC pos = 0;
    BC len = this->getLength();

    /* the hex dump */
    string hexDump;

    // Buffer for use of snprintf
    char hexbuf[32];

    while (pos < len) {

        // Each line starts with base address
        snprintf(hexbuf, sizeof(hexbuf), "0x%.4X |", pos.byte());
        hexDump.append(hexbuf);

        for (size_t iCol = 0; iCol < nCols; ++iCol) {

            /* spacing between columns */
            hexDump.append(" ");

            if (((nCols % 2) == 0) && (iCol == (nCols / 2)) && nCols >= 6) {
                /* column separation for better readability */
                hexDump.append("- ");
            }

            if ((len - pos) >= (iCol + 1)) {
                // >>> At least one byte remaining >>>
                snprintf(hexbuf, sizeof(hexbuf),
                        "%.2X", this->getByte(pos + iCol));
                hexDump.append(hexbuf);
            } else if ((len - pos) > iCol) {
                /* at least one bit (but less than one byte) remaining */
                hexDump.append("~~");
            } else {
                /* nothing left => put an empty entry */
                hexDump.append("  ");
            }
        }

        /* advance data pointer to next line */
        pos += nCols;

        /* line end */
        hexDump.append(" |\n");
    }

    return hexDump;
}


/*
 * ___________________________________________________________________________
 */
BC BufferReader::exportToFile(const string& filename) const {

    BC bc = 0;
    BC len = this->getLength();

    /* open the file to write data to */
    std::ofstream ofs(filename.data(), std::ofstream::binary);
    if (ofs.is_open()) {
        while (bc < len) {
            ofs.put((char)this->getByte(bc++));
        }
        ofs.close();
    }

    return bc;
}


/*
 * ___________________________________________________________________________
 */
bool BufferReader::isEqualTo(const BufferReader& reader) const {

    bool equal = true;

    BC len = reader.getLength();
    if (len != this->getLength()) {
        equal = false;
    }

    BC bc = 0;
    for (; equal && (bc + 1) <= len; bc++) {
        if (this->getByte(bc) != reader.getByte(bc)) {
            equal = false;
        }
    }
    for (; equal && bc < len; bc <<= 1) {
        if (this->getBit(bc) != reader.getBit(bc)) {
            equal = false;
        }
    }

    return equal;
}


/*
 * ___________________________________________________________________________
 */
BufferReader::~BufferReader() {
}
