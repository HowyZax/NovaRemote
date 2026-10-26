-- Helper script called via SWIG to strip prefix and lower-case first
-- character of word read from stdin
local s = io.read()
local prefix = arg[1]
if prefix and s:sub(1, #prefix) == prefix then
    s = s:sub(#prefix+1)
end
ch = s:sub(1,1):lower()
s = ch .. s:sub(2)
print(s)
