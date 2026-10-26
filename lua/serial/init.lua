print 'serial.init'
local core = assert(require 'serial.core')

assert(core.ports)

local function create(...)
    local port = core.SerialPort.create(...)
    --port:setOwner(port)
    return port
end

local function ports()
    local ports = { }
    for _,v in ipairs{ core.ports() } do
        if v ~= 'COM1' and v ~= 'COM2' then -- Ignore COM1/COM2
            table.insert(ports, v)
        end
    end
    return ports
end

print 'Available serial ports'
for i,v in ipairs(ports()) do
    print(i, v)
end

return {
    create = create,
    ports = ports,
}
