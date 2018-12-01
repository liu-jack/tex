#include "util_byteconvert.h"
#include "util_log.h"
#include <sstream>
#include <stdio.h>

namespace mfw
{

void ByteBufferException::PrintPosError() const
{
    ostringstream os;

    os << "Attempted to " <<  (_add ? "put" : "get")
       << " in ByteBuffer (pos: " << _pos << " size: "<< _size << ") "
       << "value with size: " << _esize << endl;

    MFW_ERROR(os.str());
}

void ByteBuffer::print_storage() const
{
    std::ostringstream os;
    os <<  "STORAGE_SIZE: " << size() << "\n";

    for (size_t i = 0; i < size(); ++i) {
        os << uint32_t(read<uint8_t>(i)) << " - ";
    }

    MFW_DEBUG(os.str());
}

void ByteBuffer::textlike() const
{
    std::ostringstream os;
    os <<  "STORAGE_SIZE: " << size() << "\n";

    for (size_t i = 0; i < size(); ++i) {
        os << read<uint8_t>(i);
    }

    MFW_DEBUG(os.str());
}

void ByteBuffer::hexlike() const
{
    std::ostringstream os;
    os <<  "STORAGE_SIZE: " << size() << "\n";

    size_t j = 1, k = 1;

    for (size_t i = 0; i < size(); ++i) {
        if ((i == (j * 8)) && ((i != (k * 16)))) {
            os << "| ";
            ++j;
        } else if (i == (k * 16)) {
            os << "\n";

            ++k;
            ++j;
        }

        char buf[4];
        snprintf(buf, 4, "%02X", read<uint8_t>(i));
        os << buf << " ";
    }

    MFW_DEBUG(os.str());
}

}
