--------------------------------------------------------------------------------
-- VSFL Network (XSAG TCP streams)
--------------------------------------------------------------------------------
VSFL.TCP = {}


local TCPIndex = 1
function VSFL.TCP.Create(port)
  local TCPServer = {}
  TCPServer.Port = port
  TCPServer.Bind = socket.bind("*", port)
  TCPServer.Bind:settimeout(0)
  TCPServer.Clients = {}
  TCPServer.ClientTimeouts = {}
  TCPServer.CheckClients = function(self)
    if TCPServer.Bind then
      local client = self.Bind:accept()
      if client then
        client:settimeout(0.0)
        table.insert(self.Clients, client)
        TCPServer.ClientTimeouts[#self.Clients] = nil
      end
    end
    
    for index,client in pairs(self.Clients) do
      if TCPServer.ClientTimeouts[index] and (curtime() - TCPServer.ClientTimeouts[index] > 4.0) then
        table.remove(self.Clients, index)
      end
    end
  end
  TCPServer.StreamByte = function(self,ch)
    if tonumber(self) then return end
    for index,client in pairs(self.Clients) do
      local bytes,error = client:send(string.char(math.min(255,math.max(0,ch))))
--      if error then table.remove(self.Clients, index) end
--      client:send(string.char(math.min(255,math.max(0,ch))))
      if error
      then TCPServer.ClientTimeouts[index] = TCPServer.ClientTimeouts[index] or curtime()
      else TCPServer.ClientTimeouts[index] = nil
      end
    end
  end
  
  -- Add callback
  AddCallback("frame","VSFL_TCP_"..TCPIndex,function(dT) TCPServer:CheckClients() end)
  TCPIndex = TCPIndex + 1
  return TCPServer
end
