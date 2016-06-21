# you can generate a self signed certificate with:
# openssl req -x509 -nodes -newkey rsa:2048 -keyout key.pem -out cert.pem -days 999

module HTTPHandler
  
  def http_request(req)
    # puts req.message
    # p [:http, req.method, req.body, req.query_string]
    # p req.headers
    
    serve_http(req, document_root: '.')
  end
  
end


manager = Mongoose::Manager.new

c = manager.bind('tcp://8080', HTTPHandler)
  .set_protocol_http_websocket()
  .set_ssl("examples/https_webserver/merged.pem")

manager.run(2000) do
  puts "tick"
end
