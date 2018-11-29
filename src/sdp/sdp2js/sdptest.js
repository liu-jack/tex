var sdp = require('sdp');
var Long = require('long');
require('Test');

var sdptest = sdptest || {};
module.exports = sdptest;

sdptest.test = function() {
    try {
        var packer = new sdp.SdpPacker();
        
        /*// 测试bool
        packer.writeBoolean(0, true, false);
        packer.writeBoolean(1, true, false);
        packer.writeBoolean(19, true, true);
        
        // 测试int8
        packer.writeInt8(20, true, -128);
        packer.writeInt8(21, true, 0);
        packer.writeInt8(22, true, 127);
        packer.writeUint8(23, true, 0);
        packer.writeUint8(24, true, 255);
        
        // 测试int16
        packer.writeInt16(25, true, -32768);
        packer.writeInt16(26, true, 32767);
        packer.writeUint16(27, true, 0);
        packer.writeUint16(28, true, 65535);
        
        // 测试int32
        packer.writeInt32(29, true, -2147483648);
        packer.writeInt32(30, true, 2147483647);
        packer.writeUint32(31, true, 0);
        packer.writeUint32(32, true, 4294967295);
        
        // 测试int64
        packer.writeInt64(33, true, Long.MAX_VALUE);
        packer.writeInt64(34, true, Long.MIN_VALUE);
        packer.writeUint64(35, true, Long.MAX_UNSIGNED_VALUE);
        
        // 测试float
        packer.writeFloat(36, true, -1.02);
        packer.writeFloat(37, true, 110.4);
        
        // 测试Double
        packer.writeDouble(38, true, 10000.4);
        packer.writeDouble(39, true, -9802883.01);
        
        // 测试字符串
        packer.writeString(40, true, "我是字符串");
        packer.writeString(41, true, "adb");
        
        // 测试vector
        var vBool = new sdp.Vector(sdp.Boolean);
        vBool.push(true);vBool.push(false);
        packer.writeVector(42, true,vBool);
        
        var vIn8 = new sdp.Vector(sdp.Int8);
        vIn8.push(127);vIn8.push(-128);
        packer.writeVector(43, true,vIn8);
        var vUin8 = new sdp.Vector(sdp.Uint8);
        vUin8.push(0);vUin8.push(255);
        packer.writeVector(44, true,vUin8);
        
        var vIn16 = new sdp.Vector(sdp.Int16);
        vIn16.push(32767);vIn16.push(-32768);
        packer.writeVector(45, true,vIn16);
        var vUin16 = new sdp.Vector(sdp.Uint16);
        vUin16.push(0);vUin16.push(65535);
        packer.writeVector(46, true,vUin16);
        
        var vIn32 = new sdp.Vector(sdp.Int32);
        vIn32.push(2147483647);vIn32.push(-2147483648);
        packer.writeVector(47, true,vIn32);
        var vUin32 = new sdp.Vector(sdp.Uint32);
        vUin32.push(0);vUin32.push(4294967295);
        packer.writeVector(48, true,vUin32);
        
        var vIn64 = new sdp.Vector(sdp.Int64);
        vIn64.push(Long.MIN_VALUE);vIn64.push(Long.MAX_VALUE);
        packer.writeVector(49, true,vIn64);
        var vUin64 = new sdp.Vector(sdp.Uint64);
        vUin64.push(Long.UZERO);vUin64.push(Long.MAX_UNSIGNED_VALUE);
        packer.writeVector(50, true,vUin64);
        
        var vF = new sdp.Vector(sdp.Float);
        vF.push(1.2);vF.push(-4.5);vF.push(14.5);
        packer.writeVector(51, true,vF);
        
        var vD = new sdp.Vector(sdp.Double);
        vD.push(1.23);vD.push(-4.51);vD.push(140.5);
        packer.writeVector(52, true,vD);
        
        var vS = new sdp.Vector(sdp.String);
        vS.push("yellia");vS.push("hello");vS.push("world");
        packer.writeVector(53, true,vS);
        
        // 测试map
        var mm = new sdp.Map(sdp.String,sdp.String);
        mm.set("k1", "v1");
        mm.set("k2", "v2");
        packer.writeMap(54, true, mm);
        
        var mm = new sdp.Map(sdp.Uint32,sdp.String);
        mm.set(111, "v1");
        mm.set(111, "v2");
        mm.set(22, "v2");
        packer.writeMap(55, true, mm);
        
        var mm = new sdp.Map(sdp.Uint32,sdp.Uint32);
        mm.set(111, 123);
        mm.set(22, 91);
        packer.writeMap(56, true, mm);
        
        var mm = new sdp.Map(sdp.Int32,sdp.Uint32);
        mm.set(-111, 123);
        mm.set(22, 91);
        packer.writeMap(57, true, mm);
        
        var mm = new sdp.Map(sdp.Int32,sdp.Uint64);
        mm.set(22, Long.fromString("1234", true, 16));
        mm.set(12, Long.fromString("2345", true, 16));
        packer.writeMap(58, true, mm);*/
        
        // 测试结构体
        var s1 = new Test.Student;
        s1.iUid = Long.fromString("1234567890",true);
        s1.sName = "学生1";
        s1.iAge = 12;
        s1.mSecret.set("arank","1");
        s1.mSecret.set("bhasphone", "false");
        packer.writeStruct(59, true, s1);
        
        var s2 = new Test.Student;
        s2.iUid = Long.fromString("1234",true);
        s2.sName = "学生2";
        s2.iAge = 10;
        s2.mSecret.set("rank","2");
        
        var t1 = new Test.Teacher;
        t1.iId = 1;
        t1.sName = "老师1";
        var t2 = new Test.Teacher;
        t2.iId = 2;
        t2.sName = "老师2";
        var ts = new Test.Teachers;
        ts.vTeacher.push(t1);
        ts.vTeacher.push(t2);
        
        var c1 = new Test.Class;
        c1.iId = 1;
        c1.sName = "一年级一班";
        c1.vStudent.push(s1);
        c1.vStudent.push(s2);
        c1.vData = new sdp.Buffer(sdp.pack(ts));
        packer.writeStruct(60, true, c1);
        
        packer.getBuff().printhex();
        
        var unpacker = new sdp.SdpUnPacker(packer.getBuff());
        /*console.log(unpacker.readBoolean(0, true, false));
        console.log(unpacker.readBoolean(1, true, false));
        console.log(unpacker.readBoolean(10, false, false));
        console.log(unpacker.readBoolean(19, true, false));
        
        console.log(unpacker.readInt8(20, true, 0));
        console.log(unpacker.readInt8(21, true, 0));
        console.log(unpacker.readInt8(22, true, 0));
        console.log(unpacker.readUint8(23, true, 0));
        console.log(unpacker.readUint8(24, true, 0));
        
        console.log(unpacker.readInt16(25, true, 0));
        console.log(unpacker.readInt16(26, true, 0));
        console.log(unpacker.readUint16(27, true, 0));
        console.log(unpacker.readUint16(28, true, 0));
        
        console.log(unpacker.readInt32(29, true, 0));
        console.log(unpacker.readInt32(30, true, 0));
        console.log(unpacker.readUint32(31, true, 0));
        console.log(unpacker.readUint32(32, true, 0));
        
        console.log(unpacker.readUint64(33, true, 0).toString(16));
        console.log(unpacker.readInt64(34, true, 0).toString(16));
        console.log(unpacker.readUint64(35, true, 0).toString(16));
        
        //console.log(unpacker.readFloat(36, true, 0));
        //console.log(unpacker.readFloat(37, true, 0));
        
        //console.log(unpacker.readDouble(38, true, 0));
        //console.log(unpacker.readDouble(39, true, 0));
        
        console.log(unpacker.readString(40, true, ""));
        console.log(unpacker.readString(41, true, ""));
        
        var vv = unpacker.readVector(42, true, sdp.Vector(sdp.Boolean));
        vv.forEach(function(v,index) {
            console.log("i="+index+",v="+v);
        });
        
        var vv = unpacker.readVector(43, true, sdp.Vector(sdp.Int8));
        vv.forEach(function(v,index) {
            console.log("i="+index+",v="+v);
        });
        var vv = unpacker.readVector(44, true, sdp.Vector(sdp.Uint8));
        vv.forEach(function(v,index) {
            console.log("i="+index+",v="+v);
        });
        
        var vv = unpacker.readVector(45, true, sdp.Vector(sdp.Int16));
        vv.forEach(function(v,index) {
            console.log("i="+index+",v="+v);
        });
        var vv = unpacker.readVector(46, true, sdp.Vector(sdp.Uint16));
        vv.forEach(function(v,index) {
            console.log("i="+index+",v="+v);
        });
        
        var vv = unpacker.readVector(47, true, sdp.Vector(sdp.Int32));
        vv.forEach(function(v,index) {
            console.log("i="+index+",v="+v);
        });
        var vv = unpacker.readVector(48, true, sdp.Vector(sdp.Uint32));
        vv.forEach(function(v,index) {
            console.log("i="+index+",v="+v);
        });
        
        var vv = unpacker.readVector(49, true, sdp.Vector(sdp.Int64));
        vv.forEach(function(v,index) {
            console.log("i="+index+",v="+v.toString(16));
        });
        var vv = unpacker.readVector(50, true, sdp.Vector(sdp.Uint64));
        vv.forEach(function(v,index) {
            console.log("i="+index+",v="+v.toString(16));
        });
        
        //var vv = unpacker.readVector(51, true, sdp.Vector(sdp.Float));
        //vv.forEach(function(v,index) {
        //    console.log("i="+index+",v="+v);
        //});
        
        //var vv = unpacker.readVector(52, true, sdp.Vector(sdp.Double));
        //vv.forEach(function(v,index) {
        //    console.log("i="+index+",v="+v);
        //});
        
        var vv = unpacker.readVector(53, true, sdp.Vector(sdp.String));
        vv.forEach(function(v,index) {
            console.log("i="+index+",v="+v);
        });
        
        var mm = unpacker.readMap(54, true, sdp.Map(sdp.String,sdp.String));
        mm.forEach(function(k,v) {
            console.log(k+"="+v);
        });
        
        var mm = unpacker.readMap(55, true, sdp.Map(sdp.Uint32,sdp.String));
        mm.forEach(function(k,v) {
            console.log(k+"="+v);
        });
        
        var mm = unpacker.readMap(56, true, sdp.Map(sdp.Uint32,sdp.Uint32));
        mm.forEach(function(k,v) {
            console.log(k+"="+v);
        }); 
        
        var mm = unpacker.readMap(57, true, sdp.Map(sdp.Int32,sdp.Uint32));
        mm.forEach(function(k,v) {
            console.log(k+"="+v);
        });

        var mm = unpacker.readMap(58, true, sdp.Map(sdp.Int32,sdp.Uint64));
        mm.forEach(function(k,v) {
            console.log(k+"="+v.toString(16));
        });*/
        
        console.log(sdp.display(unpacker.readStruct(59, true, Test.Student)));
        
        var c1 = unpacker.readStruct(60, true, Test.Class);
        console.log(sdp.display(c1));
        
        var ts = sdp.unpack(c1.vData, Test.Teachers);
        console.log(sdp.display(ts));
        
    } catch (e) {
        console.log(e.message);
    }
};