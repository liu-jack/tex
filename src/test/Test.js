var sdp = require('sdp');

var Test = window.Test || {};
window.Test = Test;

Test.NUMBER_1 = 1;
Test.NUMBER_2 = Test.NUMBER_1 + 1;

Test.Student = function() {
	this.iUid = Long.UZERO;
	this.sName = '';
	this.iAge = 0;
	this.mSecret = new sdp.Map(sdp.String,sdp.String);
};
Test.Student._classname = 'Test.Student';
Test.Student.prototype._classname = 'Test.Student';
Test.Student._write = function(os, tag, val) {
	os.writeStruct(tag, true, val);
};

Test.Student._read = function(is, tag, def) {
	return is.readStruct(tag, true, def);
};

Test.Student._readFrom = function(is) {
	var ret = new Test.Student;
	ret.iUid = is.readUint64(0, false, Long.UZERO);
	ret.sName = is.readString(1, false, '');
	ret.iAge = is.readUint32(2, false, 0);
	ret.mSecret = is.readMap(3, false, sdp.Map(sdp.String,sdp.String));
	return ret;
};

Test.Student.prototype._writeTo = function(os) {
	if (this.iUid != Long.UZERO) os.writeUint64(0, false, this.iUid);
	if (this.sName != '') os.writeString(1, false, this.sName);
	if (this.iAge != 0) os.writeUint32(2, false, this.iAge);
	if (this.mSecret.size() != 0) os.writeMap(3, false, this.mSecret);
};
Test.Teacher = function() {
	this.iId = 0;
	this.sName = '';
};
Test.Teacher._classname = 'Test.Teacher';
Test.Teacher.prototype._classname = 'Test.Teacher';
Test.Teacher._write = function(os, tag, val) {
	os.writeStruct(tag, true, val);
};

Test.Teacher._read = function(is, tag, def) {
	return is.readStruct(tag, true, def);
};

Test.Teacher._readFrom = function(is) {
	var ret = new Test.Teacher;
	ret.iId = is.readUint32(0, false, 0);
	ret.sName = is.readString(1, false, '');
	return ret;
};

Test.Teacher.prototype._writeTo = function(os) {
	if (this.iId != 0) os.writeUint32(0, false, this.iId);
	if (this.sName != '') os.writeString(1, false, this.sName);
};
Test.Teachers = function() {
	this.vTeacher = new sdp.Vector(Test.Teacher);
};
Test.Teachers._classname = 'Test.Teachers';
Test.Teachers.prototype._classname = 'Test.Teachers';
Test.Teachers._write = function(os, tag, val) {
	os.writeStruct(tag, true, val);
};

Test.Teachers._read = function(is, tag, def) {
	return is.readStruct(tag, true, def);
};

Test.Teachers._readFrom = function(is) {
	var ret = new Test.Teachers;
	ret.vTeacher = is.readVector(0, false, sdp.Vector(Test.Teacher));
	return ret;
};

Test.Teachers.prototype._writeTo = function(os) {
	if (this.vTeacher.length != 0) os.writeVector(0, false, this.vTeacher);
};
Test.Class = function() {
	this.iId = 0;
	this.sName = '';
	this.vStudent = new sdp.Vector(Test.Student);
	this.vData = new sdp.Buffer;
};
Test.Class._classname = 'Test.Class';
Test.Class.prototype._classname = 'Test.Class';
Test.Class._write = function(os, tag, val) {
	os.writeStruct(tag, true, val);
};

Test.Class._read = function(is, tag, def) {
	return is.readStruct(tag, true, def);
};

Test.Class._readFrom = function(is) {
	var ret = new Test.Class;
	ret.iId = is.readUint32(0, true, 0);
	ret.sName = is.readString(1, false, '');
	ret.vStudent = is.readVector(2, false, sdp.Vector(Test.Student));
	ret.vData = is.readBytes(3, false, sdp.Buffer);
	return ret;
};

Test.Class.prototype._writeTo = function(os) {
	os.writeUint32(0, true, this.iId);
	if (this.sName != '') os.writeString(1, false, this.sName);
	if (this.vStudent.length != 0) os.writeVector(2, false, this.vStudent);
	if (this.vData.length != 0) os.writeBytes(3, false, this.vData);
};
