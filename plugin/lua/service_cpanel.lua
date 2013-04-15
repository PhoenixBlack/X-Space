--------------------------------------------------------------------------------
-- Control panel handler
--------------------------------------------------------------------------------
local HTTP_Return = {
  "HTTP/1.1 302 OK",
  "Location: /\n"
}


--------------------------------------------------------------------------------
-- Control panel code
--------------------------------------------------------------------------------
local physicsMode = {
  [1] = "Inertial",
  [3] = "Non-inertial",
  [4] = "Disabled",
}

Net.HTTPServer.Handlers["/"] = function(request)
  local server_info = ""
  
  if Net.Server.Host then
    local uptime = math.floor(curtime())
    server_info = server_info..
      string.format("<strong>Server uptime:</strong> %d:%02d:%02d:%02d<br />",
        uptime / (24*60*60),
        (uptime / (60*60)) % 24,
        (uptime / (60)) % 60,
        (uptime) % 60)
        
    local mjd = DedicatedServerAPI.GetMJD()
    local date = os.date("%c", 86400.0 * (mjd + 2400000.5 - 2440587.5)) or "error"
    server_info = server_info.."<strong>Server date:</strong> "..date.." ("..mjd..") <br />"
    
    local delta = math.abs(DedicatedServerAPI.GetTrueMJD() - DedicatedServerAPI.GetMJD())*86400.0
    server_info = server_info..
      string.format("<strong>Date delta:</strong> %d:%02d:%02d:%02d.%03d<br />",
        delta / (24*60*60),
        (delta / (60*60)) % 24,
        (delta / (60)) % 60,
        (delta) % 60,
        (delta * 1000) % 1000)
        
    server_info = server_info.."<strong>Clients:</strong>"
--    if #Net.Server.Clients > 0 then
      server_info = server_info.."<ul>"
      for k,v in pairs(Net.Server.Clients) do
        server_info = server_info.."<li>"..k.." "..tostring(v).."</li>"
      end
      server_info = server_info.."</ul>"
--    else
--      server_info = server_info.."<br />None<br />"
--    end

    server_info = server_info..[[<strong>Vessels:</strong>
<table id="nicetable"><thead>
<tr>
  <th scope="col"><strong>ID</strong></th>
  
  <th scope="col"><strong>smA (km)</strong></th>
  <th scope="col"><strong>e</strong></th>
  <th scope="col"><strong>i (deg)</strong></th>
  <th scope="col"><strong>MnA (deg)</strong></th>
  <th scope="col"><strong>AgP (deg)</strong></th>
  <th scope="col"><strong>AsN (deg)</strong></th>
  <th scope="col"><strong>Period (min)</strong></th>
  
  <th scope="col"><strong>Orbits</strong></th>
  
  <th scope="col"><strong>Lat (deg)</strong></th>
  <th scope="col"><strong>Lon (deg)</strong></th>
  <th scope="col"><strong>Alt (m)</strong></th>
  
  <th scope="col"><strong>Mass (kg)</strong></th>
  <th scope="col"><strong>Physics</strong></th>
</tr>
</thead>
<tbody>]]
  
    for idx=0,VesselAPI.GetCount()-1 do
      local exists = VesselAPI.GetParameter(idx,0)
      local status = ""
      if exists == 0 then status = " class=\"decayed\"" end
      
      local mass_sym = "%.0f"
      if VesselAPI.GetParameter(idx,13) > 1000e3 then mass_sym = "%.3E" end
      local alt_sym = "%.0f"
      if VesselAPI.GetParameter(idx,16) > 120000e3 then alt_sym = "%.3E" end
      
      
      if VesselAPI.GetParameter(idx,2) > 1000000 then
        server_info = server_info..string.format(
          "<tr"..status..">"..
          "<td>%05d</td>"..

          "<td>---</td>"..
          "<td>---</td>"..
          "<td>---</td>"..
          "<td>---</td>"..
          "<td>---</td>"..
          "<td>---</td>"..
          "<td>---</td>"..

          "<td>---</td>"..

          "<td>%.3f</td>"..
          "<td>%.3f</td>"..
          "<td>"..alt_sym.."</td>"..

          "<td>"..mass_sym.."</td>"..
          "<td>%s</td>"..
          "</tr>",
          VesselAPI.GetParameter(idx,2),
          VesselAPI.GetParameter(idx,14),
          VesselAPI.GetParameter(idx,15),
          VesselAPI.GetParameter(idx,16),
          VesselAPI.GetParameter(idx,13),
          physicsMode[VesselAPI.GetParameter(idx,4)] or "Unknown")
      else
        server_info = server_info..string.format(
          "<tr"..status..">"..
          "<td>%05d</td>"..
          
          "<td>%.1f</td>"..
          "<td>%.5f</td>"..
          "<td>%.3f</td>"..
          "<td>%.3f</td>"..
          "<td>%.3f</td>"..
          "<td>%.3f</td>"..
          "<td>%.2f</td>"..
          
          "<td>%.2f</td>"..
  
          "<td>%.3f</td>"..
          "<td>%.3f</td>"..
          "<td>"..alt_sym.."</td>"..
          
          "<td>"..mass_sym.."</td>"..
          "<td>%s</td>"..
          "</tr>",
          VesselAPI.GetParameter(idx,2),
  --        VesselAPI.GetParameter(idx,50)*1e-3,
  --        VesselAPI.GetParameter(idx,51)*1e-3,
  --        VesselAPI.GetParameter(idx,52)*1e-3,
  
          VesselAPI.GetParameter(idx,120)*1e-3,
          VesselAPI.GetParameter(idx,121),
          math.deg(VesselAPI.GetParameter(idx,122)),
          math.deg(VesselAPI.GetParameter(idx,123)),
          math.deg(VesselAPI.GetParameter(idx,124)),
          math.deg(VesselAPI.GetParameter(idx,125)),
          VesselAPI.GetParameter(idx,127)/60,
          
          VesselAPI.GetParameter(idx,132),
          
          VesselAPI.GetParameter(idx,14),
          VesselAPI.GetParameter(idx,15),
          VesselAPI.GetParameter(idx,16),
          
          VesselAPI.GetParameter(idx,13),
          physicsMode[VesselAPI.GetParameter(idx,4)] or "Unknown")
      end
    end
    server_info = server_info.."</tbody></table>"
  end
  
  return [[
<html>
<head>
<title>VSFL Control Panel</title>
<link rel="stylesheet" type="text/css" media="screen" href="http://vsfl.wireos.com/wp-content/themes/k2/style.css" />
</head>

<body>
<div id="page">
<div class="wrapper">
<div class="primary">
  <div class="post entry-content">
    <h2>VSFL Control Panel</h2>

    <h3>Server information</h3>
    <p>]].. server_info ..[[</p>

    <h3>Server software management</h3>
    <p>
    <form method="post" action="/reload/http">
      <input type="submit" value="Reload HTTP handlers" />
    </form>
    <form method="post" action="/reload/network">
      <input type="submit" value="Reload networking code" />
    </form>
    <form method="post" action="/reload/vsfl">
      <input type="submit" value="Reload VSFL code" />
    </form>
    </p>
  </div>
</div>
</div>
</div>
</body>
</html>
]]
end


--------------------------------------------------------------------------------
-- Reloads handlers
--------------------------------------------------------------------------------
Net.HTTPServer.Handlers["/reload/http"] = function(request)
  Net.HTTPServer.InitializeHandlers()
  return HTTP_Return
end

Net.HTTPServer.Handlers["/reload/network"] = function(request)
  print("X-Space: Reloading networking code\n")
  if Net.Server and Net.Server.Stop then Net.Server.Stop() end
  Include(PluginsFolder.."lua/network.lua")
  
  InternalCallbacks.OnDedicatedServer()
  return HTTP_Return
end

Net.HTTPServer.Handlers["/reload/vsfl"] = function(request)
  print("X-Space: Reloading VSFL related code\n")
  Include(PluginsFolder.."lua/vsfl.lua")
  return HTTP_Return
end
