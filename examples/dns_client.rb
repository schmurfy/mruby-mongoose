module DNSHandler
  def dns_message(msg)
    p msg.questions
  end
  
  def connected()
    send_dns_query("google.com", 1)
  end
end

manager = Mongoose::Manager.new

manager.connect('udp://8.8.8.8:53', DNSHandler)
  .set_protocol_dns()

manager.run(10000)
