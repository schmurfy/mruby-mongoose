module HTTPHandler
  
  def http_request(req)
    # puts req.message
    # p [:http, req.method, req.body, req.query_string]
    # p req.headers
    
    serve_http(req, document_root: '.')
  end
  
end

manager = Mongoose::Manager.new

manager.bind('tcp://8080', HTTPHandler)
  .set_protocol_http_websocket()

manager.run()
