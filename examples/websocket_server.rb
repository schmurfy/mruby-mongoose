module HTTPHandler
  
  def timer
    send_websocket_frame("hey #{'%x' % object_id}")
    set_timer(2000)
  end
  
  def http_request(req)
    serve_http(req, document_root: 'examples/websocket_server')
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

$manager.bind('tcp://8080', HTTPHandler)
  .set_protocol_http_websocket()

$manager.run()
