module HTTPHandler
  
  def timer
    send_websocket_frame("hey #{format('%x', object_id)}")
    set_timer(2000)
  end
  
  def http_request(req)
    serve_http(req, document_root: 'examples/websocket_server')
  end
  
  def websocket_handshake_request
    puts "WS request."
  end
  
  def websocket_handshake_done
    puts "WS connected."
    timer()
  end
  
  def websocket_frame(msg)
    manager.connections.each do |c|
      if c != self
        c.send_websocket_frame("[#{'%x' % object_id}] #{msg}")
      end
    end
  end
  
  def closed
    if is_websocket?
      puts "WS closed."
    else
      puts "HTTP closed."
    end
  end
  
end

$manager = Mongoose::Manager.new

opts = {
  ssl_cert: "examples/https_webserver/cert.pem",
  ssl_key: "examples/https_webserver/key.pem"
}

$manager.bind('tcp://8080', HTTPHandler, nil, opts)
  .set_protocol_http_websocket()

$manager.run()
