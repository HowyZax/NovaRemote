
local _M = { }

--
-- Parse incoming packet using one or more parser functions
--
-- Modifies buf string containing incoming data stream
--
-- Multiple parsers allows mixing of Sony and Zaxcom-style packets
--
-- @return: packet or nil; true if positively identified protocol; number of errors
function _M.parse(buf, fn, parsers, name)
    local errors, good = 0, 0
    while #buf > 0 do
        local skip = 1   -- number of bytes to skip if nothing significant happens
        for _,parser in ipairs(parsers) do
            local n, failed = parser(buf, fn)
            if n == nil then            -- Definitely no valid packet
                -- pass                 -- ... skip=1 will prevail unless another parser chooses otherwise
            elseif n == 0 then          -- Possible partial packet, not sure yet
                skip = 0                -- ... so don't want to skip anything
            else                        -- Otherwise parser has handled n (non-zero) bytes
                -- Keep track of status of handler fn
                if failed then errors = errors + 1
                else good = good + 1 end
                skip = n                -- Some bytes were handled so want to skip them 
                break                   -- ... and stop parsing
            end
        end
        if skip == 0 then break end     -- Come back later with more data if one (or more) parsers reported a partial packet
        -- if skip == 1 then print(string.format('%-23s   0x%02X Discard', name, buf:byte(1))) end -- this haoppens all the time if debug commands are turned on in the RX12 hardware
        --if skip == 1 then print(string.format('%c', buf:byte(1))) end -- this haoppens all the time if debug commands are turned on in the RX12 hardware
        buf = buf:sub(skip+1)           -- Otherwise, discard bytes and retry
    end
    return buf, (good > 0), errors
end

return _M
