#include "sdpphp_imp.h"
#include <algorithm>

//#define DEBUG(x) do { cout << "*" << __LINE__ << "\t" << x << endl; } while(0)
#define DEBUG(x) do { } while (0)

#define SDP_THROW(msg) do { throw std::runtime_error(string("invalid sdp struct: ") + msg); } while(0)
#define CHECK_TYPE_OR_THROW(curtype, reqtype) do { if ((curtype) != (reqtype)) SDP_THROW("type mismatch"); } while(0)

static uint64_t parseNumber64(const char *s)
{
    return strtoull(s, NULL, 0);
}

static uint64_t parseNumber64(const string &s)
{
    return parseNumber64(s.c_str());
}

static char *printNumber64(char *buf, int size, int64_t val)
{
    snprintf(buf, size, "%lld", (long long)val);
    return buf;
}

static char *printNumber64(char *buf, int size, uint64_t val)
{
    snprintf(buf, size, "%llu", (unsigned long long)val);
    return buf;
}

static bool auxBoolValue(const zval *val)
{
    if (Z_TYPE_P(val) == IS_BOOL) {
        return Z_BVAL_P(val);
    }

    zval tmp = *val;
    convert_to_boolean(&tmp);
    return Z_BVAL(tmp);
}

static ulong auxLongValue(const zval *val)
{
    if (Z_TYPE_P(val) == IS_LONG) {
        return Z_LVAL_P(val);
    }

    zval tmp = *val;
    convert_to_long(&tmp);
    return Z_LVAL(tmp);
}

static double auxDoubleValue(const zval *val)
{
    if (Z_TYPE_P(val) == IS_DOUBLE) {
        return Z_DVAL_P(val);
    }

    zval tmp = *val;
    convert_to_double(&tmp);
    return Z_DVAL(tmp);
}

static string auxStringValue(const zval *val)
{
    if (Z_TYPE_P(val) == IS_STRING) {
        return string(Z_STRVAL_P(val), Z_STRLEN_P(val));
    }

    zval tmp = *val;
    convert_to_string(&tmp);
    string s(Z_STRVAL(tmp), Z_STRLEN(tmp));

    zval_dtor(&tmp);
    return s;
}

static string trim(const string &s)
{
    string::size_type i = 0, j = s.size();
    while (i < j && isspace(s[i])) ++i;
    while (i < j && isspace(s[j - 1])) --j;
    return s.substr(i, j - i);
}

static bool decodeVectorInnerType(const string &sType, string &sInnerType)
{
    const char *p = sType.c_str();
    const char *x = strchr(p, '<');
    const char *y = strrchr(p, '>');
    if (x == NULL || y == NULL || x + 1 >= y) {
        return false;
    }
    sInnerType = trim(string(x + 1, y));
    return !sInnerType.empty();
}

static bool decodeMapInnerType(const string &sType, string &sKeyType, string &sValType)
{
    const char *p = sType.c_str();
    const char *x = strchr(p, '<');
    const char *y = strrchr(p, '>');
    if (x == NULL || y == NULL || x + 1 >= y) {
        return false;
    }
    const char *z = strchr(x, ',');
    if (z == NULL) {
        return false;
    }
    sKeyType = trim(string(x + 1, z));
    sValType = trim(string(z + 1, y));
    return !sKeyType.empty() && !sValType.empty();
}

AuxTypeId::AuxTypeId(zval *pType)
{
    if (Z_TYPE_P(pType) == IS_LONG) {
        m_iType = auxLongValue(pType);

        char buf[32];
        snprintf(buf, sizeof(buf), "%d", m_iType);
        m_sType = buf;
    } else {
        initialize(auxStringValue(pType));
    }
}

AuxTypeId::AuxTypeId(const string &sType)
{
    initialize(sType);
}

void AuxTypeId::initialize(const string &sType)
{
    m_sType = sType;

    uint32_t iType = 0;
    if (sscanf(m_sType.c_str(), "%u", &iType) == 1) {
        m_iType = iType;
    } else {
        if (strncmp(m_sType.c_str(), "vector<", sizeof("vector<") - 1) == 0) {
            m_iType = SdpType_Vector;
            if (!decodeVectorInnerType(m_sType, m_sSubType1)) {
                SDP_THROW("vector type spec");
            }
        } else if (strncmp(m_sType.c_str(), "map<", sizeof("map<") - 1) == 0) {
            m_iType = SdpType_Map;
            if (!decodeMapInnerType(m_sType, m_sSubType1, m_sSubType2)) {
                SDP_THROW("map type spec");
            }
        } else {
            m_iType = SdpType_Struct;
        }
    }
}

template <typename Writer>
void PhpSdpValueWriter::visit(Writer &writer, uint32_t tag, bool require, const char *name) const
{
    uint32_t iType = m_auxType.getTypeId();
    DEBUG("write type: " << iType);

    switch (iType) {
    case SdpType_Bool: {
        bool bVal = auxBoolValue(m_pCurValue);
        if (m_pDefaultValue != NULL) {
            bool bDefVal = auxBoolValue(m_pDefaultValue);
            if (bVal == bDefVal) {
                break;
            }
        }
        writer.visit(tag, require, name, bVal);
    }
    break;
    case SdpType_Char:
    case SdpType_Int8:
    case SdpType_UInt8:
    case SdpType_Int16:
    case SdpType_UInt16:
    case SdpType_Int32:
    case SdpType_UInt32:
    case SdpType_Enum: {
        int64_t iVal = auxLongValue(m_pCurValue);
        if (m_pDefaultValue != NULL) {
            int64_t iDefVal = auxLongValue(m_pDefaultValue);
            if (iVal == iDefVal) {
                break;
            }
        }

        if (iType == SdpType_Char) writer.visit(tag, require, name, (char)iVal);
        else if (iType == SdpType_Int8) writer.visit(tag, require, name, (int8_t)iVal);
        else if (iType == SdpType_UInt8) writer.visit(tag, require, name, (uint8_t)iVal);
        else if (iType == SdpType_Int16) writer.visit(tag, require, name, (int16_t)iVal);
        else if (iType == SdpType_UInt16) writer.visit(tag, require, name, (uint16_t)iVal);
        else if (iType == SdpType_Int32) writer.visit(tag, require, name, (int32_t)iVal);
        else if (iType == SdpType_UInt32) writer.visit(tag, require, name, (uint32_t)iVal);
        else if (iType == SdpType_Enum) writer.visit(tag, require, name, (int32_t)iVal);
    }
    break;
    case SdpType_Float:
    case SdpType_Double: {
        double fVal = auxDoubleValue(m_pCurValue);
        if (m_pDefaultValue != NULL) {
            double fDefVal = auxDoubleValue(m_pDefaultValue);
            if (fVal == fDefVal) {
                break;
            }
        }

        if (iType == SdpType_Float) writer.visit(tag, require, name, (float)fVal);
        else if (iType == SdpType_Double) writer.visit(tag, require, name, (double)fVal);
    }
    break;
    case SdpType_Int64:
    case SdpType_UInt64:
    case SdpType_String: {
        string sVal = auxStringValue(m_pCurValue);
        if (m_pDefaultValue != NULL) {
            string sDefVal = auxStringValue(m_pDefaultValue);
            if (sVal == sDefVal) {
                break;
            }
        }

        if (iType == SdpType_Int64) writer.visit(tag, require, name, (int64_t)parseNumber64(sVal));
        else if (iType == SdpType_UInt64) writer.visit(tag, require, name, (uint64_t)parseNumber64(sVal));
        else if (iType == SdpType_String) writer.visit(tag, require, name, sVal);
    }
    break;
    case SdpType_Vector: {
        if (m_pDefaultValue != NULL) {
            CHECK_TYPE_OR_THROW(Z_TYPE_P(m_pCurValue), IS_ARRAY);
            int iSize = zend_hash_next_free_element(Z_ARRVAL_P(m_pCurValue));
            if (iSize <= 0) {
                break;
            }
        }

        SdpVectorProxy<PhpSdpVectorWriter> proxy;
        proxy.under = PhpSdpVectorWriter(m_pCurValue, m_auxType);
        writer.visit(tag, require, name, proxy);
    }
    break;
    case SdpType_Map: {
        if (m_pDefaultValue != NULL) {
            CHECK_TYPE_OR_THROW(Z_TYPE_P(m_pCurValue), IS_ARRAY);
            int iSize = zend_hash_num_elements(Z_ARRVAL_P(m_pCurValue));
            if (iSize <= 0) {
                break;
            }
        }

        SdpMapProxy<PhpSdpMapWriter> proxy;
        proxy.under = PhpSdpMapWriter(m_pCurValue, m_auxType);
        writer.visit(tag, require, name, proxy);
    }
    break;
    case SdpType_Struct: {
        CHECK_TYPE_OR_THROW(Z_TYPE_P(m_pCurValue), IS_OBJECT);
        SdpStructProxy<PhpSdpStructWriter> proxy;
        proxy.under = PhpSdpStructWriter(m_pCurValue);
        writer.visit(tag, require, name, proxy);
    }
    break;
    case SdpType_Void:
        break;
    }
}

template <typename Reader>
void PhpSdpValueReader::visit(Reader &reader, uint32_t tag, bool require, const char *name)
{
    DEBUG("read type: " << m_auxType.getTypeId());

    bool bIsNilValue = m_pCurValue == NULL ? true : false;
    switch (m_auxType.getTypeId()) {
    case SdpType_Bool: {
        bool oldval = bIsNilValue ? false : auxBoolValue(m_pCurValue);
        bool newval = oldval;
        reader.visit(tag, require, name, newval);
        if (bIsNilValue) {
            m_pCurValue = m_newval.make();
            ZVAL_BOOL(m_pCurValue, newval);
        } else if (newval != oldval) {
            zval_dtor(m_pCurValue);
            ZVAL_BOOL(m_pCurValue, newval);
        }
    }
    break;
    case SdpType_Char:
    case SdpType_Int8:
    case SdpType_UInt8:
    case SdpType_Int16:
    case SdpType_UInt16:
    case SdpType_Int32:
    case SdpType_UInt32: {
        int64_t oldval = bIsNilValue ? 0 : auxLongValue(m_pCurValue);
        int64_t newval = oldval;
        reader.visit(tag, require, name, newval);
        if (bIsNilValue) {
            m_pCurValue = m_newval.make();
            ZVAL_LONG(m_pCurValue, newval);
        } else if (newval != oldval) {
            zval_dtor(m_pCurValue);
            ZVAL_LONG(m_pCurValue, newval);
        }
    }
    break;
    case SdpType_Int64: {
        int64_t oldval = bIsNilValue ? 0 : (int64_t)parseNumber64(auxStringValue(m_pCurValue));
        int64_t newval = oldval;
        reader.visit(tag, require, name, newval);
        char buf[64];
        if (bIsNilValue) {
            m_pCurValue = m_newval.make();
            ZVAL_STRING(m_pCurValue, printNumber64(buf, sizeof(buf), newval), 1);
        } else if (newval != oldval) {
            zval_dtor(m_pCurValue);
            ZVAL_STRING(m_pCurValue, printNumber64(buf, sizeof(buf), newval), 1);
        }
    }
    break;
    case SdpType_UInt64: {
        uint64_t oldval = bIsNilValue ? 0 : (uint64_t)parseNumber64(auxStringValue(m_pCurValue));
        uint64_t newval = oldval;
        reader.visit(tag, require, name, newval);
        char buf[64];
        if (bIsNilValue) {
            m_pCurValue = m_newval.make();
            ZVAL_STRING(m_pCurValue, printNumber64(buf, sizeof(buf), newval), 1);
        } else if (newval != oldval) {
            zval_dtor(m_pCurValue);
            ZVAL_STRING(m_pCurValue, printNumber64(buf, sizeof(buf), newval), 1);
        }
    }
    break;
    case SdpType_Float: {
        float oldval = bIsNilValue ? 0 : auxDoubleValue(m_pCurValue);
        float newval = oldval;
        reader.visit(tag, require, name, newval);
        if (bIsNilValue) {
            m_pCurValue = m_newval.make();
            ZVAL_DOUBLE(m_pCurValue, newval);
        } else if (newval != oldval) {
            zval_dtor(m_pCurValue);
            ZVAL_DOUBLE(m_pCurValue, newval);
        }
    }
    break;
    case SdpType_Double: {
        double oldval = bIsNilValue ? 0 : auxDoubleValue(m_pCurValue);
        double newval = oldval;
        reader.visit(tag, require, name, newval);
        if (bIsNilValue) {
            m_pCurValue = m_newval.make();
            ZVAL_DOUBLE(m_pCurValue, newval);
        } else if (newval != oldval) {
            zval_dtor(m_pCurValue);
            ZVAL_DOUBLE(m_pCurValue, newval);
        }
    }
    break;
    case SdpType_String: {
        string oldval = bIsNilValue ? "" : auxStringValue(m_pCurValue);
        string newval = oldval;
        reader.visit(tag, require, name, newval);
        if (bIsNilValue) {
            m_pCurValue = m_newval.make();
            ZVAL_STRINGL(m_pCurValue, newval.c_str(), newval.size(), 1);
        } else if (newval != oldval) {
            zval_dtor(m_pCurValue);
            ZVAL_STRINGL(m_pCurValue, newval.c_str(), newval.size(), 1);
        }
    }
    break;
    case SdpType_Vector: {
        if (bIsNilValue) {
            m_pCurValue = m_newval.make();
            array_init(m_pCurValue);
        }

        SdpVectorProxy<PhpSdpVectorReader> proxy;
        proxy.under = PhpSdpVectorReader(m_pCurValue, m_auxType);
        reader.visit(tag, require, name, proxy);
    }
    break;
    case SdpType_Map: {
        if (bIsNilValue) {
            m_pCurValue = m_newval.make();
            array_init(m_pCurValue);
        }

        SdpMapProxy<PhpSdpMapReader> proxy;
        proxy.under = PhpSdpMapReader(m_pCurValue, m_auxType);
        reader.visit(tag, require, name, proxy);
    }
    break;
    case SdpType_Struct: {
        if (bIsNilValue) {
            TSRMLS_FETCH();
            zend_class_entry **ce;
            if (zend_lookup_class(m_auxType.getTypeStr().c_str(), m_auxType.getTypeStr().size(), &ce TSRMLS_CC) == FAILURE) {
                SDP_THROW("cannot find class " + m_auxType.getTypeStr());
            }

            m_pCurValue = m_newval.make();
            object_init_ex(m_pCurValue, *ce);

            zval ret, construct;
            ZVAL_STRING(&construct, "__construct", 0);
            if (call_user_function(NULL, &m_pCurValue, &construct, &ret, 0, NULL TSRMLS_CC) == FAILURE) {
                SDP_THROW("cannot construct class " + m_auxType.getTypeStr());
            }
        }

        SdpStructProxy<PhpSdpStructReader> proxy;
        proxy.under = PhpSdpStructReader(m_pCurValue);
        reader.visit(tag, require, name, proxy);
    }
    break;
    }
}

template <bool IsWriteOperation, bool IsWriteCodeBlock>
struct ReaderWriterSelector {
    struct Dummy {
        template <typename T>
        void visit(T &, ...) const {}
    };

    template <typename T> Dummy operator()(T &)
    {
        return Dummy();
    }
};

template <>
struct ReaderWriterSelector <true, true> {
    template <typename T> T &operator()(T &t)
    {
        return t;
    }
};

template <>
struct ReaderWriterSelector <false, false> {
    template <typename T> T &operator()(T &t)
    {
        return t;
    }
};

template <typename T, bool IsWrite>
void PhpSdpStruct::visit(T &t, bool bOpt) const
{
    DEBUG((IsWrite ? "write" : "read") << " struct");

    CHECK_TYPE_OR_THROW(Z_TYPE_P(m_val), IS_OBJECT);
    TSRMLS_FETCH();
    zend_class_entry *ce = Z_OBJCE_P(m_val);
    zval *pDefinition = zend_read_static_property(ce, (char *)"__Definition", sizeof("__Definition") - 1, true TSRMLS_CC);
    if (pDefinition == NULL) {
        SDP_THROW("missing __Definition");
    }

    CHECK_TYPE_OR_THROW(Z_TYPE_P(pDefinition), IS_ARRAY);
    int iFieldNum = zend_hash_next_free_element(Z_ARRVAL_P(pDefinition));
    for (int i = 0; i < iFieldNum; ++i) {
        zval **ppFieldName;
        if (zend_hash_index_find(Z_ARRVAL_P(pDefinition), i, (void **)&ppFieldName) == FAILURE || Z_TYPE_PP(ppFieldName) != IS_STRING) {
            SDP_THROW("missing field name");
        }
        string sName = auxStringValue(*ppFieldName);

        zval **ppFieldDefine;
        if (zend_hash_find(Z_ARRVAL_P(pDefinition), Z_STRVAL_PP(ppFieldName), Z_STRLEN_PP(ppFieldName) + 1, (void **)&ppFieldDefine) == FAILURE || Z_TYPE_PP(ppFieldDefine) != IS_ARRAY) {
            SDP_THROW("missing field define");
        }

        // tag
        zval **ppFieldTag;
        if (zend_hash_index_find(Z_ARRVAL_PP(ppFieldDefine), 0, (void **)&ppFieldTag) == FAILURE || Z_TYPE_PP(ppFieldTag) != IS_LONG) {
            SDP_THROW("missing field tag");
        }
        uint32_t iTag = auxLongValue(*ppFieldTag);

        // required
        zval **ppFieldRequired;
        if (zend_hash_index_find(Z_ARRVAL_PP(ppFieldDefine), 1, (void **)&ppFieldRequired) == FAILURE
                || (Z_TYPE_PP(ppFieldRequired) != IS_LONG && Z_TYPE_PP(ppFieldRequired) != IS_BOOL)) {
            SDP_THROW("missing field required");
        }
        bool bRequired = auxBoolValue(*ppFieldRequired);

        // type
        zval **ppFieldType;
        if (zend_hash_index_find(Z_ARRVAL_PP(ppFieldDefine), 2, (void **)&ppFieldType) == FAILURE
                || (Z_TYPE_PP(ppFieldType) != IS_LONG && Z_TYPE_PP(ppFieldType) != IS_STRING)) {
            SDP_THROW("missing field type");
        }

        // cur
        zval *pFieldCurValue = zend_read_property(ce, m_val, Z_STRVAL_PP(ppFieldName), Z_STRLEN_PP(ppFieldName), true TSRMLS_CC);
        if (pFieldCurValue == NULL) {
            SDP_THROW("missing field cur value");
        }

        DEBUG((IsWrite ? "write" : "read") << " struct field, tag: " << iTag << ", required: " << (bRequired ? "true" : "false") << ", name: " << sName);

        if (IsWrite) {
            zval *pFieldDefaultValue = NULL;
            if (!(bOpt || bRequired)) {
                // default
                zval **ppFieldDefaultValue;
                if (zend_hash_index_find(Z_ARRVAL_PP(ppFieldDefine), 3, (void **)&ppFieldDefaultValue) == FAILURE) {
                    SDP_THROW("missing field default value");
                }
                pFieldDefaultValue = *ppFieldDefaultValue;
            }

            PhpSdpValueWriter writer(pFieldCurValue, AuxTypeId(*ppFieldType), pFieldDefaultValue);
            ReaderWriterSelector<IsWrite, true>()(writer).visit(t, iTag, bRequired, sName.c_str());
        } else {
            PhpSdpValueReader reader(pFieldCurValue, AuxTypeId(*ppFieldType));
            ReaderWriterSelector<IsWrite, false>()(reader).visit(t, iTag, bRequired, sName.c_str());
        }
    }
}

template <typename Writer>
void PhpSdpStructWriter::visit(Writer &t, bool bOpt) const
{
    PhpSdpStruct::visit<Writer, true>(t, bOpt);
}

template <typename Reader>
void PhpSdpStructReader::visit(Reader &t, bool bOpt) const
{
    PhpSdpStruct::visit<Reader, false>(t, bOpt);
}

PhpSdpVectorWriter::PhpSdpVectorWriter(zval *pValue, const AuxTypeId &auxVectorType)
    : PhpSdpVector(pValue, auxVectorType),
      m_iVectorSize(0), m_iCurIndex(0)
{
    CHECK_TYPE_OR_THROW(Z_TYPE_P(m_pValue), IS_ARRAY);
    m_iVectorSize = zend_hash_next_free_element(Z_ARRVAL_P(m_pValue));
}

bool PhpSdpVectorWriter::next() const
{
    if (m_iCurIndex >= m_iVectorSize) {
        return false;
    }
    ++m_iCurIndex;
    return true;
}

template <typename Writer>
void PhpSdpVectorWriter::visit(Writer &t, uint32_t tag, bool require, const char *name) const
{
    DEBUG("write vector, index: " << m_iCurIndex);

    zval **ppElement;
    if (zend_hash_index_find(Z_ARRVAL_P(m_pValue), m_iCurIndex - 1, (void **)&ppElement) == FAILURE) {
        SDP_THROW("missing vector element");
    }

    PhpSdpValueWriter writer(*ppElement, m_auxInnerType);
    writer.visit(t, tag, require, name);
}

template <typename Reader>
void PhpSdpVectorReader::visit(Reader &t, uint32_t tag, bool require, const char *name) const
{
    ++m_iCurIndex;
    DEBUG("read vector, index: " << m_iCurIndex);

    PhpSdpValueReader reader(NULL, m_auxInnerType);
    reader.visit(t, tag, require, name);

    add_next_index_zval(m_pValue, reader.takeNewValue());
}

PhpSdpMapWriter::PhpSdpMapWriter(zval *pValue, const AuxTypeId &auxMapType)
    : PhpSdpMap(pValue, auxMapType),
      m_iMapSize(0), m_iCurIndex(0)
{
    CHECK_TYPE_OR_THROW(Z_TYPE_P(m_pValue), IS_ARRAY);
    m_iMapSize = zend_hash_num_elements(Z_ARRVAL_P(m_pValue));

    if (m_iMapSize > 0) {
        m_vSortKey.reserve(m_iMapSize);

        HashPosition iter;
        for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(m_pValue), &iter);
                zend_hash_has_more_elements_ex(Z_ARRVAL_P(m_pValue), &iter) == SUCCESS;
                zend_hash_move_forward_ex(Z_ARRVAL_P(m_pValue), &iter)) {
            char *s = NULL;
            uint len = 0;
            ulong idx = 0;
            zend_hash_get_current_key_ex(Z_ARRVAL_P(m_pValue), &s, &len, &idx, false, &iter);

            zval val;
            if (s != NULL) {
                ZVAL_STRINGL(&val, s, len - 1, 0);
            } else {
                ZVAL_LONG(&val, idx);
            }
            m_vSortKey.push_back(val);
        }

        sortKey();
    }
}

template <typename T>
void sortNumber64(vector<zval> &vUnSortKey)
{
    multimap<T, uint32_t> mVal2Pos;
    for (unsigned i = 0; i < vUnSortKey.size(); ++i) {
        zval &val = vUnSortKey[i];
        T key = 0;
        if (Z_TYPE(val) == IS_LONG) {
            key = (T)Z_LVAL(val);
        } else if (Z_TYPE(val) == IS_STRING) {
            key = (T)parseNumber64(Z_STRVAL(val));
        } else {
            key = (T)parseNumber64(auxStringValue(&val));
        }

        mVal2Pos.insert(make_pair(key, i));
    }

    vector<zval> vSortKey;
    vSortKey.reserve(vUnSortKey.size());
    for (typename multimap<T, uint32_t>::iterator first = mVal2Pos.begin(), last = mVal2Pos.end(); first != last; ++first) {
        uint32_t iPos = first->second;
        vSortKey.push_back(vUnSortKey[iPos]);
    }
    swap(vSortKey, vUnSortKey);
}

static bool compareStringVal(const zval &a, const zval &b)
{
    if (Z_TYPE(a) == IS_STRING && Z_TYPE(b) == IS_STRING) {
        return strcmp(Z_STRVAL(a), Z_STRVAL(b)) < 0;
    }
    return auxStringValue(&a) < auxStringValue(&b);
}

static bool compareLongVal(const zval &a, const zval &b)
{
    if (Z_TYPE(a) == IS_LONG && Z_TYPE(b) == IS_LONG) {
        return Z_LVAL(a) < Z_LVAL(b);
    }
    return auxLongValue(&a) < auxLongValue(&b);
}

void PhpSdpMapWriter::sortKey()
{
    if (m_vSortKey.size() <= 1) {
        return;
    }

    uint32_t iKeyType = m_auxKeyType.getTypeId();
    if (iKeyType == SdpType_Int64) {
        sortNumber64<int64_t>(m_vSortKey);
    } else if (iKeyType == SdpType_UInt64) {
        sortNumber64<uint64_t>(m_vSortKey);
    } else if (iKeyType == SdpType_String) {
        sort(m_vSortKey.begin(), m_vSortKey.end(), compareStringVal);
    } else {
        sort(m_vSortKey.begin(), m_vSortKey.end(), compareLongVal);
    }
}

bool PhpSdpMapWriter::next() const
{
    if (m_iCurIndex >= m_iMapSize) {
        return false;
    }
    ++m_iCurIndex;
    return true;
}

template <typename Writer>
void PhpSdpMapWriter::visitKey(Writer &t, uint32_t tag, bool require, const char *name) const
{
    DEBUG("write map key, index: " << m_iCurIndex);

    zval *pKey = const_cast<zval *>(&m_vSortKey[m_iCurIndex - 1]);
    PhpSdpValueWriter writer(pKey, m_auxKeyType);
    writer.visit(t, tag, require, name);
}

template <typename Writer>
void PhpSdpMapWriter::visitVal(Writer &t, uint32_t tag, bool require, const char *name) const
{
    DEBUG("write map value, index: " << m_iCurIndex);

    zval *pKey = const_cast<zval *>(&m_vSortKey[m_iCurIndex - 1]);
    zval **ppVal;
    int ret = 0;
    if (Z_TYPE_P(pKey) == IS_LONG) {
        ret = zend_hash_index_find(Z_ARRVAL_P(m_pValue), Z_LVAL_P(pKey), (void **)&ppVal);
    } else {
        ret = zend_hash_find(Z_ARRVAL_P(m_pValue), Z_STRVAL_P(pKey), Z_STRLEN_P(pKey) + 1, (void **)&ppVal);
    }
    if (ret == FAILURE) {
        SDP_THROW("cannot find map value");
    }

    PhpSdpValueWriter writer(*ppVal, m_auxValType);
    writer.visit(t, tag, require, name);
}

template <typename Reader>
void PhpSdpMapReader::visit(Reader &t, uint32_t tag, bool require, const char *name)
{
    ++m_iCurIndex;
    DEBUG("read map, index: " << m_iCurIndex);

    PhpSdpValueReader reader1(NULL, m_auxKeyType);
    reader1.visit(t, tag, require, name);

    PhpSdpValueReader reader2(NULL, m_auxValType);
    reader2.visit(t, tag, require, name);

    zval *keyval = reader1.getNewValue();
    if (Z_TYPE_P(keyval) == IS_STRING) {
        add_assoc_zval_ex(m_pValue, Z_STRVAL_P(keyval), Z_STRLEN_P(keyval) + 1, reader2.takeNewValue());
    } else {
        add_index_zval(m_pValue, Z_LVAL_P(keyval), reader2.takeNewValue());
    }
}

ZEND_FUNCTION(printSdp)
{
    try {
        zval *val = NULL, *valtype = NULL;
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char *)"z|z!", &val, &valtype) == FAILURE) {
            RETURN_NULL();
        }
        string sType = valtype == NULL ? "" : auxStringValue(valtype);

        ostringstream os;
        SdpDisplayer displayer(os);
        if (!sType.empty()) {
            PhpSdpValueWriter writer(val, AuxTypeId(sType));
            writer.visit(displayer, 0, true, NULL);
        } else {
            SdpStructProxy<PhpSdpStructWriter> proxy;
            proxy.under = PhpSdpStructWriter(val);
            displayer.display(proxy);
        }
        string s = os.str();
        RETURN_STRINGL(s.c_str(), s.size(), 1);
    } catch (std::exception &e) {
        DEBUG("exception: " << e.what());
        RETURN_NULL();
    }
}

ZEND_FUNCTION(sdpToString)
{
    try {
        zval *val = NULL, *valtype = NULL, *valtag = NULL;
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char *)"z|z!z!", &val, &valtype, &valtag) == FAILURE) {
            RETURN_NULL();
        }
        string sType = valtype == NULL ? "" : auxStringValue(valtype);
        uint32_t iTag = valtag == NULL ? 0 : auxLongValue(valtag);

        SdpPacker packer;
        if (!sType.empty()) {
            PhpSdpValueWriter writer(val, AuxTypeId(sType));
            writer.visit(packer, iTag, true, NULL);
        } else {
            SdpStructProxy<PhpSdpStructWriter> proxy;
            proxy.under = PhpSdpStructWriter(val);
            packer.pack(iTag, proxy);
        }

        string &s = packer.getData();
        RETURN_STRINGL(s.c_str(), s.size(), 1);
    } catch (std::exception &e) {
        DEBUG("exception: " << e.what());
        RETURN_NULL();
    }
}

ZEND_FUNCTION(stringToSdp)
{
    try {
        zval *valstr = NULL, *valtype = NULL, *valtag = NULL;
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char *)"zz|z!", &valstr, &valtype, &valtag) == FAILURE) {
            RETURN_NULL();
        }

        string sData = auxStringValue(valstr);
        if (Z_TYPE_P(valtype) != IS_ARRAY) {
            string sType = auxStringValue(valtype);
            uint32_t iTag = valtag == NULL ? 0 : auxLongValue(valtag);

            PhpSdpValueReader reader(NULL, AuxTypeId(sType));
            SdpUnpacker unpacker(sData);
            reader.visit(unpacker, iTag, true, NULL);

            zval *res = reader.getNewValue();
            RETURN_ZVAL(res, 1, 0);
        } else {
            AuxNewZVal result;
            zval *pResult = result.make();
            array_init(pResult);

            SdpUnpacker unpacker(sData);
            int n = zend_hash_next_free_element(Z_ARRVAL_P(valtype));
            for (int i = 0; i < n; ++i) {
                zval **ppTypeTag;
                if (zend_hash_index_find(Z_ARRVAL_P(valtype), i, (void **)&ppTypeTag) == FAILURE) {
                    SDP_THROW("stringToSdp type spec");
                }
                CHECK_TYPE_OR_THROW(Z_TYPE_PP(ppTypeTag), IS_ARRAY);

                zval **ppType, **ppTag;
                if (zend_hash_index_find(Z_ARRVAL_PP(ppTypeTag), 0, (void **)&ppType) == FAILURE
                        || zend_hash_index_find(Z_ARRVAL_PP(ppTypeTag), 1, (void **)&ppTag) == FAILURE) {
                    SDP_THROW("stringToSdp type spec");
                }

                string sType = auxStringValue(*ppType);
                uint32_t iTag = auxLongValue(*ppTag);

                PhpSdpValueReader reader(NULL, AuxTypeId(sType));
                reader.visit(unpacker, iTag, true, NULL);

                add_next_index_zval(pResult, reader.takeNewValue());
            }

            RETURN_ZVAL(pResult, 1, 0);
        }
    } catch (std::exception &e) {
        DEBUG("exception: " << e.what());
        RETURN_NULL();
    }
}
