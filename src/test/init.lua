ret={}
ret[1] = 5
ret[2] = {}
ret[2][4] = {}
ret[2][4]['143'] = 'gil'
ret[2][4]['thou'] = 9
ret[2][4]['w'] = 10000
ret[2]['mod'] = 1
ret[2]['t'] = 9
ret[2]['what'] = 'why'
ret[3] = 'test'
ret['1'] = nil
ret['a'] = 77543
ret['apr'] = 4

function test_luavalue()
    return ret
end

key = ""
function PrintTable(table , level)
  level = level or 1
  local indent = ""
  for i = 1, level do
    indent = indent.."  "
  end

  if key ~= "" then
    print(indent..key.." ".."=".." ".."{")
  else
    print(indent .. "{")
  end

  key = ""
  for k,v in pairs(table) do
     if type(v) == "table" then
        key = k
        PrintTable(v, level + 1)
     else
        local content = string.format("%s%s = %s", indent .. "  ",tostring(k), tostring(v))
      print(content)  
      end
  end
  print(indent .. "}")

end
 
PrintTable(ret,0)
