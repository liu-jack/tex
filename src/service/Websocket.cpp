#include "Websocket.h"
#include "util/util_log.h"
#include "util/util_string.h"
#include "util/util_sha.h"
#include "util/util_base64.h"
#include "NetServer.h"
#include "util/util_byteconvert.h"

#define WEBSOCKET_MAGIC_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

namespace mfw {

int Websocket::handshake(const char *pInBegin, const char *pInEnd, const char *&pNextIn, string &sOut) {
    if ((pInEnd - pInBegin) < 3) {
        return NetServer::PACKET_LESS;
    }

    if ((pInEnd - pInBegin) > 0xFFFF) {
        return -1;
    }

    std::string sIn(pInBegin, pInEnd-pInBegin);
    if (sIn.find("GET") == string::npos) {
        return -1;
    }

    string::size_type demPos = sIn.find("\r\n\r\n");
    if (demPos == string::npos)
    {
        return NetServer::PACKET_LESS;
    }

    string sHeader = sIn.substr(0, demPos);
    map<string, string> mOption;
    // 不能直接使用splitSting2，因为head的value中包含了:和空格
    vector<string> vParam =  UtilString::splitString(sHeader, "\r\n");
    for (uint32_t i = 0; i < vParam.size(); ++i) {
        string::size_type p = vParam[i].find_first_of(":");
        if (p != string::npos) {
            string sHeadKey = vParam[i].substr(0, p);
            string sHeadValue = vParam[i].substr(p+2);
            mOption[sHeadKey] = sHeadValue;
        }
    }

    string sKey;
    if (mOption.find("Sec-WebSocket-Key") == mOption.end()) {
        return -1;
    }
    sKey = mOption["Sec-WebSocket-Key"];

    string sHost;
    if (mOption.find("Host") == mOption.end()) {
        return -1;
    }
    sHost = mOption["Host"];

    string sOrigin;
    if (mOption.find("Sec-WebSocket-Origin") != mOption.end()) {
        sOrigin = mOption["Sec-WebSocket-Origin"];
    }

    sKey += WEBSOCKET_MAGIC_KEY;

    string sServerKey = UtilBase64::encode(UtilSHA::sha1bin(sKey));

    ostringstream os;
    os << "HTTP/1.1 101 Switching Protocols\r\n";
    os << "Connection: upgrade\r\n";
    os << "Sec-WebSocket-Accept: " << sServerKey << "\r\n";
    os << "Upgrade: websocket\r\n";
    if (!sOrigin.empty()) {
        os << "WebSocket-Origin: " << sOrigin << "\r\n";
    }
    os << "WebSocket-Location: ws://" << sHost << "/WebManagerSocket\r\n";
    os << "WebSocket-Protocol: WebManagerSocket\r\n\r\n";

    sOut = os.str();

    pNextIn = pInBegin+demPos+4;

    return NetServer::PACKET_FULL;
}

int Websocket::parseProtocol(const char *pInBegin, const char *pInEnd, const char *&pNextIn, string &sOut) {
    /*
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-------+-+-------------+-------------------------------+
        |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
        |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
        |N|V|V|V|       |S|             |   (if payload len==126/127)   |
        | |1|2|3|       |K|             |                               |
        +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
        |     Extended payload length continued, if payload len == 127  |
        + - - - - - - - - - - - - - - - +-------------------------------+
        |                               |Masking-key, if MASK set to 1  |
        +-------------------------------+-------------------------------+
        | Masking-key (continued)       |          Payload Data         |
        +-------------------------------- - - - - - - - - - - - - - - - +
        :                     Payload Data continued ...                :
        + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
        |                     Payload Data continued ...                |
        +---------------------------------------------------------------+
    */
    uint32_t len = pInEnd - pInBegin;
    if (len < 2) {
        return NetServer::PACKET_LESS;
    }

    ByteBuffer bb;
    bb.resize(len);
	memcpy((char*)bb.contents(), pInBegin, len);

    try {
        // 操作码
        uint8_t b1 = 0;
        bb >> b1;

        //uint8_t fin = (b1>>7) & (0x01);

        uint8_t opcode = b1 & 0x0F;
        if (opcode != 0x0  // denotes a continuation frame
        && opcode != 0x1   // denotes a text frame
        && opcode != 0x2   // denotes a binary frame
        && opcode != 0x8   // denotes a connection close
        ) {
            MFW_ERROR("websocket unsupport opcode=" << (uint32_t)opcode);
            return -1;
        }

        // 描述掩码和消息长度, 最高位用0或1来描述是否有掩码处理
        uint8_t b2 = 0;
        bb >> b2;

        uint8_t mask = (b2 >> 7) & 0x01;
        uint8_t msg_len = b2 & (~0x80);
        uint64_t payload_len = 0;
        // 由于7位消息长度最多只能描述127所以这个值会代表三种情况
        // 一种是消息内容少于126存储消息长度, 如果消息长度少于UINT16的情况此值为126
        // 当消息长度大于UINT16的情况下此值为127;
        // 这两种情况的消息长度存储到紧随后面的byte[], 分别是UINT16(2位byte)和UINT64(8位byte)
        if(msg_len <= 125) {
            payload_len = msg_len;
        } else if(msg_len == 126) {
            // 不足2字节，需要继续等待
            if (bb.left() < 2) {
                return NetServer::PACKET_LESS;
            }

            uint16_t s1 = 0;
            bb >> s1;
            payload_len = s1;
        } else if(msg_len == 127) {
            // 不足8字节，需要继续等待
            if (bb.left() < 8) {
                return NetServer::PACKET_LESS;
            }

            bb >> payload_len;
        }

        if (payload_len > 0xFFFF) {
            // 不接受payload>2个字节的消息
            MFW_ERROR("websocket payload>0xFFFF");
            return -1;
        }

        // 掩码
        uint8_t mask_key[4];
        if (mask) {
            if (bb.left() < 4) {
                return NetServer::PACKET_LESS;
            }
            bb >> mask_key[0] >> mask_key[1] >> mask_key[2] >> mask_key[3];
        }

        // payload
        if (bb.left() < (uint32_t)payload_len) {
            return NetServer::PACKET_LESS;
        }

        sOut.resize(payload_len);
        bb.read((uint8_t*)sOut.data(), payload_len);
        if (mask) {
            for(uint32_t i = 0; i < payload_len; ++i) {
                sOut[i] = sOut[i] ^ mask_key[i % 4];
            }
        }

        // 关闭
        if (opcode == 0x8) {
            uint16_t code = 0;
            string sReason;
            if (sOut.size() > 2) {
                code = (uint16_t)sOut[0] << 8 | (uint16_t)sOut[1];
                sReason.assign(sOut.c_str()+2, sOut.size()-2);
            }
            MFW_DEBUG("websocket close, code:" << code << ",reason:" << sReason);
            return -2;
        }

        assert (len > bb.left());
        pNextIn = pInBegin + (len-bb.left());

        return NetServer::PACKET_FULL;
	} catch(const ByteBufferException &ex) {
		bb.hexlike();
        return -1;
	}

    return -1;
}

string Websocket::encodeProtocol(const string &sBuffer) {
    ByteBuffer bb;

    // 只支持BINARY_FRAME
    bb << (uint8_t)0x82;

    uint32_t iLen = sBuffer.size();
    if(iLen <= 125) {
        bb << (uint8_t)iLen;
    } else if (iLen <= 65535) {
        bb << (uint8_t)126;
        bb << (uint16_t)iLen;
    } else {
        bb << (uint8_t)127;
        bb << (uint64_t)iLen;
    }

    bb.append((const uint8_t*)sBuffer.c_str(), sBuffer.size());
    return string((const char*)bb.contents(), bb.size());
}

}
