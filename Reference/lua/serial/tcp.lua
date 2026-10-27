local config = require 'config'
local core = require 'qt.core'
local bonjour = config.bonjour and require 'bonjour'
local TcpSocket = core.TcpSocket

TcpSocket.defaultTimeout = 7
TcpSocket.timeout = 0
TcpSocket.online = false
TcpSocket.server = nil

function TcpSocket:onReadyRead()
    print 'TcpSocket:onReadyRead'
    local buf = self:read()
    print ('Received bytes:', #buf, ' = ', buf)
    --self:write('Goodbye')
end

function TcpSocket:onConnected()
    print 'TcpSocket:onConnected'
    self.interface = 'TCP'
    self.info = self:peer()
    self.timeout = self.defaultTimeout or 7
    --page:registerSerial(self)
    -- TODO: sendVersion()
end

function TcpSocket:onDisconnected()
    print 'TcpSocket:onDisconnected'
    self.timeout = 1
end

function TcpSocket:onError(...)
    print ('TcpSocket:onError', ...)
    self.timeout = 1
end

function TcpSocket:subscribe(service)
    self.server = { }
	if bonjour then
		bonjour.subscribe('_zaxhub._tcp', function (msg, key, service)
				print('Found zaxhub', msg, key, service, service.host, service.port, service.ip)
				self.server.host = service.host
				self.server.port = service.port
				self.server.ip = service.ip
			end)
	end
end

function TcpSocket:tryOpen()
    local server = self.server or { }
    if server.host and server.port then
        print('Open tcpport', server.host, server.ip, server.port)
        -- Note: Use ip address because hostname doesn't connect reliably 
        -- (not sure why as QTcpSocket should resolve the host name ok)
        --self:connect(server.host, server.port)
        self:connect(server.ip, server.port)
    end
    self.timeout = self.defaultTimeout
end

-- TODO: tcpport.onReadyRead = onReadyRead
return TcpSocket

