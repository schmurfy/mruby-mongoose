

module EchoHandler #< Mongoose::ConnectionHandler
  def accepted()
    puts "new connection from #{remote_address} to #{local_address}, hello !"
  end
  
  def closed()
    puts "good bye :("
  end
  
  def data_received(data)
    puts "received #{data.inspect}, send it back"
    send_data(data)
    close()
  end
  
  
  # def data_queued(c)
  # end
  
  # def polled()
  # end
  
end

manager = Mongoose::Manager.new

# the module will be mixed into the connection object
# manager.bind('tcp://1234', EchoHandler)

manager.bind('tcp://1234', EchoHandler)


manager.run()
