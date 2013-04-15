-----------------------------------------------------------------------------
-- Exported auxiliar functions
-----------------------------------------------------------------------------
if Net.HTTPServer then Net.HTTPServer.Stop() end
Net.HTTPServer = {}



--------------------------------------------------------------------------------
-- Runs HTTP server
--------------------------------------------------------------------------------
function Net.HTTPServer.Run()
  if DEBUG_VSFL == true
  then Net.HTTPServer.Port = 18080
  else Net.HTTPServer.Port = 8080
  end
  Net.HTTPServer.Bind = socket.bind("localhost", Net.HTTPServer.Port)
  Net.HTTPServer.Bind:settimeout(0)
  Net.HTTPServer.Clients = {}
  
  -- Load handlers
  Net.HTTPServer.InitializeHandlers()
  print("X-Space: HTTP server up on port "..Net.HTTPServer.Port)
end


--------------------------------------------------------------------------------
-- Stops the server
--------------------------------------------------------------------------------
function Net.HTTPServer.Stop()
  if Net.HTTPServer.Bind then Net.HTTPServer.Bind:close() end
end


--------------------------------------------------------------------------------
-- Checks for any new connections
--------------------------------------------------------------------------------
function Net.HTTPServer.CheckClients()
  local client = Net.HTTPServer.Bind:accept()
  if client then
    client:settimeout(0.01)
    table.insert(Net.HTTPServer.Clients, client)
  end
end


--------------------------------------------------------------------------------
-- Reinitializes HTTP request handlers
--------------------------------------------------------------------------------
function Net.HTTPServer.InitializeHandlers()
  Net.HTTPServer.Handlers = {}
  Include(PluginsFolder.."lua/service_cpanel.lua")
  Include(PluginsFolder.."lua/service_grtrack.lua")
end


--------------------------------------------------------------------------------
-- Main server loop
--------------------------------------------------------------------------------
AddCallback("frame","NetworkHTTPServer",function(dT)
  if Net.HTTPServer.Bind then
    Net.HTTPServer.CheckClients()
  
    for index,client in pairs(Net.HTTPServer.Clients) do
      local data,error = client:receive()
      if error then
--        print("X-Space: HTTP server error: "..tostring(error).." on client "..tostring(client))
        table.remove(Net.HTTPServer.Clients, index)
      else
        -- Generate content from handler
        local content = ""
        
        -- Parse handler
        local reqend = string.find(data,"HTTP")
        if reqend then reqend = reqend - 2 end
        local request = ""

        if (string.find(data,"GET") == 1)
        then request = string.sub(data,5,reqend)
        else request = string.sub(data,6,reqend)
        end
        
        -- Parse any special parameters
        local parameters = {}
        local parampos = string.find(request,"?")
        if parampos then
          local paramtbl = split(string.sub(request,parampos+1),";")
          request = string.sub(request,1,parampos-1)
          for k,v in pairs(paramtbl) do
            local eqpos = string.find(v,"=")
            if eqpos then
              local eqname = string.sub(v,1,eqpos-1)
              local eqval = string.sub(v,eqpos+1)
              parameters[eqname] = eqval
            else
              parameters[eqname] = true
            end
          end
        end
        
        -- Run handler
        if Net.HTTPServer.Handlers[request] then
          local iserror,result = pcall(Net.HTTPServer.Handlers[request],parameters)
          if iserror == false
          then content = "Error: " .. result
          else content = result
          end
        else
          content = "Invalid request: " .. tostring(request)
        end
        
        -- Generate response
        local header = ""
        local response = ""
        if type(content) == "string" then
          header = "HTTP/1.1 200 OK\n" ..
                   "Server: "..socket._VERSION.."/X-Dedicated\n"..
                   "Content-Length: "..#content.."\n"..
                   "Content-Type: text/html\n\n"
          response = header .. content
        else
          if content then
            for k,v in pairs(content) do header = header .. v .. "\n" end
          else
            header = "HTTP/1.1 503 OK\n" ..
                     "Server: "..socket._VERSION.."/X-Dedicated\n"..
                     "Content-Length: 0\n"..
                     "Content-Type: text/html\n\n"
          end
          response = header
        end
        local bytesSent = client:send(response)

        client:close()
        table.remove(Net.HTTPServer.Clients, index)
      end
    end
  end
end)


Net.HTTPServer.Run()
