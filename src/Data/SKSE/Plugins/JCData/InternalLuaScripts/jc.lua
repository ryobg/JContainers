


--local assert_old = assert
--local function assert(expr, message)
--  if expr == false then print('trace: '..debug.traceback()) end
--  assert_old(expr, message)
--end

----------------------------------------------

local ffi = require("ffi")
local jclib
-- cache a pointer to JC' tes_context
local jc_context = JConstants.Context

local function stripPath(path)
  local limit = 40
  return #path < limit and path or ( '..' .. string.sub(path, -(limit - 1)) )
end

do
  -- load declarations
  local function readFileContents(file)
    local f = io.open(file, "rb")
    assert(f, 'no file at: '..file)
    local content = f:read("*all")
    f:close()
    return content
  end
  
  print ('loading C declarations at '..stripPath(JConstants.HeaderPath))

  local declarations = readFileContents(JConstants.HeaderPath)
  assert(declarations and #declarations > 0, 'file ' .. JConstants.HeaderPath .. ' should not be empty')
  ffi.cdef( declarations )
  
  print ('loading dll at '..stripPath(JConstants.DllPath))

  jclib = ffi.load(JConstants.DllPath)
  
  assert('lua, are you there?' == ffi.string(jclib.JC_hello()))
end
-- Load stage End


-- 2. Helper methods to retrieve C++ functions (a functions which were initially made for Papyrus use only)

local function getNativeFunction(className, funcName, returnType, argTypes)
  local funcPtr = jclib.JC_get_c_function(funcName, className)
  local signature = returnType .. '(__cdecl *)(void*' .. (argTypes and (', ' .. argTypes) or '') .. ')'
  --print('retrieving', funcName)
  return ffi.cast(signature, funcPtr)
end

-- returns {functionName: function} pairs (table)
local function retrieveNativeFunctions(className, functionsTemplate)
  local nativeFuncs = {}
  for func, v in pairs(functionsTemplate) do
    nativeFuncs[func] = getNativeFunction(className, func, v[1], v[2])
  end
  return nativeFuncs
end

-- Helpers methods End

local function readonlytable_error(_,_,_) error('attempt to modify readonly table') end

local function readonlytable(t)
  local mt = getmetatable(t) or {}
  assert(not mt.__newindex, 'already readonly')
  mt.__newindex = readonlytable_error
  setmetatable(t, mt)
  return t
end 

-- Native functions -----------------

local JValueNativeFuncs = retrieveNativeFunctions('JValue',
    {
      count = {'int32_t', 'handle'},
      writeToFile = {'void', 'handle, cstring'},
      readFromFile = {'handle', 'cstring'},
      objectFromPrototype = {'handle', 'cstring'},
      isArray = {'bool', 'handle'},
      isMap = {'bool', 'handle'},
      isFormMap = {'bool', 'handle'},
      clear = {'void', 'handle'},

      shallowCopy = {'handle', 'handle'},
      deepCopy = {'handle', 'handle'},
    }
  )

local JArrayNativeFuncs = retrieveNativeFunctions('JArray',
    {
      object = {'handle'},
      objectWithSize = {'handle', 'uint32_t'},
      --getInt = {'int32_t', 'handle, index, int32_t'},
      --getFlt = {'float', 'handle, index, float'},
      --setInt = {'void', 'handle, index, int32_t'},
      --setFlt = {'void', 'handle, index, float'},
      valueType = {'int32_t', 'handle, index'},
      eraseIndex = {'void', 'handle, index'},
    }
  )
  
local JMapNativeFuncs = retrieveNativeFunctions('JMap',
    {
      object = {'handle'},
      removeKey = {'bool', 'handle, cstring'},
      allKeys = {'handle', 'handle'},
      allValues = {'handle', 'handle'},
    }
  )

local JFormMapNativeFuncs = retrieveNativeFunctions('JFormMap',
    {
      object = {'handle'},
      -- not usable anymore: removeKey = {'bool', 'handle, CForm'},
      allKeys = {'handle', 'handle'},
      allValues = {'handle', 'handle'},
    }
  )
----------------------------------------

-- Tables

local JValue = {}
local JArray = {}
local JMap = {}
local JFormMap = {}

---------------------------------------
-- JArray is 1, JMap 2, JFormMap 3
local JCTypeList = {JArray, JMap, JFormMap}

JArray.typeName = 'JArray'
JMap.typeName = 'JMap'
JFormMap.typeName = 'JFormMap'

-------------------------------------

local CArray = ffi.typeof('struct { void* ___id; }')
local CMap = ffi.typeof('struct { void* ___id; }')
local CFormMap = ffi.typeof('struct { void* ___id; }')

local CTypeList = {CArray, CMap, CFormMap}

--------------------------------------
local JCObject_common_properties = {

  isEqualToTypeOf = function(meta, optr) return meta == JCTypeList[jclib.JValue_typeId(optr.___id)] end,
  typeOf = function(optr) return JCTypeList[jclib.JValue_typeId(optr.___id)] end,

  __gc = function(optr) jclib.JValue_release(optr.___id) end,
  __len = function(optr) return JValueNativeFuncs.count(jc_context, optr.___id) end,
  __eq = function(l, r) return (l and r) and l.___id == r.___id or false end,
}

--------------------------------------

-- Primitive types

local Handle = ffi.typeof('void *')

-- Carries data from C++ to Lua
local JCToLuaValue = ffi.typeof('JCToLuaValue')

-- Carries string from C++ to Lua
local CString = ffi.typeof('CString')

-- Carries value from Lua to C++
local JCValue = ffi.typeof('JCValue')

local CForm = ffi.metatype('CForm', { 
    __eq = function(l, r) return (l and r) and l.___id == r.___id or false end 
})

-------------------------------------

-- wraps and retains a jc-object handle
local function wrapJCHandle(handle)
  if handle == nil then return nil end
  
  local ctype = CTypeList[jclib.JValue_typeId(handle)]
  assert(ctype, 'invalid typeid?')

  jclib.JValue_retain(handle)
  return ctype(handle)
end

local function wrapJCHandleAsNumber(number)
  return wrapJCHandle(ffi.cast(Handle, number))
end

local JCValueType = {
  no_item = 0,
  none = 1,
  integer = 2,
  real = 3,
  form = 4,
  object = 5,
  string = 6,
}

-- Converts and returns JCToLuaValue as a lua type (string, number) or as a CForm, JCObject
-- Also frees JCToLuaValue's underlying string
local function returnLuaValue(item)
  local tp = item.type
  local v

  if item == nil or tp == JCValueType.no_item or tp == JCValueType.none then
    --v = nil
  elseif tp == JCValueType.string then
    v = ffi.string(item.string, item.stringLength)
    jclib.JCToLuaValue_free(item)
  elseif tp == JCValueType.integer then
    v = item.integer
  elseif tp == JCValueType.real then
    v = item.real
  elseif tp == JCValueType.form then
    v = CForm(item.form)
  elseif tp == JCValueType.object then
    v = wrapJCHandle(item.object)
  end
  --print('returnLuaValue result', v)
  return v
end

-- Converts Lua variable into 'JCValue'
-- I'm afraid this may turn into the most performance expensive part
local function returnJCValue(luaVar)
  local tp = type(luaVar)
  local v

  local function makeJCValue(tp, value)
    local var = JCValue(JCValueType[tp])
    var[tp] = value
    return var
  end
  
  if tp == 'number' then
    v = makeJCValue('real', luaVar)
  elseif tp == 'string' then
    v = makeJCValue('string', luaVar)
  elseif tp == 'boolean' then
    v = makeJCValue('integer', luaVar == true)
  elseif ffi.istype(CForm, luaVar) then
    v = makeJCValue('form', luaVar)
  elseif ffi.istype(CArray, luaVar) or ffi.istype(CMap, luaVar) or ffi.istype(CFormMap, luaVar) then
    v = makeJCValue('object', luaVar.___id)
  end
  
  return v
end

-- Mix common propeties into metatables
do

  local function addCommonProperties(jc_object_metatable)
    for k, v in pairs(JCObject_common_properties) do
      assert(jc_object_metatable[k] == nil)
      jc_object_metatable[k] = v
    end
  end

  addCommonProperties(JValue)
  addCommonProperties(JArray)
  addCommonProperties(JMap)
  addCommonProperties(JFormMap)
end

-- JValue
function JValue.readFromFile (path)
  return wrapJCHandle(JValueNativeFuncs.readFromFile(jc_context, path))
end

function JValue.writeToFile  ( optr, path )
  JValueNativeFuncs.writeToFile(jc_context, optr.___id, path)
end

function JValue.objectFromPrototype(json_proto)
  return wrapJCHandle( JValueNativeFuncs.objectFromPrototype(jc_context, json_proto) )
end

function JValue.clear (optr)
  JValueNativeFuncs.clear(jc_context, optr.___id)
end

function JValue.shallowCopy (optr)
  return wrapJCHandle(JValueNativeFuncs.shallowCopy(jc_context, optr.___id))
end

function JValue.deepCopy (optr)
  return wrapJCHandle(JValueNativeFuncs.deepCopy(jc_context, optr.___id))
end

function JValue.solvePath(optr, path)
  return returnLuaValue(jclib.JValue_solvePath(jc_context, optr.___id, path))
end

-- JArray
do
  -- converts 1-based positive indexes to 0-based, doesn't change negative ones
  local function convertIndex(idx) return idx >= 0 and idx - 1 or idx end 

  function JArray.object()
    return wrapJCHandle(JArrayNativeFuncs.object(jc_context))
  end

  function JArray.objectWithSize(size)
    return wrapJCHandle(JArrayNativeFuncs.objectWithSize(jc_context, size))
  end

  function JArray.valueType(optr, idx)
    return JArrayNativeFuncs.valueType(jc_context, optr.___id, convertIndex(idx))
  end

  function JArray.eraseIndex(optr, idx)
    JArrayNativeFuncs.eraseIndex(jc_context, optr.___id, convertIndex(idx))
  end

  function JArray.objectWithArray (array)
    local object = JArray.objectWithSize(#array)
    for i,v in ipairs(array) do
      object[i] = v
    end
    return object
  end

  function JArray.insert(optr, value, idx)
    jclib.JArray_insert(optr.___id, returnJCValue(value), convertIndex(idx or -1))
  end

  function JArray.__index (optr, idx)
    return returnLuaValue(jclib.JArray_getValue(optr.___id, convertIndex(idx)))
  end

  function JArray.__newindex (optr, idx, value)
    jclib.JArray_setValue(optr.___id, convertIndex(idx), returnJCValue(value))
  end

  function JArray.__ipairs (optr)
    local iterator = function(optr, idx)
      idx = idx + 1
      if idx <= #optr then
        return idx, optr[idx]
      end
    end
    
    return iterator, optr, 0
  end

  JArray.__pairs = JArray.__ipairs
end
--------------------------------------
-- JMap stuff
do
  function JMap.object (optr)
    return wrapJCHandle(JMapNativeFuncs.object(jc_context))
  end

  function JMap.objectWithTable (t)
    local object = JMap.object()
    for k,v in pairs(t) do
      object[k] = v
    end
    return object
  end

  function JMap.__index (optr, key)
    return returnLuaValue(jclib.JMap_getValue(optr.___id, key))
  end

  function JMap.__newindex (optr, key, value)
    if value then
      jclib.JMap_setValue(optr.___id, key, returnJCValue(value))
    else
      JMapNativeFuncs.removeKey(jc_context, optr.___id, key)
    end
  end

  function JMap.__pairs (optr)
    local function iterator (optr, key)
      -- nextKey is CString cdata which must be freed
      local nextKey = jclib.JMap_nextKey(optr.___id, key)
      if nextKey.str ~= nil then
        local luaString = ffi.string(nextKey.str, nextKey.length)
        jclib.CString_free(nextKey)
        return luaString, optr[luaString]
      end
    end

    return iterator, optr, nil
  end

  function JMap.allKeys(optr)
    return wrapJCHandle(JMapNativeFuncs.allKeys(jc_context, optr.___id))
  end

  function JMap.allValues(optr)
    return wrapJCHandle(JMapNativeFuncs.allValues(jc_context, optr.___id))
  end
end
---------------------------------------
-- FormMap stuff
do
  function JFormMap.object (optr)
    return wrapJCHandle(JFormMapNativeFuncs.object(jc_context))
  end

  function JFormMap.objectWithTable (table)
    local object = JFormMap.object()
    for k,v in pairs(table) do
      object[k] = v
    end
    return object
  end

  function JFormMap.__index (optr, key)
    return returnLuaValue(jclib.JFormMap_getValue(optr.___id, key))
  end

  function JFormMap.__newindex (optr, key, value)
    if value then
      jclib.JFormMap_setValue(optr.___id, key, returnJCValue(value))
    else
      jclib.JFormMap_removeKey(optr.___id, key)
    end
  end

  function JFormMap.__pairs (optr)
    local function iterator (optr, key)
      local nextKey = jclib.JFormMap_nextKey(optr.___id, key)
      if nextKey.___id ~= 0 then
        return nextKey, optr[nextKey]
      end
    end

    return iterator, optr, CForm(0)
  end

  function JFormMap.allKeys(optr)
    return wrapJCHandle(JFormMapNativeFuncs.allKeys(jc_context, optr.___id))
  end

  function JFormMap.allValues(optr)
    return wrapJCHandle(JFormMapNativeFuncs.allValues(jc_context, optr.___id))
  end
end
--------------------------------------- 

-- Associate CTypes with metatables
do
  assert(#CTypeList == #JCTypeList)
  for i, ctype in ipairs(CTypeList) do
    local metatable = JCTypeList[i]
    readonlytable(metatable)
    ffi.metatype(ctype, metatable)
  end
end

-- TESTS
local function testJC()
  
end

--testJC()
--collectgarbage('collect')

return {

  public = {
      JValue = JValue,
      JArray = JArray,
      JMap = JMap,
      JFormMap = JFormMap,
      JDB = wrapJCHandle(jclib.JDB_instance(JConstants.Context)),

      Form = CForm,
  },

  testJC = testJC,
  wrapJCHandle = wrapJCHandle,
  returnJCValue = returnJCValue,
  wrapJCHandleAsNumber = wrapJCHandleAsNumber,
}

