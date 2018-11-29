local print = print
local setmetatable = setmetatable
local ipairs = ipairs
local rawget = rawget
local rawset = rawset
local type = type
local tostring = tostring
local tonumber = tonumber
local string = string
local error = error
local sdplua = require('sdplua')

module('sdp')
--- @module sdp

--- @field [parent=#global] #sdp sdp

--local debug = print
local debug = function () end

local SdpType_Void = 0
local SdpType_Bool = 1
local SdpType_Char = 2
local SdpType_Int8 = 3
local SdpType_UInt8 = 4
local SdpType_Int16 = 5
local SdpType_UInt16 = 6
local SdpType_Int32 = 7
local SdpType_UInt32 = 8
local SdpType_Int64 = 9
local SdpType_UInt64 = 10
local SdpType_Float = 11
local SdpType_Double = 12
local SdpType_String = 13
local SdpType_Vector = 14
local SdpType_Map = 15
local SdpType_Enum = 16
local SdpType_Struct = 17

local typedesc_index = {}
local SdpTypeInstanceMeta = {}

local function createFromTypeDesc(typedesc, default)
  if type(typedesc) == 'table' then
    if typedesc.TypeId == SdpType_Vector then
      local o = {}
      debug('new vector instance', tostring(o) .. '('.. tostring(typedesc.InnerType) .. ')')
      return o
    elseif typedesc.TypeId == SdpType_Map then
      local o = {}
      debug('new map instance', tostring(o) .. '('.. tostring(typedesc.KeyType) .. ',' .. tostring(typedesc.ValType) .. ')')
      return o
    elseif typedesc.TypeId == SdpType_Struct then
      local o = {}
      o[typedesc_index] = typedesc
      debug('new struct instance', tostring(o) .. '('.. typedesc.StructName .. ')')
      for i = 1, #typedesc.Definition do
        local k = typedesc.Definition[i]
        local define = typedesc.Definition[k]
        local v = createFromTypeDesc(define[3], define[4])
        o[k] = v
      end
      return o
    end
  else
    debug('new primitive type', typedesc, default)
    return default
  end
end

function SdpTypeInstanceMeta.__call(self)
  return createFromTypeDesc(self)
end

SdpStruct = {}
local SdpStructTypeMeta = {}
setmetatable(SdpStruct, SdpStructTypeMeta)

function SdpStructTypeMeta.__call(self, name)
  local o = {}
  o.TypeId = SdpType_Struct
  o.StructName = name
  o.new = function() 
    return createFromTypeDesc(o)
  end
  o.pack = function(obj)
    return _M.pack(obj)
  end
  o.unpack = function(data)
    return _M.unpack(data, o)
  end
  setmetatable(o, SdpTypeInstanceMeta)
  debug('declare struct', tostring(o) .. '('.. name .. ')')
  return o
end

SdpVector = {}
local SdpVectorTypeMeta = {}
setmetatable(SdpVector, SdpVectorTypeMeta)
local SdpVectorTypesKnown = {}

function SdpVectorTypeMeta.__call(self, innertype)
  local o = SdpVectorTypesKnown[innertype]
  if o == nil then
    o = {}
    o.TypeId = SdpType_Vector
    o.InnerType = innertype
    setmetatable(o, SdpTypeInstanceMeta)
    SdpVectorTypesKnown[innertype] = o
    debug('declare vector type', tostring(o) .. '('.. tostring(innertype) .. ')')
  end
  return o
end

SdpMap = {}
local SdpMapTypeMeta = {}
setmetatable(SdpMap, SdpMapTypeMeta)
local SdpMapTypesKnown = {}

function SdpMapTypeMeta.__call(self, keytype, valtype)
  local vtypes = SdpMapTypesKnown[keytype]
  local o = nil
  if vtypes ~= nil then
    o = vtypes[valtype]
  end
  
  if o == nil then
    o = {}
    o.TypeId = SdpType_Map
    o.KeyType = keytype
    o.ValType = valtype
    setmetatable(o, SdpTypeInstanceMeta)
    if vtypes == nil then
      vtypes = {}
      SdpMapTypesKnown[keytype] = vtypes
    end
    vtypes[valtype] = o
    debug('declare map type', tostring(o) .. '('.. tostring(keytype) .. ',' .. tostring(valtype) .. ')')
  end
  return o
end

--- @function [parent=#sdp] display
-- @return #string
function display(o)
  local typedesc = o[typedesc_index]
  if typedesc == nil then
    error('not a sdp struct', 2)
  end
  return sdplua.display(o, typedesc)
end

--- @function [parent=#sdp] pack
-- @return #string
function pack(o)
  local typedesc = o[typedesc_index]
  if typedesc == nil then
    error('not a sdp struct', 2)
  end
  return sdplua.pack(o, typedesc);
end

--- @function [parent=#sdp] unpack
function unpack(data, typedesc)
  if typedesc == nil then
    error('not a sdp struct', 2)
  end
  
  local o = typedesc()
  return sdplua.unpack(data, o, typedesc);
end

--- @function [parent=#sdp] gettype
function gettype(o)
  return o[typedesc_index]
end
