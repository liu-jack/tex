#ifndef _SDP_PHP_IMP_H_
#define _SDP_PHP_IMP_H_

#include <string>
#include <iostream>
#include "php_sdpphp.h"
#include "Sdp.h"
#include "SdpTypeId.h"
using namespace std;
using namespace mfw;
using namespace mfw::sdp;

class AuxNewZVal
{
public:
	AuxNewZVal() : m_val(NULL) {}
	~AuxNewZVal()
	{
		if (m_val != NULL)
		{
			zval_ptr_dtor(&m_val);
			m_val = NULL;
		}
	}

	zval *make() { ALLOC_INIT_ZVAL(m_val); return m_val; }
	zval *get() const { return m_val; }
	zval *take() { zval *t = m_val; m_val = NULL; return t; }

private:
	zval *m_val;
};

class AuxTypeId
{
public:
	AuxTypeId() : m_iType(0) {}
	explicit AuxTypeId(zval *pType);
	explicit AuxTypeId(const string &sType);

	uint32_t getTypeId() const { return m_iType; }
	const string &getTypeStr() const { return m_sType; }

	const string &getSubType1() const { return m_sSubType1; }
	const string &getSubType2() const { return m_sSubType2; }

private:
	void initialize(const string &sType);

private:
	uint32_t m_iType;
	string m_sType;
	string m_sSubType1;
	string m_sSubType2;
};

class PhpSdpValue
{
public:
	PhpSdpValue(zval *pCurValue, const AuxTypeId &auxType) : m_pCurValue(pCurValue), m_auxType(auxType) {}

protected:
	zval *m_pCurValue;
	AuxTypeId m_auxType;
};

class PhpSdpValueWriter : public PhpSdpValue
{
public:
	PhpSdpValueWriter(zval *pCurValue, const AuxTypeId &auxType, zval *pDefaultValue = NULL) : PhpSdpValue(pCurValue, auxType), m_pDefaultValue(pDefaultValue) {}

	template <typename Writer>
	void visit(Writer &writer, uint32_t tag, bool require, const char *name) const;

private:
	zval *m_pDefaultValue;
};

class PhpSdpValueReader : public PhpSdpValue
{
public:
	PhpSdpValueReader(zval *pCurValue, const AuxTypeId &auxType) : PhpSdpValue(pCurValue, auxType) {}

	template <typename Reader>
	void visit(Reader &reader, uint32_t tag, bool require, const char *name);

	bool hasNewValue() const { return m_newval.get() != NULL; }
	zval *getNewValue() const { return m_newval.get(); }
	zval *takeNewValue() { return m_newval.take(); }

private:
	AuxNewZVal m_newval;
};

class PhpSdpStruct
{
public:
	PhpSdpStruct() : m_val(NULL) {}
	explicit PhpSdpStruct(zval *val) : m_val(val) {}

	template <typename T, bool IsWrite>
	void visit(T &t, bool bOpt) const;

protected:
	zval *m_val;
};

class PhpSdpStructWriter : public PhpSdpStruct
{
public:
	PhpSdpStructWriter() {}
	explicit PhpSdpStructWriter(zval *val) : PhpSdpStruct(val) {}

	template <typename Writer>
	void visit(Writer &t, bool bOpt) const;
};

class PhpSdpStructReader : public PhpSdpStruct
{
public:
	PhpSdpStructReader() {}
	explicit PhpSdpStructReader(zval *val) : PhpSdpStruct(val) {}

	template <typename Reader>
	void visit(Reader &t, bool bOpt) const;
};

class PhpSdpVector
{
public:
	PhpSdpVector() : m_pValue(NULL) {}
	PhpSdpVector(zval *pValue, const AuxTypeId &auxVectorType) : m_pValue(pValue), m_auxInnerType(auxVectorType.getSubType1()) {}

protected:
	zval *m_pValue;
	AuxTypeId m_auxInnerType;
};

class PhpSdpVectorWriter : public PhpSdpVector
{
public:
	PhpSdpVectorWriter() : m_iVectorSize(0), m_iCurIndex(0) {}
	PhpSdpVectorWriter(zval *pValue, const AuxTypeId &auxVectorType);

	uint32_t size() const { return m_iVectorSize; }
	bool next() const;

	template <typename Writer>
	void visit(Writer &t, uint32_t tag, bool require, const char *name) const;

private:
	uint32_t m_iVectorSize;
	mutable uint32_t m_iCurIndex;
};

class PhpSdpVectorReader : public PhpSdpVector
{
public:
	PhpSdpVectorReader() : m_iCurIndex(0) {}
	PhpSdpVectorReader(zval *pValue, const AuxTypeId &auxVectorType) : PhpSdpVector(pValue, auxVectorType), m_iCurIndex(0) {}

	template <typename Reader>
	void visit(Reader &t, uint32_t tag, bool require, const char *name) const;

private:
	mutable uint32_t m_iCurIndex;
};

class PhpSdpMap
{
public:
	PhpSdpMap() : m_pValue(NULL) {}
	PhpSdpMap(zval *pValue, const AuxTypeId &auxMapType) : m_pValue(pValue), m_auxKeyType(auxMapType.getSubType1()), m_auxValType(auxMapType.getSubType2()) {}

protected:
	zval *m_pValue;
	AuxTypeId m_auxKeyType;
	AuxTypeId m_auxValType;
};

class PhpSdpMapWriter : public PhpSdpMap
{
public:
	PhpSdpMapWriter() : m_iMapSize(0), m_iCurIndex(0) {}
	PhpSdpMapWriter(zval *pValue, const AuxTypeId &auxMapType);

	uint32_t size() const { return m_iMapSize; }
	bool next() const;

	template <typename Writer>
	void visitKey(Writer &t, uint32_t tag, bool require, const char *name) const;

	template <typename Writer>
	void visitVal(Writer &t, uint32_t tag, bool require, const char *name) const;

private:
	void sortKey();

private:
	uint32_t m_iMapSize;
	vector<zval> m_vSortKey;
	mutable uint32_t m_iCurIndex;
};

class PhpSdpMapReader : public PhpSdpMap
{
public:
	PhpSdpMapReader() : m_iCurIndex(0) {}
	PhpSdpMapReader(zval *pValue, const AuxTypeId &auxMapType) : PhpSdpMap(pValue, auxMapType), m_iCurIndex(0) {}

	template <typename Reader>
	void visit(Reader &t, uint32_t tag, bool require, const char *name);

private:
	mutable uint32_t m_iCurIndex;
};

ZEND_FUNCTION(printSdp);
ZEND_FUNCTION(sdpToString);
ZEND_FUNCTION(stringToSdp);

#endif
