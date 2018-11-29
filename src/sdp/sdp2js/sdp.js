var Long = require('long');

var sdp = sdp || {};
module.exports = sdp;

sdp.CONST_7F = Long.fromInt(0x7F, true);
sdp.CONST_80 = Long.fromInt(0x80, true);
sdp._ARRAY_BUFFER_8 = new ArrayBuffer(8);
sdp._FLOAT32_ARRAY = new Float32Array(sdp._ARRAY_BUFFER_8);
sdp._FLOAT64_ARRAY = new Float64Array(sdp._ARRAY_BUFFER_8);
sdp._UINT32_ARRAY = new Uint32Array(sdp._ARRAY_BUFFER_8);

sdp.SdpException = function(msg) {
    this.message = msg;
};
sdp.SdpException.prototype = Object.create(Error);
sdp.SdpException.prototype.constructor = sdp.SdpException;

sdp.SdpRequireNotExist = function(tag) {
    this.message = 'field not exist, tag:'+tag;
};
sdp.SdpRequireNotExist.prototype = Object.create(sdp.SdpException);
sdp.SdpRequireNotExist.prototype.constructor = sdp.SdpRequireNotExist;

sdp.SdpUnkownPackType = function(type) {
    this.message = 'get wrong type:'+type;
};
sdp.SdpUnkownPackType.prototype = Object.create(sdp.SdpException);
sdp.SdpUnkownPackType.prototype.constructor = sdp.SdpUnkownPackType;

sdp.SdpMismatchType = function(tag, type1, type2) {
    this.message = 'tag:'+tag+',type dismatch,real:'+sdp.packTypeToString(type1)+',expect:'+sdp.packTypeToString(type2);
};
sdp.SdpMismatchType.prototype = Object.create(sdp.SdpException);
sdp.SdpMismatchType.prototype.constructor = sdp.SdpMismatchType;

sdp.Buffer = function(buff) {
    this._cap = 0;
    this._pos = 0;
    this._len = 0;
    this._buff = null;
    this._u8a = null;
    if (buff instanceof ArrayBuffer) {
        this._buff = buff;
        this._u8a = new Uint8Array(this._buff);
        this._len = this._u8a.length;
        this._cap = this._len;
    } else if (buff instanceof Uint8Array) {
        this._buff = buff.buffer;
        this._u8a = buff;
        this._len = this._u8a.length;
        this._cap = this._len;
    } else if (buff instanceof sdp.Buffer) {
        this._buff = buff._buff;
        this._u8a = buff._u8a;
        this._len = this._u8a.length;
        this._cap = this._len;
    }
    this._classname = 'bytes';
};
sdp.Buffer.prototype.reset = function() {
    this._pos = 0;
    this._len = 0;
};
sdp.Buffer.prototype.getBuff = function() {
    return this._buff.slice(0, this._len);
};
sdp.Buffer.prototype._alloc = function(bytes) {
    if (this._pos + bytes < this._cap) return;
    this._cap = Math.max(512, (this._pos + bytes) * 2);
    var tmpbuff = new ArrayBuffer(this._cap);
    var tmpu8a = new Uint8Array(tmpbuff);
    if (this._buff != null) {
        tmpu8a.set(this._u8a);
    }
    this._buff = tmpbuff;
    this._u8a = tmpu8a;
};
sdp.Buffer.prototype._checkSize = function(bytes) {
    if (this._pos + bytes > this._cap) {
        throw 'sdp not enought data';
    }
};
sdp.Buffer.prototype.utf16ToUtf8 = function(utf16Str) {
    var utf8Arr = [];
    var byteSize = 0;
    for (var i = 0; i < utf16Str.length; i++) {
        var code = utf16Str.charCodeAt(i);
        if (code >= 0x00 && code <= 0x7f) {
            byteSize += 1;
            utf8Arr.push(code);
        } else if (code >= 0x80 && code <= 0x7ff) {
            byteSize += 2;
            utf8Arr.push((192 | (31 & (code >> 6))));
            utf8Arr.push((128 | (63 & code)));
        } else if ((code >= 0x800 && code <= 0xd7ff)
            || (code >= 0xe000 && code <= 0xffff)) {
            byteSize += 3;
            utf8Arr.push((224 | (15 & (code >> 12))));
            utf8Arr.push((128 | (63 & (code >> 6))));
            utf8Arr.push((128 | (63 & code)));
        } else if(code >= 0x10000 && code <= 0x10ffff ){
            byteSize += 4;
            utf8Arr.push((240 | (7 & (code >> 18))));
            utf8Arr.push((128 | (63 & (code >> 12))));
            utf8Arr.push((128 | (63 & (code >> 6))));
            utf8Arr.push((128 | (63 & code)));
        }
    }
    return utf8Arr;
};
sdp.Buffer.prototype.utf8ToUtf16 = function(utf8Arr) {
    var utf16Str = '';
    for (var i = 0; i < utf8Arr.length; i++) {
        var one = utf8Arr[i].toString(2);
        var v = one.match(/^1+?(?=0)/);
        if (v && one.length == 8) {
            var bytesLength = v[0].length;
            var store = utf8Arr[i].toString(2).slice(7 - bytesLength);
            for (var st = 1; st < bytesLength; st++) {
                store += utf8Arr[st + i].toString(2).slice(2)
            }
            utf16Str += String.fromCharCode(parseInt(store, 2));
            i += bytesLength - 1
        } else {
            utf16Str += String.fromCharCode(utf8Arr[i])
        }
    }

    return utf16Str;
};
sdp.Buffer.prototype.printhex = function() {
    var ss = "";
    for (var i = 0; i < this._len; ++i) {
        var v = this._u8a[i];
        if (v <= 0xF) { ss += '0';}
        ss += v.toString(16);
        if ((i+1)%16 == 0) ss += " ";
    }
    console.log(ss.toUpperCase());
};
sdp.Buffer.prototype.toString = function() {
    var ss = "";
    for (var i = 0; i < this._len; ++i) {
        var v = this._u8a[i];
        if (v <= 0xF) { ss += '0';}
        ss += v.toString(16);
    }
    return ss;
};
sdp.Buffer.prototype.writeUint32 = function(value) {
    value = +value;
    this._alloc(5);
    var n = 0;
    while (value > 0x7f) {
        var ua = (value&0x7f)|0x80;
        this._u8a[this._pos+n] = ua;
        value >>>= 7;
        ++n;
    }
    this._u8a[this._pos+n++] = value&0x7f;
    this._pos += n;
    this._len = this._pos;
};
sdp.Buffer.prototype.readUint32 = function() {
    this._checkSize(1);
    var value = this._u8a[this._pos]&0x7f;
    this._pos += 1;
    
    var n = 1;
    while (this._u8a[this._pos-1] > 0x7f) {
        this._checkSize(1);
        var h = this._u8a[this._pos]&0x7f;
        value |= h << (7*n);
        n++;
        this._pos += 1;
    }
    value >>>= 0;
    return value;
};
sdp.Buffer.prototype.writeUint64 = function(value) {
    this._alloc(10);
    var n = 0;
    while (value.gt(sdp.CONST_7F)) {
        var ua = value.and(sdp.CONST_7F).toInt()|0x80;
        this._u8a[this._pos+n] = ua;
        value = value.shru(7);
        ++n;
    }
    this._u8a[this._pos+n++] = value.toInt()&0x7f;
    this._pos += n;
    this._len = this._pos;
};
sdp.Buffer.prototype.readUint64 = function() {
    this._checkSize(1);
    var value = Long.fromInt(this._u8a[this._pos]&0x7f, true);
    this._pos += 1;
    
    var n = 1;
    while (this._u8a[this._pos-1] > 0x7f) {
        this._checkSize(1);
        var h = Long.fromInt(this._u8a[this._pos]&0x7f, true);
        value = value.or(h.shl(7*n));
        n++;
        this._pos += 1;
    }
    return value;
};
sdp.Buffer.prototype.writeFloat = function(value) {
    value = +value;
    var fa = sdp._FLOAT32_ARRAY;
    fa[0] = value;
    var ia = sdp._UINT32_ARRAY;
    this.writeUint32(ia[0]);
};
sdp.Buffer.prototype.readFloat = function() {
    var u32 = this.readUint32();
    var ia = sdp._UINT32_ARRAY;
    ia[0] = u32;
    var fa = sdp._FLOAT32_ARRAY;
    return fa[0];
};
sdp.Buffer.prototype.writeDouble = function(value) {
    value = +value;
    var da = sdp._FLOAT64_ARRAY;
    da[0] = value;
    var ua = sdp._UINT32_ARRAY;
    this.writeUint64(Long.fromBits(ua[0], ua[1], true));
};
sdp.Buffer.prototype.readDouble = function() {
    var val = this.readUint64();
    var ua = sdp._UINT32_ARRAY;
    ua[0] = val.getLowBits();
    ua[1] = val.getHighBits();
    var da = sdp._FLOAT64_ARRAY;
    return da[0];
};
sdp.Buffer.prototype.writeString = function(value) {
    var u8a = this.utf16ToUtf8(value);
    this.writeUint32(u8a.length);
    
    this._alloc(u8a.length);
    this._u8a.set(u8a, this._pos);
    this._pos += u8a.length;
    this._len = this._pos;
};
sdp.Buffer.prototype.readString = function() {
    var len = this.readUint32();
    this._checkSize(len);
    var u8a = this._u8a.slice(this._pos, this._pos+len);
    this._pos += len;
    
    return this.utf8ToUtf16(u8a);
};
sdp.Buffer.prototype.writeBytes = function(buff) {
    if (!(buff instanceof sdp.Buffer)) {
        throw sdp.SdpException("not a sdp.Buffer");
    }
    var u8a = new Uint8Array(buff.getBuff());
    this.writeUint32(u8a.length);
    
    this._alloc(u8a.length);
    this._u8a.set(u8a, this._pos);
    this._pos += u8a.length;
    this._len = this._pos;
};
sdp.Buffer.prototype.readBytes = function() {
    var len = this.readUint32();
    this._checkSize(len);
    var u8a = this._u8a.slice(this._pos, this._pos+len);
    this._pos += len;
    
    return new sdp.Buffer(u8a);
};
sdp.Buffer.prototype.writeByte = function(c) {
    this._alloc(1);
    this._u8a[this._pos] = c;
    this._pos += 1;
    this._len = this._pos;
};
sdp.Buffer.prototype.readByte = function() {
    this._checkSize(1);
    var c = this._u8a[this._pos];
    this._pos += 1;
    
    return c;
};

sdp.Buffer.prototype.__defineGetter__("length",   function () { return this._len; });
sdp.Buffer.prototype.__defineGetter__("capacity",   function () { return this._cap; });

// sdp支持的数据类型
sdp.Boolean = {
    _write  : function(os, tag, val) { os.writeBoolean(tag, true, val); },
    _read   : function(is, tag, def) { return is.readBoolean(tag, true, def); },
    _classname : 'bool'
};

sdp.Char = {
    _write  : function(os, tag, val) { os.writeUint8(tag, true, val); },
    _read   : function(is, tag, def) { return is.readUint8(tag, true, def); },
    _classname : 'char'
};

sdp.Int8 = {
    _write  : function(os, tag, val) { os.writeInt8(tag, true, val); },
    _read   : function(is, tag, def) { return is.readInt8(tag, true, def); },
    _classname : 'int8'
};

sdp.Int16 = {
    _write  : function(os, tag, val) { os.writeInt16(tag, true, val); },
    _read   : function(is, tag, def) { return is.readInt16(tag, true, def); },
    _classname : 'int16'
};

sdp.Int32 = {
    _write  : function(os, tag, val) { os.writeInt32(tag, true, val); },
    _read   : function(is, tag, def) { return is.readInt32(tag, true, def); },
    _classname : 'int32'
};

sdp.Int64 = {
    _write  : function(os, tag, val) { os.writeInt64(tag, true, val); },
    _read   : function(is, tag, def) { return is.readInt64(tag, true, def); },
    _classname : 'int64'
};

sdp.Uint8 = {
    _write  : function(os, tag, val) { os.writeUint8(tag, true, val); },
    _read   : function(is, tag, def) { return is.readUint8(tag, true, def); },
    _classname : 'uint8'
};

sdp.Uint16 = {
    _write  : function(os, tag, val) { os.writeUint16(tag, true, val); },
    _read   : function(is, tag, def) { return is.readUint16(tag, true, def); },
    _classname : 'uint16'
};

sdp.Uint32 = {
    _write  : function(os, tag, val) { os.writeUint32(tag, true, val); },
    _read   : function(is, tag, def) { return is.readUint32(tag, true, def); },
    _classname : 'uint32'
};

sdp.Uint64 = {
    _write  : function(os, tag, val) { os.writeUint64(tag, true, val); },
    _read   : function(is, tag, def) { return is.readUint64(tag, true, def); },
    _classname : 'uint64'
};

sdp.Float = {
    _write  : function(os, tag, val) { os.writeFloat(tag, true, val); },
    _read   : function(is, tag, def) { return is.readFloat(tag, true, def); },
    _classname : 'float'
};

sdp.Double = {
    _write  : function(os, tag, val) { os.writeDouble(tag, true, val); },
    _read   : function(is, tag, def) { return is.readDouble(tag, true, def); },
    _classname : 'double'
};

sdp.String = {
    _write  : function(os, tag, val) { os.writeString(tag, true, val); },
    _read   : function(is, tag, def) { return is.readString(tag, true, def); },
    _classname : 'string'
};

sdp._Vector = function(proto) {
    this._proto = proto;
    this.value = new Array();
    this._classname = 'vector<'+this._proto._classname+'>';
};
sdp._Vector.prototype._write = function(os, tag, val) { os.writeVector(tag, true, val); };
sdp._Vector.prototype._read = function(is, tag, def) { return is.readVector(tag, true, def); };
sdp._Vector.prototype.push = function(value) { this.value.push(value); };
sdp._Vector.prototype.forEach = function(callback) {
    for (var i = 0; i < this.value.length; i++) {
        if (callback.call(null, this.value[i], i, this.value) == false) break;
    }  
};
sdp._Vector.prototype.__defineGetter__("length", function () { return this.value.length; });

sdp.Vector = function(proto) {
    return new sdp._Vector(proto);
};

sdp._Map = function(kproto, vproto) {
    this._kproto = kproto;
    this._vproto = vproto;
    this.value = new Object();
    this._classname = 'map<'+this._kproto._classname+','+this._vproto._classname+'>';
};
sdp._Map.prototype._write = function(os, tag, val) { os.writeMap(tag, true, val); };
sdp._Map.prototype._read = function(is, tag, def) { return is.readMap(tag, true, def); };
sdp._Map.prototype.set = function(key, val) { this.insert(key, val); };
sdp._Map.prototype.insert = function(key, val) { this.value[key] = val; };
sdp._Map.prototype.remove = function(key) { delete this.value[key]; };
sdp._Map.prototype.size = function() { return Object.keys(this.value || {}).length; };
sdp._Map.prototype.has = function(key) { return this.value.hasOwnProperty(key); };
sdp._Map.prototype.get = function(key) { return this.value[key]; };
sdp._Map.prototype.clear = function() { delete this.value; this.value = new Object(); };
sdp._Map.prototype.forEach = function(callback) {
    var keys = Object.keys(this.value || {});
    for(var i = 0; i < keys.length; i++) {
        var key = keys[i];
        switch (this._kproto){
            case sdp.Boolean:
            case sdp.Int8:
            case sdp.Int16:
            case sdp.Int32:
            case sdp.Int64:
            case sdp.Uint8:
            case sdp.Uint16:
            case sdp.Uint32:
            case sdp.Uint64:
            case sdp.Float:
            case sdp.Double:
                key=Number(key);
                break;
        }
        if (callback.call(null, key, this.value[key]) == false)
            break;
    }
};
sdp.Map = function(kproto, vproto) {
    return new sdp._Map(kproto, vproto);
};

// 数据类型
sdp.SdpType_Void = 0;
sdp.SdpType_Boolean = 1;
sdp.SdpType_Char = 2;
sdp.SdpType_Int8 = 3;
sdp.SdpType_Uint8 = 4;
sdp.SdpType_Int16 = 5;
sdp.SdpType_Uint16 = 6;
sdp.SdpType_Int32 = 7;
sdp.SdpType_Uint32 = 8;
sdp.SdpType_Int64 = 9;
sdp.SdpType_Uint64 = 10;
sdp.SdpType_Float = 11;
sdp.SdpType_Double = 12;
sdp.SdpType_String = 13;
sdp.SdpType_Vector = 14;
sdp.SdpType_Map = 15;
sdp.SdpType_Enum = 16;
sdp.SdpType_Struct = 17;

// 数据打包类型
sdp.SdpPackDataType_Integer_Positive = 0;
sdp.SdpPackDataType_Integer_Negative = 1;
sdp.SdpPackDataType_Integer64_Positive = 2;
sdp.SdpPackDataType_Integer64_Negative = 3;
sdp.SdpPackDataType_Float = 4;
sdp.SdpPackDataType_Double = 5;
sdp.SdpPackDataType_String = 6;
sdp.SdpPackDataType_Vector = 7;
sdp.SdpPackDataType_Map = 8;
sdp.SdpPackDataType_StructBegin = 9;
sdp.SdpPackDataType_StructEnd = 10;
sdp.SdpPackDataType_Bytes = 11;
sdp.packTypeToString = function(type) {
    var ret = 'unknown';
    switch(type) {
        case sdp.SdpPackDataType_Integer_Positive:
            ret = 'uint32_positive';
            break;
        case sdp.SdpPackDataType_Integer_Negative:
            ret = 'uint32_negative';
            break;
        case sdp.SdpPackDataType_Integer64_Positive:
            ret = 'uint64_positive';
            break;
        case sdp.SdpPackDataType_Integer64_Negative:
            ret = 'uint64_negative';
            break;
        case sdp.SdpPackDataType_Float:
            ret = 'float';
            break;
        case sdp.SdpPackDataType_Double:
            ret = 'double';
            break;
        case sdp.SdpPackDataType_String:
            ret = 'string';
            break;
        case sdp.SdpPackDataType_Vector:
            ret = 'vector';
            break;
        case sdp.SdpPackDataType_Map:
            ret = 'map';
            break;
        case sdp.SdpPackDataType_StructBegin:
            ret = 'struct begin';
            break;
        case sdp.SdpPackDataType_StructEnd:
            ret = 'struct end';
            break;
        case sdp.SdpPackDataType_Bytes:
            ret = 'bytes';
            break;
        default:break;
    }
    return ret;
};

// 编解码类
sdp.SdpPacker = function() {
    this._buff = new sdp.Buffer;
};
sdp.SdpPacker.prototype.reset = function() {
    this._buff.reset();
};
sdp.SdpPacker.prototype.getBuff = function() {
    return this._buff;
};
sdp.SdpPacker.prototype._pack32 = function(tag, val) {
    if (val < 0) {
        this._packHeader(tag, sdp.SdpPackDataType_Integer_Negative);
        this._buff.writeUint32(-val);
    } else {
        this._packHeader(tag, sdp.SdpPackDataType_Integer_Positive);
        this._buff.writeUint32(val);
    }
};
sdp.SdpPacker.prototype._packHeader = function(tag, type) {
    tag = +tag;
    type = +type;
    var header = type << 4;
    if (tag < 15) {
        header |= tag;
        this._buff.writeByte(header);
    } else {
        header |= 0xF;
        this._buff.writeByte(header);
        this._buff.writeUint32(tag);
    }
};
sdp.SdpPacker.prototype._pack64 = function(tag, val) {
    if (val.isNegative()) {
        this._packHeader(tag, sdp.SdpPackDataType_Integer64_Negative);
        this._buff.writeUint64(val.toUnsigned());
    } else {
        this._packHeader(tag, sdp.SdpPackDataType_Integer64_Positive);
        this._buff.writeUint64(val);
    }
};
sdp.SdpPacker.prototype.writeBoolean = function(tag, require, val) {
    this._pack32(tag, val?1:0);
};
sdp.SdpPacker.prototype.writeInt8 = function(tag, require, val) {
    this._pack32(tag, val);
};
sdp.SdpPacker.prototype.writeUint8 = function(tag, require, val) {
    this._pack32(tag, val);
};
sdp.SdpPacker.prototype.writeInt16 = function(tag, require, val) {
    this._pack32(tag, val);
};
sdp.SdpPacker.prototype.writeUint16 = function(tag, require, val) {
    this._pack32(tag, val);
};
sdp.SdpPacker.prototype.writeInt32 = function(tag, require, val) {
    this._pack32(tag, val);
};
sdp.SdpPacker.prototype.writeUint32 = function(tag, require, val) {
    this._pack32(tag, val);
};
sdp.SdpPacker.prototype.writeInt64 = function(tag, require, val) {
    this._pack64(tag, val);
};
sdp.SdpPacker.prototype.writeUint64 = function(tag, require, val) {
    this._pack64(tag, val);
};
sdp.SdpPacker.prototype.writeFloat = function(tag, require, val) {
    this._packHeader(tag, sdp.SdpPackDataType_Float);
    this._buff.writeFloat(val);
};
sdp.SdpPacker.prototype.writeDouble = function(tag, require, val) {
    this._packHeader(tag, sdp.SdpPackDataType_Double);
    this._buff.writeDouble(val);
};
sdp.SdpPacker.prototype.writeString = function(tag, require, val) {
    this._packHeader(tag, sdp.SdpPackDataType_String);
    this._buff.writeString(val);
};
sdp.SdpPacker.prototype.writeVector = function(tag, require, val) {
    this._packHeader(tag, sdp.SdpPackDataType_Vector);
    this._buff.writeUint32(val.length);
    for (var i = 0; i < val.length; ++i) {
        val._proto._write(this, 0, val.value[i]);
    }
};
sdp.SdpPacker.prototype.writeMap = function(tag, require, val) {
    this._packHeader(tag, sdp.SdpPackDataType_Map);
    this._buff.writeUint32(val.size());
    var self = this;
    val.forEach(function (k, v) {
        val._kproto._write(self, 0, k);
        val._vproto._write(self, 0, v);
    });
};
sdp.SdpPacker.prototype.writeStruct = function(tag, require, val) {
    if (require) {
        this._packHeader(tag, sdp.SdpPackDataType_StructBegin);
        val._writeTo(this);
        this._packHeader(0, sdp.SdpPackDataType_StructEnd);
    } else {
        var p1 = this._buff._pos;
        this._packHeader(tag, sdp.SdpPackDataType_StructBegin);
        
        var p2 = this._buff._pos;
        val._writeTo(this);
        
        if (this._buff._pos == p2) {
            this._buff._pos = p1;
            this._buff._len = this._buff._pos;
        } else {
            this._packHeader(0, sdp.SdpPackDataType_StructEnd);
        }
    }
};
sdp.SdpPacker.prototype.writeBytes = function(tag, require, val) {
    this._packHeader(tag, sdp.SdpPackDataType_Bytes);
    this._buff.writeBytes(val);
};

sdp.SdpUnPacker = function(buff) {
    if (buff) {
        this._buff = buff;
        this._buff._pos = 0;
    }
};
sdp.SdpUnPacker.prototype.setBuff = function(buff) {
    if (buff instanceof sdp.Buffer) {
        this._buff = buff;
    } else if (buff instanceof ArrayBuffer) {
        this._buff = new sdp.Buffer(buff);
    } else {
        throw sdp.SdpException("UnSupport buff type");
    }
    this._buff._pos = 0;
};
sdp.SdpUnPacker.prototype._readHeader = function() {
    var type = this._buff.readByte();
    var tag = type&0xF;
    if (tag == 0xf) {
        tag = this._buff.readUint32();
    }
    type = (type&0xF0)>>>4;
    return {tag:tag,type:type};
};
sdp.SdpUnPacker.prototype._peekHeader = function() {
    var pos = this._buff._pos;
    var header = this._readHeader();
    header.size = this._buff._pos - pos;
    this._buff._pos = pos;
    return header;
};
sdp.SdpUnPacker.prototype._skipTag = function(type) {
    switch (type) {
        case sdp.SdpPackDataType_Integer_Negative:
        case sdp.SdpPackDataType_Integer_Positive:
            this._buff.readUint32();
            break;
        case sdp.SdpPackDataType_Integer64_Negative:
        case sdp.SdpPackDataType_Integer64_Positive:
            this._buff.readUint64();
            break;
        case sdp.SdpPackDataType_Float:
            this._buff.readFloat();
            break;
        case sdp.SdpPackDataType_Double:
            this._buff.readDouble();
            break;
        case sdp.SdpPackDataType_String:
            this._buff.readString();
            break;
        case sdp.SdpPackDataType_Bytes:
            this._buff.readBytes();
            break;
        case sdp.SdpPackDataType_Vector:
            var size = this._buff.readUint32();
            for (var i = 0; i < size; ++i) {
                this._skipTag2();
            }
            break;
        case sdp.SdpPackDataType_Map:
            var size = this._buff.readUint32();
            for (var i = 0; i < size; ++i) {
                this._skipTag2();
                this._skipTag2();
            }
            break;
        case sdp.SdpPackDataType_StructBegin:
            this._skipToStructEnd();
            break;
        case sdp.SdpPackDataType_StructEnd:
            break;
        default:
            throw sdp.SdpUnkownPackType(type);
    }
    //console.log('skip type:' + sdp.packTypeToString(type));
};
sdp.SdpUnPacker.prototype._skipTag2 = function() {
    var header = this._readHeader();
    this._skipTag(header.type);
};
sdp.SdpUnPacker.prototype._skipToStructEnd = function() {
    while (true) {
        var header = this._readHeader();
        if (header.type == sdp.SdpPackDataType_StructEnd) {
            break;
        }
        this._skipTag(header.type);
    }
};
sdp.SdpUnPacker.prototype._skipToTag = function(tag, require) {
    while (this._buff._pos < this._buff._len) {
        //console.log('pos:'+this._buff._pos+',len:'+this._buff._len);
        var header = this._peekHeader();
        //console.log('tag:'+header.tag+',size:'+header.size+',type:'+sdp.packTypeToString(header.type));
        if (header.type == sdp.SdpPackDataType_StructEnd
        || header.tag > tag) {
            break;
        }
        
        if (header.tag == tag) {
            return true;
        }
        this._buff._pos += header.size;
        this._skipTag(header.type);
        //console.log('pos:'+this._buff._pos+',len:'+this._buff._len);
    }
    if (require) {
        throw new sdp.SdpRequireNotExist(tag);
    }
    return false;
};
sdp.SdpUnPacker.prototype.readBoolean = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    if (header.type != sdp.SdpPackDataType_Integer_Positive) {
        throw new sdp.SdpMismatchType(tag, header.type, sdp.SdpPackDataType_Integer_Positive);
    }
    return this._buff.readUint32() ? true : false;
};
sdp.SdpUnPacker.prototype.readInt8 = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    def = this._buff.readUint32();
    if (header.type == sdp.SdpPackDataType_Integer_Negative) def = -def;
    return def;
};
sdp.SdpUnPacker.prototype.readUint8 = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    def = this._buff.readUint32();
    if (header.type == sdp.SdpPackDataType_Integer_Negative) def = -def;
    return def;
};
sdp.SdpUnPacker.prototype.readInt16 = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    def = this._buff.readUint32();
    if (header.type == sdp.SdpPackDataType_Integer_Negative) def = -def;
    return def;
};
sdp.SdpUnPacker.prototype.readUint16 = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    def = this._buff.readUint32();
    if (header.type == sdp.SdpPackDataType_Integer_Negative) def = -def;
    return def;
};
sdp.SdpUnPacker.prototype.readInt32 = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    def = this._buff.readUint32();
    if (header.type == sdp.SdpPackDataType_Integer_Negative) {
        def = -def;
    }
    return def;
};
sdp.SdpUnPacker.prototype.readUint32 = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    this._readHeader();
    return this._buff.readUint32();
};
sdp.SdpUnPacker.prototype.readInt64 = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    def = this._buff.readUint64();
    if (header.type == sdp.SdpPackDataType_Integer64_Negative) {
        def = def.toSigned();
    }
    return def;
};
sdp.SdpUnPacker.prototype.readUint64 = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    def = this._buff.readUint64();
    return def;
};
sdp.SdpUnPacker.prototype.readFloat = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    if (header.type != sdp.SdpPackDataType_Float) {
        throw new sdp.SdpMismatchType(tag, header.type, sdp.SdpPackDataType_Float);
    }
    def = this._buff.readFloat();
    return def;
};
sdp.SdpUnPacker.prototype.readDouble = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    if (header.type != sdp.SdpPackDataType_Double) {
        throw new sdp.SdpMismatchType(tag, header.type, sdp.SdpPackDataType_Double);
    }
    def = this._buff.readDouble();
    return def;
};
sdp.SdpUnPacker.prototype.readString = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    if (header.type != sdp.SdpPackDataType_String) {
        throw new sdp.SdpMismatchType(tag, header.type, sdp.SdpPackDataType_String);
    }
    def = this._buff.readString();
    return def;
};
sdp.SdpUnPacker.prototype.readVector = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    if (header.type != sdp.SdpPackDataType_Vector) {
        throw new sdp.SdpMismatchType(tag, header.type, sdp.SdpPackDataType_Vector);
    }
    
    var size = this._buff.readUint32();
    var ret = new sdp.Vector(def._proto);
    for (var i = 0; i < size; ++i) {
        ret.value.push(ret._proto._read(this, 0, ret._proto));
    }
    return ret;
};
sdp.SdpUnPacker.prototype.readMap = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return def;
    
    var header = this._readHeader();
    if (header.type != sdp.SdpPackDataType_Map) {
        throw new sdp.SdpMismatchType(tag, header.type, sdp.SdpPackDataType_Map);
    }
    
    var size = this._buff.readUint32();
    var ret = new sdp.Map(def._kproto, def._vproto);
    for (var i = 0; i < size; ++i) {
        var k = ret._kproto._read(this, 0, ret._kproto);
        var v = ret._vproto._read(this, 0, ret._vproto);
        ret.insert(k,v);
    }
    return ret;
};
sdp.SdpUnPacker.prototype.readStruct = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return new def();
    
    var header = this._readHeader();
    if (header.type != sdp.SdpPackDataType_StructBegin) {
        throw new sdp.SdpMismatchType(tag, header.type, sdp.SdpPackDataType_StructBegin);
    }
    
    var ret = def._readFrom(this);
    this._skipToStructEnd();
    return ret;
};

sdp.SdpUnPacker.prototype.readBytes = function(tag, require, def) {
    if (!this._skipToTag(tag, require)) return new def();
    
    var header = this._readHeader();
    if (header.type != sdp.SdpPackDataType_Bytes) {
        throw new sdp.SdpMismatchType(tag, header.type, sdp.SdpPackDataType_Bytes);
    }
    
    return this._buff.readBytes();
};

sdp.SdpDisplayer = function() {
    this._tab = 0;
    this._ret = "";
};

sdp.SdpDisplayer.prototype._pf = function(name) {
    for (var i = 0; i < this._tab; ++i) {
        this._ret += '\t';
    }
    if (name != null) {
        this._ret += name + ": ";
    }
};

sdp.SdpDisplayer.prototype.display = function(sdpobj) {
    if (!("_classname" in sdpobj)) {
        throw new sdp.SdpException('not a sdp type');
    }
    
    this._ret = "";
    this._tab = 0;
    
    this.displayObj(null, sdpobj);
    return this._ret;
};
sdp.SdpDisplayer.prototype.displayObj = function(name, sdpobj) {
    if (typeof(sdpobj) != 'object' || (sdpobj instanceof sdp.Buffer)) {
        this.displaySimple(name, sdpobj);
    } else {
        if (!("_classname" in sdpobj)) {
            throw new sdp.SdpException('not a sdp type');
        }
        
        var cln = sdpobj._classname;
        //console.log(cln);
        if (cln.indexOf("vector") === 0) {
            this.displayVector(name, sdpobj);
        } else if(cln.indexOf("map") === 0) {
            this.displayMap(name, sdpobj);
        } else {
            var keys = Object.keys(sdpobj);
            if (this._tab != 0 || name != null) {
                this._pf(name);this._ret += "{\n";this._tab += 1;
            }
            for (var i = 0; i < keys.length; ++i) {
                if (keys[i] == "_classname") continue;
                //console.log('k='+keys[i]);
                var v = sdpobj[keys[i]];
                if (typeof(v) != 'object' || v instanceof Long || v instanceof sdp.Buffer) {
                    this.displaySimple(keys[i], v);
                } else {
                    var cln = v._classname;
                    //console.log(cln);
                    if (cln.indexOf("vector") === 0) {
                        this.displayVector(keys[i], v);
                    } else if(cln.indexOf("map") === 0) {
                        this.displayMap(keys[i], v);
                    } else {
                        this.displayObj(keys[i], v);
                    }
                }
            }
            if (this._tab != 0 || name != null) {
                this._tab -= 1;this._pf(null);this._ret += "}\n";
            }
        }
    }
};
sdp.SdpDisplayer.prototype.displaySimple = function(name, val) {
    this._pf(name);
    this._ret += val.toString() + "\n";
};
sdp.SdpDisplayer.prototype.displayVector = function(name, vec) {
    this._pf(name);this._ret += vec.length + ",[\n";
    
    this._tab += 1;
    var self = this;
    vec.forEach(function(v,index) {
        self.displayObj(null, v);
    });
    this._tab -= 1;
    
    this._pf(null);this._ret += "]\n";
};
sdp.SdpDisplayer.prototype.displayMap = function(name, map) {
    this._pf(name);this._ret += map.size() + ",{\n";
    
    this._tab += 1;
    var self = this;
    map.forEach(function(k,v) {
        self._pf(null);self._ret += "(\n";self._tab += 1;
        self.displayObj(null, k);
        self.displayObj(null, v);
        self._tab -= 1;self._pf(null);self._ret += ")\n";
    });
    this._tab -= 1;
    
    this._pf(null);this._ret += "}\n";
};

// 打包一个sdp结构体
sdp.pack = function(obj) {
    if (!sdp._PACKER) {
        sdp._PACKER = new sdp.SdpPacker;
    }
    var packer = sdp._PACKER;
    packer.reset();
    packer.writeStruct(0, true, obj);
    return packer.getBuff().getBuff();
};

// 解包一个sdp结构体
sdp.unpack = function(ab, proto) {
    if (!sdp._UNPACKER) {
        sdp._UNPACKER = new sdp.SdpUnPacker;
    }
    var unpacker = sdp._UNPACKER;
    unpacker.setBuff(ab);
    return unpacker.readStruct(0, true, proto);
};

// 显示一个sdp结构体内容
sdp.display = function(obj) {
    if (!sdp._DISPLAY) {
        sdp._DISPLAY = new sdp.SdpDisplayer;
    }
    var displayer = sdp._DISPLAY;
    return displayer.display(obj);
};