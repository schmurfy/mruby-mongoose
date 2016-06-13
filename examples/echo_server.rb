

# the module will be mixed into the connection object
module EchoHandler
  def accepted()
    puts "new connection from #{remote_address} to #{local_address}, hello !"
  end
  
  def closed()
    puts "good bye :("
  end
  
  def timer
    send_data "timer triggered !"
  end
  
  def data_received(data)
    case data[0]
    when '$' then data = data[1..-1]; close_after_send()
    when '%' then data = data[1..-1]; close()
    when 'd'
      t = data.split(" ")[1].to_i
      set_timer(t)
      return
    end
    
    puts "received #{data.inspect}, send it back"
    send_data(data)
  end
  
end

manager = Mongoose::Manager.new

# the module will be mixed into the connection object
# manager.bind('tcp://1234', EchoHandler)

# ipv6
# manager.bind('tcp://[::1]:1234', EchoHandler)

# ipv4
manager.bind('tcp://127.0.0.1:1234', EchoHandler)

manager.run()
