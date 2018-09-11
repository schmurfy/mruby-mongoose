
module HTTPHandler
  
  def http_reply(req)
    p req
    puts req.message
    # p [:http, req.method, req.body, req.query_string]
    # p req.headers
  end
  
  def http_chunk(req)
    unless req.body.empty?
      p [:chunk, JSON.parse(req.body)]
    end
  end
  
end


def http_get(c, path)
  data = [
    "GET #{path} HTTP/1.1",
    'Host: 127.0.0.1',
    '',
    ''
  ]

  c.send_data(data.join("\r\n"))
end


manager = Mongoose::Manager.new

c = manager.connect('tcp://127.0.0.1:3344', HTTPHandler)
  .set_protocol_http_websocket()

http_get(c, '/events')

manager.run()
