assert(_TARGET == 'zx')

require 'ui.runtime.zx.page'
require 'ui.runtime.zx.control'
local despatch = require 'ui.runtime.despatch'

require 'settings.server'

local despatch = despatch.despatch
local server = require 'network.server'
local metercount = 0
local lcdCanvas = PageCanvas()
local updateCounter = 0
local timer = Timer.create()

local system = { }

function system:onStartup()
    server:sendAll()
end

function system:onShutdown()
end

function system:onPulse()
    lcdCanvas:sync()
    metercount = metercount + 1
    if metercount >= 5 then
        metercount = 0
        server:sendMeters()
    end

    MetersUpdate()
    server:update()

    updateCounter = updateCounter + 1
    if timer:elapsed() > 1000 then
        ShowStatus('Memory: ' .. math.floor(collectgarbage('count')) .. 'KB, Update rate: ' .. updateCounter .. ' fps' )
        updateCounter = 0
        timer:start()
        collectgarbage()
    end

    io.flush()
end

function MsgHandler(target, method, ...)
    if target then
        local owner = target:owner()
        if owner then
            return despatch(owner, method, ...)
        end
    else
        target = system
        if target[method] then
            target[method](...)
        end
    end
end



